#!/usr/bin/env python

# Script to create initrd for Bifferboard.
# bifferos@yahoo.co.uk, 2012.
#


######################################################################
# Create these directories
dirs = """sbin dev etc/init.d home proc sys tmp usr/bin usr/sbin
          usr/share/udhcpc var/log var/run config"""


import os, sys, glob, stat, shutil, subprocess, re, tarfile
from StringIO import StringIO
from ftplib import FTP


def FindDirWithPrefix(name):
  "Find the directory starting with 'name' prefix, make sure there's one and only one"
  found = [ i for i in glob.glob(name+"*") if os.path.isdir(i) ]
  if len(found) > 1:
    raise IOError("Found more than one %s... directory" % name)
  if len(found) == 0:
    raise IOError("Couldn't find %s... directory" % name)
  return found[0]


def GetKernelDir():
  return os.path.abspath(FindDirWithPrefix("linux-"))


def GetBuildrootDir():
  return os.path.abspath(FindDirWithPrefix("buildroot-"))


def P2C(val,fp):
  return fp.write("%08lx" % val)

def C2P(fp):
  return eval("0x"+fp.read(8))

def CPIO2Python_all(fp):
  d = {}
  if fp.read(6) != "070701":
    raise ValueError("Unrecognised CPIO format")
  d["ino"] = C2P(fp)
  d["mode"] = C2P(fp)
  d["uid"] = C2P(fp)
  d["gid"] = C2P(fp)
  d["nlink"] = C2P(fp)
  d["mtime"] = C2P(fp)
  d["filesize"] = C2P(fp)
  d["devmajor"] = C2P(fp)
  d["devminor"] = C2P(fp)
  d["rdevmajor"] = C2P(fp)
  d["rdevminor"] = C2P(fp)
  d["namesize"] = C2P(fp)
  d["check"] = C2P(fp)
  
  # size includes the null padding.
  d["name"] = fp.read(d["namesize"])

  # pad the name.
  while ((2+len(d["name"]))%4):
    d["name"] += fp.read(1)
      
  # Now the file, if there is one.
  d["file"] = fp.read(d["filesize"])
  # pad the data.
  while (len(d["file"])%4):
    d["file"] += fp.read(1)
  
  return d


def Python2CPIO_all(d,fp):
  fp.write("070701")   # fixed
  P2C(d["ino"],fp)     # unique number (except for hard links)
  P2C(d["mode"],fp)   # 0xa1ff : busybox links
                      # 0x41ed : directories
                      # 0x81ed : Busybox exe itself
                      # 0x81a4 : /etc files, shell scripts
                      # stat().st_mode : /dev files copied from /dev
  P2C(d["uid"],fp)    # 0
  P2C(d["gid"],fp)    # 0
  P2C(d["nlink"],fp)  # possibly get away with 1
  P2C(d["mtime"],fp)      # 0x4a1261af
  P2C(d["filesize"],fp)   # size of file, or size of link text for link.
  P2C(d["devmajor"],fp)   
  P2C(d["devminor"],fp)
  P2C(d["rdevmajor"],fp)
  P2C(d["rdevminor"],fp)
  P2C(d["namesize"],fp)
  P2C(d["check"],fp)  
  fp.write(d["name"])
  fp.write(d["file"])


class CpioArchive:
  def __init__(self, fp):
    self.files = []
    while True:
      d = CPIO2Python_all(fp)
      self.files.append(d)
      if d["name"].startswith("TRAILER!!!"):
        break
    # read the rest of the padding (nulls for 512-byte block)
    self.final_padding = fp.read()
    
    
  def SetOwner(self, uid,gid):
    for i in self.files:
      i["uid"] = uid
      i["gid"] = gid


  def AddConsoleDevice(self):
    "Replace anything with /dev/ in the path with correct dev nodes"
    for i in self.files:
      name = i["name"].replace("\x00","")
      if name == "dev/console":
        # override major/minor versions
        i["rdevmajor"],i["rdevminor"] = 5,1
        i["mode"] = 8576
        print "Adjusted /dev/console maj/min device number"
        return
    
    raise IOError("Unable to find dummy console device, it's needed for boot!")
        
    
  def WriteAs(self,fname):
    "So long as we don't change the filenames, we don't need to change the padding"
    fp = file(fname,"wb")
    for d in self.files:
      Python2CPIO_all(d,fp)
    # read the rest of the padding.
    fp.write(self.final_padding)


def RemovePath(fname):
  if os.path.exists(fname):
    os.system("rm -Rf %s" % fname)


def MakeDirs(root, files):
  for i in files.split():
    if i:
      os.makedirs(os.path.join(root, i))


def SymLink(root, pref, rel, files):
  before = os.getcwd()
  dest = os.path.join(root,pref)
  os.chdir(dest)
  for i in files.split():
    if i:
      if not os.path.exists(i):
        print "symlink", rel, i
        os.symlink(rel, i)
  os.chdir(before)


def GetCPIO(root):
  before = os.getcwd()
  os.chdir(root)
  
  # Before we run cpio, make sure there's a dev/console file, this is needed for a boot.
  if not os.path.exists("dev"):
    os.mkdir("dev")
    
  if not os.path.exists("dev/console"):
    file("dev/console","wb").write("")
  
  # Switch to new subprocess module.
  p = subprocess.Popen("find . | cpio -H newc -o", shell=True, stdout=subprocess.PIPE, close_fds=True)  
  os.chdir(before)
  return p.stdout.read()


def RemoveTarAndDir(ver):
  tar = ver + ".tar.bz2"
  if os.path.exists(tar):
    os.unlink(tar)
  if os.path.exists(ver):
    shutil.rmtree(ver)


def Toolchain():
  "Make a toolchain by downloading buildroot"
  ver = "buildroot-2011.11"
  tar = ver + ".tar.bz2"
  RemoveTarAndDir(ver)
  os.system("wget http://buildroot.uclibc.org/downloads/" + tar)
  os.system("tar xf " + tar)
  
  # Copy the buildroot config first, sets arch to 486, needed for Bifferboard.
  # Also sets the build to prefer static libs.
  shutil.copyfile("configs/%s-config" % ver, ver + "/.config")
  
  # Copy the busybox default config over the stored defaults, to force busybox static-linked
  cfg = "/busybox-1.19.x.config"
  shutil.copyfile("configs"+cfg, ver+"/package/busybox"+cfg)
  
  # Finally build the toolchain and busybox.
  os.system("make -C "+ver)
  
  
def Kernel():
  "Download the kernel and copy config"
  ver = "linux-2.6.37.6"
  RemoveTarAndDir(ver)
  tar = ver + ".tar.bz2"
  url = "http://www.kernel.org/pub/linux/kernel/v2.6/"+tar
  os.system("wget "+url)
  os.system("tar xf " + tar)
  shutil.copyfile("configs/%s-config" % ver, ver + "/.config")


def ExtractBusySymlinks(busybin):
  "We do this by running the binary and parsing the output, but it relies on us being an intel system"
  p = subprocess.Popen(busybin, shell=True, 
          stdin=subprocess.PIPE, stdout=subprocess.PIPE, close_fds=True)
  
  txt = p.stdout.read()
  txt = " ".join(txt.split("\n"))
  rex = re.compile(".*(Currently defined functions:)(.*)$")
  m = rex.search(txt)
  if m:
    apps = m.group(2).replace(",", " ").replace("\t", " ").split()
    skip = ["[","[["]
    apps = [ i for i in apps if i not in skip ]
    return apps
  
  raise IOError("Unable to get applets from busybox binary")


def InitBusyBox(root):
  dest = os.path.join(root,"sbin/busybox")
  
  busy = GetBuildrootDir()+"/output/target/bin/busybox"
  shutil.copyfile(busy, dest)
  os.chmod(dest, 0755)

  busy_sbin = " ".join(ExtractBusySymlinks(busy))
  SymLink(root, "sbin", "./busybox", busy_sbin)


def CopyRootTree(sroot, droot):
  "Copy, skipping .svn files"
  for root, dirs, names in os.walk(sroot):
    for i in names:
      p = os.path.join(root, i)
      if os.path.isdir(p): continue
      if p.find("/.svn/") != -1: continue
      if p.endswith("~"): continue
      src = p
      dest = p.replace(sroot,droot,1)
      print src, "-->", dest
      shutil.copy(src,dest)


def BiffInitrd():

  RemovePath("initramfs.cpio")
  root = "initramfs"
  RemovePath(root)
  os.mkdir(root)

  MakeDirs(root, dirs)

  # Do stuff to make a busybox system
  InitBusyBox(root)

  # Copy /etc and other files needed for startup
  CopyRootTree("files", root)

  SymLink(root, "./", "sbin", "bin")


  # Create cpio image based on the rootfs we've just created.
  data = GetCPIO(root)
  
  fp = StringIO(data)
  cpio = CpioArchive(fp)
  cpio.AddConsoleDevice()
  cpio.WriteAs("initramfs.cpio")


def BuildBusyBox():
  "Will refresh the compiled-in applets if the config has changed"
  os.system('make -C "%s"' % GetBuildrootDir())


def ConfigureBusyBox():
  "Congigure compiled-in busybox applets"
  os.system('make -C "%s" busybox-menuconfig' % GetBuildrootDir())


def Compile():
  BuildBusyBox()
  BiffInitrd()
  os.environ["PATH"] = os.environ["PATH"]+":"+GetBuildrootDir()+"/output/host/usr/bin"
  os.system('make -C "%s" CROSS_COMPILE=i486-unknown-linux-uclibc- ARCH=i386' % GetKernelDir())
  image = os.path.join(GetKernelDir(), "arch/x86/boot/bzImage")
  shutil.copyfile(image, "bzImage")
  print "Written 'bzImage'"


def MakeRootTarball(out, src):
  "Remove the directory prefix (src), add the files underneath it"
  tar = tarfile.open(out, "w:gz")
  os.chdir(src)
  for root, dirs,names in os.walk("."):
    for i in names:
      p = os.path.join( root, i)
      tar.add(p)
  tar.close()
  os.chdir("..")
  

def ExtractFromTar(data, fname):
  tar = tarfile.open(mode="r:gz",fileobj=StringIO(data))
  fp = tar.extractfile(fname)
  return fp.read()


def OpkgToRootfs(pkg, root):
  "Read in an opkg and extract (install?) its contents to the given directory"
  print "Reading",pkg
  total = file(pkg,"rb").read()
  data = ExtractFromTar(total, "./data.tar.gz")
  
  tar = tarfile.open(mode="r:gz",fileobj=StringIO(data))
  tar.extractall(root)


def PrepareUpload(root, addr):
  "Prepares our application"
  # Clean up any stale module directory.
  os.system("rm -Rf "+root)
  os.environ["PATH"] = os.environ["PATH"]+":"+GetBuildrootDir()+"/output/host/usr/bin"
  # Deal with kernel modules
  os.system('make -C "%s" modules_install CROSS_COMPILE=i486-unknown-linux-uclibc- ARCH=i386 INSTALL_MOD_PATH=../%s' % (GetKernelDir(),root))

  for pkg in glob.glob("packages/*_rdc.ipk"):
    OpkgToRootfs(pkg, root)

  # Create tarball from all the stuff to be added to the base booted image.
  MakeRootTarball(root+".tgz", root)

  ftp_put_file(root+".tgz",addr)
  print "tgz file uploaded"


def ftp_put_file(fname, addr):
  ftp = FTP(addr)
  ftp.login()
  ftp.storbinary('STOR %s'%fname, file(fname, "rb"))
  ftp.quit()


def ConfigureKernel():
  os.environ["PATH"] = os.environ["PATH"]+":"+GetBuildrootDir()+"/output/host/usr/bin"
  os.system('make -C "%s" CROSS_COMPILE=i486-unknown-linux-uclibc- ARCH=i386 menuconfig' % GetKernelDir())


def Help():
  print "Usage: mkbiffrd.py <command>"
  print 
  print "Utility to compile up firmware to fit in 1MB flash."
  print "<command> can be:"
  print "  toolchain: Compile a toolchain including buildroot and busybox"
  print "  compile: Re-make the ramdisk and compile the kernel"
  print "  config-kernel: Configure the kernel"
  print "  config-busybox: Configure the busybox"
  print "  upload <addr>: Generate root tarball and upload it(needs address argument)"
  print
  sys.exit(1)


if __name__ == "__main__":


  if not sys.argv[1:]:
    Help()

  cmd = sys.argv[1]

  if cmd == "toolchain":
    #Toolchain()
    Kernel()
  elif cmd == "compile":
    Compile()
  elif cmd == "modules":
    Modules()
  elif cmd == "config-kernel":
    ConfigureKernel()
  elif cmd == "config-busybox":
    ConfigureBusyBox()
  elif cmd == "upload":
    if not sys.argv[2:]:
      Help()
    PrepareUpload("ftp", sys.argv[2])
  else:
    Help()


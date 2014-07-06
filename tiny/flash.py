# Module to deal with access to the pod.

import pexpect
import sys, os, re
from ftplib import FTP

g_target_addr = "test"


def ftp_put(fname):
  ftp = FTP(g_target_addr)
  ftp.login()
  ftp.storbinary('STOR %s'%fname, file(fname, "rb"))
  ftp.quit()


def ftp_get(fname):
  ftp = FTP(g_target_addr)
  ftp.login()
  ftp.retrbinary('RETR %s'%fname, file(fname, "wb").write)
  ftp.quit()


class Flash:
  def __init__(self, ip):
    self.c = pexpect.spawn('telnet %s' % ip)
    self.c.expect('/ # ')
  
  def Cmd(self,txt, exp=True):
    self.c.sendline(txt)
    if exp:
      self.c.expect('/ # ')
      
  def FlashKernel(self, fw):
    ftp_put(fw)
    self.Cmd('cat /%s > /dev/biffkernel' % fw)
    sys.stdout.write(self.c.before)
    sys.stdout.flush()    
    self.Cmd('reboot')
    sys.stdout.write(self.c.before)
    sys.stdout.flush()    
    
    
  def Close(self):
    self.Cmd('exit', False)


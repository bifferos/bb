#!/usr/bin/env python
# -*- coding: windows-1258 -*-

"""
 Look where the symbolic links are pointing, and convert the 
 feeds from links into real copies of the target files.
 
 This converts the following

 openwrt/feeds/<openwrt_packages>
 openwrt/package/feeds/packages/<some_links>
 
 into:
 
 openwrt/feeds/<openwrt_packages>
 openwrt/package/feeds/packages/<openwrt_packages>
 

 Instructions:
 
 - checkout openwrt
 - make package/symlinks (will require make menuconfig)
 - Run this script from the root of openwrt.
 - Copy the package/feeds/packages dir somewhere safe
 - Get openwrt again
 - copy the package directory back
 - Archvive the result as a fully self-contained openwrt release
   resplendent with accompanying packages.
 
"""

import os, glob, shutil


def copy_ignore(src, names):
  out = []
  for i in names:
    if i == ".svn":
      print "Ignoring .svn in %s" % src
      out.append(i)
  return out


if __name__ == "__main__":

  wc = "package/feeds/*/*"
  for i in glob.glob(wc):
    
    if os.path.islink(i) :
      # get the directory
      pkg_dir = os.path.split(i)[0]
      feed_dir = os.readlink(i)
      src = os.path.join(pkg_dir, feed_dir)
      print "removing symbolic link:", i
      os.unlink(i)
      
      dest = i
      print "moving to:", dest
      shutil.copytree(src, dest, ignore=copy_ignore)
      
    else:
      print "non-link", i
    #p = os.path.join(base, i)
    #for i in os.listdir(p):
  
  



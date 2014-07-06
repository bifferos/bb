#!/usr/bin/env python

import flash
import sys


def Help():
  print "Usage: upload.py <kernel>"
  sys.exit(-1)


if __name__ == "__main__":

  k = "bzImage"
  if sys.argv[1:]:
    k = sys.argv[1]

  p = flash.Flash("test")
  print "Programming"
  p.FlashKernel(k)
  p.Close()

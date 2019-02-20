#!/usr/bin/env python2

import os
import argparse
import sys


TYPE_SOCKET = 0o0140000
TYPE_SLINK = 0o0120000
TYPE_FILE = 0o0100000
TYPE_NODE_BLOCK = 0o0060000
TYPE_DIR = 0o0040000
TYPE_NODE_CHAR = 0o0020000
TYPE_PIPE = 0o0010000


def read8(fp):
    return int(fp.read(8), 16)


def padding(fp):
    while fp.tell() & 3:
        fp.read(1)


g_inodes = {}


def process_entry(fp, out_dir):
    if fp.read(6) != "070701":
        raise ValueError("Only 'new' CPIO format is accepted")
    ino = read8(fp)
    cpio_mode = read8(fp)
    uid = read8(fp)
    gid = read8(fp)
    nlink = read8(fp)
    mtime = read8(fp)
    filesize = read8(fp)
    devmajor = read8(fp)
    devminor = read8(fp)
    rdevmajor = read8(fp)
    rdevminor = read8(fp)
    namesize = read8(fp) - 1
    check = read8(fp)

    # read name
    name = fp.read(namesize)
    # terminator
    fp.read(1)

    if name == "TRAILER!!!":
        return False
    padding(fp)

    if name == ".":
        return True

    # Now the file, if there is one.
    file_data = fp.read(filesize)
    padding(fp)

    entry = 0o0170000 & cpio_mode
    mode = cpio_mode & 0o7777

    if entry == TYPE_SOCKET:
        sys.stdout.write("sock %s %o %d %d\n" % (name, mode, uid, gid))
    elif entry == TYPE_SLINK:
        sys.stdout.write("slink %s %s %o %d %d\n" % (name, file_data, mode, uid, gid))
    elif entry == TYPE_FILE:
        if ino in g_inodes:
            raise ValueError("duplicate ino value, hard links are not supported")
        g_inodes[ino] = None
        path = os.path.join(out_dir, name)
        dir_name = os.path.dirname(path)
        if not os.path.isdir(dir_name):
            os.makedirs(dir_name)
        sys.stdout.write("file %s %s %o %d %d\n" % (name, path, mode, uid, gid))
        open(path, "wb").write(file_data)
        os.utime(path, (mtime, mtime))
    elif entry == TYPE_NODE_BLOCK:
        sys.stdout.write("nod %s %o %d %d b %d %d\n" % (name, mode, uid, gid, rdevmajor, rdevminor))
    elif entry == TYPE_DIR:
        sys.stdout.write("dir %s %o %d %d\n" % (name, mode, uid, gid))
    elif entry == TYPE_NODE_CHAR:
        sys.stdout.write("nod %s %o %d %d c %d %d\n" % (name, mode, uid, gid, rdevmajor, rdevminor))
    elif entry == TYPE_PIPE:
        sys.stdout.write("dir %s %o %d %d\n" % (name, mode, uid, gid))
    else:
        raise ValueError("Invalid type")

    return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("cpio", help="cpio file to read from")
    parser.add_argument("dir", help="output directory to dump files into")
    args = parser.parse_args()
    fp = open(args.cpio, "rb")
    while process_entry(fp, args.dir):
        pass


if __name__ == "__main__":
    main()

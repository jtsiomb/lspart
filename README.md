lspart
======

About
-----
lspart is a simple command-line tool for listing partitions on a disk or image
file. To use it simply run it with a device file or image file as argument.

Examples
--------
    $ lspart /dev/sdc
    partitions:
    0* (pri) linux                start: 2048       size: 1949329408 [929.5 gb]
    1  (pri) linux swap/solaris   start: 1949331456 size: 4193712    [2.0 gb]

    $ lspart raspbian-jessie-lite.img 
    partitions:
    0  (pri) fat32 (lba)          start: 8192       size: 122880     [60.0 mb]
    1  (pri) linux                start: 131072     size: 2717696    [1.3 gb]

License
-------
Copyright (C) 2011-2018 John Tsiombikas <nuclear@member.fsf.org>

This program is free software. Feel free to use, modify, and/or redistribute it
under the terms of the GNU General Public License version 3, or at your option
any later version published by the Free Software Foundation. See COPYING for
details.

#!/bin/sh

umount ../disk
qemu-nbd -d /dev/nbd1

rm -rf ../disk

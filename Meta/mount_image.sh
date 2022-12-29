#!/bin/sh

# $ qemu-nbd -c /dev/nbd0 disk.img
# $ mount -t ext4 /dev/nbd0p1 disk/

mkdir ../disk

qemu-nbd -c /dev/nbd1 ../disk.img
mount -t ext4 /dev/nbd1p1 ../disk

#!/bin/sh

./mount_image.sh

# mkdir ../disk/dev/
# mkdir ../disk/sys/
# mkdir ../disk/proc/
# mkdir ../disk/run/
# mkdir ../disk/root/
# mkdir ../disk/etc/
# mkdir ../disk/sbin/
# mkdir ../disk/usr/
# 
# mkdir ../disk/etc/services
# 
# mkdir ../disk/usr/bin/
# mkdir ../disk/usr/include/

# cp -r ../Base/etc/services/* ../disk/etc/services
# cp -r ../Base/etc/interfaces/* ../disk/etc/interfaces
# cp -r ../Base/usr/include/* ../disk/usr/include

cp -r ../Base/* ../disk/*
# cp -A ../Base/* ../disk/*

cp -r ../Base/etc/* ../disk/etc/*
cp -r ../Base/sbin/* ../disk/sbin/*

# cp ../Build/System/Init/init ../disk/sbin/init
# cp ../Build/System/NetworkD/networkd ../disk/sbin/networkd
# cp ../Build/Interface/Init/interface ../disk/usr/bin/interface

./umount_image.sh

#!/bin/sh

./mount_image.sh

cp -r ../Base/etc/services/* ../disk/etc/services
cp -r ../Base/etc/interfaces/* ../disk/etc/interfaces

cp ../Build/System/Init/init ../disk/sbin/init
cp ../Build/System/NetworkD/networkd ../disk/sbin/networkd
cp ../Build/Interface/Init/interface ../disk/usr/bin/interface

./umount_image.sh

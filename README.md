# MapleOS

custom userspace for linux. No external libraries.

## Running

Enter the Meta directory and execute the `run.sh` script.

## Creating disk.img

```shell
qemu-img create disk.img -f qcow2 20G
```

## Mounting disk.img

Mounting disk image requires nbd.
On most linux distros it should be easy to enable.

```shell
sudo modprobe nbd
``` 

## Getting initrd.img

MapleOS needs initrd. Just copy from your current installation.

```shell
sudo cp /boot/initrd.img initrd.img
```

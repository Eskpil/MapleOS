#!/usr/bin/env bash

if [ -d "../Build" ]
then
  echo "Found and already existing build, building once again to ensure we have everything."
  cd ../Build
  ninja
  cd ../Meta
else
  echo "Found no suitable build, making a new one."

  cd ..
  mkdir Build
  cd Build
  cmake -G=Ninja ..

  ninja
  RESULT=$?
  if [ $? -eq 0 ]; then
    echo success
  else
    echo failed
    exit
  fi

  cd ../Meta
fi

echo "Copying build files into image"
sudo ./image.sh

KERNEL_IMAGE_PATH=../Thirdparty/linux/kernel.img

echo "Using Kernel: $KERNEL_IMAGE_PATH"


qemu-system-x86_64 \
  -m 4G \
  -smp 4 \
  -display sdl,gl=off \
  -nographic \
  -device VGA,vgamem_mb=64 \
  -device virtio-serial,max_ports=2 \
  -device virtio-rng-pci \
  -device pci-bridge,chassis_nr=1,id=bridge1 \
  -device e1000,bus=bridge1 \
  -device i82801b11-bridge,bus=bridge1,id=bridge2 \
  -device sdhci-pci,bus=bridge2 \
  -device i82801b11-bridge,id=bridge3 \
  -device sdhci-pci,bus=bridge3 \
  -device ich9-ahci,bus=bridge3 \
  -drive file=../disk.img,format=qcow2,index=0,media=disk,id=disk \
  -qmp unix:qmp-sock,server,nowait \
  -cpu max,-x2apic \
  -name MapleOS \
  -d guest_errors \
  -usb \
  -chardev qemu-vdagent,clipboard=on,mouse=off,id=vdagent,name=vdagent \
  -device virtserialport,chardev=vdagent,nr=1 \
  -enable-kvm \
  -object filter-dump,id=hue,netdev=breh,file=e1000.pcap \
  -netdev user,id=breh \
  -device e1000,netdev=breh \
  -kernel $KERNEL_IMAGE_PATH \
  -initrd ../initrd.img \
  -append "root=/dev/sda1 rw console=ttyS0 selinux=0 init=/sbin/init"

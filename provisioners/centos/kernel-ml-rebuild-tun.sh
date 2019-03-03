#!/bin/bash

set -e

uname_r="`uname -r`"
kver="${uname_r%%-*}"
kver="${kver%%.0}"
kmaj="${uname_r%%.*}"

curl -LO https://www.kernel.org/pub/linux/kernel/v"$kmaj".x/linux-"$kver".tar.xz
tar xf linux-"$kver".tar.xz
cd linux-"$kver"/

cp /boot/config-"$uname_r" .config
sed -i '/\.ndo_select_queue.*=/d' drivers/net/tun.c

yum install -y \
  bc \
  bison \
  elfutils-libelf-devel \
  flex \
  openssl-devel

make drivers/net/tun.ko

install -m 644 -D drivers/net/tun.ko "/lib/modules/${uname_r}/extra/kmod-rxtx-tun/tun.ko"
depmod -a

modprobe tun

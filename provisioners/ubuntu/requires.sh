#!/bin/bash

set -e

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y \
  gcc \
  git \
  libelf-dev \
  libpcap-dev \
  "linux-headers-`uname -r`" \
  make \
  tcpdump

echo 'search extra' > /etc/depmod.d/extra.conf
depmod -a

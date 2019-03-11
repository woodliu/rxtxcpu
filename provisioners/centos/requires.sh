#!/bin/bash

set -e

rpm --import \
  /etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7

yum install -y \
  gcc \
  git \
  "kernel-devel-uname-r == `uname -r`" \
  libpcap-devel \
  make \
  tcpdump

#!/bin/bash

set -e

yum install -y \
  https://elrepo.org/elrepo-release-7.0-3.el7.elrepo.noarch.rpm

rpm --import \
  /etc/pki/rpm-gpg/RPM-GPG-KEY-elrepo.org

yum install -y --enablerepo=elrepo-kernel \
  kernel-ml

yum swap -y --enablerepo=elrepo-kernel \
  kernel-headers -- kernel-ml-headers

grubby --set-default /boot/vmlinuz-*.el7.elrepo.x86_64

#!/bin/bash

set -e

. <(grep '^ID=' /etc/os-release | sed 's/^/OS_RELEASE_/')

[[ $OS_RELEASE_ID == "centos" ]] && {

  rpm --import \
    /etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7

  yum install -y \
    gcc \
    libpcap-devel \
    make
}

[[ $OS_RELEASE_ID == "ubuntu" ]] && {
  export DEBIAN_FRONTEND=noninteractive
  apt-get update
  apt-get install -y \
    gcc \
    libpcap-dev \
    make
}

true

#!/bin/bash

set -e

git clone https://github.com/luigirizzo/netmap/

cd netmap
./configure --no-drivers
make
make install
cd -

depmod -a
modprobe netmap

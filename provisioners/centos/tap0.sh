#!/bin/bash

set -e

cd /vagrant/helpers

make clean
make

make install

install -m 644 tap0.sysconf /etc/sysconfig/tap-mq-pong/tap0

systemctl start tap-mq-pong@tap0
systemctl enable tap-mq-pong@tap0

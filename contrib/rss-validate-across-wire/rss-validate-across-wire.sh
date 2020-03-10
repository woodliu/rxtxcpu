#!/bin/bash

(( $# == 2 )) || {
  echo >&2 "usage: $0 <interface> <ipv4-to-arp>"
  exit 1
}

ifname="$1"
via="$2"

set -x

sudo ip r add 161.142.100.80/32 via "$via" dev "$ifname"
sudo ip r add 65.69.140.83/32   via "$via" dev "$ifname"
sudo ip r add 12.22.207.184/32  via "$via" dev "$ifname"
sudo ip r add 209.142.163.6/32  via "$via" dev "$ifname"
sudo ip r add 202.188.127.2/32  via "$via" dev "$ifname"

sudo hping3 -S -c 3 -i u1 -I "$ifname" -a 66.9.149.187   -ks 2794  161.142.100.80 -p 1766
sudo hping3 -S -c 3 -i u1 -I "$ifname" -a 199.92.111.2   -ks 14230 65.69.140.83   -p 4739
sudo hping3 -S -c 3 -i u1 -I "$ifname" -a 24.19.198.95   -ks 12898 12.22.207.184  -p 38024
sudo hping3 -S -c 3 -i u1 -I "$ifname" -a 38.27.205.30   -ks 48228 209.142.163.6  -p 2217
sudo hping3 -S -c 3 -i u1 -I "$ifname" -a 153.39.163.191 -ks 44251 202.188.127.2  -p 1303

sudo ip r del 161.142.100.80/32 via "$via" dev "$ifname"
sudo ip r del 65.69.140.83/32   via "$via" dev "$ifname"
sudo ip r del 12.22.207.184/32  via "$via" dev "$ifname"
sudo ip r del 209.142.163.6/32  via "$via" dev "$ifname"
sudo ip r del 202.188.127.2/32  via "$via" dev "$ifname"

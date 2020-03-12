#!/bin/bash

(( $# < 2 || $# > 3 )) && {
  echo >&2 "usage: $0 <interface> <ipv4-to-arp> [l4-protocol]"
  exit 1
}

ifname="$1"
via="$2"
proto="${3:-"tcp"}"

[[ "$proto" != "tcp" ]] && [[ "$proto" != "udp" ]] && {
  echo >&2 "Unsupported l4-protocol '$proto'"
  exit 1
}

_proto=""

[[ "$proto" == "udp" ]] && {
  _proto="--udp"
}

set -x

sudo ip r add 161.142.100.80/32 via "$via" dev "$ifname"
sudo ip r add 65.69.140.83/32   via "$via" dev "$ifname"
sudo ip r add 12.22.207.184/32  via "$via" dev "$ifname"
sudo ip r add 209.142.163.6/32  via "$via" dev "$ifname"
sudo ip r add 202.188.127.2/32  via "$via" dev "$ifname"

sudo hping3 $_proto -S -c 2 -i u1 -I "$ifname" -a 66.9.149.187   -ks 2794  161.142.100.80 -p 1766
sudo hping3 $_proto -S -c 2 -i u1 -I "$ifname" -a 199.92.111.2   -ks 14230 65.69.140.83   -p 4739
sudo hping3 $_proto -S -c 2 -i u1 -I "$ifname" -a 24.19.198.95   -ks 12898 12.22.207.184  -p 38024
sudo hping3 $_proto -S -c 2 -i u1 -I "$ifname" -a 38.27.205.30   -ks 48228 209.142.163.6  -p 2217
sudo hping3 $_proto -S -c 2 -i u1 -I "$ifname" -a 153.39.163.191 -ks 44251 202.188.127.2  -p 1303

sudo ip r del 161.142.100.80/32 via "$via" dev "$ifname"
sudo ip r del 65.69.140.83/32   via "$via" dev "$ifname"
sudo ip r del 12.22.207.184/32  via "$via" dev "$ifname"
sudo ip r del 209.142.163.6/32  via "$via" dev "$ifname"
sudo ip r del 202.188.127.2/32  via "$via" dev "$ifname"

sudo ip r add 66.9.149.187/32   via "$via" dev "$ifname"
sudo ip r add 199.92.111.2/32   via "$via" dev "$ifname"
sudo ip r add 24.19.198.95/32   via "$via" dev "$ifname"
sudo ip r add 38.27.205.30/32   via "$via" dev "$ifname"
sudo ip r add 153.39.163.191/32 via "$via" dev "$ifname"

sudo hping3 $_proto -SA -c 2 -i u1 -I "$ifname" 66.9.149.187   -p 2794  -a 161.142.100.80 -ks 1766
sudo hping3 $_proto -SA -c 2 -i u1 -I "$ifname" 199.92.111.2   -p 14230 -a 65.69.140.83   -ks 4739
sudo hping3 $_proto -SA -c 2 -i u1 -I "$ifname" 24.19.198.95   -p 12898 -a 12.22.207.184  -ks 38024
sudo hping3 $_proto -SA -c 2 -i u1 -I "$ifname" 38.27.205.30   -p 48228 -a 209.142.163.6  -ks 2217
sudo hping3 $_proto -SA -c 2 -i u1 -I "$ifname" 153.39.163.191 -p 44251 -a 202.188.127.2  -ks 1303

sudo ip r del 66.9.149.187/32   via "$via" dev "$ifname"
sudo ip r del 199.92.111.2/32   via "$via" dev "$ifname"
sudo ip r del 24.19.198.95/32   via "$via" dev "$ifname"
sudo ip r del 38.27.205.30/32   via "$via" dev "$ifname"
sudo ip r del 153.39.163.191/32 via "$via" dev "$ifname"

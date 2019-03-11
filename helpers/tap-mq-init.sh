#!/bin/bash

set -e

is_valid_hwaddr() {
  [[ "$1" =~ ^[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}$ ]] \
    || return 1
  return 0
}

is_valid_ipv4() {
  [[ "$1" =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]] \
    || return 1
  for i in `echo "$1" | sed 's/\./ /g'`; do
    is_valid_ipv4_octet "$i" \
      || return 1
  done
  return 0
}

is_valid_ipv4_octet() {
  [[ "$1" =~ ^[0-9]{1,3}$ ]] \
    && (( "$1" >= 0 )) \
    && (( "$1" <= 255 )) \
    && return 0
  return 1
}

is_valid_prefix() {
  [[ "$1" =~ ^[0-9]{1,2}$ ]] \
    && (( "$1" >= 0 )) \
    && (( "$1" <= 32 )) \
    && return 0
  return 1
}

[[ "$EUID" -ne 0 ]] && {
  echo >&2 "This script must be run as root."
  exit 1
}

[[ "$#" -eq 5 ]] || {
  echo >&2 "usage: $0 <interface> <local_hwaddr> <remote_hwaddr> <local_ipaddr/prefix> <remote_ipaddr[/prefix]>"
  exit 1
}

interface="$1"; shift
local_hwaddr="$1"; shift
remote_hwaddr="$1"; shift
local_ipaddr="$1"; shift
remote_ipaddr="$1"; shift

[[ -n "$interface" ]] || {
  echo >&2 "Non-empty interface name required."
  exit 1
}

is_valid_hwaddr "$local_hwaddr" || {
  echo >&2 "Invalid local hwaddr '$local_hwaddr'."
  exit 1
}

is_valid_hwaddr "$remote_hwaddr" || {
  echo >&2 "Invalid remote hwaddr '$remote_hwaddr'."
  exit 1
}

is_valid_ipv4 "${local_ipaddr%%/*}" || {
  echo >&2 "Invalid ipv4 address '$local_ipaddr'."
  exit 1
}

is_valid_prefix "${local_ipaddr##*/}" || {
  echo >&2 "Invalid ipv4 prefix '$local_ipaddr'."
  exit 1
}

remote_ipaddr="${remote_ipaddr%%/*}"

is_valid_ipv4 "$remote_ipaddr" || {
  echo >&2 "Invalid ipv4 address '$remote_ipaddr'."
  exit 1
}

ip tuntap add "$interface" mode tap multi_queue

echo 1 > "/proc/sys/net/ipv6/conf/${interface}/disable_ipv6"

ip link set "$interface" address "$local_hwaddr" up

ip neighbor add "$remote_ipaddr" lladdr "$remote_hwaddr" dev "$interface"

ip address add "$local_ipaddr" dev "$interface"

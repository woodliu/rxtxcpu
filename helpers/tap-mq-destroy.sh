#!/bin/bash

set -e

[[ "$EUID" -ne 0 ]] && {
  echo >&2 "This script must be run as root."
  exit 1
}

[[ "$#" -eq 1 ]] || {
  echo >&2 "usage: $0 <interface>"
  exit 1
}

interface="$1"; shift

[[ -n "$interface" ]] || {
  echo >&2 "Non-empty interface name required."
  exit 1
}

ip tuntap del "$interface" mode tap multi_queue

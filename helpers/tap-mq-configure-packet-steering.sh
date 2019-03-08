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

[[ -e /sys/class/net/"$interface" ]] || {
  echo >&2 "Interface '$interface' not found."
  exit 1
}

timeout=5
time=0

nprocs="$(
  getconf _NPROCESSORS_ONLN
)"

final_queue="$((nprocs - 1))"
final_rps_cpus="/sys/class/net/${interface}/queues/rx-${final_queue}/rps_cpus"
final_xps_cpus="/sys/class/net/${interface}/queues/tx-${final_queue}/xps_cpus"

while \
  [[ ! -f "$final_rps_cpus" ]] \
    && [[ ! -f "$final_xps_cpus" ]]
do
  sleep 1
  ((time++))
  [[ "$time" -ge "$timeout" ]] && {
    echo >&2 "Timeout reached before 'rx-$final_queue' and 'tx-$final_queue'" \
             "for interface '$interface' existed."
    exit 1
  }
done

for i in \
  /sys/class/net/${interface}/queues/rx-*/rps_cpus \
  /sys/class/net/${interface}/queues/tx-*/xps_cpus
do
  queue="$(
    echo "$i" \
      | sed 's%^.*/[rt]x-%%;s%/[rx]ps_cpus$%%'
  )"
  cpumask="$((2 ** queue))"
  echo "$cpumask" > "$i"
done

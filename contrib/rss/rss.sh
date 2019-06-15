#!/bin/bash

[[ $# == "4" ]] || {
  echo >&2 "usage: $0 <src-ip> <src-port> <dst-ip> <dst-port>"
  exit 1
}

ethtool_x="$(
  cat
)"

echo "$ethtool_x" | grep -q "^RSS hash function:$" && {
  echo "$ethtool_x" | grep -q "toeplitz: on" || {
    echo >&2 "$0: only toeplitz hash function is currently supported."
    exit 1
  }
}

key="$(
  echo "$ethtool_x" | grep -A1 "^RSS hash key:$" | tail -1
)"

echo "$key" | grep -q '^[0-9a-fA-F:]*$' || {
  echo >&2 "$0: failed to parse valid key."
  exit 1
}

((verbose)) &&
  echo "key: $key"

indir_table=($(
  echo "$ethtool_x" | grep -P '^ +[0-9]+:[ 0-9]*' | sed 's/^.*: *//'
))

num_entries="${#indir_table[@]}"

hamming_weight="$(
  echo "obase=2;$num_entries" | bc | grep -o 1 | wc -l
)"

((hamming_weight != 1)) && {
  echo >&2 "$0: failed to parse valid indir table; idx and queue will be omitted."
  no_indir=1
}

((verbose)) && {
  echo "indir_table: ${indir_table[@]}"
  echo "num_entries: $num_entries"
  echo "hamming_weight: $hamming_weight"
}

mask="$(($num_entries - 1))"

((verbose)) &&
  echo "mask: $mask"

rss_hash="$(
  ../rss-hash/rss-hash "$@" "$key"
)"

echo "hash:  $rss_hash"

((no_indir)) || {
  idx="$((0x$rss_hash & mask))"
  echo "idx:   $idx"

  queue="${indir_table[$idx]}"
  echo "queue: $queue"
}

exit 0

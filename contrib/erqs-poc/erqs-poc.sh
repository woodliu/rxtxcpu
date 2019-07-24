#!/bin/bash

each_lpr_port() {
  first="$1"

  read lpr_start lpr_end < /proc/sys/net/ipv4/ip_local_port_range

  if ((first < lpr_start)) || (( first > lpr_end)); then
    first="$lpr_start"
  fi

  lpr_reserved=($(
    cat /proc/sys/net/ipv4/ip_local_reserved_ports \
      | expand_klist
  ))

  {
    if ((first == lpr_start)); then
      seq "$lpr_start" "$lpr_end"
    else
      seq "$first"     "$lpr_end"
      seq "$lpr_start" "$((first - 1))"
    fi
  } | while read line; do
        [[ " ${lpr_reserved[@]} " =~ " ${line} " ]] \
          || echo "$line"
      done
}

expand_klist() {
  sed 's/,/\n/g' | while read line; do
    if echo "$line" | grep -Pq '^[0-9]+$'; then
      echo "$line"
    elif echo "$line" | grep -Pq '^[0-9]+-[0-9]+$'; then
      seq `echo "$line" \
        | sed 's/-/ /'`
    fi
  done
}

get_queue_for_interface_by_cpu() {
  interface="$1"

  num_queues="$(
    ls -d /sys/class/net/"$interface"/queues/rx-[0-9] \
      | wc -l
  )"

  ((num_queues < 2)) && {
    echo 0
    return
  }

  ls /sys/class/net/"$interface"/device/msi_irqs/ \
    | while read irq; do
        description="$(
          basename "`ls -d /proc/irq/"$irq"/*/`"
        )"

        echo "$description" \
          | grep -i rx \
          | grep -qi tx \
          || continue

        cat /proc/irq/"$irq"/smp_affinity_list \
          | expand_klist \
          | grep -q '^'$cpu'$' \
          || continue

        echo "$description" \
          | sed 's/^.*-//' \
          && break
      done
}

cpulist="$(
  taskset -cp "$$" | awk '{print $NF}'
)"

echo "$cpulist" | grep -q ',\|-' && {
  echo >&2 "$0: only supports being bound to a single cpu."
  exit 1
}

cpu="$cpulist"

echo "cpu: $cpu"

url="$1"

echo "$url" | grep -q '^[a-z]*:' || {
  url="http://$url"
}

read scheme hostname < <(
  cat <<__EOF | sed 's/^ *//' | python
    from urlparse import urlparse
    u = urlparse("$url")
    print u.scheme, u.netloc
__EOF
)

echo "scheme: $scheme"

if [[ "$scheme" == 'http' ]]; then
  dst_port=80
elif [[ "$scheme" == 'https' ]]; then
  dst_port=443
else
  dst_port=0
fi

echo "$hostname" | grep -q ':' && {
  read hostname dst_port < <(echo "$hostname" | sed 's/:/ /')
}

echo "hostname: $hostname"
echo "dst_port: $dst_port"

dst_ip="$(
  getent ahostsv4 "$hostname" | awk '{print $1; exit}'
)"

echo "dst_ip: $dst_ip"

ip_r_get="$(
  ip r get "$dst_ip"
)"

src_ip="$(
  echo "$ip_r_get" | grep -o ' src [^ ]*' | sed 's/^.* //'
)"

echo "src_ip: $src_ip"

interface="$(
  echo "$ip_r_get" | grep -o ' dev [^ ]*' | sed 's/^.* //'
)"

echo "interface: $interface"

queue="$(
  get_queue_for_interface_by_cpu "$interface" "$cpu"
)"

echo "queue: $queue"

ethtool_x="$(
  ethtool -x "$interface"
)"

read lpr_start lpr_end < /proc/sys/net/ipv4/ip_local_port_range

lpr_cardinality="$((
  lpr_end + 1 - lpr_start
))"

random_lpr_port="$((
  RANDOM % lpr_cardinality + lpr_start
))"

src_port=0
for i in `each_lpr_port "$random_lpr_port"`; do
  rss_sh="$(
    echo "$ethtool_x" \
      | ../rss/rss.sh "$dst_ip" "$dst_port" "$src_ip" "$i"
  )"

  echo "$rss_sh" \
    | grep -q '^queue: '"$queue"'$' \
    && {
         echo "$rss_sh" \
           | sed 's/^/rss_/;s/  */ /g'
         src_port="$i"
         break
       }
done

((src_port)) || {
  echo >&2 "failed to find suitable src_port"
  exit 1
}

echo "src_port: $src_port"

curl --local-port "$src_port" --resolve "$hostname:$dst_port:$dst_ip" "$url"

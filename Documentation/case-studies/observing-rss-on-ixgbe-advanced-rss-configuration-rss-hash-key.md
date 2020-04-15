# Observing RSS on ixgbe: Advanced RSS Configuration - RSS Hash Key

This case study leverages commands which may be unsafe in your environment
(i.e. commands which change interface configuration, send packets with spoofed
source address, etc.). Be sure you understand the implications of, and have
taken suitable precations for, each command before running.

Setup details:
* `ixgbe0` is an `82599ES 10-Gigabit SFI/SFP+ Network Connection (Ethernet
    Server Adapter X520-2)` using the **in-tree** ixgbe driver
* `eth0` is a nic in the same layer 2 network as `ixgbe0`
* `localhost` is the host which contains `ixgbe0`
* `remotehost` is the host which contains `eth0`
* `192.168.24.10/24` is the ipv4 address assigned to `ixgbe0`
* `192.168.24.11/24` is the ipv4 address assigned to `eth0`
* Flow Director is disabled as documented in [Advanced RSS Configuration](./observing-rss-on-ixgbe-advanced-rss-configuration.md)

As is sometimes the case when exploring related configuration items, we've
already encountered RSS hash key configuration in a few previous case studies
([Basic RSS Validation](./observing-rss-on-ixgbe-basic-rss-validation.md) and
[RSS Hash Fields](./observing-rss-on-ixgbe-advanced-rss-configuration-rss-hash-fields.md)).
To showcase hash key configuration, let's look at a common hash key change with
practial benefits for some workloads; setting a hash key which results in
symmetric RSS.

http://www.ndsl.kaist.edu/~kyoungsoo/papers/TR-symRSS.pdf

Symmetric RSS is used to preserve packet order when a nic is expected to
*receive* packets from both directions in a non-locally-terminated connection.
[Suricata](https://github.com/OISF/suricata) is an example application which
has this need.

With normal, asymmetric RSS, packets from the same rx flow result in the same
hash value, land, therefore, in the same indirection table bucket, and are
mapped to the same queue. This is great when the connection is locally
terminated; packets from the tx flow originate on the localhost and have no
reason to traverse the RSS mechanism in the local rx path.

With symmetric RSS, packets from the same connection gain the same property;
packets in both flows (both directions) result in the same hash value,
indirection table bucket, and queue. Coupled with affinitization between queue
and cpu, the same cpu receives ordered packets from that connection. Now an
application such as Suricata can observe the connection accurately, despite
terminating neither end.

In order to get the same hash values, we need to set the same hash key. Let's
use the one provided in the symmetric RSS paper. Note that this key is the
first 2-bytes (4-nibbles) of the hash key from the Microsoft verification
document, repeated until the required 40-byte length is reached. As was
mentioned in ([Basic RSS Validation](./observing-rss-on-ixgbe-basic-rss-validation.md),
the hash key is properly secret data to prevent volumetric attacks against a
single queue. I recommend using the first 2-bytes of an interface's original
hash key, in the same repeating pattern, for any production use of symmetric
RSS.

You will also need the same number of queues as I use to get the same queue
results.
```
[user@localhost:~]$ sudo ethtool -X ixgbe0 hkey 6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a
[user@localhost:~]$ sudo ethtool -L ixgbe0 combined 16
[user@localhost:~]$ ethtool -x ixgbe0
RX flow hash indirection table for ixgbe0 with 16 RX ring(s):
    0:      0     1     2     3     4     5     6     7
    8:      8     9    10    11    12    13    14    15
   16:      0     1     2     3     4     5     6     7
   24:      8     9    10    11    12    13    14    15
   32:      0     1     2     3     4     5     6     7
   40:      8     9    10    11    12    13    14    15
   48:      0     1     2     3     4     5     6     7
   56:      8     9    10    11    12    13    14    15
   64:      0     1     2     3     4     5     6     7
   72:      8     9    10    11    12    13    14    15
   80:      0     1     2     3     4     5     6     7
   88:      8     9    10    11    12    13    14    15
   96:      0     1     2     3     4     5     6     7
  104:      8     9    10    11    12    13    14    15
  112:      0     1     2     3     4     5     6     7
  120:      8     9    10    11    12    13    14    15
RSS hash key:
6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a
```

This time, we simply expect that packets, in both directions, from the same
connection will land on the same queue. Using `rxqueue` we'll collect per-queue
packet captures of `ixgbe0`.
```
[user@localhost:~]$ sudo rxqueue -w test.pcap ixgbe0
...
```

We'll leave `rxqueue` running on `localhost`.

[`rss-validate-symmetric-across-wire.sh`](../../contrib/rss-validate-across-wire/)
can be used on `remotehost` to send packets for each ipv4 4-tuple in the
verification data as well as packets with source and destination information
swapped. Alternatively, the following commands preceeded by `+` can be run
manually.
```
[user@remotehost:~/rxtxcpu/contrib/rss-validate-across-wire]$ ./rss-validate-symmetric-across-wire.sh eth0 192.168.24.10
+ sudo ip r add 161.142.100.80/32 via 192.168.24.10 dev eth0
+ sudo ip r add 65.69.140.83/32 via 192.168.24.10 dev eth0
+ sudo ip r add 12.22.207.184/32 via 192.168.24.10 dev eth0
+ sudo ip r add 209.142.163.6/32 via 192.168.24.10 dev eth0
+ sudo ip r add 202.188.127.2/32 via 192.168.24.10 dev eth0
+ sudo hping3 -S -c 2 -i u1 -I eth0 -a 66.9.149.187 -ks 2794 161.142.100.80 -p 1766
HPING 161.142.100.80 (eth0 161.142.100.80): S set, 40 headers + 0 data bytes

--- 161.142.100.80 hping statistic ---
2 packets transmitted, 0 packets received, 100% packet loss
round-trip min/avg/max = 0.0/0.0/0.0 ms
+ sudo hping3 -S -c 2 -i u1 -I eth0 -a 199.92.111.2 -ks 14230 65.69.140.83 -p 4739
HPING 65.69.140.83 (eth0 65.69.140.83): S set, 40 headers + 0 data bytes

--- 65.69.140.83 hping statistic ---
2 packets transmitted, 0 packets received, 100% packet loss
round-trip min/avg/max = 0.0/0.0/0.0 ms
+ sudo hping3 -S -c 2 -i u1 -I eth0 -a 24.19.198.95 -ks 12898 12.22.207.184 -p 38024
HPING 12.22.207.184 (eth0 12.22.207.184): S set, 40 headers + 0 data bytes

--- 12.22.207.184 hping statistic ---
2 packets transmitted, 0 packets received, 100% packet loss
round-trip min/avg/max = 0.0/0.0/0.0 ms
+ sudo hping3 -S -c 2 -i u1 -I eth0 -a 38.27.205.30 -ks 48228 209.142.163.6 -p 2217
HPING 209.142.163.6 (eth0 209.142.163.6): S set, 40 headers + 0 data bytes

--- 209.142.163.6 hping statistic ---
2 packets transmitted, 0 packets received, 100% packet loss
round-trip min/avg/max = 0.0/0.0/0.0 ms
+ sudo hping3 -S -c 2 -i u1 -I eth0 -a 153.39.163.191 -ks 44251 202.188.127.2 -p 1303
HPING 202.188.127.2 (eth0 202.188.127.2): S set, 40 headers + 0 data bytes

--- 202.188.127.2 hping statistic ---
2 packets transmitted, 0 packets received, 100% packet loss
round-trip min/avg/max = 0.0/0.0/0.0 ms
+ sudo ip r del 161.142.100.80/32 via 192.168.24.10 dev eth0
+ sudo ip r del 65.69.140.83/32 via 192.168.24.10 dev eth0
+ sudo ip r del 12.22.207.184/32 via 192.168.24.10 dev eth0
+ sudo ip r del 209.142.163.6/32 via 192.168.24.10 dev eth0
+ sudo ip r del 202.188.127.2/32 via 192.168.24.10 dev eth0
+ sudo ip r add 66.9.149.187/32 via 192.168.24.10 dev eth0
+ sudo ip r add 199.92.111.2/32 via 192.168.24.10 dev eth0
+ sudo ip r add 24.19.198.95/32 via 192.168.24.10 dev eth0
+ sudo ip r add 38.27.205.30/32 via 192.168.24.10 dev eth0
+ sudo ip r add 153.39.163.191/32 via 192.168.24.10 dev eth0
+ sudo hping3 -SA -c 2 -i u1 -I eth0 66.9.149.187 -p 2794 -a 161.142.100.80 -ks 1766
HPING 66.9.149.187 (eth0 66.9.149.187): SA set, 40 headers + 0 data bytes

--- 66.9.149.187 hping statistic ---
2 packets transmitted, 0 packets received, 100% packet loss
round-trip min/avg/max = 0.0/0.0/0.0 ms
+ sudo hping3 -SA -c 2 -i u1 -I eth0 199.92.111.2 -p 14230 -a 65.69.140.83 -ks 4739
HPING 199.92.111.2 (eth0 199.92.111.2): SA set, 40 headers + 0 data bytes

--- 199.92.111.2 hping statistic ---
2 packets transmitted, 0 packets received, 100% packet loss
round-trip min/avg/max = 0.0/0.0/0.0 ms
+ sudo hping3 -SA -c 2 -i u1 -I eth0 24.19.198.95 -p 12898 -a 12.22.207.184 -ks 38024
HPING 24.19.198.95 (eth0 24.19.198.95): SA set, 40 headers + 0 data bytes

--- 24.19.198.95 hping statistic ---
2 packets transmitted, 0 packets received, 100% packet loss
round-trip min/avg/max = 0.0/0.0/0.0 ms
+ sudo hping3 -SA -c 2 -i u1 -I eth0 38.27.205.30 -p 48228 -a 209.142.163.6 -ks 2217
HPING 38.27.205.30 (eth0 38.27.205.30): SA set, 40 headers + 0 data bytes

--- 38.27.205.30 hping statistic ---
2 packets transmitted, 0 packets received, 100% packet loss
round-trip min/avg/max = 0.0/0.0/0.0 ms
+ sudo hping3 -SA -c 2 -i u1 -I eth0 153.39.163.191 -p 44251 -a 202.188.127.2 -ks 1303
HPING 153.39.163.191 (eth0 153.39.163.191): SA set, 40 headers + 0 data bytes

--- 153.39.163.191 hping statistic ---
2 packets transmitted, 0 packets received, 100% packet loss
round-trip min/avg/max = 0.0/0.0/0.0 ms
+ sudo ip r del 66.9.149.187/32 via 192.168.24.10 dev eth0
+ sudo ip r del 199.92.111.2/32 via 192.168.24.10 dev eth0
+ sudo ip r del 24.19.198.95/32 via 192.168.24.10 dev eth0
+ sudo ip r del 38.27.205.30/32 via 192.168.24.10 dev eth0
+ sudo ip r del 153.39.163.191/32 via 192.168.24.10 dev eth0
```

Over on `localhost` we can now stop our `rxqueue` with Ctrl-C or similar.
```
[user@localhost:~]$ sudo rxqueue -w test.pcap ixgbe0
^C
0 packets captured on queue 0.
0 packets captured on queue 1.
0 packets captured on queue 2.
0 packets captured on queue 3.
0 packets captured on queue 4.
0 packets captured on queue 5.
0 packets captured on queue 6.
4 packets captured on queue 7.
0 packets captured on queue 8.
0 packets captured on queue 9.
0 packets captured on queue 10.
0 packets captured on queue 11.
4 packets captured on queue 12.
0 packets captured on queue 13.
12 packets captured on queue 14.
0 packets captured on queue 15.
20 packets captured total.
```

Using `tcpdump` we can see that the packets, in both directions, from the same
simulated "connection" did in fact land on the same queue.
```
[user@localhost:~]$ for i in test-{7,12,14}.pcap; do tcpdump -Snnr "$i"; done
reading from file test-7.pcap, link-type EN10MB (Ethernet)
14:58:20.000000 IP 199.92.111.2.14230 > 65.69.140.83.4739: Flags [S], seq 207004445, win 512, length 0
14:58:20.000000 IP 199.92.111.2.14230 > 65.69.140.83.4739: Flags [S], seq 1696402199, win 512, length 0
14:58:21.000000 IP 65.69.140.83.4739 > 199.92.111.2.14230: Flags [S.], seq 2051839150, ack 302624297, win 512, length 0
14:58:21.000000 IP 65.69.140.83.4739 > 199.92.111.2.14230: Flags [S.], seq 238505757, ack 252586873, win 512, length 0
reading from file test-12.pcap, link-type EN10MB (Ethernet)
14:58:20.000000 IP 66.9.149.187.2794 > 161.142.100.80.1766: Flags [S], seq 207004445, win 512, length 0
14:58:20.000000 IP 66.9.149.187.2794 > 161.142.100.80.1766: Flags [S], seq 1696402199, win 512, length 0
14:58:20.000000 IP 161.142.100.80.1766 > 66.9.149.187.2794: Flags [S.], seq 207004445, ack 492316771, win 512, length 0
14:58:20.000000 IP 161.142.100.80.1766 > 66.9.149.187.2794: Flags [S.], seq 1696402199, ack 397780074, win 512, length 0
reading from file test-14.pcap, link-type EN10MB (Ethernet)
14:58:20.000000 IP 24.19.198.95.12898 > 12.22.207.184.38024: Flags [S], seq 207004445, win 512, length 0
14:58:20.000000 IP 24.19.198.95.12898 > 12.22.207.184.38024: Flags [S], seq 1696402199, win 512, length 0
14:58:20.000000 IP 38.27.205.30.48228 > 209.142.163.6.2217: Flags [S], seq 207004445, win 512, length 0
14:58:20.000000 IP 38.27.205.30.48228 > 209.142.163.6.2217: Flags [S], seq 1696402199, win 512, length 0
14:58:20.000000 IP 153.39.163.191.44251 > 202.188.127.2.1303: Flags [S], seq 207004445, win 512, length 0
14:58:20.000000 IP 153.39.163.191.44251 > 202.188.127.2.1303: Flags [S], seq 1696402199, win 512, length 0
14:58:21.000000 IP 12.22.207.184.38024 > 24.19.198.95.12898: Flags [S.], seq 2051839150, ack 302624297, win 512, length 0
14:58:21.000000 IP 12.22.207.184.38024 > 24.19.198.95.12898: Flags [S.], seq 238505757, ack 252586873, win 512, length 0
14:58:21.000000 IP 209.142.163.6.2217 > 38.27.205.30.48228: Flags [S.], seq 2051839150, ack 302624297, win 512, length 0
14:58:21.000000 IP 209.142.163.6.2217 > 38.27.205.30.48228: Flags [S.], seq 238505757, ack 252586873, win 512, length 0
14:58:21.000000 IP 202.188.127.2.1303 > 153.39.163.191.44251: Flags [S.], seq 2051839150, ack 302624297, win 512, length 0
14:58:21.000000 IP 202.188.127.2.1303 > 153.39.163.191.44251: Flags [S.], seq 238505757, ack 252586873, win 512, length 0
```

RSS hash key configuration has been observed.

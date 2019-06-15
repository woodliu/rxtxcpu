#!/bin/bash

# Verification data was obtained from here:
#
#   https://docs.microsoft.com/en-us/windows-hardware/drivers/network/verifying-the-rss-hash-calculation
#

key="6d:5a:56:da:25:5b:0e:c2:41:67:25:3d:43:a3:8f:b0:d0:ca:2b:cb:ae:7b:30:b4:77:cb:2d:a3:80:30:f2:0c:6a:42:b7:3b:be:ac:01:fa"

failures=0

_test() {
  stdout="$(
    ./rss-hash "$1" "$2" "$3" "$4" "$key"
  )"
  if [[ "0x$stdout" == "$5" ]]; then
    echo "success: $@"
  else
    ((failures++))
    echo " failure: $1 $2 $3 $4"
    echo "expected: $5"
    echo "     got: 0x$stdout"
  fi
}

_test 66.9.149.187       0 161.142.100.80    0 0x323e8fc2
_test 66.9.149.187    2794 161.142.100.80 1766 0x51ccc178
_test 199.92.111.2       0 65.69.140.83      0 0xd718262a
_test 199.92.111.2   14230 65.69.140.83   4739 0xc626b0ea
_test 24.19.198.95       0 12.22.207.184     0 0xd2d0a5de
_test 24.19.198.95   12898 12.22.207.184 38024 0x5c2b394a
_test 38.27.205.30       0 209.142.163.6     0 0x82989176
_test 38.27.205.30   48228 209.142.163.6  2217 0xafc7327f
_test 153.39.163.191     0 202.188.127.2     0 0x5d1809c5
_test 153.39.163.191 44251 202.188.127.2  1303 0x10e828a2

_test 3ffe:2501:200:1fff::7                   0 3ffe:2501:200:3::1           0 0x2cc18cd5
_test 3ffe:2501:200:1fff::7                2794 3ffe:2501:200:3::1        1766 0x40207d3d
_test 3ffe:501:8::260:97ff:fe40:efab          0 ff02::1                      0 0x0f0c461c
_test 3ffe:501:8::260:97ff:fe40:efab      14230 ff02::1                   4739 0xdde51bbf
_test 3ffe:1900:4545:3:200:f8ff:fe21:67cf     0 fe80::200:f8ff:fe21:67cf     0 0x4b61e985
_test 3ffe:1900:4545:3:200:f8ff:fe21:67cf 44251 fe80::200:f8ff:fe21:67cf 38024 0x02d1feef

exit $failures

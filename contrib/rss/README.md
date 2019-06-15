# rss.sh

Compute the rss hash, indirection table index, and queue for an rx flow given a specific interface configuration.

Please note, the queue value is the queue which rss would select, nothing more. Some nics have advanced features which steer packets using alternate mechanisms (e.g. Accelerated RFS, Intel ATR, ntuple rules) under certain conditions; the queue chosen for a real packet traversing the nic may differ from the rss queue value.

## Building

Build dependencies.
```
make
```

## Usage

```
ethtool -x <interface> | rss.sh <src-ip> <src-port> <dst-ip> <dst-port>
```

### Examples

```
ethtool -x eth0 | rss.sh 169.254.254.1 32768 169.254.254.2 443
```

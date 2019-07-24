# erqs-poc.sh

erqs (Ephemeral port Receive Queue Steering) is a pattern to achieve receive queue steering via intelligent source port selection on embryonic outbound connections. The intent of the pattern is to provide packet locality at the connection level for outbound (tx first) connections.

Quite often, it isn't required due to alternate features which align rx flows to tx flows (RFS, Accelerated RFS, Intel ATR). However, there are times when these or an equivalent mechanism is not available or impractical to use.

erqs differs from the mentioned alternate features in that it doesn't require a table to track flows. This has benefits (no memory consumption for a table, no table imposed limits) and drawbacks (more complex ephemeral port selection, tasks using erqs need to be pinned).

This poc simply demonstrates the pattern; it makes no attempt to be efficient, nor to cover every corner case (e.g. it would be desirable to provide physical core locality or numa locality when logical core locality isn't an option).

## Building

Build dependencies.
```
make
```

## Usage

`erqs-poc.sh` must be pinned to a single cpu.

```
taskset -c <cpu> ./erqs-poc.sh <url>
```

### Examples

```
taskset -c 1 ./erqs-poc.sh stackpath.com
taskset -c 2 ./erqs-poc.sh http://stackpath.com
taskset -c 8 ./erqs-poc.sh https://stackpath.com
taskset -c 9 ./erqs-poc.sh https://www.stackpath.com/company/about-us/

taskset -c 1 ./erqs-poc.sh http://example.com:8080
taskset -c 2 ./erqs-poc.sh https://example.com:8443
taskset -c 3 ./erqs-poc.sh ftp://example.com:21
```

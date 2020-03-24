# Observing RSS on ixgbe: Advanced RSS Configuration

These case studies leverage commands which may be unsafe in your environment
(i.e. commands which change interface configuration, send packets with spoofed
source address, etc.). Be sure you understand the implications of, and have
taken suitable precations for, each command before running.

The methods by which multiqueue adapters steer packets to receive queues have
become more complex over time. Several of these methods can be altered via
config (e.g. changes to RSS key or fields). Frequently, these methods will be
layered with fallback (e.g. an ATR table miss falls back to RSS). Other times,
a method will permit custom rules implementing custom policy (e.g. ntuple rules
[EP] or programmable RSS).

Observing how these methods act and interact can help one tune adapters to meet
specific perfomance requirements. Here we look in depth at RSS on ixgbe.

## Setup

It's simplest to observe RSS, including hash algorithm, field, indirection
table, and key configuration changes, with Flow Director completely disabled;
both EP and ATR modes.

### Disable Flow Director EP (Externally Programmed) Mode

For both in-tree and out-of-tree ixgbe, EP mode can be disabled with `ethtool`.
```
[user@localhost:~]$ sudo ethtool -K ixgbe0 ntuple off
[user@localhost:~]$ ethtool -k ixgbe0 | grep ntuple
ntuple-filters: off
```

#### Older ixgbe Drivers

On older out-of-tree ixgbe versions (v2.0.34.3..v3.9.17), the module parameter
`FdirMode` can also influence Flow Director modes.

### Disable Flow Director ATR (Application Targeting Routing) Mode

For out-of-tree ixgbe, ATR mode can be disabled with `ethtool` (since v4.4.6).
```
[user@localhost:~]$ sudo ethtool --set-priv-flags ixgbe0 flow-director-atr off
[user@localhost:~]$ ethtool --show-priv-flags ixgbe0 | grep flow-director-atr
flow-director-atr: off
```

For in-tree ixgbe, ATR mode can be disabled by enabling EP mode. A bit
counterintuitive given that we just disabled EP mode, however, in order to be
in effect, EP mode needs not only to be enabled, but also programmed with
rules. When EP mode is enabled, but no rules are present, ixgbe falls back to
RSS to select a receive queue.
```
[user@localhost:~]$ ethtool -k ixgbe0 | grep ntuple
ntuple-filters: off
[user@localhost:~]$ sudo ethtool -K ixgbe0 ntuple on
[user@localhost:~]$ ethtool -k ixgbe0 | grep ntuple
ntuple-filters: on
[user@localhost:~]$ ethtool -n ixgbe0
16 RX rings available
Total 0 rules

```

#### Older ixgbe Drivers

For older out-of-tree ixgbe versions, the module parameter `AtrSampleRate=0`
can be used to disable ATR. This still works as of v5.6.5, but unlike the
suggested `ethtool` method can only be set at module load time.

On older out-of-tree ixgbe versions (v2.0.34.3..v3.9.17), the module parameter
`FdirMode` can also influence Flow Director modes.

## Configuration Topics

* [RSS Hash Fields](./observing-rss-on-ixgbe-advanced-rss-configuration-rss-hash-fields.md)
* [RSS Hash Key](./observing-rss-on-ixgbe-advanced-rss-configuration-rss-hash-key.md)
* [RSS Hash Indirection Table](./observing-rss-on-ixgbe-advanced-rss-configuration-rss-hash-indirection-table.md)

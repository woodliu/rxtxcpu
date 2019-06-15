# rss-hash

A userspace implementation of rss hashing.

## Building

Build.
```
make
```

Test.
```
make test
```

## Usage

```
rss-hash <src-ip> <src-port> <dst-ip> <dst-port> <rss-key>
```

### Examples

These are equivalent since `6d:5a...01:fa` is the default key.

```
rss-hash 169.254.254.1 32768 169.254.254.2 443
rss-hash 169.254.254.1 32768 169.254.254.2 443 "6d:5a:56:da:25:5b:0e:c2:41:67:25:3d:43:a3:8f:b0:d0:ca:2b:cb:ae:7b:30:b4:77:cb:2d:a3:80:30:f2:0c:6a:42:b7:3b:be:ac:01:fa"
```

This example demonstrates using the symmetric key as described in [this paper](http://www.ndsl.kaist.edu/~kyoungsoo/papers/TR-symRSS.pdf).

```
rss-hash 169.254.254.1 32768 169.254.254.2 443 "6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a"
rss-hash 169.254.254.2 443 169.254.254.1 32768 "6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a:6d:5a"
```

## TODO

rss-hash currently only supports the toeplitz hashing algorithm. Some nics also have xor and crc32 hashing algorithms available; adding support for them is desirable. I suspect doing so will require some changes to the cli; toeplitz produces the correct value for subsets of rx-flow-hash "sdfn" (e.g. "sd" or "s") by passing 0 for unwanted ports, 0.0.0.0 for unwanted IPv4 addrs, and ::0 for unwanted IPv6 addrs--if xor is as I imagine it to be, it will also work this way--but for crc32 passing a null byte into the state machine alters the value, which I expect to be problematic.

Some nics offer additional entropy fields beyond sdfn (i.e. m, v, and t); adding support for these would be nice too.

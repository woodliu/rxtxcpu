/*
 * Copyright (c) 2014 Adrian Chadd <adrian@FreeBSD.org>
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Much of this source code was originally released under the BSD-3-Clause
 * license here:
 *
 *   https://github.com/erikarn/freebsd-rss/blob/45607c93eadf7ad73e8d13e902b2ea55b4b37ca1/rss-hash/main.c
 *
 * I reached out to the original author, Adrian Chadd <adrian.chadd@gmail.com>,
 * and obtained his permission to rerelease the copied code here under the MIT
 * license.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <err.h>

#define DEFAULT_RSS_KEYSTR "6d:5a:56:da:25:5b:0e:c2:41:67:25:3d:43:a3:8f:b0:d0:ca:2b:cb:ae:7b:30:b4:77:cb:2d:a3:80:30:f2:0c:6a:42:b7:3b:be:ac:01:fa"

int parse_rss_key(char *rss_keystr, uint8_t *rss_key, u_int rss_keysize) {
        char *tofree, *p;

        tofree = p = strdup(rss_keystr);

        for (int i = 0; i < rss_keysize; i++) {

                if (p[2] == ':') {
                        p[2] = '\0';
                }

                /*
                 * Format dictates the third char started as a ':' or '\0'.
                 * ':'s were just converted, so '\0' is the only valid option.
                 */
                if (p[2] != '\0') {
                        free(tofree);
                        return -1;
                }

                rss_key[i] = (uint8_t) strtol(p, NULL, 16);

                p = p + 3;
        }

        free(tofree);
        return 0;
}

uint32_t
toeplitz_hash(u_int keylen, const uint8_t *key, u_int datalen,
    const uint8_t *data)
{
        uint32_t hash = 0, v;
        u_int i, b;

        /* XXXRW: Perhaps an assertion about key length vs. data length? */

        v = (key[0]<<24) + (key[1]<<16) + (key[2] <<8) + key[3];
        for (i = 0; i < datalen; i++) {
                for (b = 0; b < 8; b++) {
                        if (data[i] & (1<<(7-b)))
                                hash ^= v;
                        v <<= 1;
                        if ((i + 4) < keylen &&
                            (key[i+4] & (1<<(7-b))))
                                v |= 1;
                }
        }
        return (hash);
}

/*
 * Hash an IPv4 4-tuple.
 */
uint32_t
rss_hash_ip4_4tuple(uint8_t *rss_key, u_int rss_keysize, struct in_addr src, u_short srcport, struct in_addr dst,
    u_short dstport)
{
        uint8_t data[sizeof(src) + sizeof(dst) + sizeof(srcport) +
            sizeof(dstport)];
        u_int datalen;

        datalen = 0;
        bcopy(&src, &data[datalen], sizeof(src));
        datalen += sizeof(src);
        bcopy(&dst, &data[datalen], sizeof(dst));
        datalen += sizeof(dst);
        bcopy(&srcport, &data[datalen], sizeof(srcport));
        datalen += sizeof(srcport);
        bcopy(&dstport, &data[datalen], sizeof(dstport));
        datalen += sizeof(dstport);
	return (toeplitz_hash(rss_keysize, rss_key, datalen, data));
}

/*
 * Hash an IPv6 4-tuple.
 */
uint32_t
rss_hash_ip6_4tuple(uint8_t *rss_key, u_int rss_keysize, struct in6_addr src, u_short srcport,
    struct in6_addr dst, u_short dstport)
{
        uint8_t data[sizeof(src) + sizeof(dst) + sizeof(srcport) +
            sizeof(dstport)];
        u_int datalen;

        datalen = 0;
        bcopy(&src, &data[datalen], sizeof(src));
        datalen += sizeof(src);
        bcopy(&dst, &data[datalen], sizeof(dst));
        datalen += sizeof(dst);
        bcopy(&srcport, &data[datalen], sizeof(srcport));
        datalen += sizeof(srcport);
        bcopy(&dstport, &data[datalen], sizeof(dstport));
        datalen += sizeof(dstport);
        return (toeplitz_hash(rss_keysize, rss_key, datalen, data));
}

int
main(int argc, char *argv[])
{
	struct in_addr src, dst;
	struct in6_addr src6, dst6;
	int af_family;
	u_short srcport, dstport;
	struct addrinfo *ai, a;
	int r;

	/* Lookup */
	bzero(&a, sizeof(a));
	a.ai_flags = AI_NUMERICHOST;
	a.ai_family = AF_UNSPEC;

	r = getaddrinfo(argv[1], NULL, &a, &ai);
	if (r < 0) {
		err(1, "%s: getaddrinfo(src)", argv[0]);
	}

	if (ai == NULL) {
		fprintf(stderr, "%s: src (%s) couldn't be decoded!\n", argv[0], argv[1]);
		exit(1);
	}

	af_family = -1;
	if (ai->ai_family == AF_INET) {
		af_family = AF_INET;
		printf("src=ipv4\n");
		src = ((struct sockaddr_in *) ai->ai_addr)->sin_addr;
	} else if (ai->ai_family == AF_INET6) {
		af_family = AF_INET6;
		printf("src=ipv6\n");
		src6 = ((struct sockaddr_in6 *) ai->ai_addr)->sin6_addr;
	} else {
		fprintf(stderr, "%s: src (%s) isn't ipv4 or ipv6!\n", argv[0], argv[1]);
	}

	srcport = htons(atoi(argv[2]));

	r = getaddrinfo(argv[3], NULL, &a, &ai);
	if (r < 0) {
		err(1, "%s: getaddrinfo(dst)", argv[0]);
	}

	if (ai == NULL) {
		fprintf(stderr, "%s: dst (%s) couldn't be decoded!\n", argv[0], argv[3]);
		exit(1);
	}

	/* XXX should check that this matches src type */
	if (ai->ai_family == AF_INET) {
		dst = ((struct sockaddr_in *) ai->ai_addr)->sin_addr;
	} else if (ai->ai_family == AF_INET6) {
		af_family = AF_INET6;
		dst6 = ((struct sockaddr_in6 *) ai->ai_addr)->sin6_addr;
	} else {
		fprintf(stderr, "%s: dst (%s) isn't ipv4 or ipv6!\n", argv[0], argv[3]);
	}

	dstport = htons(atoi(argv[4]));

        char *rss_keystr = argv[5];
        if (!rss_keystr) {
                rss_keystr = DEFAULT_RSS_KEYSTR;
        }
        u_int rss_keysize = (strlen(rss_keystr) + 1) / 3;
        uint8_t rss_key[rss_keysize];

        if (parse_rss_key(rss_keystr, rss_key, rss_keysize) < 0) {
                fprintf(stderr, "%s: failed to parse rss key!\n", argv[0]);
                exit(1);
        }

	if (af_family == AF_INET) {
		printf("(v4) hash: 0x%08x\n",
		    rss_hash_ip4_4tuple(rss_key, rss_keysize, src, srcport, dst, dstport));
	} else if (af_family == AF_INET6) {
		printf("(v6) hash: 0x%08x\n",
		    rss_hash_ip6_4tuple(rss_key, rss_keysize, src6, srcport, dst6, dstport));
	}

	exit (0);
}

/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * rxtx.c -- multi-stream (per-cpu, per-queue, etc.) packet captures
 */

#define _GNU_SOURCE

#include "rxtx.h"

#include "rxtx_error.h"    // for RXTX_ERRBUF_SIZE, RXTX_ERROR
#include "rxtx_savefile.h" // for rxtx_savefile_close(), rxtx_savefile_dump(),
                           //     rxtx_savefile_open()
#include "rxtx_stats.h"    // for rxtx_stats_destroy(),
                           //     rxtx_stats_destroy_with_mutex(),
                           //     rxtx_stats_get_packets_received(),
                           //     rxtx_stats_get_packets_unreliable(),
                           //     rxtx_stats_increment_packets_received(),
                           //     rxtx_stats_increment_packets_unreliable(),
                           //     rxtx_stats_init(),
                           //     rxtx_stats_init_with_mutex()

#include "ext.h"       // for ext(), noext_copy()
#include "interface.h" // for interface_set_promisc_on()
#include "sig.h"       // for keep_running

#include <arpa/inet.h>       // for htons()
#include <linux/if_packet.h> // for PACKET_FANOUT, PACKET_OUTGOING,
                             //     PACKET_RX_RING, PACKET_STATISTICS,
                             //     PACKET_TX_RING, sockaddr_ll, tpacket_req,
                             //     tpacket_stats
#include <net/ethernet.h>    // for ETH_P_ALL
#include <net/if.h>          // for if_nametoindex()
#include <sys/socket.h>      // for AF_PACKET, bind(), getsockopt(),
                             //     recvfrom(), setsockopt(), SO_RCVTIMEO,
                             //     SOCK_RAW, sockaddr, socket(), socklen_t,
                             //     SOL_PACKET, SOL_SOCKET
#include <sys/time.h>        // for timeval

#include <errno.h>    // for errno
#include <pcap.h>     // for bpf_u_int32, pcap_pkthdr
#include <pthread.h>  // for pthread_self()
#include <sched.h>    // for sched_getcpu()
#include <stdbool.h>  // for true
#include <stdio.h>    // for asprintf(), fprintf(), NULL, stderr, stdout
#include <stdlib.h>   // for calloc(), exit(), free()
#include <string.h>   // for memset(), strcmp(), strdup(), strerror(), strlen()
#include <time.h>     // for time()
#include <unistd.h>   // for getpid()

#define EXIT_FAIL 1

#define INCREMENT_STEP 1

#define PACKET_BUFFER_SIZE 65535

#define packet_direction_is_rx(sll) (!packet_direction_is_tx(sll))
#define packet_direction_is_tx(sll) ((sll)->sll_pkttype == PACKET_OUTGOING)

char errbuf[RXTX_ERRBUF_SIZE] = "";
char *program_basename = NULL;

static void rxtx_desc_init(struct rxtx_desc *p, struct rxtx_args *args);
static void rxtx_desc_destroy(struct rxtx_desc *p);
static void
rxtx_ring_init(struct rxtx_ring *p, struct rxtx_desc *rtd, int ring_idx);
static void rxtx_ring_destroy(struct rxtx_ring *p);
static void rxtx_increment_counters(struct rxtx_ring *ring);

static void rxtx_desc_init(struct rxtx_desc *p, struct rxtx_args *args) {
  int i, status;

  p->args = args;
  p->rings = calloc(args->ring_count, sizeof(*p->rings));
  p->stats = calloc(1, sizeof(*p->stats));

  status = rxtx_stats_init_with_mutex(p->stats, errbuf);
  if (status == RXTX_ERROR) {
    fprintf(stderr, "%s: %s\n", program_basename, errbuf);
    exit(EXIT_FAIL);
  }

  p->ifindex = 0;
  p->fanout_group_id = 0;

  /*
   * If we were supplied an ifname, we need to lookup the ifindex. Otherwise,
   * we want ifindex to be 0 to match any interface.
   *
   * When an ifindex lookup is needed, we want the ifindex before entering our
   * ring init loop and do not yet have a socket fd. It's more clear to create,
   * use, and discard a temporary socket fd for our SIOCGIFINDEX ioctl request
   * than to try to use one of the socket fds we'll create later.
   *
   * if_nametoindex() does exactly what we want.
   */
  if (args->ifname) {
    p->ifindex = if_nametoindex(args->ifname);

    if (!p->ifindex) {
      fprintf(
        stderr,
        "%s: Failed to get ifindex for interface '%s'\n",
        program_basename,
        args->ifname
      );
      exit(EXIT_FAIL);
    }
  }

  if (args->verbose) {
    if (p->ifindex == 0) {
      fprintf(stderr, "Using ifindex '%u' for any interface.\n", p->ifindex);
    } else {
      fprintf(
        stderr,
        "Using ifindex '%u' for interface '%s'.\n",
        p->ifindex,
        args->ifname
      );
    }
  }

  /*
   * We only have to enable promiscuity once and it will stick around for the
   * duration of the process.
   */
  if (args->promiscuous) {
    if (p->ifindex == 0) {
      if (args->verbose) {
        fprintf(
          stderr,
          "Skipping promiscuous mode for ifindex '%u' (any interface).\n",
          p->ifindex
        );
      }
    } else if (interface_set_promisc_on(p->ifindex) == -1) {
      fprintf(
        stderr,
        "%s: Failed to enable promiscuous mode for ifindex '%u'.\n",
        program_basename,
        p->ifindex
      );
      exit(EXIT_FAIL);
    }
  }

  /*
   * We need an id to add socket fds to our fanout group.
   */
  p->fanout_group_id = getpid() & 0xffff;

  if (args->verbose) {
    fprintf(stderr, "Generated fanout group id '%d'.\n", p->fanout_group_id);
  }

  /*
   * This loop creates our rings, including per-ring socket fds. We need the
   * instantiation order to follow ring index order.
   */
  for_each_ring(i, p) {
    rxtx_ring_init(&(p->rings[i]), p, i);
  }

  /*
   * Any packets which were enqueued before all socket(), bind(), and
   * setsockopt() operations were completed for our rings should be considered
   * unreliable. These packets could be from another interface (due to being
   * enqueued before bind() set the ifindex) or these packets could belong to
   * another ring (due to being enqueued before all fds are added to our fanout
   * group).
   *
   * Regardless of the reason for the packets being unreliable, we want to skip
   * them and knowing their quantity is one way to do so.
   */
  for_each_set_ring(i, p) {
    struct tpacket_stats stats;
    socklen_t len = sizeof(stats);
    if (getsockopt(p->rings[i].fd, SOL_PACKET, PACKET_STATISTICS, &stats, &len) < 0 ) {
      fprintf(
        stderr,
        "%s: Failed to get packet statistics: %s\n",
        program_basename,
        strerror(errno)
      );
      exit(EXIT_FAIL);
    }
    p->rings[i].unreliable_packet_count = stats.tp_packets - stats.tp_drops;
  }

  /*
   * Open savefiles only for rings on which we're capturing.
   */
  for_each_set_ring(i, p) {
    if (args->pcap_filename) {
      p->rings[i].savefile = calloc(1, sizeof(*p->rings[i].savefile));

      char *filename;

      if (strcmp(args->pcap_filename, "-") == 0) {
        filename = strdup(args->pcap_filename);
      } else {
        char *copy = noext_copy(args->pcap_filename);
        asprintf(
          &filename,
          "%s-%d.%s",
          copy,
          i,
          ext(args->pcap_filename)
        );
        free(copy);
      }

      status = rxtx_savefile_open(p->rings[i].savefile, filename, errbuf);
      free(filename);
      if (status == RXTX_ERROR) {
        fprintf(stderr, "%s: %s\n", program_basename, errbuf);
        exit(EXIT_FAIL);
      }
    }
  }
}

static void rxtx_desc_destroy(struct rxtx_desc *p) {
  int i;

  p->fanout_group_id = 0;
  p->ifindex = 0;
  rxtx_stats_destroy_with_mutex(p->stats);
  free(p->stats);
  p->stats = NULL;
  for_each_ring(i, p) {
    rxtx_ring_destroy(&(p->rings[i]));
  }
  free(p->rings);
  p->rings = NULL;
  p->args = NULL;
}

static void
rxtx_ring_init(struct rxtx_ring *p, struct rxtx_desc *rtd, int ring_idx) {
  p->rtd = rtd;
  p->stats = calloc(1, sizeof(*p->stats));
  rxtx_stats_init(p->stats, errbuf);
  p->idx = ring_idx;
  p->fd = -1;
  p->unreliable_packet_count = 0;

  if (rtd->args->fanout_mode != NO_PACKET_FANOUT) {
    /*
     * The AF_PACKET address family gives us a packet socket at layer 2. The
     * SOCK_RAW socket type gives us packets which include the layer 2 header.
     * The htons(ETH_P_ALL) protocol gives us all packets from any protocol.
     */
    p->fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (p->fd == -1) {
      fprintf(
        stderr,
        "%s: Failed to create socket: %s\n",
        program_basename,
        strerror(errno)
      );
      exit(EXIT_FAIL);
    }

    /*
     * We're using the default, TPACKET_V1, presently. Updating to TPACKET_V3
     * would be cool.
     *
     * PACKET_RX_RING sets up a mmapped ring buffer for async packet reception.
     * PACKET_TX_RING sets up a mmapped ring buffer for packet transmission.
     *
     * I'm not sure if packets marked with PACKET_OUTGOING end up in the rx
     * ring buffer or the tx ring buffer. PACKET_TX_RING may only be used for
     * packets sent by this application, but it doesn't seem to cost much to
     * set it up.
     */
    struct tpacket_req req;
    memset(&req, 0, sizeof(req));
    setsockopt(p->fd, SOL_PACKET, PACKET_RX_RING, (void *)&req, sizeof(req));
    setsockopt(p->fd, SOL_PACKET, PACKET_TX_RING, (void *)&req, sizeof(req));

    /*
     * SO_RCVTIMEO sets a timeout for socket i/o system calls like recvfrom().
     * This gives us an easy way to keep our worker loops from getting stuck
     * waiting on a packet which may never arrive.
     */
    struct timeval receive_timeout;
    /* no need for memset(), we're initializing every member */
    receive_timeout.tv_sec = 0;
    receive_timeout.tv_usec = 10;
    setsockopt(
      p->fd,
      SOL_SOCKET,
      SO_RCVTIMEO,
      &receive_timeout,
      sizeof(receive_timeout)
    );

    /*
     * Per packet(7), we need to set sll_family, sll_protocol, and sll_ifindex
     * in the sockaddr_ll we're passing to bind(). The values for sll_family
     * and sll_protocol are straight out of our earlier call to socket(). The
     * value for sll_ifindex is from our lookup.
     */
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = rtd->ifindex;

    if (bind(p->fd, (struct sockaddr *)&sll, sizeof(sll)) == -1) {
      fprintf(stderr, "%s: Failed to bind socket.\n", program_basename);
      exit(EXIT_FAIL);
    }

    /*
     * Add the socket to our fanout group using the fanout mode supplied.
     */
    int fanout_arg = (rtd->fanout_group_id | (rtd->args->fanout_mode << 16));
    if (setsockopt(
          p->fd,
          SOL_PACKET,
          PACKET_FANOUT,
          &fanout_arg,
          sizeof(fanout_arg)
        ) < 0) {
      fprintf(stderr, "%s: Failed to configure fanout.\n", program_basename);
      exit(EXIT_FAIL);
    }

    /*
     * TODO: Should we be calling shutdown() on sockets for rings not in our
     *       ring_set?
     *
     *         int shutdown (int socket, int how)
     */
  }
}

static void rxtx_ring_destroy(struct rxtx_ring *p) {
  /*
   * TODO: Should we be calling close() on sockets?
   *
   *         int close (int filedes)
   */
  p->fd = 0;
  rxtx_stats_destroy(p->stats);
  free(p->stats);
  p->stats = NULL;
  p->rtd = NULL;
  if (p->savefile) {
    int status = rxtx_savefile_close(p->savefile);
    free(p->savefile);
    if (status == RXTX_ERROR) {
      fprintf(stderr, "%s: %s\n", program_basename, errbuf);
      exit(EXIT_FAIL);
    }
  }
  p->savefile = NULL;
  p->idx = 0;
}

static void rxtx_increment_counters(struct rxtx_ring *ring) {
  rxtx_stats_increment_packets_received(ring->rtd->stats, INCREMENT_STEP);
  rxtx_stats_increment_packets_received(ring->stats, INCREMENT_STEP);
}

int rxtx_open(struct rxtx_desc *rtd, struct rxtx_args *args) {
  rxtx_desc_init(rtd, args);
  return 0;
}

int rxtx_close(struct rxtx_desc *rtd) {
  rxtx_desc_destroy(rtd);
  return 0;
}

void *rxtx_loop(void *r) {
  struct rxtx_ring *ring = r;
  struct rxtx_desc *rtd = ring->rtd;
  struct rxtx_savefile *savefile = ring->savefile;
  struct rxtx_args *args = rtd->args;

  if (args->verbose) {
    fprintf(
      stderr,
      "Worker '%lu' handling ring '%d' running on cpu '%d'.\n",
      pthread_self(),
      ring->idx,
      sched_getcpu()
    );
  }

  int seen_empty_ring = 0;
  while (keep_running) {

    if (args->packet_count &&
        rxtx_stats_get_packets_received(rtd->stats) >= args->packet_count) {
      break;
    }

    struct sockaddr_ll sll;
    unsigned char packet_buffer[PACKET_BUFFER_SIZE];

    unsigned int sll_length = sizeof(sll);
    int packet_length = recvfrom(
      ring->fd,
      packet_buffer,
      sizeof(packet_buffer),
      0,
      (struct sockaddr *)&sll,
      &sll_length
    );

    if (packet_length == -1) {
      seen_empty_ring = 1;
      continue;
    }

    /*
     * If we've seen the ring buffer go empty, we know all unreliable packets
     * have been skipped. Alternatively, if we've directly processed all
     * unreliable packets, we know all unreliable packets have been skipped.
     * Otherwise, this packet should be seen as unreliable.
     */
    if (!seen_empty_ring &&
        rxtx_stats_get_packets_unreliable(ring->stats) < ring->unreliable_packet_count) {
      rxtx_stats_increment_packets_unreliable(ring->stats, INCREMENT_STEP);
      continue;
    }

    if (!args->capture_rx && packet_direction_is_rx(&sll)) {
      continue;
    }

    if (!args->capture_tx && packet_direction_is_tx(&sll)) {
      continue;
    }

    rxtx_increment_counters(ring);

    if (savefile) {
      /* no need for memset(), we're initializing every member */
      struct pcap_pkthdr pcap_packet_header;
      pcap_packet_header.caplen     = (bpf_u_int32)packet_length;
      pcap_packet_header.len        = (bpf_u_int32)packet_length;
      pcap_packet_header.ts.tv_sec  = time(NULL);
      pcap_packet_header.ts.tv_usec = 0;

      int status;
      status = rxtx_savefile_dump(savefile, &pcap_packet_header, packet_buffer, args->packet_buffered);

      if (status == RXTX_ERROR) {
        fprintf(stderr, "%s: %s\n", program_basename, errbuf);
        exit(EXIT_FAIL);
      }
    }
  }
  return NULL;
}

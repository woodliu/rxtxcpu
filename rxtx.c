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
#include "rxtx_ring.h"     // for rxtx_ring_clear_unreliable_packets_in_buffer(),
                           //     rxtx_ring_destroy(), rxtx_ring_init(),
                           //     rxtx_ring_mark_packets_in_buffer_as_unreliable(),
                           //     rxtx_ring_savefile_open()
#include "rxtx_savefile.h" // for rxtx_savefile_dump()
#include "rxtx_stats.h"    // for rxtx_stats_destroy_with_mutex(),
                           //     rxtx_stats_get_packets_received(),
                           //     rxtx_stats_get_packets_unreliable(),
                           //     rxtx_stats_increment_packets_received(),
                           //     rxtx_stats_increment_packets_unreliable(),
                           //     rxtx_stats_init_with_mutex()

#include "interface.h" // for interface_set_promisc_on()
#include "sig.h"       // for keep_running

#include <linux/if_packet.h> // for PACKET_OUTGOING, sockaddr_ll
#include <net/if.h>          // for if_nametoindex()
#include <sys/socket.h>      // for recvfrom(), setsockopt(), sockaddr,

#include <pcap.h>     // for bpf_u_int32, PCAP_D_IN, PCAP_D_OUT, pcap_pkthdr
#include <pthread.h>  // for pthread_self()
#include <sched.h>    // for sched_getcpu()
#include <stdio.h>    // for fprintf(), NULL, stderr
#include <stdlib.h>   // for calloc(), exit(), free()
#include <time.h>     // for time()
#include <unistd.h>   // for getpid()

#define EXIT_FAIL 1

#define INCREMENT_STEP 1

#define PACKET_BUFFER_SIZE 65535

#define packet_direction_is_rx(sll) (!packet_direction_is_tx(sll))
#define packet_direction_is_tx(sll) ((sll)->sll_pkttype == PACKET_OUTGOING)

char errbuf[RXTX_ERRBUF_SIZE] = "";
char *program_basename = NULL;

volatile sig_atomic_t rxtx_breakloop = 0;

/* ========================================================================= */
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
  p->breakloop = 0;

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

  p->initialized_ring_count = 0;

  /*
   * This loop creates our rings, including per-ring socket fds. We need the
   * instantiation order to follow ring index order.
   */
  for_each_ring(i, p) {
    status = rxtx_ring_init(&(p->rings[i]), p, errbuf);
    if (status == RXTX_ERROR) {
      fprintf(stderr, "%s: %s\n", program_basename, errbuf);
      exit(EXIT_FAIL);
    }
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
   * them in our workers and need to record which are unreliable sooner rather
   * than later.
   */
  for_each_set_ring(i, p) {
    status = rxtx_ring_mark_packets_in_buffer_as_unreliable(&(p->rings[i]));
    if (status == RXTX_ERROR) {
      fprintf(stderr, "%s: %s\n", program_basename, errbuf);
      exit(EXIT_FAIL);
    }
  }

  /*
   * Open savefiles only for rings on which we're capturing.
   */
  for_each_set_ring(i, p) {
    if (args->savefile_template) {
      status = rxtx_ring_savefile_open(&(p->rings[i]), args->savefile_template);
      if (status == RXTX_ERROR) {
        fprintf(stderr, "%s: %s\n", program_basename, errbuf);
        exit(EXIT_FAIL);
      }
    }
  }
}

/* ========================================================================= */
static void rxtx_desc_destroy(struct rxtx_desc *p) {
  int i, status;

  p->breakloop = 0;
  p->fanout_group_id = 0;
  p->ifindex = 0;
  rxtx_stats_destroy_with_mutex(p->stats);
  free(p->stats);
  p->stats = NULL;
  for_each_ring(i, p) {
    status = rxtx_ring_destroy(&(p->rings[i]));
    if (status == RXTX_ERROR) {
      fprintf(stderr, "%s: %s\n", program_basename, errbuf);
      exit(EXIT_FAIL);
    }
  }
  free(p->rings);
  p->rings = NULL;
  p->args = NULL;
}

/* ========================================================================= */
static void rxtx_increment_counters(struct rxtx_ring *ring) {
  rxtx_stats_increment_packets_received(ring->rtd->stats, INCREMENT_STEP);
  rxtx_stats_increment_packets_received(ring->stats, INCREMENT_STEP);
}

/* ========================================================================= */
int rxtx_open(struct rxtx_desc *rtd, struct rxtx_args *args) {
  rxtx_desc_init(rtd, args);
  return 0;
}

/* ========================================================================= */
int rxtx_close(struct rxtx_desc *rtd) {
  rxtx_desc_destroy(rtd);
  return 0;
}


/* ---------------------------- start of getters --------------------------- */
/* ========================================================================= */
int rxtx_breakloop_isset(struct rxtx_desc *p) {
  if (rxtx_breakloop) {
    return -1;
  }
  return p->breakloop;
}

/* ========================================================================= */
int rxtx_get_initialized_ring_count(struct rxtx_desc *p) {
  return p->initialized_ring_count;
}
/* ----------------------------- end of getters ---------------------------- */

/* ---------------------------- start of setters --------------------------- */
/* ========================================================================= */
void rxtx_increment_initialized_ring_count(struct rxtx_desc *p) {
  p->initialized_ring_count++;
}

/* ========================================================================= */
void rxtx_set_breakloop(struct rxtx_desc *p) {
  p->breakloop++;
}

/* ========================================================================= */
void rxtx_set_breakloop_global(void) {
  rxtx_breakloop = 1;
}
/* ----------------------------- end of setters ---------------------------- */

/* ========================================================================= */
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

  rxtx_ring_clear_unreliable_packets_in_buffer(ring);

  while (!rxtx_breakloop_isset(rtd)) {

    if (args->packet_count &&
        rxtx_stats_get_packets_received(rtd->stats) >= args->packet_count) {
      break;
    }

    struct sockaddr_ll sll;
    unsigned char packet[PACKET_BUFFER_SIZE];

    unsigned int sll_length = sizeof(sll);
    int packet_length = recvfrom(
      ring->fd,
      packet,
      sizeof(packet),
      0,
      (struct sockaddr *)&sll,
      &sll_length
    );

    if (packet_length == -1) {
      continue;
    }

    if (args->direction == PCAP_D_OUT && packet_direction_is_rx(&sll)) {
      continue;
    }

    if (args->direction == PCAP_D_IN && packet_direction_is_tx(&sll)) {
      continue;
    }

    rxtx_increment_counters(ring);

    if (savefile) {
      /* no need for memset(), we're initializing every member */
      struct pcap_pkthdr header;
      header.caplen     = (bpf_u_int32)packet_length;
      header.len        = (bpf_u_int32)packet_length;
      header.ts.tv_sec  = time(NULL);
      header.ts.tv_usec = 0;

      int status;
      status = rxtx_savefile_dump(savefile, &header, packet, args->packet_buffered);

      if (status == RXTX_ERROR) {
        fprintf(stderr, "%s: %s\n", program_basename, errbuf);
        exit(EXIT_FAIL);
      }
    }
  }
  return NULL;
}

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

#include "rxtx_error.h" // for RXTX_ERRBUF_SIZE, RXTX_ERROR
#include "rxtx_ring.h" // for rxtx_ring_destroy(), rxtx_ring_init(),
                       //     rxtx_ring_mark_packets_in_buffer_as_unreliable(),
                       //     rxtx_ring_savefile_open()
#include "rxtx_stats.h" // for rxtx_stats_destroy_with_mutex(),
                        //     rxtx_stats_get_packets_received(),
                        //     rxtx_stats_get_packets_unreliable(),
                        //     rxtx_stats_increment_packets_received(),
                        //     rxtx_stats_increment_packets_unreliable(),
                        //     rxtx_stats_init_with_mutex()

#include "interface.h" // for interface_set_promisc_on()
#include "sig.h"       // for keep_running

#include <net/if.h>     // for if_nametoindex()
#include <sys/socket.h> // for setsockopt()

#include <sched.h>  // for sched_getcpu()
#include <stdio.h>  // for fprintf(), NULL, stderr
#include <stdlib.h> // for calloc(), exit(), free()
#include <unistd.h> // for getpid()

#define EXIT_FAIL 1

#define INCREMENT_STEP 1

char errbuf[RXTX_ERRBUF_SIZE] = "";
char *program_basename = NULL;

volatile sig_atomic_t rxtx_breakloop = 0;

/* ========================================================================= */
static void rxtx_desc_init(struct rxtx_desc *p, struct rxtx_args *args) {
  int i, status;

  p->errbuf = errbuf;

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
      fprintf(stderr, "%s: Failed to get ifindex for interface '%s'\n",
                                               program_basename, args->ifname);
      exit(EXIT_FAIL);
    }
  }

  if (args->verbose) {
    if (p->ifindex == 0) {
      fprintf(stderr, "Using ifindex '%u' for any interface.\n", p->ifindex);
    } else {
      fprintf(stderr, "Using ifindex '%u' for interface '%s'.\n", p->ifindex,
                                                                 args->ifname);
    }
  }

  /*
   * We only have to enable promiscuity once and it will stick around for the
   * duration of the process.
   */
  if (args->promiscuous) {
    if (p->ifindex == 0) {
      if (args->verbose) {
        fprintf(stderr, "Skipping promiscuous mode for ifindex '%u' (any"
                                                 " interface).\n", p->ifindex);
      }
    } else {
      status = interface_set_promisc_on(p->ifindex);
      if (status == -1) {
        fprintf(stderr, "%s: Failed to enable promiscuous mode for ifindex"
                                     " '%u'.\n", program_basename, p->ifindex);
        exit(EXIT_FAIL);
      }
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
      status = rxtx_ring_savefile_open(&(p->rings[i]),
                                                      args->savefile_template);
      if (status == RXTX_ERROR) {
        fprintf(stderr, "%s: %s\n", program_basename, errbuf);
        exit(EXIT_FAIL);
      }
    }
  }
}

/* ========================================================================= */
int rxtx_open(struct rxtx_desc *p, struct rxtx_args *args) {
  rxtx_desc_init(p, args);
  return 0;
}

/* ========================================================================= */
int rxtx_close(struct rxtx_desc *p) {
  int i, status;

  p->breakloop = 0;
  p->fanout_group_id = 0;
  p->ifindex = 0;

  status = rxtx_stats_destroy_with_mutex(p->stats);
  if (status == RXTX_ERROR) {
    return RXTX_ERROR;
  }

  free(p->stats);
  p->stats = NULL;

  for_each_ring(i, p) {
    status = rxtx_ring_destroy(&(p->rings[i]));
    if (status == RXTX_ERROR) {
      return RXTX_ERROR;
    }
  }

  free(p->rings);
  p->rings = NULL;

  p->args = NULL;
  p->errbuf = NULL;

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
pcap_direction_t rxtx_get_direction(struct rxtx_desc *p) {
  return p->args->direction;
}

/* ========================================================================= */
int rxtx_get_fanout_arg(struct rxtx_desc *p) {
  return p->fanout_group_id | (p->args->fanout_mode << 16);
}

/* ========================================================================= */
unsigned int rxtx_get_ifindex(struct rxtx_desc *p) {
  return p->ifindex;
}

/* ========================================================================= */
int rxtx_get_initialized_ring_count(struct rxtx_desc *p) {
  return p->initialized_ring_count;
}

/* ========================================================================= */
uintmax_t rxtx_get_packets_received(struct rxtx_desc *p) {
  return rxtx_stats_get_packets_received(p->stats);
}

/* ========================================================================= */
int rxtx_packet_buffered_isset(struct rxtx_desc *p) {
  return p->args->packet_buffered;
}

/* ========================================================================= */
int rxtx_packet_count_reached(struct rxtx_desc *p) {
  if (!p->args->packet_count) {
    return 0;
  }

  if (rxtx_stats_get_packets_received(p->stats) < p->args->packet_count) {
    return 0;
  }

  return 1;
}

/* ========================================================================= */
int rxtx_verbose_isset(struct rxtx_desc *p) {
  return p->args->verbose;
}
/* ----------------------------- end of getters ---------------------------- */

/* ---------------------------- start of setters --------------------------- */
/* ========================================================================= */
void rxtx_increment_initialized_ring_count(struct rxtx_desc *p) {
  p->initialized_ring_count++;
}

/* ========================================================================= */
int rxtx_increment_packets_received(struct rxtx_desc *p) {
  int status = rxtx_stats_increment_packets_received(p->stats, INCREMENT_STEP);
  if (status == RXTX_ERROR) {
    return RXTX_ERROR;
  }

  return 0;
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

/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * rxtx.h -- header file to use rxtx.c
 */

#ifndef _RXTX_H_
#define _RXTX_H_

struct rxtx_desc;
struct rxtx_ring;

#include "rxtx_ring.h"     // for rxtx_ring
#include "rxtx_stats.h"    // for rxtx_stats

#include "ring_set.h" // for for_each_ring_in_size(),
                      //     for_each_set_ring_in_size()

#include <pcap.h>    // for pcap_direction_t
#include <pthread.h> // for pthread_mutex_t
#include <sched.h>   // for cpu_set_t
#include <signal.h> // for sig_atomic_t
#include <stdbool.h> // for bool
#include <stdint.h>  // for uintmax_t

#define NO_PACKET_FANOUT -1

#define for_each_ring(ring, rtd) \
  for_each_ring_in_size((ring), (rtd)->args->ring_count)

#define for_each_set_ring(ring, rtd) \
  for_each_set_ring_in_size((ring), &((rtd)->args->ring_set), (rtd)->args->ring_count)

extern char *program_basename;
extern volatile sig_atomic_t rxtx_breakloop;

struct rxtx_args {
  pcap_direction_t direction;
  char      *ifname;
  int       fanout_mode;
  char      *savefile_template;
  uintmax_t packet_count;
  int       ring_count;
  cpu_set_t ring_set;
  bool      packet_buffered;
  bool      promiscuous;
  bool      verbose;
};

struct rxtx_desc {
  struct rxtx_args  *args;
  struct rxtx_ring  *rings;
  struct rxtx_stats *stats;
  unsigned int      ifindex;
  int               fanout_group_id;
  int               initialized_ring_count;
  int               breakloop;
};

int rxtx_open(struct rxtx_desc *rtd, struct rxtx_args *args);
int rxtx_close(struct rxtx_desc *rtd);
void *rxtx_loop(void *r);

int rxtx_breakloop_isset(struct rxtx_desc *p);
int rxtx_get_initialized_ring_count(struct rxtx_desc *rtd);

void rxtx_increment_initialized_ring_count(struct rxtx_desc *rtd);
void rxtx_set_breakloop(struct rxtx_desc *p);
void rxtx_set_breakloop_global(void);

#endif // _RXTX_H_

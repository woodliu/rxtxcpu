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

#include "rxtx_ring.h"  // for rxtx_ring
#include "rxtx_stats.h" // for rxtx_stats

#include "ring_set.h" // for for_each_ring_in_size(),
                      //     for_each_set_ring_in_size()

#include <pcap.h>    // for pcap_direction_t
#include <pthread.h> // for pthread_mutex_t
#include <sched.h>   // for cpu_set_t
#include <signal.h>  // for sig_atomic_t
#include <stdbool.h> // for bool
#include <stdint.h>  // for uintmax_t

#define for_each_ring(ring, rtd) \
  for_each_ring_in_size((ring), (rtd)->ring_count)

#define for_each_set_ring(ring, rtd) \
  for_each_set_ring_in_size((ring), &((rtd)->ring_set), (rtd)->ring_count)

extern char *program_basename;
extern volatile sig_atomic_t rxtx_breakloop;

struct rxtx_desc {
  struct rxtx_ring  *rings;
  struct rxtx_stats *stats;

  char *ifname;
  char *savefile_template;

  int              breakloop;
  pcap_direction_t direction;
  int              fanout_group_id;
  int              fanout_mode;
  unsigned int     ifindex;
  int              initialized_ring_count;
  int              is_active;
  uintmax_t        packet_count;
  int              ring_count;
  cpu_set_t        ring_set;
  int              packet_buffered;
  int              promiscuous;
  int              verbose;

  char *errbuf;
};

void rxtx_init(struct rxtx_desc *p, char *errbuf);
int rxtx_activate(struct rxtx_desc *p);
int rxtx_close(struct rxtx_desc *p);

int rxtx_breakloop_isset(struct rxtx_desc *p);
pcap_direction_t rxtx_get_direction(struct rxtx_desc *p);
int rxtx_get_fanout_arg(struct rxtx_desc *p);
int rxtx_get_fanout_group_id(struct rxtx_desc *p);
int rxtx_get_fanout_mode(struct rxtx_desc *p);
unsigned int rxtx_get_ifindex(struct rxtx_desc *p);
const char *rxtx_get_ifname(struct rxtx_desc *p);
int rxtx_get_initialized_ring_count(struct rxtx_desc *p);
uintmax_t rxtx_get_packet_count(struct rxtx_desc *p);
uintmax_t rxtx_get_packets_received(struct rxtx_desc *p);
struct rxtx_ring *rxtx_get_ring(struct rxtx_desc *p, unsigned int idx);
int rxtx_get_ring_count(struct rxtx_desc *p);
const ring_set_t *rxtx_get_ring_set(struct rxtx_desc *p);
const char *rxtx_get_savefile_template(struct rxtx_desc *p);
int rxtx_packet_buffered_isset(struct rxtx_desc *p);
int rxtx_packet_count_reached(struct rxtx_desc *p);
int rxtx_promiscuous_isset(struct rxtx_desc *p);
int rxtx_verbose_isset(struct rxtx_desc *p);

int rxtx_increment_initialized_ring_count(struct rxtx_desc *p);
int rxtx_increment_packets_received(struct rxtx_desc *p);
int rxtx_set_breakloop(struct rxtx_desc *p);
void rxtx_set_breakloop_global(void);
int rxtx_set_direction(struct rxtx_desc *p, pcap_direction_t direction);
int rxtx_set_fanout_group_id(struct rxtx_desc *p, int group_id);
int rxtx_set_fanout_mode(struct rxtx_desc *p, int mode);
int rxtx_set_ifindex(struct rxtx_desc *p, unsigned int ifindex);
int rxtx_set_ifname(struct rxtx_desc *p, const char *ifname);
int rxtx_set_packet_count(struct rxtx_desc *p, uintmax_t count);
int rxtx_set_ring_count(struct rxtx_desc *p, unsigned int count);
int rxtx_set_ring_set(struct rxtx_desc *p, const ring_set_t *set);
int rxtx_set_savefile_template(struct rxtx_desc *p, const char *template);
int rxtx_set_packet_buffered(struct rxtx_desc *p);
int rxtx_set_promiscuous(struct rxtx_desc *p);
void rxtx_set_verbose(struct rxtx_desc *p);
int rxtx_unset_packet_buffered(struct rxtx_desc *p);
int rxtx_unset_promiscuous(struct rxtx_desc *p);
void rxtx_unset_verbose(struct rxtx_desc *p);

#endif // _RXTX_H_

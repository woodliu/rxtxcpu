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

#include <pcap.h>    // for pcap_dumper_t, pcap_t
#include <pthread.h> // for pthread_mutex_t
#include <sched.h>   // for cpu_set_t
#include <stdbool.h> // for bool
#include <stdint.h>  // for uintmax_t

#define NO_PACKET_FANOUT -1

extern char *program_basename;

struct rxtx_args {
  bool      capture_rx;
  bool      capture_tx;
  char      *ifname;
  int       fanout_mode;
  char      *pcap_filename;
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
};

struct rxtx_pcap {
  pcap_t          *desc;
  char            *filename;
  pcap_dumper_t   *fp;
  pthread_mutex_t *mutex;
  int             owner_idx;
};

struct rxtx_ring {
  struct rxtx_pcap  *pcap;
  struct rxtx_desc  *rtd;
  struct rxtx_stats *stats;
  int               idx;
  int               fd;
};

struct rxtx_stats {
  uintmax_t       packets_received;
  pthread_mutex_t *mutex;
};

int rxtx_close(struct rxtx_desc *rtd);
void *rxtx_loop(void *r);
int rxtx_open(struct rxtx_desc *rtd, struct rxtx_args *args);

#endif // _RXTX_H_

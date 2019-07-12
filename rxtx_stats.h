/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef _RXTX_STATS_H_
#define _RXTX_STATS_H_

#include <pthread.h> // for pthread_mutex_t
#include <stdint.h>  // for uintmax_t

struct rxtx_stats {
  uintmax_t packets_received;
  uintmax_t packets_unreliable;
  pthread_mutex_t *mutex;
  char *errbuf;
};

int rxtx_stats_init(struct rxtx_stats *p, char *errbuf);
int rxtx_stats_init_with_mutex(struct rxtx_stats *p, char *errbuf);
int rxtx_stats_destroy(struct rxtx_stats *p);
int rxtx_stats_destroy_with_mutex(struct rxtx_stats *p);
int rxtx_stats_mutex_init(struct rxtx_stats *p);
int rxtx_stats_mutex_destroy(struct rxtx_stats *p);
uintmax_t rxtx_stats_get_packets_received(struct rxtx_stats *p);
uintmax_t rxtx_stats_get_packets_unreliable(struct rxtx_stats *p);
int rxtx_stats_increment_packets_received(struct rxtx_stats *p, int step);
int rxtx_stats_increment_packets_unreliable(struct rxtx_stats *p, int step);

#endif // _RXTX_STATS_H_

/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef _RXTX_RING_H_
#define _RXTX_RING_H_

struct rxtx_desc;
struct rxtx_ring;

#include "rxtx.h"          // for rxtx_desc
#include "rxtx_savefile.h" // for rxtx_savefile
#include "rxtx_stats.h"    // for rxtx_stats

struct rxtx_ring {
  struct rxtx_desc  *rtd;
  struct rxtx_savefile *savefile;
  struct rxtx_stats *stats;
  int               idx;
  int               fd;
  unsigned int      unreliable;
  char              *errbuf;
};

int rxtx_ring_init(struct rxtx_ring *p, struct rxtx_desc *rtd, char *errbuf);
int rxtx_ring_destroy(struct rxtx_ring *p);
void rxtx_ring_clear_unreliable_packets_in_buffer(struct rxtx_ring *p);
int rxtx_ring_mark_packets_in_buffer_as_unreliable(struct rxtx_ring *p);
int rxtx_ring_savefile_open(struct rxtx_ring *p, const char *template);
int rxtx_ring_update_tpacket_stats(struct rxtx_ring *p);

#endif // _RXTX_RING_H_

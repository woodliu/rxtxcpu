/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef _RXTX_RING_H_
#define _RXTX_RING_H_

#include "rxtx.h" // for rxtx_ring

int rxtx_ring_destroy(struct rxtx_ring *p);
int rxtx_ring_mark_packets_in_buffer_as_unreliable(struct rxtx_ring *p);
int rxtx_ring_savefile_open(struct rxtx_ring *p, const char *template);
int rxtx_ring_update_tpacket_stats(struct rxtx_ring *p);

#endif // _RXTX_RING_H_

/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "rxtx_ring.h"
#include "rxtx_error.h" // for RXTX_ERROR, rxtx_fill_errbuf()
#include "rxtx_stats.h" // for rxtx_stats_increment_tp_packets(),
                        //     rxtx_stats_increment_tp_drops()

#include <linux/if_packet.h> // for PACKET_STATISTICS, tpacket_stats
#include <sys/socket.h>      // for getsockopt(), socklen_t, SOL_PACKET

#include <string.h> // for strerror()
#include <errno.h>  // for errno

int rxtx_ring_mark_packets_in_buffer_as_unreliable(struct rxtx_ring *p) {
  int status = rxtx_ring_update_tpacket_stats(p);
  if (status == RXTX_ERROR) {
    return RXTX_ERROR;
  }

  p->unreliable_packet_count = rxtx_stats_get_tp_packets(p->stats)
                                 - rxtx_stats_get_tp_drops(p->stats);

  return 0;
}

int rxtx_ring_update_tpacket_stats(struct rxtx_ring *p) {
  int status = 0;
  struct tpacket_stats tp_stats;
  socklen_t len = sizeof(tp_stats);

  status = getsockopt(p->fd, SOL_PACKET, PACKET_STATISTICS, &tp_stats, &len);
  if (status < 0) {
    rxtx_fill_errbuf(p->errbuf, "error collecting packet statistics: %s", strerror(errno));
    return RXTX_ERROR;
  }

  status = rxtx_stats_increment_tp_packets(p->stats, tp_stats.tp_packets);
  if (status == RXTX_ERROR) {
    return RXTX_ERROR;
  }

  return rxtx_stats_increment_tp_drops(p->stats, tp_stats.tp_drops);
}

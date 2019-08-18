/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define _GNU_SOURCE

#include "rxtx_ring.h"
#include "rxtx_error.h"    // for RXTX_ERROR, rxtx_fill_errbuf()
#include "rxtx_savefile.h" // for rxtx_savefile_open()
#include "rxtx_stats.h"    // for rxtx_stats_increment_tp_packets(),
                           //     rxtx_stats_increment_tp_drops()

#include "ext.h" // for ext(), noext_copy()

#include <linux/if_packet.h> // for PACKET_STATISTICS, tpacket_stats
#include <sys/socket.h>      // for getsockopt(), socklen_t, SOL_PACKET

#include <stdio.h>  // for asprintf()
#include <stdlib.h> // for calloc(), free()
#include <string.h> // for strcmp(), strdup(), strerror()
#include <errno.h>  // for errno

int rxtx_ring_mark_packets_in_buffer_as_unreliable(struct rxtx_ring *p) {
  int status = rxtx_ring_update_tpacket_stats(p);
  if (status == RXTX_ERROR) {
    return RXTX_ERROR;
  }

  p->unreliable = rxtx_stats_get_tp_packets(p->stats)
                    - rxtx_stats_get_tp_drops(p->stats);

  return 0;
}

int rxtx_ring_savefile_open(struct rxtx_ring *p, const char *template) {
  int status = 0;
  char *filename = NULL;
  char *noext = NULL;

  if (!template) {
    rxtx_fill_errbuf(p->errbuf, "error opening savefile: invalid template");
    return RXTX_ERROR;
  }

  p->savefile = calloc(1, sizeof(*p->savefile));
  if (!p->savefile) {
    rxtx_fill_errbuf(p->errbuf, "error opening savefile: %s", strerror(errno));
    return RXTX_ERROR;
  }

  if (strcmp(template, "-") == 0) {
    filename = strdup(template);
    if (!filename) {
      rxtx_fill_errbuf(p->errbuf, "error opening savefile: %s", strerror(errno));
      return RXTX_ERROR;
    }
  } else {
    noext = noext_copy(template);
    if (!noext) {
      rxtx_fill_errbuf(p->errbuf, "error opening savefile: %s", strerror(errno));
      return RXTX_ERROR;
    }

    status = asprintf(&filename, "%s-%d.%s", noext, p->idx, ext(template));
    if (status == -1) {
      rxtx_fill_errbuf(p->errbuf, "error opening savefile: %s", strerror(errno));
      return RXTX_ERROR;
    }

    free(noext);
  }

  status = rxtx_savefile_open(p->savefile, filename, p->errbuf);
  if (status == RXTX_ERROR) {
    return RXTX_ERROR;
  }

  free(filename);

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

/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "rxtx_stats.h"
#include "rxtx_error.h" // for RXTX_ERROR, rxtx_fill_errbuf()

#include <stdlib.h> // for calloc(), free()

#ifdef TESTING
  #include "tests/rxtx_stats/helper.h"
#endif

int rxtx_stats_init(struct rxtx_stats *p, char *errbuf) {
  p->errbuf = errbuf;
  p->packets_received = 0;
  p->packets_unreliable = 0;

  return 0;
}

int rxtx_stats_destroy(struct rxtx_stats *p) {
  p->packets_unreliable = 0;
  p->packets_received = 0;
  p->errbuf = NULL;

  return 0;
}

int rxtx_stats_mutex_init(struct rxtx_stats *p) {
  p->mutex = calloc(1, sizeof(*p->mutex));

  if (!p->mutex) {
    rxtx_fill_errbuf(p->errbuf, "error allocating memory for stats mutex");
    return RXTX_ERROR;
  }

  if (pthread_mutex_init(p->mutex, NULL) != 0) {
    rxtx_fill_errbuf(p->errbuf, "error initializing stats mutex");
    return RXTX_ERROR;
  }

  return 0;
}

int rxtx_stats_mutex_destroy(struct rxtx_stats *p) {
  if (pthread_mutex_destroy(p->mutex) != 0) {
    rxtx_fill_errbuf(p->errbuf, "error destroying stats mutex");
    return RXTX_ERROR;
  }

  free(p->mutex);
  p->mutex = NULL;

  return 0;
}

int rxtx_stats_init_with_mutex(struct rxtx_stats *p, char *errbuf) {
  rxtx_stats_init(p, errbuf);
  return rxtx_stats_mutex_init(p);
}

int rxtx_stats_destroy_with_mutex(struct rxtx_stats *p) {
  int status = rxtx_stats_mutex_destroy(p);
  rxtx_stats_destroy(p);
  return status;
}

uintmax_t rxtx_stats_get_packets_received(struct rxtx_stats *p) {
  return p->packets_received;
}

uintmax_t rxtx_stats_get_packets_unreliable(struct rxtx_stats *p) {
  return p->packets_unreliable;
}

int rxtx_stats_increment_packets_received(struct rxtx_stats *p, int step) {
  if (p->mutex && pthread_mutex_lock(p->mutex) != 0) {
    rxtx_fill_errbuf(p->errbuf, "error locking stats mutex");
    return RXTX_ERROR;
  }

  p->packets_received += step;

  if (p->mutex && pthread_mutex_unlock(p->mutex) != 0) {
    rxtx_fill_errbuf(p->errbuf, "error unlocking stats mutex");
    return RXTX_ERROR;
  }

  return 0;
}

int rxtx_stats_increment_packets_unreliable(struct rxtx_stats *p, int step) {
  if (p->mutex && pthread_mutex_lock(p->mutex) != 0) {
    rxtx_fill_errbuf(p->errbuf, "error locking stats mutex");
    return RXTX_ERROR;
  }

  p->packets_unreliable += step;

  if (p->mutex && pthread_mutex_unlock(p->mutex) != 0) {
    rxtx_fill_errbuf(p->errbuf, "error unlocking stats mutex");
    return RXTX_ERROR;
  }

  return 0;
}

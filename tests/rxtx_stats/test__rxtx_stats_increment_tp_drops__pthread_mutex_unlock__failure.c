/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "../../rxtx_error.h"
#include "../../rxtx_stats.h"

#include <assert.h>
#include <string.h>

int main(void) {

  struct rxtx_stats rts;
  char errbuf[RXTX_ERRBUF_SIZE];
  int status;

  status = rxtx_stats_init(&rts, errbuf);
  assert(status == 0);

  status = rxtx_stats_mutex_init(&rts);
  assert(status == 0);

  status = rxtx_stats_increment_tp_drops(&rts, 1);
  assert(status == -1);

  status = strcmp(errbuf, "error unlocking stats mutex: Invalid argument");
  assert(status == 0);

  rxtx_stats_mutex_destroy(&rts);

  rxtx_stats_destroy(&rts);

  return 0;
}

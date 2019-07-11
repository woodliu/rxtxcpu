/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "../../rxtx_error.h"
#include "../../rxtx_savefile.h"

#include <assert.h>
#include <string.h>

int main(void) {

  struct rxtx_savefile rtp;
  char errbuf[RXTX_ERRBUF_SIZE];
  int status;

  status = rxtx_savefile_open(&rtp, "/dev/null", errbuf);
  assert(status == -1);

  status = strcmp(errbuf, "error opening savefile '/dev/null'");
  assert(status == 0);

  return 0;
}

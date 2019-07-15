/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ring_set.h"

int find_next_set_ring(int ring, ring_set_t *ring_set) {
  for (; ring < RING_SETSIZE; ring++) {
    if (RING_ISSET(ring, ring_set)) {
      return ring;
    }
  }
  return RING_SETSIZE;
}

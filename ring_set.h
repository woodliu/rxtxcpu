/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef _RING_SET_H_
#define _RING_SET_H_

#define _GNU_SOURCE

#include <sched.h> // for CPU_AND(), CPU_CLR(), CPU_COUNT(), CPU_EQUAL(),
                   //     CPU_ISSET(), CPU_OR(), CPU_SET(), CPU_SETSIZE,
                   //     cpu_set_t, CPU_XOR(), CPU_ZERO()

#define ring_set_t      cpu_set_t

#define RING_SETSIZE    CPU_SETSIZE

#define RING_CLR(...)   CPU_CLR(__VA_ARGS__)
#define RING_COUNT(...) CPU_COUNT(__VA_ARGS__)
#define RING_ISSET(...) CPU_ISSET(__VA_ARGS__)
#define RING_SET(...)   CPU_SET(__VA_ARGS__)
#define RING_ZERO(...)  CPU_ZERO(__VA_ARGS__)

#define RING_AND(...)   CPU_AND(__VA_ARGS__)
#define RING_EQUAL(...) CPU_EQUAL(__VA_ARGS__)
#define RING_OR(...)    CPU_OR(__VA_ARGS__)
#define RING_XOR(...)   CPU_XOR(__VA_ARGS__)

#define for_each_ring_in_range(ring, first, last) \
  for (                                           \
    (ring) = (first);                             \
    (ring) <= (last) && (ring) < RING_SETSIZE;    \
    (ring)++                                      \
  )

#define for_each_ring_in_size(ring, cardinality) \
  for_each_ring_in_range((ring), 0, (cardinality) - 1)

int find_next_set_ring(int ring, ring_set_t *ring_set);

#define for_each_set_ring_in_range(ring, ring_set, first, last) \
  for (                                                         \
    (ring) = find_next_set_ring((first), (ring_set));           \
    (ring) <= (last) && (ring) < RING_SETSIZE;                  \
    (ring) = find_next_set_ring((ring) + 1, (ring_set))         \
  )

#define for_each_set_ring_in_size(ring, ring_set, cardinality) \
  for_each_set_ring_in_range((ring), (ring_set), 0, (cardinality) - 1)

#endif // _RING_SET_H_

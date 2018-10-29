/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * worker.h -- header file to use worker.c
 */

#ifndef _WORKER_H_
#define _WORKER_H_

#include <stdbool.h> // for bool
#include <stdint.h>  // for uintmax_t

extern uintmax_t total_packets_received;

struct worker_results {
  uintmax_t worker_packets_received;
};

struct worker_arguments {
  char      *pcap_filename;
  char      *program_basename;
  int       socket_fd;
  uintmax_t packet_count;
  bool      capture_rx;
  bool      capture_tx;
  bool      packet_buffered;
  bool      verbose;
  struct worker_results results;
};

void *worker(void *args);

#endif // _WORKER_H_

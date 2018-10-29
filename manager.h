/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * manager.h -- header file to use manager.c
 */

#ifndef _MANAGER_H_
#define _MANAGER_H_

#include <sched.h>   // for cpu_set_t
#include <stdbool.h> // for bool
#include <stdint.h>  // for uintmax_t

struct manager_arguments {
  char      *ifname;
  char      *pcap_filename;
  char      *program_fullname;
  char      *program_basename;
  uintmax_t packet_count;
  int       processor_count;
  cpu_set_t capture_cpu_set;
  bool      capture_rx;
  bool      capture_tx;
  bool      packet_buffered;
  bool      promiscuous;
  bool      verbose;
};

int manager(struct manager_arguments *args);

#endif // _MANAGER_H_

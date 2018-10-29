/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * cpu.h -- header file to use cpu.c
 */

#ifndef _CPU_H_
#define _CPU_H_

#define _GNU_SOURCE

#include <sched.h> // for cpu_set_t

int get_online_cpu_set(cpu_set_t *cpu_set);
int parse_cpu_list(char *cpu_list, cpu_set_t *cpu_set);
int parse_cpu_mask(char *cpu_mask, cpu_set_t *cpu_set);

#endif // _CPU_H_

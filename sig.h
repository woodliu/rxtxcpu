/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * sig.h -- header file to use sig.c
 */

#ifndef _SIG_H_
#define _SIG_H_

#include <signal.h> // for sig_atomic_t

extern char *program_basename;
extern volatile sig_atomic_t keep_running;

void sigint_handler(int signal);
int setup_signals(void);

#endif // _SIG_H_

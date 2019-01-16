/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * sig.c -- setup signal handlers
 */

#include "sig.h"

#include "rxtx.h" // for program_basename

#include <signal.h> // for sigaction, sigaction(), SIGINT, sigfillset()
#include <stdio.h>  // for fprintf()
#include <unistd.h> // for STDERR_FILENO, write()

volatile sig_atomic_t keep_running = 1;

void sigint_handler(int signal) {
  keep_running = 0;
  write(STDERR_FILENO, "\n", 1);
}

int setup_signals(void) {
  struct sigaction sa;

  /*
   * Setup our signal handler.
   */
  sa.sa_handler = &sigint_handler;

  /*
   * Resume any interrupted library functions when signal handler returns.
   */
  sa.sa_flags = SA_RESTART;

  /*
   * Block other signals while signal handler runs.
   */
  sigfillset(&sa.sa_mask);

  if (sigaction(SIGINT, &sa, NULL) == -1) {
    fprintf(
      stderr,
      "%s: Failed to setup signal handler for SIGINT.\n",
      program_basename
    );
    return -1;
  }

  return 0;
}

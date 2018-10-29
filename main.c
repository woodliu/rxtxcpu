/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define _GNU_SOURCE

#include "cli.h"     // for cli()
#include "manager.h" // for manager(), manager_arguments

#include <string.h>  // for memset()

#define EXIT_OK          0
#define EXIT_FAIL        1
#define EXIT_FAIL_OPTION 2

int main(int argc, char **argv) {
  struct manager_arguments mgr_args;
  memset(&mgr_args, 0, sizeof(mgr_args));

  if (cli(&mgr_args, argc, argv) == -1) {
    return EXIT_FAIL_OPTION;
  }

  if (manager(&mgr_args) == -1) {
    return EXIT_FAIL;
  }

  return EXIT_OK;
}

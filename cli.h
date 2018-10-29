/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * cli.h -- header file to use cli.c
 */

#ifndef _CLI_H_
#define _CLI_H_

#include "manager.h"  // for manager_arguments

int cli(struct manager_arguments *mgr_args, int argc, char **argv);

#endif // _CLI_H_

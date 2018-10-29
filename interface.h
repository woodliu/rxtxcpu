/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * interface.h -- header file to use interface.c
 */

#ifndef _INTERFACE_H_
#define _INTERFACE_H_

int interface_set_promisc_on(const unsigned int ifindex);

#endif // _INTERFACE_H_

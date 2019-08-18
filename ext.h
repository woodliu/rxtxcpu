/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * ext.h -- header file to use ext.c
 */

#ifndef _EXT_H_
#define _EXT_H_

char *ext(const char *path);
char *noext(char *path);
char *noext_copy(const char *path);

#endif // _EXT_H_

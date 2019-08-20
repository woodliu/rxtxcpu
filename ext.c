/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * ext.c -- parse or strip file extensions from filenames
 */

#define _GNU_SOURCE // for GNU basename()

#include "ext.h"

#include <stdio.h>  // for NULL
#include <string.h> // for GNU basename(), strdup(), strlen(), strrchr()

/* These functions are currently incompatible with the POSIX implementation of
 * basename(). We need the GNU implementation instead.
 *
 * Do not include libgen.h otherwise you'll get the POSIX version.
 */

/* ========================================================================= */
char *ext(const char *path) {
  char *filename = basename(path);
  char *p = strrchr(filename, '.');
  /*
   * When p is null filename is for an unhidden file without an extension. When
   * strlens are the same filename is for a hidden file without an extension.
   * In either case, we want to return an empty string.
   */
  if (!p || strlen(p) == strlen(filename)) {
    return "";
  }
  /*
   * The rest of the time, p + 1 returns the extension without the dot.
   */
  return p + 1;
}

/* ========================================================================= */
char *noext(char *path) {
  char *filename = basename(path);
  char *p = strrchr(filename, '.');
  /*
   * When p is null filename is for an unhidden file without an extension. When
   * strlens are the same filename is for a hidden file without an extension.
   * In either case, we want to return path unaltered.
   */
  if (!p || strlen(p) == strlen(filename)) {
    return path;
  }
  /*
   * The rest of the time we strip off the extension including the dot.
   */
  p[0] = '\0';
  return path;
}

/* ========================================================================= */
char *noext_copy(const char *path) {
  char *copy;
  copy = strdup(path);
  if (!copy) {
    return NULL;
  }
  return noext(copy);
}

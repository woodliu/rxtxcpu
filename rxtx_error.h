/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef _RXTX_ERROR_H_
#define _RXTX_ERROR_H_

#include <stdio.h>  // for snprintf()
#include <string.h> // for strlen()

#define RXTX_ERRBUF_SIZE 512
#define RXTX_ERROR -1

#define rxtx_fill_errbuf(buf, ...)                           \
  do {                                                       \
    int asklen = 0, gotlen = 0;                              \
    asklen = snprintf(buf, RXTX_ERRBUF_SIZE, ##__VA_ARGS__); \
    gotlen = strlen(buf);                                    \
    if (asklen > gotlen && gotlen > 3) {                     \
      buf[RXTX_ERRBUF_SIZE - 4] = '.';                       \
      buf[RXTX_ERRBUF_SIZE - 3] = '.';                       \
      buf[RXTX_ERRBUF_SIZE - 2] = '.';                       \
    }                                                        \
  } while (0)

#endif // _RXTX_ERROR_H_

/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef _TEST_RXTX_STATS_HELPER_H_
#define _TEST_RXTX_STATS_HELPER_H_

#include <errno.h> // for EBUSY, EINVAL

#ifdef TEST_CALLOC_FAILURE
  #define calloc(...) NULL
#endif

#ifdef TEST_PTHREAD_MUTEX_INIT_FAILURE
  #define pthread_mutex_init(...) EBUSY
#endif

#ifdef TEST_PTHREAD_MUTEX_DESTROY_FAILURE
  #define pthread_mutex_destroy(...) EBUSY
#endif

#ifdef TEST_PTHREAD_MUTEX_LOCK_FAILURE
  #define pthread_mutex_lock(...) EINVAL
#endif

#ifdef TEST_PTHREAD_MUTEX_UNLOCK_FAILURE
  #define pthread_mutex_unlock(...) EINVAL
#endif

#endif // _TEST_RXTX_STATS_HELPER_H_

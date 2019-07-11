/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef _TEST_RXTX_SAVEFILE_HELPER_H_
#define _TEST_RXTX_SAVEFILE_HELPER_H_

#ifdef TEST_STRDUP_FAILURE
  #define strdup(...) NULL
#endif

#ifdef TEST_PCAP_OPEN_DEAD_FAILURE
  #define pcap_open_dead(...) NULL
#endif

#ifdef TEST_PCAP_DUMP_OPEN_FAILURE
  #define pcap_dump_open(...) NULL
  #define pcap_geterr(...) "some error from libpcap"
#endif

#ifdef TEST_PCAP_DUMP_FLUSH_FAILURE
  #include <errno.h> // for ENOSPC

  #define pcap_dump_flush(...) -1
  #undef errno
  #define errno ENOSPC
#endif

#endif // _TEST_RXTX_SAVEFILE_HELPER_H_

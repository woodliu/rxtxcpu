/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef _RXTX_SAVEFILE_H_
#define _RXTX_SAVEFILE_H_

#include <pcap.h> // for pcap_dumper_t, pcap_pkthdr, pcap_t

struct rxtx_savefile {
  char *name;
  pcap_t *pd;
  pcap_dumper_t *pdd;
  char *errbuf;
};

int rxtx_savefile_open(struct rxtx_savefile *p, const char *filename,
                                                                 char *errbuf);
int rxtx_savefile_dump(struct rxtx_savefile *p, struct pcap_pkthdr *header,
                                                    u_char *packet, int flush);
int rxtx_savefile_close(struct rxtx_savefile *p);

#endif // _RXTX_SAVEFILE_H_

/*
 * Copyright (c) 2019-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "../../rxtx_error.h"
#include "../../rxtx_savefile.h"

#include <assert.h>
#include <pcap.h>
#include <string.h>

u_char packet[] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* ethernet destination address */
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* ethernet source address */
  0x08, 0x06,                         /* ethertype is arp */
  0x00, 0x01,                         /* arp hardware type */
  0x08, 0x00,                         /* arp protocol type */
  0x06,                               /* arp hardware address length */
  0x04,                               /* arp protocol address length */
  0x00, 0x01,                         /* arp operation */
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* arp sender hardware address */
  0x7f, 0x00, 0x00, 0x01,             /* arp sender protocol address */
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* arp target hardware address */
  0x7f, 0x00, 0x00, 0x01,             /* arp target protocol address */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* padding */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* padding */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00  /* padding */
};

int main(void) {

  struct rxtx_savefile rtp;
  char errbuf[RXTX_ERRBUF_SIZE];
  int status;

  status = rxtx_savefile_open(&rtp, "/dev/null", errbuf);
  assert(status == 0);

  struct pcap_pkthdr header;
  header.caplen     = (bpf_u_int32) sizeof(packet);
  header.len        = (bpf_u_int32) sizeof(packet);
  header.ts.tv_sec  = 0;
  header.ts.tv_usec = 0;

  status = rxtx_savefile_dump(&rtp, &header, packet, 1);
  assert(status == -1);

  status = strcmp(errbuf, "error writing to savefile '/dev/null'");
  assert(status == 0);

  rxtx_savefile_close(&rtp);

  return 0;
}

/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * interface.c -- control interface settings
 */

#include <linux/if_packet.h> // for PACKET_ADD_MEMBERSHIP, PACKET_MR_PROMISC,
                             //     packet_mreq
#include <sys/socket.h>      // for AF_PACKET, setsockopt(), SOCK_DGRAM,
                             //     socket(), SOL_PACKET
#include <string.h>          // for memset()

/* ========================================================================= */
int interface_set_promisc_on(const unsigned int ifindex) {
  struct packet_mreq mr;
  memset(&mr, 0, sizeof(mr));
  mr.mr_type = PACKET_MR_PROMISC;
  mr.mr_ifindex = ifindex;

  int fd = socket(AF_PACKET, SOCK_DGRAM, 0);

  if (fd < 0) {
    return -1;
  }

  return setsockopt(fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, (void *)&mr,
                                                                   sizeof(mr));
}

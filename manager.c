/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * manager.c -- setup sockets, spin up workers, and print results
 */

#define _GNU_SOURCE          // for CPU_SET(), cpu_set_t, CPU_ZERO()

#include "ext.h"             // for ext(), noext_copy()
#include "interface.h"       // for interface_set_promisc_on()
#include "manager.h"         // for manager_arguments
#include "sig.h"             // for setup_signals()
#include "worker.h"          // for total_packets_received, worker_arguments

#include <arpa/inet.h>       // for htons()
#include <linux/if_packet.h> // for PACKET_FANOUT, PACKET_FANOUT_CPU,
                             //     PACKET_RX_RING, PACKET_TX_RING, sockaddr_ll,
                             //     tpacket_req
#include <net/ethernet.h>    // for ETH_P_ALL
#include <net/if.h>          // for if_nametoindex()
#include <sys/socket.h>      // for AF_PACKET, bind(), setsockopt(),
                             //     SO_RCVTIMEO, SOCK_RAW, sockaddr, socket(),
                             //     SOL_PACKET, SOL_SOCKET
#include <sys/time.h>        // for timeval
#include <errno.h>           // for errno
#include <pthread.h>         // for pthread_attr_destroy(),
                             //     pthread_attr_init(),
                             //     pthread_attr_setaffinity_np(),
                             //     pthread_attr_t, pthread_create(),
                             //     pthread_join(), pthread_t
#include <sched.h>           // for CPU_SET(), cpu_set_t, CPU_ZERO()
#include <stdio.h>           // for FILE, fprintf(), NULL, stderr, sprintf()
#include <stdlib.h>          // for malloc()
#include <string.h>          // for memset(), strcmp(), strerror(), strlen()
#include <unistd.h>          // for getpid()

#define RETURN_BAD -1
#define RETURN_GOOD 0

/*
 * The largest positive number an int can hold is 2,147,483,647. The smallest
 * negative number an int can hold is -2,147,483,648. Therefore, the longest
 * strlen() value for string produced by printf() and friends is 11 characters.
 *
 *   strlen("-2147483648");
 */
#define MAX_INT_AS_STRING_LENGTH 11

int manager(struct manager_arguments *args) {

  /*
   * If we were supplied an ifname, we need to lookup the ifindex. Otherwise,
   * we want ifindex to be 0 to match any interface.
   *
   * When an ifindex lookup is needed, we want the ifindex before entering our
   * loop and do not yet have a socket fd. It's more clear to create, use, and
   * discard a temporary socket fd for our SIOCGIFINDEX ioctl request than to
   * try to use one of the socket fds we'll create in our loop.
   * if_nametoindex() does exactly what we want.
   */
  unsigned int ifindex;
  if (args->ifname) {
    ifindex = if_nametoindex(args->ifname);

    if (!ifindex) {
      fprintf(stderr, "%s: Failed to get ifindex for interface '%s'\n", args->program_basename, args->ifname);
      return RETURN_BAD;
    }
  } else {
    ifindex = 0;
  }

  if (args->verbose) {
    if (ifindex == 0) {
      fprintf(stderr, "Using ifindex '%u' for any interface.\n", ifindex);
    } else {
      fprintf(stderr, "Using ifindex '%u' for interface '%s'.\n", ifindex, args->ifname);
    }
  }

  /*
   * We only have to enable promiscuity once and it will stick around for the
   * duration of the process.
   */
  if (args->promiscuous) {
    if (ifindex == 0) {
      if (args->verbose) {
        fprintf(stderr, "Skipping promiscuous mode for ifindex '%u' (any interface).\n", ifindex);
      }
    } else if (interface_set_promisc_on(ifindex) == -1) {
      fprintf(stderr, "%s: Failed to enable promiscuous mode for ifindex '%u'.\n", args->program_basename, ifindex);
      return RETURN_BAD;
    }
  }

  /*
   * We need an id to add socket fds to our fanout group.
   */
  int fanout_group_id = getpid() & 0xffff;

  if (args->verbose) {
    fprintf(stderr, "Generated fanout group id '%d'.\n", fanout_group_id);
  }

  /*
   * This loop primarily creates our socket fds. We need the instantiation order indexed
   * by processor id.
   *
   * Secondarily, it creates our per-processor pcap filenames.
   */
  struct worker_arguments worker_args[args->processor_count];
  pthread_t worker_threads[args->processor_count];

  for (int i = 0; i < args->processor_count; i++) {
    /*
     * The AF_PACKET address family gives us a packet socket at layer 2. The
     * SOCK_RAW socket type gives us packets which include the layer 2 header.
     * The htons(ETH_P_ALL) protocol gives us all packets from any protocol.
     */
    int socket_fd;
    socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (socket_fd == -1) {
      fprintf(stderr, "%s: Failed to create socket: %s\n", args->program_basename, strerror(errno));
      return RETURN_BAD;
    }

    /*
     * We're using the default, TPACKET_V1, presently. Updating to TPACKET_V3
     * would be cool.
     *
     * PACKET_RX_RING sets up a mmapped ring buffer for async packet reception.
     * PACKET_TX_RING sets up a mmapped ring buffer for packet transmission.
     *
     * I'm not sure if packets marked with PACKET_OUTGOING end up in the rx
     * ring buffer or the tx ring buffer. PACKET_TX_RING may only be used for
     * packets sent by this application, but it doesn't seem to cost much to
     * set it up.
     */
    struct tpacket_req req;
    memset(&req, 0, sizeof(req));
    setsockopt(socket_fd, SOL_PACKET, PACKET_RX_RING, (void *)&req, sizeof(req));
    setsockopt(socket_fd, SOL_PACKET, PACKET_TX_RING, (void *)&req, sizeof(req));

    /*
     * SO_RCVTIMEO sets a timeout for socket i/o system calls like recvfrom().
     * This gives us an easy way to keep our worker loops from getting stuck
     * waiting on a packet which may never arrive.
     */
    struct timeval receive_timeout;
    /* no need for memset(), we're initializing every member */
    receive_timeout.tv_sec = 0;
    receive_timeout.tv_usec = 10;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &receive_timeout, sizeof(receive_timeout));

    /*
     * Per packet(7), we need to set sll_family, sll_protocol, and sll_ifindex
     * in the sockaddr_ll we're passing to bind(). The values for sll_family
     * and sll_protocol are straight out of our earlier call to socket(). The
     * value for sll_ifindex is from our lookup.
     */
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = ifindex;

    if (bind(socket_fd, (struct sockaddr *)&sll, sizeof(sll)) == -1) {
      fprintf(stderr, "%s: Failed to bind socket.\n", args->program_basename);
      return RETURN_BAD;
    }

    /*
     * Per packet(7), "PACKET_FANOUT_CPU selects the socket based on the CPU
     * that the packet arrived on."
     *
     * The implementation in fanout_demux_cpu() (net/packet/af_packet.c) is a
     * simple modulo operaton.
     *
     *   smp_processor_id() % num;
     *
     * num is the number of sockets in the fanout group. The sockets are
     * indexed in the same order as they were added to the fanout group. So, we
     * simply need one socket per processor added to the fanout group in
     * processor id order.
     */
    int fanout_arg = (fanout_group_id | (PACKET_FANOUT_CPU << 16));
    if (setsockopt(
          socket_fd,
          SOL_PACKET,
          PACKET_FANOUT,
          &fanout_arg,
          sizeof(fanout_arg)
        ) < 0) {
      fprintf(stderr, "%s: Failed to configure fanout.\n", args->program_basename);
      return RETURN_BAD;
    }

    worker_args[i].socket_fd = socket_fd;

    if (!CPU_ISSET(i, &(args->capture_cpu_set))) {
      continue;
    }

    if (args->pcap_filename) {
      if (strcmp(args->pcap_filename, "-") == 0) {
        worker_args[i].pcap_filename = args->pcap_filename;
      } else {
        worker_args[i].pcap_filename = malloc(
          strlen(args->pcap_filename)
          + MAX_INT_AS_STRING_LENGTH
          + 2  /* for - and \0 */
        );
        char *copy = noext_copy(args->pcap_filename);
        sprintf(
          worker_args[i].pcap_filename,
          "%s-%d.%s",
          copy,
          i,
          ext((char *)args->pcap_filename)
        );
        free(copy);
      }
    } else {
      /*
       * args->pcap_filename is NULL.
       */
      worker_args[i].pcap_filename = NULL;
    }

    worker_args[i].program_basename = args->program_basename;
    worker_args[i].packet_count     = args->packet_count;
    worker_args[i].capture_rx       = args->capture_rx;
    worker_args[i].capture_tx       = args->capture_tx;
    worker_args[i].packet_buffered  = args->packet_buffered;
    worker_args[i].verbose          = args->verbose;
  }

  /*
   * Setup our signal handlers before spinning up workers.
   */
  setup_signals(args->program_basename);

  /*
   * This loop spins up our workers. Each worker is affine to a single
   * processor and is passed the socket fd which will receive packets for that
   * processor.
   */
  cpu_set_t cpu_set;
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  for (int i = 0; i < args->processor_count; i++) {
    if (!CPU_ISSET(i, &(args->capture_cpu_set))) {
      continue;
    }
    CPU_ZERO(&cpu_set);
    CPU_SET(i, &cpu_set);
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set), &cpu_set);
    pthread_create(&worker_threads[i], &attr, worker, (void *)&worker_args[i]);
  }

  /*
   * This loop joins our workers and prints the per-worker results.
   */
  FILE *out = stdout;
  if (args->pcap_filename &&
      strcmp(args->pcap_filename, "-") == 0) {
    out = stderr;
  }
  for (int i = 0; i < args->processor_count; i++) {
    if (!CPU_ISSET(i, &(args->capture_cpu_set))) {
      continue;
    }
    pthread_join(worker_threads[i], NULL);
    fprintf(
      out,
      "%ju packets captured on cpu%d.\n",
      worker_args[i].results.worker_packets_received,
      i
    );
    free(worker_args[i].pcap_filename);
  }

  pthread_attr_destroy(&attr);

  fprintf(out, "%ju packets captured total.\n", total_packets_received);

  return RETURN_GOOD;
}

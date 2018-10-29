/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * worker.c -- capture packets, count them, and possibly write to a pcap
 */

#define _GNU_SOURCE          // for sched_getcpu()

#include "sig.h"             // for keep_running
#include "worker.h"          // for worker_arguments

#include <linux/if_packet.h> // for PACKET_OUTGOING, sockaddr_ll
#include <sys/socket.h>      // for recvfrom(), sockaddr
#include <pcap.h>            // for bpf_u_int32, DLT_EN10MB, pcap_close(),
                             //     pcap_dump(), pcap_dump_close(),
                             //     pcap_dump_flush(), pcap_dump_open(),
                             //     pcap_dumper_t, pcap_geterr(),
                             //     pcap_open_dead(), pcap_pkthdr, pcap_t
#include <pthread.h>         // for pthread_self()
#include <sched.h>           // for sched_getcpu()
#include <stdbool.h>         // for true
#include <stdint.h>          // for uintmax_t
#include <stdlib.h>          // for exit()
#include <stdio.h>           // for fprintf(), NULL, stderr, stdout
#include <string.h>          // for strcmp()
#include <time.h>            // for time()

#define RETURN_BAD -1
#define RETURN_GOOD 0

#define SNAPLEN 65535
#define PACKET_BUFFER_SIZE 65535

#define packet_direction_is_rx(sll) (!packet_direction_is_tx(sll))
#define packet_direction_is_tx(sll) ((sll)->sll_pkttype == PACKET_OUTGOING)

uintmax_t total_packets_received = 0;

void *worker(void *args) {
  struct worker_arguments *worker_args = args;

  if (worker_args->verbose) {
    fprintf(
      stderr,
      "Worker '%lu' with socket fd '%i' running on cpu '%d'.\n",
      pthread_self(),
      worker_args->socket_fd,
      sched_getcpu()
    );
  }

  worker_args->results.worker_packets_received = 0;

  pcap_t *pcap;             /* packet capture descriptor */
  pcap_dumper_t *pcap_file; /* pointer to the dump file */

  if (worker_args->pcap_filename) {
    pcap = pcap_open_dead(DLT_EN10MB, SNAPLEN);

    if ((pcap_file = pcap_dump_open(pcap, worker_args->pcap_filename)) == NULL) {
      fprintf(
        stderr,
        "%s: Error opening dump file '%s' for writing: %s\n",
        worker_args->program_basename,
        worker_args->pcap_filename,
        pcap_geterr(pcap)
      );
      exit(1);
    }
  }

  while (keep_running) {

    if (worker_args->packet_count && total_packets_received >= worker_args->packet_count) {
      break;
    }

    struct sockaddr_ll sll;
    unsigned char packet_buffer[PACKET_BUFFER_SIZE];

    unsigned int sll_length = sizeof(sll);
    int packet_length = recvfrom(
      worker_args->socket_fd,
      packet_buffer,
      sizeof(packet_buffer),
      0,
      (struct sockaddr *)&sll,
      &sll_length
    );

    if (packet_length == -1) {
      continue;
    }

    if (!worker_args->capture_rx && packet_direction_is_rx(&sll)) {
      continue;
    }

    if (!worker_args->capture_tx && packet_direction_is_tx(&sll)) {
      continue;
    }

    worker_args->results.worker_packets_received++;
    total_packets_received++;

    if (worker_args->pcap_filename) {
      /* no need for memset(), we're initializing every member */
      struct pcap_pkthdr pcap_packet_header;
      pcap_packet_header.caplen     = (bpf_u_int32)packet_length;
      pcap_packet_header.len        = (bpf_u_int32)packet_length;
      pcap_packet_header.ts.tv_sec  = time(NULL);
      pcap_packet_header.ts.tv_usec = 0;

      pcap_dump((unsigned char *)pcap_file, &pcap_packet_header, packet_buffer);

      if (worker_args->packet_buffered) {
        if ((pcap_dump_flush(pcap_file)) == -1) {
          fprintf(
            stderr,
            "%s: Error writing to dump file '%s'.\n",
            worker_args->program_basename,
            worker_args->pcap_filename
          );
          exit(1);
        }
      }
    }
  }

  if (worker_args->pcap_filename) {
    /* protect against silent write failures */
    if ((pcap_dump_flush(pcap_file)) == -1) {
      fprintf(
        stderr,
        "%s: Error writing to dump file '%s'.\n",
        worker_args->program_basename,
        worker_args->pcap_filename
      );
      exit(1);
    }

    pcap_dump_close(pcap_file);
    pcap_close(pcap);
  }

  return RETURN_GOOD;
}

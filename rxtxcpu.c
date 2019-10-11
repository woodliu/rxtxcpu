/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define _GNU_SOURCE // for GNU basename()

#include "cpu.h"       // for get_online_cpu_set(), parse_cpu_list(),
                       //     parse_cpu_mask()
#include "ring_set.h"  // for for_each_ring_in_size(), RING_CLR(),
                       //     RING_COUNT(), RING_ISSET(), RING_SET()
#include "rxtx.h"      // for for_each_ring(), for_each_set_ring(),
                       //     program_basename, rxtx_activate(), rxtx_close(),
                       //     rxtx_desc, rxtx_get_packets_received(),
                       //     rxtx_get_ring(), rxtx_get_ring_count(),
                       //     rxtx_get_savefile_template(), rxtx_init(),
                       //     rxtx_set_direction(), rxtx_set_fanout_mode(),
                       //     rxtx_set_ifname(), rxtx_set_packet_buffered(),
                       //     rxtx_set_promiscuous(), rxtx_set_ring_count(),
                       //     rxtx_set_ring_set(),
                       //     rxtx_set_savefile_template(), rxtx_set_verbose(),
                       //     rxtx_verbose_isset()
#include "rxtx_error.h" // for RXTX_ERRBUF_SIZE, RXTX_ERROR
#include "rxtx_ring.h" // for rxtx_ring, rxtx_ring_get_packets_received(),
                       //     rxtx_ring_loop()
#include "sig.h"       // for setup_signals()

#include <linux/if_packet.h> // for PACKET_FANOUT_CPU

#include <ctype.h>    // for isspace()
#include <errno.h>    // for EBUSY
#include <getopt.h>   // for getopt_long(), optarg, optind, option, optopt
#include <inttypes.h> // for strtoumax()
#include <limits.h>   // for INT_MAX
#include <pcap.h>     // for PCAP_D_IN, PCAP_D_INOUT, PCAP_D_OUT
#include <pthread.h>  // for pthread_attr_destroy(), pthread_attr_init(),
                      //     pthread_attr_setaffinity_np(), pthread_attr_t,
                      //     pthread_create(), pthread_t, pthread_tryjoin_np()
#include <sched.h>    // for CPU_COUNT(), CPU_ISSET(), CPU_SET(), cpu_set_t,
                      //     CPU_ZERO()
#include <stdbool.h>  // for bool, false, true
#include <stdint.h>   // for intptr_t
#include <stdio.h>    // for asprintf(), FILE, fprintf(), fputs(), NULL,
                      //     printf(), putchar(), puts(), stderr, stdout
#include <stdlib.h>   // for malloc()
#include <string.h>   // for GNU basename(), memset(), strcmp(), strerror(),
                      //     strlen()
#include <unistd.h>   // for _SC_NPROCESSORS_CONF, sysconf()

#define EXIT_OK          0
#define EXIT_FAIL        1
#define EXIT_FAIL_OPTION 2

#define OPTION_COUNT_BASE 10

#define USAGE_PRINT_OPT_COL_SEP "# "
#define USAGE_PRINT_OPT_COL_IND "  "

#define USAGE_PRINT_OPT_COL_SEP_LEN (strlen(USAGE_PRINT_OPT_COL_SEP))
#define USAGE_PRINT_OPT_COL_IND_LEN (strlen(USAGE_PRINT_OPT_COL_IND))

#define USAGE_PRINT_OPT_TOTAL_LEN     79
#define USAGE_PRINT_OPT_FIRST_COL_LEN 31
#define USAGE_PRINT_OPT_SECOND_COL_LEN (USAGE_PRINT_OPT_TOTAL_LEN \
                 - USAGE_PRINT_OPT_FIRST_COL_LEN - USAGE_PRINT_OPT_COL_SEP_LEN)
#define USAGE_PRINT_OPT_IND_SECOND_COL_LEN (USAGE_PRINT_OPT_SECOND_COL_LEN \
                                                 - USAGE_PRINT_OPT_COL_IND_LEN)

static const struct option long_options[] = {
  {"count",           required_argument, NULL, 'c'},
  {"direction",       required_argument, NULL, 'd'},
  {"help",            no_argument,       NULL, 'h'},
  {"cpu-list",        required_argument, NULL, 'l'},
  {"cpu-mask",        required_argument, NULL, 'm'},
  {"promiscuous",     no_argument,       NULL, 'p'},
  {"packet-buffered", no_argument,       NULL, 'U'},
  {"verbose",         no_argument,       NULL, 'v'},
  {"version",         no_argument,       NULL, 'V'},
  {"write",           required_argument, NULL, 'w'},
  {0, 0, NULL, 0}
};

struct usage_opt {
  int        val;
  const char *arg;
  char *description;
};

static const struct usage_opt usage_options[] = {
  {'c', "N",         "Exit after receiving N packets."},
  {'d', "DIRECTION", "Capture only packets matching DIRECTION. DIRECTION can"
                        " be 'rx', 'tx', or 'rxtx'. Default matches invocation"
                         " (i.e. DIRECTION defaults to 'rx' when invocation is"
                                 " 'rxcpu', 'tx' when 'txcpu', and 'rxtx' when"
                                                               " 'rxtxcpu')."},
  {'h', NULL,        "Display this help and exit."},
  {'l', "CPULIST",   "Capture only on cpus in CPULIST (e.g. if CPULIST is"
                      " '0,2-4,6', only packets on cpus 0, 2, 3, 4, and 6 will"
                                                             " be captured)."},
  {'m', "CPUMASK",   "Capture only on cpus in CPUMASK (e.g. if CPUMASK is"
                        " '5d', only packets on cpus 0, 2, 3, 4, and 6 will be"
                                                                " captured)."},
  {'p', NULL,        "Put the interface into promiscuous mode."},
  {'U', NULL,        "When writing to a pcap file, the write buffer will be"
                           " flushed just after each packet is placed in it."},
  {'v', NULL,        "Display more verbose output."},
  {'V', NULL,        "Display the version and exit."},
  {'w', "FILE",      "Write packets to FILE in pcap format. FILE is used as a"
                      " template for per-cpu filenames (e.g. if capturing on a"
                           " system with 2 cpus, cpus 0 and 1, and FILE set to"
                            " 'out.pcap', the cpu 0 capture will be written to"
                       " 'out-0.pcap' and the cpu 1 capture will be written to"
                            " 'out-1.pcap'). Writing to stdout is supported by"
                           " setting FILE to '-', but only when capturing on a"
                                                               " single cpu."},
  {0, NULL, NULL}
};

static char *usage_short_opt_order = "h:c:lm:d:U:p:v:V:w";

/* ========================================================================= */
static void usage_print_opt(int val, const char *name, const char *arg,
                                                           char *description) {
  int consumed = 0;
  int padding = 0;
  int remaining = 0;

  char *start = NULL;
  char *end = NULL;

  consumed = printf("  -%c, [--%s%s%s]  ", val, name, arg ? "=" : "",
                                                               arg ? arg : "");

  if (consumed < 0) {
    /* no point in continuing */
    return;
  }

  padding = USAGE_PRINT_OPT_FIRST_COL_LEN - consumed;

  if (padding < 0) {
    padding = 0;
  }

  printf("%*s%s", padding, "", USAGE_PRINT_OPT_COL_SEP);

  start = description;

  remaining = USAGE_PRINT_OPT_SECOND_COL_LEN;

  while(1) {
    while(isspace(*start)) {
      start++;
    }

    int len = strlen(start);

    if (len < remaining) {
      printf("%s\n", start);
      break;
    }

    end = start + remaining;

    while(!isspace(*end) && end > start) {
      end--;
    }

    if (start == end) {
      /* no whitespace found in second column width, bail */
      printf("%s\n", start);
      break;
    }

    while(start < end) {
      putchar(*start);
      start++;
    }

    printf("\n%*s%s%s", USAGE_PRINT_OPT_FIRST_COL_LEN, "",
                             USAGE_PRINT_OPT_COL_SEP, USAGE_PRINT_OPT_COL_IND);

    remaining = USAGE_PRINT_OPT_IND_SECOND_COL_LEN;
  }
}

/* ========================================================================= */
static const char *usage_opt_get_arg(int val) {
  int i = 0;
  int v = 0;

  for (i = 0; i < INT_MAX; i++) {
    v = usage_options[i].val;

    if (!v) {
      return NULL;
    }

    if (v == val) {
      return usage_options[i].arg;
    }
  }

  return NULL;
}

/* ========================================================================= */
static const char * usage_opt_get_name(int val) {
  int i = 0;
  int v = 0;

  for (i = 0; i < INT_MAX; i++) {
    v = long_options[i].val;

    if (!v) {
      return NULL;
    }

    if (v == val) {
      return long_options[i].name;
    }
  }

  return NULL;
}

/* ========================================================================= */
static void usage(void) {
  int i = 0;
  int val = 0;
  const char *name;

  puts("Usage:");
  printf("  %s [OPTIONS] [INTERFACE]\n\n", program_basename);

  puts("Options:");

  for (i = 0; i < INT_MAX; i++) {
    val = usage_options[i].val;

    if (!val) {
      break;
    }

    name = usage_opt_get_name(val);

    if (name) {
      usage_print_opt(val, name, usage_options[i].arg,
                                                 usage_options[i].description);
    }
  }
}

/* ========================================================================= */
static void usage_short(void) {
  int i = 0;
  int indent = 0;
  int lookahead = 0;
  int projected = 0;
  int remaining = 0;
  int val = 0;

  char *p = NULL;

  const char *arg;
  const char *name;

  indent = fprintf(stderr, "Usage: %s", program_basename);

  if (indent < 0) {
    /* no point in continuing */
    return;
  }

  remaining = USAGE_PRINT_OPT_TOTAL_LEN - indent;

  p = usage_short_opt_order;

  while (*p) {
    lookahead = 0;
    projected = 0;

    while (p[lookahead] && p[lookahead] != ':') {
      val = p[lookahead];

      name = usage_opt_get_name(val);
      arg = usage_opt_get_arg(val);

      if (name) {
        if (!lookahead) {
          projected += 4; // ' [--'
        } else {
          projected += 3; // '|--'
        }

        projected += strlen(name);

        if (arg) {
          projected += 1; // '='
          projected += strlen(arg);
        }
      }

      lookahead++;
    }

    projected += 1; // ']'

    if (projected > remaining) {
      fprintf(stderr, "\n%*s", indent, "");
      remaining = USAGE_PRINT_OPT_TOTAL_LEN - indent;
    }

    for (i = 0; i < lookahead; i++) {
      val = p[i];

      name = usage_opt_get_name(val);
      arg = usage_opt_get_arg(val);

      if (name) {
        if (!i) {
          remaining -= fprintf(stderr, " [--");
        } else {
          remaining -= fprintf(stderr, "|--");
        }

        remaining -= fprintf(stderr, "%s", name);

        if (arg) {
          remaining -= fprintf(stderr, "=%s", arg);
        }
      }
    }

    p += lookahead;

    remaining -= fprintf(stderr, "]");

    if (*p == ':') {
      p++;
    }
  }

  fprintf(stderr, "\n");
}

/* ========================================================================= */
int main(int argc, char **argv) {
  program_basename = basename(argv[0]);

  int c = 0;
  int cpus = 0;
  int i = 0;
  int status = 0;
  int worker_count = 0;

  bool help = false;

  char *badopt = NULL;
  char *cpu_list = NULL;
  char *cpu_mask = NULL;
  char *endptr = NULL;

  FILE *out = stdout;

  char errbuf[RXTX_ERRBUF_SIZE] = "";

  struct rxtx_desc rtd;
  rxtx_init(&rtd, errbuf);

  struct rxtx_ring* ring;

  /*
   * Per packet(7), "PACKET_FANOUT_CPU selects the socket based on the CPU that
   * the packet arrived on."
   *
   * The implementation in fanout_demux_cpu() (net/packet/af_packet.c) is a
   * simple modulo operaton.
   *
   *   smp_processor_id() % num;
   *
   * num is the number of sockets in the fanout group. The sockets are indexed
   * in the same order as they were added to the fanout group. So, we simply
   * need one socket per processor added to the fanout group in processor id
   * order.
   *
   * rxtx_activate() will handle the ordering, we just need to set the fanout
   * mode.
   */
  status = rxtx_set_fanout_mode(&rtd, PACKET_FANOUT_CPU);
  if (status == RXTX_ERROR) {
    fprintf(stderr, "%s: %s\n", program_basename, errbuf);
    return EXIT_FAIL;
  }

  /*
   * direction default is based on the invocation.
   */
  if (strcmp(program_basename, "rxcpu") == 0) {
    status = rxtx_set_direction(&rtd, PCAP_D_IN);
  } else if (strcmp(program_basename, "txcpu") == 0) {
    status = rxtx_set_direction(&rtd, PCAP_D_OUT);
  } else {
    status = rxtx_set_direction(&rtd, PCAP_D_INOUT);
  }
  if (status == RXTX_ERROR) {
    fprintf(stderr, "%s: %s\n", program_basename, errbuf);
    return EXIT_FAIL;
  }

  ring_set_t ring_set;
  RING_ZERO(&ring_set);

  /*
   * optstring must start with ":" so ':' is returned for a missing option
   * argument. Otherwise '?' is returned for both invalid option and missing
   * option argument.
   */
  while ((c = getopt_long(argc, argv, ":c:d:hl:m:pUvVw:", long_options, 0))
                                                                       != -1) {
    switch (c) {
      case 'c':
        status = rxtx_set_packet_count(&rtd, strtoumax(optarg, &endptr,
                                                           OPTION_COUNT_BASE));
        if (status == RXTX_ERROR) {
          fprintf(stderr, "%s: %s\n", program_basename, errbuf);
          return EXIT_FAIL;
        }
        if (*endptr) {
          fprintf(stderr, "%s: Invalid count '%s'.\n", program_basename,
                                                                       optarg);
          usage_short();
          return EXIT_FAIL_OPTION;
        }
        break;

      case 'd':
        if (strcmp(optarg, "rx") == 0) {
          status = rxtx_set_direction(&rtd, PCAP_D_IN);
        } else if (strcmp(optarg, "tx") == 0) {
          status = rxtx_set_direction(&rtd, PCAP_D_OUT);
        } else if (strcmp(optarg, "rxtx") == 0) {
          status = rxtx_set_direction(&rtd, PCAP_D_INOUT);
        } else {
          fprintf(stderr, "%s: Invalid direction '%s'.\n", program_basename,
                                                                       optarg);
          usage_short();
          return EXIT_FAIL_OPTION;
        }
        if (status == RXTX_ERROR) {
          fprintf(stderr, "%s: %s\n", program_basename, errbuf);
          return EXIT_FAIL;
        }
        break;

      case 'h':
        help = true;
        break;

      case 'l':
        cpu_list = optarg;
        if (parse_cpu_list(optarg, &ring_set)) {
          fprintf(stderr, "%s: Invalid cpu list '%s'.\n", program_basename,
                                                                       optarg);
          usage_short();
          return EXIT_FAIL_OPTION;
        }
        break;

      case 'm':
        cpu_mask = optarg;
        if (parse_cpu_mask(optarg, &ring_set)) {
          fprintf(stderr, "%s: Invalid cpu mask '%s'.\n", program_basename,
                                                                       optarg);
          usage_short();
          return EXIT_FAIL_OPTION;
        }
        break;

      case 'p':
        status = rxtx_set_promiscuous(&rtd);
        if (status == RXTX_ERROR) {
          fprintf(stderr, "%s: %s\n", program_basename, errbuf);
          return EXIT_FAIL;
        }
        break;

      case 'U':
        status = rxtx_set_packet_buffered(&rtd);
        if (status == RXTX_ERROR) {
          fprintf(stderr, "%s: %s\n", program_basename, errbuf);
          return EXIT_FAIL;
        }
        break;

      case 'v':
        rxtx_set_verbose(&rtd);
        break;

      case 'V':
        printf("%s version %s\n", program_basename, RXTXCPU_VERSION);
        return EXIT_OK;

      case 'w':
        status = rxtx_set_savefile_template(&rtd, optarg);
        if (status == RXTX_ERROR) {
          fprintf(stderr, "%s: %s\n", program_basename, errbuf);
          return EXIT_FAIL;
        }
        break;

      case ':':  /* missing option argument */
        fprintf(stderr, "%s: Option '%s' requires an argument.\n",
                                             program_basename, argv[optind-1]);
        usage_short();
        return EXIT_FAIL_OPTION;

      case '?':  /* invalid option */
      default:   /* invalid option in optstring */
        /*
         * optopt is NULL for long options. optind is the index of the next
         * argv to be processed. It is therefore tempting to use
         * argv[optind-1] to retrieve the invalid option, be it short or long.
         *
         * This method works with long options and with non-bundled short
         * options. However, it fails under certain conditions with bundled
         * short options.
         *
         * When the first invalid option in the bundle is also the last option
         * in the bundle, argv[optind-1] works.
         *
         * When the first invalid option in the bundle is not also the last
         * option in the bundle, optind is not incremented. This makes sense
         * since the bundle argv still needs further processing.
         *
         * Relying solely on optind leaves us with two difficult to distinguish
         * possibilities.
         *   1. The last or only short option in argv[optind-1] was invalid and
         *      optind was incremented.
         *   2. Any short option other than the last in argv[optind] was
         *      invalid and optind was not incremented.
         *
         * As previously mentioned, optopt is NULL for long options. It is also
         * reliable for finding the invalid short option with one caveat; we
         * need to keep our optstring clean. If we arrive here via the default
         * case optopt will be NULL and we'll have a corner case for the
         * invalid short option mistakenly in optstring.
         *
         * As long as we keep optstring clean, we can use optopt for short
         * options and argv[optind-1] for long options.
         */
        if (optopt) {
          asprintf(&badopt, "-%c", optopt);
        } else {
          badopt = argv[optind-1];
        }
        fprintf(stderr, "%s: Unrecognized option '%s'.\n", program_basename,
                                                                       badopt);
        if (optopt) {
          free(badopt);
        }
        usage_short();
        return EXIT_FAIL_OPTION;
    }
  }

  if (help) {
    usage();
    return EXIT_OK;
  }

  if (cpu_list && cpu_mask) {
    fprintf(stderr, "%s: -l [--cpu-list] and -m [--cpu-mask] are mutually"
                                            " exclusive.\n", program_basename);
    usage_short();
    return EXIT_FAIL_OPTION;
  }

  /*
   * We need to know how many processors are configured.
   */
  cpus = sysconf(_SC_NPROCESSORS_CONF);
  if (cpus <= 0) {
    fprintf(stderr, "%s: Failed to get processor count.\n", program_basename);
    return EXIT_FAIL;
  }

  status = rxtx_set_ring_count(&rtd, cpus);
  if (status == RXTX_ERROR) {
    fprintf(stderr, "%s: %s\n", program_basename, errbuf);
    return EXIT_FAIL;
  }

  if (rxtx_verbose_isset(&rtd)) {
    fprintf(stderr, "Found '%d' processors.\n", rxtx_get_ring_count(&rtd));
  }

  if (RING_COUNT(&ring_set) == 0) {
    for_each_ring(i, &rtd) {
      RING_SET(i, &ring_set);
    }
  }

  worker_count = 0;
  for_each_ring(i, &rtd) {
    if (RING_ISSET(i, &ring_set)) {
      worker_count++;
    }
  }
  if (!worker_count) {
    fprintf(stderr, "%s: No configured cpus present in cpu %s.\n",
                                 program_basename, cpu_list ? "list" : "mask");
    usage_short();
    return EXIT_FAIL_OPTION;
  }

  cpu_set_t online_cpu_set;
  if (get_online_cpu_set(&online_cpu_set) != 0) {
    fprintf(stderr, "%s: Failed to get online cpu set.\n", program_basename);
    return EXIT_FAIL_OPTION;
  }

  if (CPU_COUNT(&online_cpu_set) != rxtx_get_ring_count(&rtd)) {
    for_each_ring(i, &rtd) {
      if (!CPU_ISSET(i, &online_cpu_set)) {
        RING_CLR(i, &ring_set);
        if (rxtx_verbose_isset(&rtd)) {
          fprintf(stderr, "Skipping cpu '%d' since it is offline.\n", i);
        }
      }
    }
  }

  worker_count = 0;
  for_each_ring(i, &rtd) {
    if (RING_ISSET(i, &ring_set)) {
      worker_count++;
    }
  }
  if (!worker_count) {
    fprintf(stderr, "%s: No online cpus present in cpu %s.\n",
                                 program_basename, cpu_list ? "list" : "mask");
    usage_short();
    return EXIT_FAIL_OPTION;
  }

  if (rxtx_get_savefile_template(&rtd) &&
                          strcmp(rxtx_get_savefile_template(&rtd), "-") == 0 &&
                                                  RING_COUNT(&ring_set) != 1) {
    fprintf(stderr, "%s: Write file '-' (stdout) is only permitted when"
                            " capturing on a single cpu.\n", program_basename);
    usage_short();
    return EXIT_FAIL_OPTION;
  }

  status = rxtx_set_ring_set(&rtd, &ring_set);
  if (status == RXTX_ERROR) {
    fprintf(stderr, "%s: %s\n", program_basename, errbuf);
    return EXIT_FAIL;
  }

  if ((optind + 1) < argc) {
    fprintf(stderr, "%s: Only one interface argument is allowed (got [ ",
                                                             program_basename);
    for (; optind < argc; optind++) {
      fprintf(stderr, "'%s'", argv[optind]);
      if ((optind + 1) < argc)
        fputs(", ", stderr);
    }
    fputs(" ]).\n", stderr);
    usage_short();
    return EXIT_FAIL_OPTION;
  }

  if (optind != argc) {
    status = rxtx_set_ifname(&rtd, argv[optind]);
    if (status == RXTX_ERROR) {
      fprintf(stderr, "%s: %s\n", program_basename, errbuf);
      return EXIT_FAIL;
    }
  }

  status = rxtx_activate(&rtd);
  if (status == RXTX_ERROR) {
    fprintf(stderr, "%s: %s\n", program_basename, errbuf);
    return EXIT_FAIL;
  }

  /*
   * Setup our signal handlers before spinning up threads.
   */
  setup_signals();

  /*
   * This loop spins up our threads. Each thread is affine to a single
   * processor and is passed the ring containing the socket fd which will
   * receive packets for that processor.
   */
  cpu_set_t cpu_set;
  pthread_t threads[rxtx_get_ring_count(&rtd)];
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  for_each_set_ring(i, &rtd) {
    CPU_ZERO(&cpu_set);
    CPU_SET(i, &cpu_set);
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set), &cpu_set);

    ring = rxtx_get_ring(&rtd, (unsigned int)i);
    if (!ring) {
      fprintf(stderr, "%s: %s\n", program_basename, errbuf);
      return EXIT_FAIL;
    }
    pthread_create(&threads[i], &attr, rxtx_ring_loop, (void *)ring);
  }

  /*
   * This loop joins our threads.
   */
  int ebusy = 0;

  void *vpstatus = NULL;

  int joined[rxtx_get_ring_count(&rtd)];
  memset(joined, 0, sizeof(int) * rxtx_get_ring_count(&rtd));

  while (1) {
    ebusy = 0;

    for_each_set_ring(i, &rtd) {
      if (!joined[i]) {
        status = pthread_tryjoin_np(threads[i], &vpstatus);

        if (status == EBUSY) {
          ebusy++;
          continue;
        }

        if (status) {
          fprintf(stderr, "%s: error joining ring threads: %s\n",
                                           program_basename, strerror(status));
          return EXIT_FAIL;
        }

        joined[i] = 1;
        if ((intptr_t)vpstatus == (intptr_t)RXTX_ERROR) {
          fprintf(stderr, "%s: %s\n", program_basename, errbuf);
          return EXIT_FAIL;
        }
      }
    }

    if (ebusy) {
      usleep(10);
    } else {
      break;
    }
  }

  pthread_attr_destroy(&attr);

  /*
   * This loop prints our per-ring, in this case per-cpu, results.
   */
  out = stdout;
  if (rxtx_get_savefile_template(&rtd) &&
                          strcmp(rxtx_get_savefile_template(&rtd), "-") == 0) {
    out = stderr;
  }

  for_each_set_ring(i, &rtd) {
    ring = rxtx_get_ring(&rtd, (unsigned int)i);
    if (!ring) {
      fprintf(stderr, "%s: %s\n", program_basename, errbuf);
      return EXIT_FAIL;
    }

    fprintf(out, "%ju packets captured on cpu%d.\n",
                                      rxtx_ring_get_packets_received(ring), i);
  }

  fprintf(out, "%ju packets captured total.\n",
                                              rxtx_get_packets_received(&rtd));

  status = rxtx_close(&rtd);
  if (status == RXTX_ERROR) {
    fprintf(stderr, "%s: %s\n", program_basename, errbuf);
    return EXIT_FAIL;
  }

  return EXIT_OK;
}

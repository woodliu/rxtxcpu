/*
 * Copyright (c) 2018-present StackPath, LLC
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define _GNU_SOURCE // for GNU basename()

#define PCAP_DONT_INCLUDE_PCAP_BPF_H 1

#include "cpu.h"       // for get_numa_cpu_set(), get_online_cpu_set(),
                       //     parse_cpu_list(), parse_cpu_mask()
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

#include <linux/bpf.h>       // for bpf_attr, bpf_insn, BPF_PROG_LOAD,
                             //     BPF_PROG_TYPE_SOCKET_FILTER
#include <linux/if_packet.h> // for PACKET_FANOUT_EBPF
#include <linux/unistd.h>    // for __NR_bpf

#include <ctype.h>    // for isspace()
#include <dirent.h>   // for closedir(), DIR, dirent, opendir(), readdir()
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
#include <unistd.h>   // for syscall(), usleep()

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

#define SUBJECT "numa"
#define USUBJECT "NUMA"
#define RXTXSELF "rxtx" SUBJECT
#define RXSELF "rx" SUBJECT
#define TXSELF "tx" SUBJECT
#define FSUBJECT SUBJECT " node"
#define HSUBJECT SUBJECT "-node"
#define FSUBJECTS FSUBJECT "s"
#define FLIST SUBJECT " list"
#define HLIST SUBJECT "-list"
#define ULIST USUBJECT "LIST"
#define FMASK SUBJECT " mask"
#define HMASK SUBJECT "-mask"
#define UMASK USUBJECT "MASK"

static const struct option long_options[] = {
  {"count",           required_argument, NULL, 'c'},
  {"direction",       required_argument, NULL, 'd'},
  {"help",            no_argument,       NULL, 'h'},
  {HLIST,             required_argument, NULL, 'l'},
  {HMASK,             required_argument, NULL, 'm'},
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
                       " '" RXSELF "', 'tx' when '" TXSELF "', and 'rxtx' when"
                                                          " '" RXTXSELF "')."},
  {'h', NULL,        "Display this help and exit."},
  {'l', ULIST,       "Capture only on " FSUBJECTS " in " ULIST " (e.g. if "
                        ULIST " is '0,2-4,6', only packets on " FSUBJECTS " 0,"
                                         " 2, 3, 4, and 6 will be captured)."},
  {'m', UMASK,       "Capture only on " FSUBJECTS " in " UMASK " (e.g. if "
                       UMASK " is '5d', only packets on " FSUBJECTS " 0, 2, 3,"
                                               " 4, and 6 will be captured)."},
  {'p', NULL,        "Put the interface into promiscuous mode."},
  {'U', NULL,        "When writing to a pcap file, the write buffer will be"
                           " flushed just after each packet is placed in it."},
  {'v', NULL,        "Display more verbose output."},
  {'V', NULL,        "Display the version and exit."},
  {'w', "FILE",      "Write packets to FILE in pcap format. FILE is used as a"
                            " template for per-" HSUBJECT " filenames (e.g. if"
                                " capturing on a system with 2 " FSUBJECTS ", "
                         FSUBJECTS " 0 and 1, and FILE set to 'out.pcap', the "
                          FSUBJECT " 0 capture will be written to 'out-0.pcap'"
                           " and the " FSUBJECT " 1 capture will be written to"
                            " 'out-1.pcap'). Writing to stdout is supported by"
                           " setting FILE to '-', but only when capturing on a"
                                                      " single " FSUBJECT "."},
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
static int sysfs_count(const char *path, const char *prefix) {
  int greatest = -1;

  DIR *d = opendir(path);
  if (!d) {
    return -1;
  }

  struct dirent *dir = NULL;
  while ((dir = readdir(d)) != NULL) {
    char *endptr = NULL;

    char *p = dir->d_name;
    if (!p) {
      continue;
    }

    if (strncmp(p, prefix, strlen(prefix))) {
      continue;
    }
    p += strlen(prefix);

    int current = strtol(p, &endptr, 10);
    if (*endptr) {
      continue;
    }

    if (greatest < current) {
      greatest = current;
    }
  }

  closedir(d);

  return greatest + 1;
}

/* ========================================================================= */
int num_numa(void) {
  return sysfs_count("/sys/devices/system/node/", "node");
}

/* ========================================================================= */
int get_online_numa_set(cpu_set_t *numa_set) {
  CPU_ZERO(numa_set);

  int i = 0;
  int n = num_numa();

  cpu_set_t online;
  cpu_set_t current_numa;
  cpu_set_t current_numa_online;

  CPU_ZERO(&online);

  if (get_online_cpu_set(&online) != 0) {
    return -1;
  }

  for (i = 0; i < n; i++) {
    CPU_ZERO(&current_numa);
    CPU_ZERO(&current_numa_online);

    if (get_numa_cpu_set(&current_numa, i) != 0) {
      return -1;
    }

    CPU_AND(&current_numa_online, &online, &current_numa);

    if (CPU_COUNT(&current_numa_online) > 0) {
      CPU_SET(i, numa_set);
    }
  }

  return 0;
}

/* ========================================================================= */
int main(int argc, char **argv) {
  program_basename = basename(argv[0]);

  int c = 0;
  int rings = 0;
  int i = 0;
  int status = 0;
  int worker_count = 0;

  bool help = false;

  char *badopt = NULL;
  char *list = NULL;
  char *mask = NULL;
  char *endptr = NULL;

  FILE *out = stdout;

  char errbuf[RXTX_ERRBUF_SIZE] = "";

  struct rxtx_desc rtd;
  rxtx_init(&rtd, errbuf);

  struct rxtx_ring* ring;

  status = rxtx_set_fanout_mode(&rtd, PACKET_FANOUT_EBPF);
  if (status == RXTX_ERROR) {
    fprintf(stderr, "%s: %s\n", program_basename, errbuf);
    return EXIT_FAIL;
  }

  static char log_buf[65536];

  struct bpf_insn prog[] = {
    /*
     * PACKET_FANOUT_EBPF_NUMA - New functionality! I guess numa really is the
     *                           answer to life, the universe, and everything.
     *
     *   return bpf_get_numa_node_id();
     */
    { 0x85, 0x0, 0x0, 0x0000, 0x0000002a }, // call 42
    { 0x95, 0x0, 0x0, 0x0000, 0x00000000 }  // exit
  };

  union bpf_attr battr;
  int pfd;

  memset(&battr, 0, sizeof(battr));
  battr.prog_type = BPF_PROG_TYPE_SOCKET_FILTER;
  battr.insns = (unsigned long) prog;
  battr.insn_cnt = sizeof(prog) / sizeof(prog[0]);
  battr.license = (unsigned long) "Dual MIT/GPL";
  battr.log_buf = (unsigned long) log_buf,
  battr.log_size = sizeof(log_buf),
  battr.log_level = 1,

  pfd = syscall(__NR_bpf, BPF_PROG_LOAD, &battr, sizeof(battr));
  if (pfd < 0) {
    fprintf(stderr, "%s: error loading bpf program: bpf verifier:\n%s\n",
                                                    program_basename, log_buf);
    return EXIT_FAIL;
  }

  status = rxtx_set_fanout_data_fd(&rtd, pfd);
  if (status == RXTX_ERROR) {
    fprintf(stderr, "%s: %s\n", program_basename, errbuf);
    return EXIT_FAIL;
  }

  /*
   * direction default is based on the invocation.
   */
  if (strcmp(program_basename, RXSELF) == 0) {
    status = rxtx_set_direction(&rtd, PCAP_D_IN);
  } else if (strcmp(program_basename, TXSELF) == 0) {
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
        list = optarg;
        if (parse_cpu_list(optarg, &ring_set)) {
          fprintf(stderr, "%s: Invalid " FLIST " '%s'.\n", program_basename,
                                                                       optarg);
          usage_short();
          return EXIT_FAIL_OPTION;
        }
        break;

      case 'm':
        mask = optarg;
        if (parse_cpu_mask(optarg, &ring_set)) {
          fprintf(stderr, "%s: Invalid " FMASK " '%s'.\n", program_basename,
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
        printf("%s version %s\n", program_basename, RXTXUTILS_VERSION);
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

  if (list && mask) {
    fprintf(stderr, "%s: -l [--" HLIST "] and -m [--" HMASK "] are mutually"
                                            " exclusive.\n", program_basename);
    usage_short();
    return EXIT_FAIL_OPTION;
  }

  rings = num_numa();
  if (rings <= 0) {
    fprintf(stderr, "%s: Failed to get " FSUBJECT " count.\n",
                                                             program_basename);
    return EXIT_FAIL;
  }

  status = rxtx_set_ring_count(&rtd, rings);
  if (status == RXTX_ERROR) {
    fprintf(stderr, "%s: %s\n", program_basename, errbuf);
    return EXIT_FAIL;
  }

  if (rxtx_verbose_isset(&rtd)) {
    fprintf(stderr, "Found '%d' " FSUBJECTS ".\n", rxtx_get_ring_count(&rtd));
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
    fprintf(stderr, "%s: No configured " FSUBJECTS " present in %s.\n",
                                       program_basename, list ? FLIST : FMASK);
    usage_short();
    return EXIT_FAIL_OPTION;
  }

  cpu_set_t online;
  if (get_online_numa_set(&online) != 0) {
    fprintf(stderr, "%s: Failed to get online " FSUBJECT " set.\n",
                                                             program_basename);
    return EXIT_FAIL_OPTION;
  }

  if (CPU_COUNT(&online) != rxtx_get_ring_count(&rtd)) {
    for_each_ring(i, &rtd) {
      if (!CPU_ISSET(i, &online)) {
        RING_CLR(i, &ring_set);
        if (rxtx_verbose_isset(&rtd)) {
          fprintf(stderr, "Skipping " FSUBJECT " '%d' since it is offline.\n",
                                                                            i);
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
    fprintf(stderr, "%s: No online " FSUBJECTS " present in %s.\n",
                                       program_basename, list ? FLIST : FMASK);
    usage_short();
    return EXIT_FAIL_OPTION;
  }

  if (rxtx_get_savefile_template(&rtd) &&
                          strcmp(rxtx_get_savefile_template(&rtd), "-") == 0 &&
                                                  RING_COUNT(&ring_set) != 1) {
    fprintf(stderr, "%s: Write file '-' (stdout) is only permitted when"
                   " capturing on a single " FSUBJECT ".\n", program_basename);
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
    get_numa_cpu_set(&cpu_set, i);
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
   * This loop prints our per-ring results.
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

    fprintf(out, "%ju packets captured on " FSUBJECT " %d.\n",
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

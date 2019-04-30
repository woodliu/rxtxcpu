#define _GNU_SOURCE

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_tun.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define EXIT_OK          0
#define EXIT_FAIL        1
#define EXIT_FAIL_OPTION 2

char *program_basename;

volatile sig_atomic_t keep_running = 1;

static void sigint_handler(int signal) {
  keep_running = 0;
  write(STDERR_FILENO, "\n", 1);
}

static int setup_signals() {
  struct sigaction sa;
  sa.sa_handler = &sigint_handler;
  sa.sa_flags = SA_RESTART;
  sigfillset(&sa.sa_mask);
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    fprintf(
      stderr,
      "%s: Failed to setup signal handler for SIGINT.\n",
      program_basename
    );
    return -1;
  }
  return 0;
}

static void usage_short(void) {
  fprintf(
    stderr,
    "Usage: %s\n",
    program_basename
  );
}

int main(int argc, char **argv) {
  program_basename = basename(argv[0]);

  if (argc > 0) {
    fprintf(
      stderr,
      "%s: Invalid number of arguments supplied.\n",
      program_basename
    );
    usage_short();
    return EXIT_FAIL_OPTION;
  }

  int queue_count = sysconf(_SC_NPROCESSORS_CONF);
  if (queue_count <= 0) {
    fprintf(stderr, "%s: Failed to get processor count.\n", program_basename);
    return EXIT_FAIL;
  }

  int i;
  int *fds;
  fds = (int *)malloc(queue_count * sizeof(int));

  for (i = 0; i < queue_count; i++) {
    fds[i] = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fds[i] == -1) {
      fprintf(
        stderr,
        "%s: Failed to create socket: %s\n",
        program_basename,
        strerror(errno)
      );
      exit(EXIT_FAIL);
    }

    int fanout_group_id = 0;
    int fanout_arg = (fanout_group_id | (PACKET_FANOUT_CPU << 16));
    if (setsockopt(
          fds[i],
          SOL_PACKET,
          PACKET_FANOUT,
          &fanout_arg,
          sizeof(fanout_arg)
        ) < 0) {
      fprintf(stderr, "%s: Failed to configure fanout.\n", program_basename);
      exit(EXIT_FAIL);
    }
  }

  setup_signals();

  while(keep_running) {
    sleep(1);
  }

  free(fds);

  return 0;
}

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

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

#define PACKET_BUFFER_SIZE 65535

#define ETHERNET_HEADER_LEN 14
#define IPV4_HEADER_LEN     20
#define ICMP_HEADER_LEN      8

#define MAC_ADDR_LEN  6
#define IPV4_ADDR_LEN 4

#define ETHERNET_HEADER_OFFSET 0
#define IPV4_HEADER_OFFSET     ETHERNET_HEADER_LEN
#define ICMP_HEADER_OFFSET     (ETHERNET_HEADER_LEN + IPV4_HEADER_LEN)
#define PAYLOAD_OFFSET         (ETHERNET_HEADER_LEN + IPV4_HEADER_LEN + ICMP_HEADER_LEN)

#define MAC_ADDRS_OFFSET ETHERNET_HEADER_OFFSET

#define IP_VERSION_OFFSET IPV4_HEADER_OFFSET
#define IPV4_IDENT_OFFSET (IPV4_HEADER_OFFSET +  4)
#define IPV4_FLAGS_OFFSET (IPV4_HEADER_OFFSET +  6)
#define IPV4_PROTO_OFFSET (IPV4_HEADER_OFFSET +  9)
#define IPV4_CKSM_OFFSET  (IPV4_HEADER_OFFSET + 10)
#define IPV4_ADDRS_OFFSET (IPV4_HEADER_OFFSET + IPV4_HEADER_LEN - IPV4_ADDR_LEN * 2)

#define ICMP_TYPE_OFFSET ICMP_HEADER_OFFSET
#define ICMP_CKSM_OFFSET (ICMP_HEADER_OFFSET + 2)

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

static void checksum_plus(unsigned char *p, unsigned int n) {
  unsigned int checksum = ntohs(
    (uint16_t) (p[0] | ((unsigned int) p[1] << 8))
  );
  checksum = ((unsigned long) checksum + n) % 65535;
  p[0] = (unsigned char) (htons(checksum) & 0xff);
  p[1] = (unsigned char) ((htons(checksum) >> 8) & 0xff);
}

static void checksum_minus(unsigned char *p, unsigned int n) {
  checksum_plus(p, ((unsigned long) 65535 - n));
}

/*
 * Treat len bytes as a ring and rotate all bytes in the ring counterclockwise
 * one position (i.e. one index lower).
 *
 * For example, when len is 4, the following transformation occurs.
 *   [0][1][2][3] ->
 *   [1][2][3][0]
 */
static void rotate_bytes(unsigned char *p, unsigned int len) {
  unsigned int i = 0;
  unsigned char tmp = p[i];
  for (++i; i < len; i++) {
    p[i - 1] = p[i];
  }
  p[i - 1] = tmp;
}

static void zero_bytes(unsigned char *p, unsigned int len) {
  for (unsigned int i = 0; i < len; i++) {
    p[i] = '\0';
  }
}

/*
 * This implementation of tun_alloc_mq() is copied nearly verbatim from the
 * tuntap documentation.
 *
 *   https://github.com/torvalds/linux/blob/f422d2a04fe2e661fd439c19197a162cc9a36416/Documentation/networking/tuntap.txt#L124-L158
 *
 * Due to the lack of SPDX License Identifiers in this file, I reached out to
 * the original author of tun_alloc_mq(), Jason Wang <jasowang@redhat.com>, and
 * obtained his permission to release this function here under the MIT license.
 */
static int tun_alloc_mq(char *dev, int queues, int *fds) {
  struct ifreq ifr;
  int fd, err, i;

  if (!dev) {
    return -1;
  }

  memset(&ifr, 0, sizeof(ifr));
  /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
   *        IFF_TAP   - TAP device
   *
   *        IFF_NO_PI - Do not provide packet information
   *        IFF_MULTI_QUEUE - Create a queue of multiqueue device
   */
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI | IFF_MULTI_QUEUE;
  strcpy(ifr.ifr_name, dev);

  for (i = 0; i < queues; i++) {
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
      goto err;
    }
    err = ioctl(fd, TUNSETIFF, (void *)&ifr);
    if (err) {
      close(fd);
      goto err;
    }
    fds[i] = fd;
  }

  return 0;
err:
  for (--i; i >= 0; i--) {
    close(fds[i]);
  }
  return err;
}

struct queue_ping_handler_arguments {
  int fd;
};

static void *queue_ping_handler(void *args) {
  struct queue_ping_handler_arguments *qargs = args;
  int fd = qargs->fd;

  while(keep_running) {
    unsigned char packet_buffer[PACKET_BUFFER_SIZE];
    int packet_length = read(fd, packet_buffer, sizeof(packet_buffer));

    if (
      packet_length < 0
        || (packet_buffer[IP_VERSION_OFFSET] & 0xf0) != 0x40 // Must be IPv4.
        || packet_buffer[IPV4_PROTO_OFFSET] != 1             // Must be ICMP.
        || packet_buffer[ICMP_TYPE_OFFSET] != 8      // Must be echo request.
    ) {
      continue;
    }

    /*
     * Swap our src and dst mac addrs; to do this our bytes are treated as being
     * in a ring and rotated halfway around the ring.
     */
    for (int i = 0; i < MAC_ADDR_LEN; i++) {
      rotate_bytes(&packet_buffer[MAC_ADDRS_OFFSET], MAC_ADDR_LEN * 2);
    }

    checksum_minus(&packet_buffer[IPV4_CKSM_OFFSET], (unsigned int) 1);
    checksum_plus(&packet_buffer[IPV4_IDENT_OFFSET], (unsigned int) 1);

    checksum_plus(
      &packet_buffer[IPV4_CKSM_OFFSET],
      (unsigned int) ntohs(
        (uint16_t) (
          packet_buffer[IPV4_FLAGS_OFFSET]
            | ((unsigned int) packet_buffer[IPV4_FLAGS_OFFSET + 1] << 8)
        )
      )
    );
    zero_bytes(&packet_buffer[IPV4_FLAGS_OFFSET], 1);

    /*
     * Swap our src and dst ip addrs; to do this our bytes are treated as being
     * in a ring and rotated halfway around the ring.
     */
    for (int i = 0; i < IPV4_ADDR_LEN; i++) {
      rotate_bytes(&packet_buffer[IPV4_ADDRS_OFFSET], IPV4_ADDR_LEN * 2);
    }

    checksum_plus(
      &packet_buffer[ICMP_CKSM_OFFSET],
      ((unsigned int) packet_buffer[ICMP_TYPE_OFFSET] << 8)
    );
    zero_bytes(&packet_buffer[ICMP_TYPE_OFFSET], 1);

    write(fd, packet_buffer, packet_length);
  }

  return 0;
}

static void usage_short(void) {
  fprintf(
    stderr,
    "Usage: %s <interface>\n",
    program_basename
  );
}

int main(int argc, char **argv) {
  program_basename = basename(argv[0]);

  if (argc < 1) {
    fprintf(
      stderr,
      "%s: Invalid number of arguments supplied.\n",
      program_basename
    );
    usage_short();
    return EXIT_FAIL_OPTION;
  }

  char *ifname = argv[1];

  int queue_count = sysconf(_SC_NPROCESSORS_CONF);
  if (queue_count <= 0) {
    fprintf(stderr, "%s: Failed to get processor count.\n", program_basename);
    return EXIT_FAIL;
  }

  int i;
  int err;
  int *fds;
  fds = (int *)malloc(queue_count * sizeof(int));

  err = tun_alloc_mq(ifname, queue_count, fds);
  if (err) {
    fprintf(stderr, "%s: Failed to allocate queues.\n", program_basename);
    return EXIT_FAIL;
  }

  struct queue_ping_handler_arguments queue_args[queue_count];
  pthread_t queue_threads[queue_count];
  for (i = 0; i < queue_count; i++) {
    int flags = fcntl(fds[i], F_GETFL, 0);
    fcntl(fds[i], F_SETFL, flags | O_NONBLOCK);

    queue_args[i].fd = fds[i];
  }

  setup_signals();

  pthread_attr_t attr;
  pthread_attr_init(&attr);

  for (i = 0; i < queue_count; i++) {
    pthread_create(&queue_threads[i], &attr, queue_ping_handler, (void *)&queue_args[i]);
  }

  for (i = 0; i < queue_count; i++) {
    pthread_join(queue_threads[i], NULL);
  }

  free(fds);

  return 0;
}

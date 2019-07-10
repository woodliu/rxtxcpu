CC = gcc
CFLAGS = -Wall -Wcast-align -Wcast-qual -Wimplicit -Wpointer-arith -Wredundant-decls -Wreturn-type -Wshadow

.PHONY: all
all: \
  test__rxtx_savefile_open__strdup__failure \
  test__rxtx_savefile_open__pcap_open_dead__failure \
  test__rxtx_savefile_open__pcap_dump_open__failure \
  test__rxtx_savefile_dump__pcap_dump_flush__failure \
  test__rxtx_savefile_close__pcap_dump_flush__failure

test__rxtx_savefile_open__strdup__failure: EXTRA_CFLAGS = \
	-DTEST_STRDUP_FAILURE

test__rxtx_savefile_open__pcap_open_dead__failure: EXTRA_CFLAGS = \
	-DTEST_PCAP_OPEN_DEAD_FAILURE

test__rxtx_savefile_open__pcap_dump_open__failure: EXTRA_CFLAGS = \
	-DTEST_PCAP_DUMP_OPEN_FAILURE

test__rxtx_savefile_dump__pcap_dump_flush__failure \
  test__rxtx_savefile_close__pcap_dump_flush__failure: EXTRA_CFLAGS = \
	-DTEST_PCAP_DUMP_FLUSH_FAILURE

%: %.c ../../rxtx_savefile.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $^ -lpcap -DTESTING

.PHONY: test
test: all
	./test__rxtx_savefile_open__strdup__failure
	./test__rxtx_savefile_open__pcap_open_dead__failure
	./test__rxtx_savefile_open__pcap_dump_open__failure
	./test__rxtx_savefile_dump__pcap_dump_flush__failure
	./test__rxtx_savefile_close__pcap_dump_flush__failure

.PHONY: clean
clean:
	rm -f \
	  test__rxtx_savefile_open__strdup__failure \
	  test__rxtx_savefile_open__pcap_open_dead__failure \
	  test__rxtx_savefile_open__pcap_dump_open__failure \
	  test__rxtx_savefile_dump__pcap_dump_flush__failure \
	  test__rxtx_savefile_close__pcap_dump_flush__failure

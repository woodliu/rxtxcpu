CC = gcc
CFLAGS = -Wall -Wcast-align -Wcast-qual -Wimplicit -Wpointer-arith -Wredundant-decls -Wreturn-type -Wshadow
PREFIX = /usr/local

include VERSION

all: rxtxcpu rxcpu txcpu

rxtxcpu.o: EXTRA_CFLAGS = \
	-std=c99 \
	'-DRXTXCPU_VERSION="$(RXTXCPU_VERSION)"'

cpu.o rxtx.o: EXTRA_CFLAGS = \
	-std=c99

%.o: %.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $<

rxtxcpu rxcpu txcpu: cpu.o ext.o interface.o rxtx.o rxtx_savefile.o rxtx_stats.o rxtxcpu.o sig.o
	$(CC) $(CFLAGS) -o rxtxcpu $^ -lpcap -lpthread
	rm -f rxcpu txcpu
	ln -s rxtxcpu rxcpu
	ln -s rxtxcpu txcpu

.PHONY: clean
clean:
	rm -f cpu.o ext.o interface.o rxtx.o rxtx_savefile.o rxtx_stats.o rxtxcpu.o sig.o rxtxcpu rxcpu txcpu

.PHONY: install
install: rxtxcpu rxcpu txcpu
	mkdir -p $(DESTDIR)$(PREFIX)/sbin
	install $^ $(DESTDIR)$(PREFIX)/sbin/

.PHONY: uninstall
uninstall:
	rm $(DESTDIR)$(PREFIX)/sbin/rxtxcpu
	rm $(DESTDIR)$(PREFIX)/sbin/rxcpu
	rm $(DESTDIR)$(PREFIX)/sbin/txcpu

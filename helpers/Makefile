CC = gcc
CFLAGS = -Wall -Wcast-align -Wcast-qual -Wimplicit -Wpointer-arith -Wredundant-decls -Wreturn-type -Wshadow

ifndef PREFIX
PREFIX = /usr
endif

ifndef SBINDIR
SBINDIR = $(PREFIX)/sbin
endif

ifndef BINDIR
BINDIR = $(PREFIX)/bin
endif

ifndef SYSCONFDIR
SYSCONFDIR = /etc
endif

ifndef UNITDIR
UNITDIR = $(PREFIX)/lib/systemd/system
endif

all: hold-fanout-group-id-zero tap-mq-pong

tap-mq-pong: tap_mq_pong.c
	$(CC) $(CFLAGS) -std=c99 -D_POSIX_C_SOURCE=200809L -o $@ $^ -lpthread

hold-fanout-group-id-zero: hold_fanout_group_id_zero.c
	$(CC) $(CFLAGS) -std=c99 -D_POSIX_C_SOURCE=200809L -o $@ $^ -lpthread

.PHONY: clean
clean:
	rm -f tap-mq-pong

.PHONY: install
# This Makefile is tailored for our testing environment; this install target
# is naive and may have undesirable results in other environments. Require
# /vagrant/Vagrantfile as a simple guard to (hopefully) ensure we're in our
# testing environment.
install: /vagrant/Vagrantfile tap-mq-configure-packet-steering.sh tap-mq-destroy.sh tap-mq-init.sh tap-mq-pong tap-mq-pong@.service
	mkdir -p $(DESTDIR)$(SBINDIR)
	mkdir -p $(DESTDIR)$(UNITDIR)
	mkdir -p $(DESTDIR)$(SYSCONFDIR)/sysconfig/tap-mq-pong/
	install -m 755 hold-fanout-group-id-zero           $(DESTDIR)$(SBINDIR)/
	install -m 755 tap-mq-configure-packet-steering.sh $(DESTDIR)$(SBINDIR)/tap-mq-configure-packet-steering
	install -m 755 tap-mq-destroy.sh                   $(DESTDIR)$(SBINDIR)/tap-mq-destroy
	install -m 755 tap-mq-init.sh                      $(DESTDIR)$(SBINDIR)/tap-mq-init
	install -m 755 tap-mq-pong                         $(DESTDIR)$(BINDIR)/
	install -m 644 tap-mq-pong@.service                $(DESTDIR)$(UNITDIR)/
	systemctl daemon-reload

.PHONY: uninstall
# See /vagrant/Vagrantfile comment above install target; the same applies to
# uninstall.
uninstall: /vagrant/Vagrantfile
	rm $(DESTDIR)$(SBINDIR)/hold-fanout-group-id-zero
	rm $(DESTDIR)$(SBINDIR)/tap-mq-configure-packet-steering
	rm $(DESTDIR)$(SBINDIR)/tap-mq-destroy
	rm $(DESTDIR)$(SBINDIR)/tap-mq-init
	rm $(DESTDIR)$(BINDIR)/tap-mq-pong
	rm $(DESTDIR)$(UNITDIR)/tap-mq-pong@.service
	systemctl daemon-reload

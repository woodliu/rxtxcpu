CC = gcc
CFLAGS = -Wall -Wcast-align -Wcast-qual -Wimplicit -Wpointer-arith -Wredundant-decls -Wreturn-type -Wshadow

rss-hash: main.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: test
test: rss-hash
	./test.sh

.PHONY: clean
clean:
	rm -f rss-hash

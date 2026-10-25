CC ?= cc
CFLAGS ?= -std=c89 -O2 -Wall -Wextra -pedantic

.PHONY: all clean test install

all: asmc

asmc: asmc.c
	$(CC) $(CFLAGS) -o $@ $<

test: asmc hello.s
	./asmc hello.s /tmp/asmc-test.bin
	test "$$(wc -c < /tmp/asmc-test.bin)" -eq 16

install: asmc
	mkdir -p $(DESTDIR)/usr/bin
	cp asmc $(DESTDIR)/usr/bin/asmc

clean:
	rm -f asmc /tmp/asmc-test.bin

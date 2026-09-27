CC ?= cc
PKG_CONFIG ?= pkg-config
CFLAGS ?= -O2
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic

TCL_CFLAGS := $(shell $(PKG_CONFIG) --cflags tcl 2>/dev/null)
TCL_LIBS := $(shell $(PKG_CONFIG) --libs tcl 2>/dev/null)

ifeq ($(strip $(TCL_LIBS)),)
TCL_LIBS := -ltcl
endif

.PHONY: all clean check

all: ticle

ticle: src/ticle.c src/irc.c src/irc.h src/config.c src/config.h src/state.c src/state.h src/backoff.c src/backoff.h src/nick.c src/nick.h src/outqueue.c src/outqueue.h
	$(CC) $(CFLAGS) $(TCL_CFLAGS) -o $@ src/ticle.c src/irc.c src/config.c src/state.c src/backoff.c src/nick.c src/outqueue.c $(TCL_LIBS)

tests/test_irc: tests/test_irc.c src/irc.c src/irc.h
	$(CC) $(CFLAGS) -o $@ tests/test_irc.c src/irc.c

tests/test_config: tests/test_config.c src/config.c src/config.h
	$(CC) $(CFLAGS) -o $@ tests/test_config.c src/config.c

tests/test_state: tests/test_state.c src/state.c src/state.h
	$(CC) $(CFLAGS) -o $@ tests/test_state.c src/state.c

tests/test_backoff: tests/test_backoff.c src/backoff.c src/backoff.h
	$(CC) $(CFLAGS) -o $@ tests/test_backoff.c src/backoff.c

tests/test_nick: tests/test_nick.c src/nick.c src/nick.h
	$(CC) $(CFLAGS) -o $@ tests/test_nick.c src/nick.c

tests/test_outqueue: tests/test_outqueue.c src/outqueue.c src/outqueue.h
	$(CC) $(CFLAGS) -o $@ tests/test_outqueue.c src/outqueue.c

check: ticle tests/test_irc tests/test_config tests/test_state tests/test_backoff tests/test_nick tests/test_outqueue
	./ticle 2>/dev/null || true
	./tests/test_irc
	./tests/test_config
	./tests/test_state
	./tests/test_backoff
	./tests/test_nick
	./tests/test_outqueue
	tclsh tests/test_core.tcl

clean:
	rm -f ticle tests/test_irc tests/test_config tests/test_state tests/test_backoff tests/test_nick tests/test_outqueue tests/.test-config.tmp

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

ticle: src/ticle.c src/irc.c src/irc.h src/config.c src/config.h
	$(CC) $(CFLAGS) $(TCL_CFLAGS) -o $@ src/ticle.c src/irc.c src/config.c $(TCL_LIBS)

tests/test_irc: tests/test_irc.c src/irc.c src/irc.h
	$(CC) $(CFLAGS) -o $@ tests/test_irc.c src/irc.c

tests/test_config: tests/test_config.c src/config.c src/config.h
	$(CC) $(CFLAGS) -o $@ tests/test_config.c src/config.c

check: ticle tests/test_irc tests/test_config
	./ticle 2>/dev/null || true
	./tests/test_irc
	./tests/test_config
	tclsh tests/test_core.tcl

clean:
	rm -f ticle tests/test_irc tests/test_config tests/.test-config.tmp

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

ticle: src/ticle.c src/irc.c src/irc.h
	$(CC) $(CFLAGS) $(TCL_CFLAGS) -o $@ src/ticle.c src/irc.c $(TCL_LIBS)

tests/test_irc: tests/test_irc.c src/irc.c src/irc.h
	$(CC) $(CFLAGS) -o $@ tests/test_irc.c src/irc.c

check: ticle tests/test_irc
	./ticle 2>/dev/null || true
	./tests/test_irc
	tclsh tests/test_core.tcl

clean:
	rm -f ticle tests/test_irc

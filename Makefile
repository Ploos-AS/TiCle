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

ticle: src/ticle.c
	$(CC) $(CFLAGS) $(TCL_CFLAGS) -o $@ $< $(TCL_LIBS)

check: ticle
	./ticle 2>/dev/null || [ $$$$? -eq 2 ]
	tclsh tests/test_core.tcl

clean:
	rm -f ticle

#
#                     OpenBIOS - free your system!
#                             ( Utilities )
#
#  This program is part of a free implementation of the IEEE 1275-1994
#  Standard for Boot (Initialization Configuration) Firmware.
#
#  Copyright (C) 2006-2009 coresystems GmbH
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; version 2 of the License.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
#

VERSION:=$(shell grep "^\#.*TOKE_VERSION" < toke/toke.c |cut -f2 -d\" )

DESTDIR = /usr/local
CC      ?= gcc
STRIP	= strip
INCLUDES=-I shared -I.

# Normal Flags:
CFLAGS  = -O2 -Wall #-Wextra
LDFLAGS =

# Coverage:
#CFLAGS  := $(CFLAGS) -fprofile-arcs -ftest-coverage
#LDFLAGS := $(LDFLAGS) -lgcov

# Debugging:
#CFLAGS := $(CFLAGS) -g

PROGRAMS = toke/toke detok/detok romheaders/romheaders

TOKE_SOURCE  = toke/clflags.c toke/conditl.c toke/devnode.c toke/dictionary.c
TOKE_SOURCE += toke/emit.c toke/errhandler.c toke/flowcontrol.c toke/macros.c
TOKE_SOURCE += toke/nextfcode.c toke/parselocals.c toke/scanner.c toke/stack.c
TOKE_SOURCE += toke/stream.c toke/strsubvocab.c toke/ticvocab.c toke/toke.c
TOKE_SOURCE += toke/tokzesc.c toke/tracesyms.c toke/usersymbols.c shared/classcodes.c
TOKE_OBJECTS := $(TOKE_SOURCE:.c=.o)

DETOK_SOURCE  = detok/addfcodes.c detok/decode.c detok/detok.c detok/dictionary.c
DETOK_SOURCE += detok/pcihdr.c detok/printformats.c detok/stream.c shared/classcodes.c
DETOK_OBJECTS := $(DETOK_SOURCE:.c=.o)

ROMHEADERS_SOURCE = romheaders/romheaders.c shared/classcodes.c
ROMHEADERS_OBJECTS := $(ROMHEADERS_SOURCE:.c=.o)

all: .dependencies $(PROGRAMS)

toke/toke: $(TOKE_OBJECTS)
	@echo "    LD    $@"
	@$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^
	@$(STRIP) -s $@

detok/detok: $(DETOK_OBJECTS)
	@echo "    LD    $@"
	@$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^
	@$(STRIP) -s $@

romheaders/romheaders: $(ROMHEADERS_OBJECTS)
	@echo "    LD    $@"
	@$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^
	@$(STRIP) -s $@

%.o: %.c
	@echo "    CC    $@"
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $^

.dependencies: $(TOKE_SOURCE)  $(DETOK_SOURCE) $(ROMHEADERS_SOURCE)
	@$(CC) $(CFLAGS) $(INCLUDES) -MM $^ > .dependencies

clean:
	rm -rf $(PROGRAMS) $(TOKE_OBJECTS) $(DETOK_OBJECTS) $(ROMHEADERS_OBJECTS)
	rm -f .dependencies
	$(MAKE) -C testsuite clean

-include .dependencies

install:
	mkdir -p $(DESTDIR)/bin
	cp -a $(PROGRAMS) $(DESTDIR)/bin

distclean: clean
	$(MAKE) -C testsuite distclean
	find . -name "*.gcda" -exec rm -f \{\} \;
	find . -name "*.gcno" -exec rm -f \{\} \;

tests: all
	cp toke/toke testsuite
	cp detok/detok testsuite
	cp romheaders/romheaders testsuite
	$(MAKE) -C testsuite all CygTestLogs=`pwd`/testlogs/testlogs-ppc-linux
	#$(MAKE) -C testsuite all CygTestLogs=`pwd`/testlogs/testlogs-x86-cygwin

# lcov required for html reports
coverage:
	@testsuite/GenCoverage . fcode-suite-$(VERSION) "FCode suite $(VERSION)"
	@testsuite/GenCoverage toke toke-$(VERSION) "Toke $(VERSION)"

.PHONY: all clean distclean tests


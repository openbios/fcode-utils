#
#                     OpenBIOS - free your system!
#                             ( Utilities )
#
#  This program is part of a free implementation of the IEEE 1275-1994
#  Standard for Boot (Initialization Configuration) Firmware.
#
#  Copyright (C) 2001-2006  Stefan Reinauer <stepan@openbios.org>
#  Copyright (C) 2006-2009 coresystems GmbH <info@coresystems.de>
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

PROGRAM = toke

CC      ?= gcc
STRIP	= strip
INCLUDES = -I../shared

# Normal flags
CFLAGS  = -O2 -Wall #-Wextra 
LDFLAGS =

# Coverage:
#CFLAGS  := $(CFLAGS) -fprofile-arcs -ftest-coverage
#LDFLAGS := $(LDFLAGS) -lgcov

# Debugging:
# CFLAGS  := $(CFLAGS) -g

# for gcc 4. remove if you fixed the code.
_GCC4_CFLAGS=$(shell echo 'main(int argc, char **argv){return 0;}' > .test.c;   \
		$(CC) -Wno-pointer-sign -c .test.c -o .test.o > /dev/null 2>&1 \
		&& echo -Wno-pointer-sign; rm .test.c .test.o )
CFLAGS  := $(CFLAGS) $(_GCC4_CFLAGS)

OBJS  = clflags.o conditl.o devnode.o dictionary.o emit.o errhandler.o     \
        flowcontrol.o macros.o nextfcode.o parselocals.o scanner.o stack.o \
	stream.o strsubvocab.o ticvocab.o toke.o tokzesc.o tracesyms.o     \
	usersymbols.o ../shared/classcodes.o

all: .dependencies $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) -o $(PROGRAM) $(OBJS) $(LDFLAGS)
	$(STRIP) $(PROGRAM)

clean:
	rm -f $(OBJS) *~
	rm -f $(PROGRAM) .dependencies

.dependencies: *.c 
	@$(CC) $(CFLAGS) $(INCLUDES) -MM *.c > .dependencies

documentation:: *.c *.h toke.doxygen
	@doxygen toke.doxygen

.PHONY: all clean

-include .dependencies

.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@ 


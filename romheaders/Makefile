#
#                     OpenBIOS - free your system! 
#                             ( Utilities )
#                          
#  This program is part of a free implementation of the IEEE 1275-1994 
#  Standard for Boot (Initialization Configuration) Firmware.
#
#  Copyright (C) 2002-2010 Stefan Reinauer <stepan@openbios.org>
#  Copyright (C) 2009 coresystems GmbH <info@coresystems.de>
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

CC	 ?= gcc
STRIP    = strip
CFLAGS   = -O2 -Wall -Wextra
INCLUDES = -I../shared

SOURCES = romheaders.c ../shared/classcodes.c

.SUFFIXES: .c

all: romheaders

romheaders: $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCES) -o $@
	$(STRIP) romheaders

clean:
	rm -f *~
	rm -f romheaders

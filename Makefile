#
#                     OpenBIOS - free your system!
#                             ( Utilities )
#
#  This program is part of a free implementation of the IEEE 1275-1994
#  Standard for Boot (Initialization Configuration) Firmware.
#
#  Copyright (C) 2006 coresystems GmbH <info@coresystems.de>
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

all:
	make -C toke
	make -C detok
	make -C romheaders

install:
	make -C toke install
	make -C detok install
	make -C romheaders install

clean:	
	make -C toke clean
	make -C detok clean
	make -C romheaders clean
	make -C testsuite clean

distclean: clean
	make -C toke distclean
	make -C detok distclean
	make -C romheaders distclean
	make -C testsuite distclean
	find . -name "*.gcda" -exec rm -f \{\} \;
	find . -name "*.gcno" -exec rm -f \{\} \;

tests: all
	cp toke/toke testsuite
	cp detok/detok testsuite
	cp romheaders/romheaders testsuite
	make -C testsuite all CygTestLogs=`pwd`/testlogs


.PHONY: all clean distclean toke detok romheaders tests


#
#                     OpenBIOS - free your system!
#                             ( Utilities )
#
#  This program is part of a free implementation of the IEEE 1275-1994
#  Standard for Boot (Initialization Configuration) Firmware.
#
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

VERSION:=$(shell grep "^\#.*TOKE_VERSION" < toke/toke.c |cut -f2 -d\" )

all:
	$(MAKE) -C toke
	$(MAKE) -C detok
	$(MAKE) -C romheaders

install:
	$(MAKE) -C toke install
	$(MAKE) -C detok install
	$(MAKE) -C romheaders install

clean:	
	$(MAKE) -C toke clean
	$(MAKE) -C detok clean
	$(MAKE) -C romheaders clean
	$(MAKE) -C testsuite clean

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

.PHONY: all clean distclean toke detok romheaders tests


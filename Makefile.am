# ptyopen - controls command in a pseudo-terminal
# Makefile.am
# Copyright (C) 2000, 2001 Philippe Troin <phil@fifi.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

bin_PROGRAMS         = ptyopen
ptyopen_SOURCES      = ptyopen.c
man_MANS             = ptyopen.1

EXTRA_DIST           = $(man_MANS) autoboot

CONFIG_FILES         = $(srcdir)/install-sh \
		       $(srcdir)/missing \
		       $(srcdir)/mkinstalldirs

CLEANFILES           = *~ $(srcdir)/*~ *.da *.bb *.bbg *.gcov

MAINTAINERCLEANFILES = $(CONFIG_FILES) \
		       $(srcdir)/Makefile.in \
		       $(srcdir)/configure \
		       $(srcdir)/aclocal.m4 \
                       $(srcdir)/stamp.h \
		       $(srcdir)/stamp-h.in

INSTALL_PROGRAM = $(INSTALL) @exeinstflags@ ${AM_INSTALL_PROGRAM_FLAGS}

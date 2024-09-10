# ptyopen - controls command in a pseudo-terminal
# configure.in
# Copyright (C) 2000-2001, 2024 Philippe Troin <phil+github-commits@fifi.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
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

AC_INIT([ptyopen],[0.94])
AC_CONFIG_AUX_DIR([config.aux])
FI_PROJECT()

# Get started
AC_CONFIG_SRCDIR([ptyopen.c])
AC_LANG([C])

### 1) Checking for system

### 2) Checking for programs
AC_PROG_CC
FI_COMPILER_WARNINGS
AC_PROG_CPP
FI_FORMAT_MAN([ptyopen.1], [html pdf])

FI_GIT_REVISION

### 3) Checking for libraries
### 4) Checking for header files
AC_CHECK_HEADERS([stropts.h])

### 5) Checking for typedefs
### 6) Checking for structures
### 7) Checking for compiler characteristics
### 8) Checking for library functions
AC_CHECK_FUNCS([strsignal getpt grantpt unlockpt isastream])
if test "$ac_cv_func_grantpt" = "yes"
then
    exeinstflags="-m 755"
else
    exeinstflags="-m 4755 -o root -g root"
fi
AC_SUBST(exeinstflags)

### 9) Check for "system services"

### END

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

# Local variables:
# mode: sh
# end:

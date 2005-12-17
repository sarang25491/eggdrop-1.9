dnl config.m4
dnl
dnl Copyright (C) 2004 Eggheads Development Team
dnl
dnl This program is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU General Public License
dnl as published by the Free Software Foundation; either version 2
dnl of the License, or (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
dnl
dnl $Id: config.m4,v 1.2 2005/12/17 01:25:06 sven Exp $
dnl

EGG_MODULE_START(pythonscript, [python scripting support], "yes")

AC_ARG_WITH(python, AC_HELP_STRING([--with-python=PATH], [path to the python interpreter]), [pythoncmd="$withval"])

if test x"$pythoncmd" = x; then
	AC_PATH_PROG(pythoncmd, python)
fi

PYTHON_PREFIX=`echo 'import sys; print sys.prefix' | $pythoncmd`
PYTHON_MAJOR_VERSION=`echo 'import sys; print sys.version_info.__getitem__(0)' | $pythoncmd`
PYTHON_MINOR_VERSION=`echo 'import sys; print sys.version_info.__getitem__(1)' | $pythoncmd`
PYTHON_VER="$PYTHON_MAJOR_VERSION$PYTHON_MINOR_VERSION"
PYTHON_VERSION="$PYTHON_MAJOR_VERSION.$PYTHON_MINOR_VERSION"

if test "$PYTHON_VER" -ge 22 ; then
	PYTHON_INCLUDE=${PYTHON_PREFIX}/include/python${PYTHON_VERSION}
	PYTHON_LIB_DIR=${PYTHON_PREFIX}/lib/python${PYTHON_VERSION}

	if test -d $PYTHON_INCLUDE && test -e $PYTHON_LIB_DIR/config/Makefile ; then
		py_libs=`grep '^LIBS=' $PYTHON_LIB_DIR/config/Makefile | sed -e 's/^.*=\t*//'`
		py_libc=`grep '^LIBC=' $PYTHON_LIB_DIR/config/Makefile | sed -e 's/^.*=\t*//'`
		py_libm=`grep '^LIBM=' $PYTHON_LIB_DIR/config/Makefile | sed -e 's/^.*=\t*//'`
		py_liblocalmod=`grep '^LOCALMODLIBS=' $PYTHON_LIB_DIR/config/Makefile | sed -e 's/^.*=\t*//'`
		py_libbasemod=`grep '^BASEMODLIBS=' $PYTHON_LIB_DIR/config/Makefile | sed -e 's/^.*=\t*//'`
		py_linkerflags=`grep '^LINKFORSHARED=' $PYTHON_LIB_DIR/config/Makefile | sed -e 's/^.*=\t*//'`

		PYTHON_LDFLAGS="-L$PYTHON_LIB_DIR/config $py_libs $py_libc $py_libm -lpython$PYTHON_VERSION $py_liblocalmod $py_libbasemod $py_linkerflags"

		AC_SUBST(PYTHON_INCLUDE)
  	AC_SUBST(PYTHON_LDFLAGS)
		EGG_MOD_ENABLED="yes"
	else
		EGG_MOD_ENABLED="no"

		AC_MSG_WARN([

  Unable to locate python include or lib files.
  pythonscript module will therefore be disabled.

  Make sure you have the python development packages instaled.

		])
	fi
else
	EGG_MOD_ENABLED="no"

	AC_MSG_WARN([

  Your system does not seem to provide Python 2.2 or later. The
  pythonscript module will therefore be disabled.

  To manually specify python's location, re-run configure using
    --with-python=/path/to/python
  where the path is the directory containing the python interpreter.

	])
fi

EGG_MODULE_END()

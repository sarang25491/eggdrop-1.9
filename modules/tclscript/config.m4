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
dnl $Id: config.m4,v 1.3 2004/06/16 06:33:45 wcc Exp $
dnl

EGG_MODULE_START(tclscript, [tcl script support], "yes")

# XXX: maybe someone find a way how to remove the modules/<mod>/
# XXX: prefix. According to m4 man page there's an environment
# XXX: variable named M4PATH but this doesn't seem to work.
sinclude(modules/tclscript/tcl.m4)

SC_PATH_TCLCONFIG
SC_LOAD_TCLCONFIG

# We only support version 8
if test "$TCL_MAJOR_VERSION" = 8; then
  if test x"${TCL_INCLUDE_SPEC}" = "x" ; then
    if test -d ${TCL_PREFIX}/include/tcl${TCL_VERSION} ; then
      TCL_INCLUDE_SPEC=-I${TCL_PREFIX}/include/tcl${TCL_VERSION}
    else
      TCL_INCLUDE_SPEC=-I${TCL_PREFIX}/include
    fi
  fi

  AC_SUBST(TCL_INCLUDE_SPEC)
  AC_SUBST(TCL_LIBS)
  EGG_MOD_ENABLED="yes"
else
  EGG_MOD_ENABLED="no"

  AC_MSG_WARN([

  Your system does not seem to provide Tcl 8.0 or later. The
  tclscript module will therefore be disabled.

  To manually specify tcl's location, re-run configure using
    --with-tcl=/path/to/tcl
  where the path is the directory containing tclConfig.sh.

  ])
fi

EGG_MODULE_END()

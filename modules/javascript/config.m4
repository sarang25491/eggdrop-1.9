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
dnl $Id: config.m4,v 1.2 2004/06/16 06:33:44 wcc Exp $
dnl

EGG_MODULE_START(javascript, [javascript support], "yes")


AC_ARG_WITH(jslib, AC_HELP_STRING([--with-jslib=PATH], [path to javascript library directory]), [jslibname="$withval"])
AC_ARG_WITH(jsinc, AC_HELP_STRING([--with-jsinc=PATH], [path to javascript headers directory]), [jsincname="$withval"])


CPPFLAGS_save="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -DXP_UNIX"
if test "${jsincname+set}" = "set"; then
  CPPFLAGS="$CPPFLAGS -I$jsincname"
fi


LDFLAGS_save="$LDFLAGS"
if test "${jslibname+set}" = "set"; then
  LDFLAGS="$LDFLAGS -L$jslibname"
fi


AC_CHECK_LIB(js, JS_NewObject, [JSLIBS="-ljs"])
AC_CHECK_HEADER(jsapi.h)


# Disable the module if either the header file or the library
# are missing.
if test "${JSLIBS+set}" != "set"; then
  AC_MSG_WARN([

  Your system does not provide a working javascript library. The
  javascript module will therefore be disabled.

  ])
  EGG_MOD_ENABLED="no"
else
  if test "$ac_cv_header_jsapi_h" != "yes"; then
    AC_MSG_WARN([

  Your system does not provide the necessary javascript header files. The
  javascript module will therefore be disabled.

    ])
    EGG_MOD_ENABLED="no"
  else
    AC_SUBST(JSLIBS)
    if test "${jslibname+set}" = "set"; then
      JSLDFLAGS="-L$jslibname"
    fi
    AC_SUBST(JSLDFLAGS)

    if test "${jsincname+set}" = "set"; then
      JSINCL="-I$jsincname"
    fi
    AC_SUBST(JSINCL)
  fi
fi


CPPFLAGS="${CPPFLAGS_save}"
LDFLAGS="${LDFLAGS_save}"


EGG_MODULE_END()

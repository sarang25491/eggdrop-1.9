EGG_MODULE_START(javascript, [javascript support], "yes")

AC_ARG_WITH(jslib, 
            AC_HELP_STRING([--with-jslib=PATH],
                           [path to javascript library directory]),
            jslibname="$withval")
AC_ARG_WITH(jsinc,
            AC_HELP_STRING([--with-jsinc=PATH],
                           [path to javascript headers directory]),
            jsincname="$withval")

CPPFLAGS_save="$CPPFLAGS"
LDFLAGS_save="$LDFLAGS"

CPPFLAGS="$CPPFLAGS -DXP_UNIX"
if test "${jsincname+set}" = set
then
  CPPFLAGS="$CPPFLAGS -I$jsincname"
fi
if test "${jslibname+set}" = set
then
  LDFLAGS="$LDFLAGS -L$jslibname"
fi

AC_CHECK_LIB(js, JS_NewObject, JSLIBS="-ljs")
AC_CHECK_HEADER(jsapi.h)

# Disable the module if either the header file or the library
# are missing.
if test "${JSLIBS+set}" != set
then
  AC_MSG_WARN([

  Your system does not provide a working javascript library. The
  javascript module will therefore be disabled.

])
  EGG_MOD_ENABLED=no
else
  if test "$ac_cv_header_jsapi_h" != yes
  then
    AC_MSG_WARN([

  Your system does not provide the necessary javascript header files. The
  javascript module will therefore be disabled.

])
    EGG_MOD_ENABLED=no
  else
    AC_SUBST(JSLIBS)
    if test "${jslibname+set}" = set
    then
      JSLDFLAGS="-L$jslibname"
    fi
    AC_SUBST(JSLDFLAGS)
    if test "${jsincname+set}" = set
    then
      JSINCL="-I$jsincname"
    fi   
    AC_SUBST(JSINCL)
  fi
fi

CPPFLAGS="${CPPFLAGS_save}" 
LDFLAGS="${LDFLAGS_save}"

EGG_MODULE_END()

dnl acinclude.m4
dnl   macros autoconf uses when building configure from configure.in
dnl
dnl $Id: acinclude.m4,v 1.37 2004/06/14 14:45:53 wingman Exp $
dnl


dnl  EGG_MSG_CONFIGURE_START()
dnl
AC_DEFUN([EGG_MSG_CONFIGURE_START], [dnl
AC_MSG_RESULT([
This is Eggdrop's GNU configure script.
It's going to run a bunch of strange tests to hopefully
make your compile work without much twiddling.
])
])


dnl  EGG_MSG_CONFIGURE_END()
dnl
AC_DEFUN([EGG_MSG_CONFIGURE_END], [dnl

AC_MSG_RESULT([
------------------------------------------------------------------------
Configuration:

  Source code location : $srcdir
  Compiler             : $CC
  Compiler flags       : $CFLAGS
  Host System Type     : $host
  Install path         : $prefix
  Enabled Modules      : $EGG_MODULES_ENABLED
  Disabled Modules     : $EGG_MODULES_DISABLED

See config.h for further configuration information.
------------------------------------------------------------------------
])
AC_MSG_RESULT([
Configure is done.

Type 'make' to create the bot.
])

])


dnl  EGG_CHECK_CC()
dnl
dnl  FIXME: make a better test
dnl
AC_DEFUN([EGG_CHECK_CC], [dnl
if test "${cross_compiling+set}" != set
then
  AC_MSG_ERROR([

  This system does not appear to have a working C compiler.
  A working C compiler is required to compile Eggdrop.

])
fi
])


dnl  EGG_CHECK_CCPIPE()
dnl
dnl  Checks whether the compiler supports the `-pipe' flag, which
dnl  speeds up the compilation.
AC_DEFUN([EGG_CHECK_CCPIPE], [dnl
if test -z "$no_pipe"
then
  if test -n "$GCC"
  then
    AC_CACHE_CHECK(whether the compiler understands -pipe, egg_cv_var_ccpipe, [dnl
      ac_old_CC="$CC"
      CC="$CC -pipe"
      AC_TRY_COMPILE(,, egg_cv_var_ccpipe=yes, egg_cv_var_ccpipe=no)
      CC="$ac_old_CC"
    ])
    if test "$egg_cv_var_ccpipe" = yes
    then
      CC="$CC -pipe"
    fi
  fi
fi
])


dnl  EGG_CHECK_CFLAGS_WALL()
dnl
dnl  Checks whether the compiler supports the `-WAll' flag.
AC_DEFUN([EGG_CHECK_CFLAGS_WALL], [dnl
if test -z "$no_wall"
then
  if test -n "$GCC"
  then
    AC_CACHE_CHECK(whether the compiler understands -Wall, egg_cv_var_ccwall, [dnl
      ac_old_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS -Wall"
      AC_TRY_COMPILE(,, egg_cv_var_ccwall=yes, egg_cv_var_ccwall=no)
      CFLAGS="$ac_old_CFLAGS"
    ])
    if test "$egg_cv_var_ccwall" = yes
    then
      CFLAGS="$CFLAGS -Wall"
    fi
  fi
fi
])
							

dnl  EGG_PROG_AWK()
dnl
AC_DEFUN([EGG_PROG_AWK], [dnl
# awk is needed for Tcl library and header checks, and eggdrop version subst
AC_PROG_AWK
if test "${AWK+set}" != set
then
  AC_MSG_ERROR([

  This system seems to lack a working 'awk' command.
  A working 'awk' command is required to compile Eggdrop.

])
fi
])


dnl  EGG_PROG_BASENAME()
dnl
AC_DEFUN([EGG_PROG_BASENAME], [dnl
# basename is needed for Tcl library and header checks
AC_CHECK_PROG(BASENAME, basename, basename)
if test "${BASENAME+set}" != set
then
  cat << 'EOF' >&2
configure: error:

  This system seems to lack a working 'basename' command.
  A working 'basename' command is required to compile Eggdrop.

EOF
  exit 1
fi
])


dnl  EGG_DISABLE_CC_OPTIMIZATION()
dnl
dnl check if user requested to remove -O2 cflag 
dnl would be usefull on some weird *nix
AC_DEFUN([EGG_DISABLE_CC_OPTIMIZATION], [dnl
  AC_ARG_ENABLE(cc-optimization,
                AC_HELP_STRING([--disable-cc-optimization], [disable -O2 cflag]),  
    CFLAGS=`echo $CFLAGS | sed 's/\-O2//'`)
])dnl


dnl  EGG_CHECK_OS()
dnl
dnl  FIXME/NOTICE:
dnl    This function is obsolete. Any NEW code/checks should be written
dnl    as individual tests that will be checked on ALL operating systems.
dnl
AC_DEFUN([EGG_CHECK_OS], [dnl
AC_REQUIRE([AC_CANONICAL_HOST])

IRIX=no
EGG_CYGWIN=no

AC_CACHE_CHECK(system type, egg_cv_var_system_type, egg_cv_var_system_type=`$EGG_UNAME -s`)
AC_CACHE_CHECK(system release, egg_cv_var_system_release, egg_cv_var_system_release=`$EGG_UNAME -r`)

case $host_os in
  cygwin)
    EGG_CYGWIN=yes
    AC_DEFINE(CYGWIN_HACKS, 1, [Define if running under Cygwin.])
    case "`echo $egg_cv_var_system_release | cut -c 1-3`" in
      1.*)
	AC_PROG_CC_WIN32
	CC="$CC $WIN32FLAGS"
	AC_MSG_CHECKING(for /usr/lib/libbinmode.a)
	if test -r /usr/lib/libbinmode.a
	then
          AC_MSG_RESULT(yes)
          LIBS="$LIBS /usr/lib/libbinmode.a"
        else
          AC_MSG_RESULT(no)
          AC_MSG_WARN(Make sure the directory Eggdrop is installed into is mounted in binary mode.)
        fi
      ;;
      *)
        AC_MSG_WARN(Make sure the directory Eggdrop is installed into is mounted in binary mode.)
      ;;
    esac
  ;;
  hpux*)
    AC_DEFINE(HPUX_HACKS, 1,
              [Define if running on HPUX that supports dynamic linking.])
    case $host_os in
    hpux10)
      AC_DEFINE(HPUX10_HACKS, 1,
                [Define if running on HPUX 10.x.])
    ;;
    esac
  ;;
  irix*)
    IRIX=yes
  ;;
  osf*)
    AC_DEFINE(STOP_UAC, 1,
              [Define if running on OSF/1 platform.])
  ;;
  next*)
    AC_DEFINE(BORGCUBES, 1,
              [Define if running on NeXT Step.])
  ;;
esac

])


dnl  EGG_CHECK_LIBS()
dnl
AC_DEFUN([EGG_CHECK_LIBS], [dnl
# FIXME: this needs to be fixed so that it works on IRIX
if test "$IRIX" = yes
then
  AC_MSG_WARN(Skipping library tests because they CONFUSE Irix.)
else
  AC_CHECK_LIB(socket, socket)
  AC_CHECK_LIB(nsl, connect)
  AC_CHECK_LIB(dns, gethostbyname)
  AC_CHECK_LIB(m, tan, EGG_MATH_LIB="-lm")
  # This is needed for Tcl libraries compiled with thread support
  AC_CHECK_LIB(pthread, pthread_mutex_init, [dnl
  ac_cv_lib_pthread_pthread_mutex_init=yes
  ac_cv_lib_pthread="-lpthread"], [dnl
    AC_CHECK_LIB(pthread, __pthread_mutex_init, [dnl
    ac_cv_lib_pthread_pthread_mutex_init=yes
    ac_cv_lib_pthread="-lpthread"], [dnl
      AC_CHECK_LIB(pthreads, pthread_mutex_init, [dnl
      ac_cv_lib_pthread_pthread_mutex_init=yes
      ac_cv_lib_pthread="-lpthreads"], [dnl
        AC_CHECK_FUNC(pthread_mutex_init, [dnl
        ac_cv_lib_pthread_pthread_mutex_init=yes
        ac_cv_lib_pthread=""],
        ac_cv_lib_pthread_pthread_mutex_init=no)])])])
fi
])


dnl  EGG_C_LONG_LONG
dnl
AC_DEFUN([EGG_C_LONG_LONG], [dnl
# Code borrowed from Samba
AC_CACHE_CHECK([for long long], egg_cv_have_long_long, [
AC_TRY_RUN([
#include <stdio.h>
int main() {
	long long x = 1000000;
	x *= x;
	exit(((x / 1000000) == 1000000) ? 0: 1);
}
],
egg_cv_have_long_long=yes,
egg_cv_have_long_long=no,
egg_cv_have_long_long=cross)])
if test "$egg_cv_have_long_long" = yes
then
  AC_DEFINE(HAVE_LONG_LONG, 1,
	    [Define if system supports long long.])
fi
])


dnl  EGG_FUNC_C99_VSNPRINTF
dnl
AC_DEFUN([EGG_FUNC_C99_VSNPRINTF], [dnl
# Code borrowed from Samba
AC_CACHE_CHECK([for C99 vsnprintf], egg_cv_have_c99_vsnprintf, [
AC_TRY_RUN([
#include <sys/types.h>
#include <stdarg.h>
void foo(const char *format, ...) { 
	va_list ap;
	int len;
	char buf[5];

	va_start(ap, format);
	len = vsnprintf(0, 0, format, ap);
	va_end(ap);
	if (len != 5) exit(1);

	if (snprintf(buf, 3, "hello") != 5 || strcmp(buf, "he") != 0) exit(1);

	exit(0);
}
int main(){
	foo("hello");
}
],
egg_cv_have_c99_vsnprintf=yes,
egg_cv_have_c99_vsnprintf=no,
egg_cv_have_c99_vsnprintf=cross)])
if test "$egg_cv_have_c99_vsnprintf" = yes
then
  AC_DEFINE(HAVE_C99_VSNPRINTF, 1,
            [Define if vsnprintf is C99 compliant.])
else
  egg_replace_snprintf="yes"
fi
])


dnl  EGG_FUNC_GETOPT_LONG
dnl
AC_DEFUN([EGG_FUNC_GETOPT_LONG], [dnl
AC_CHECK_FUNCS(getopt_long , ,
             [AC_LIBOBJ(getopt)
              AC_LIBOBJ(getopt1)])
])


dnl  EGG_REPLACE_SNPRINTF
dnl
AC_DEFUN([EGG_REPLACE_SNPRINTF], [dnl
if test "$egg_replace_snprintf" = yes
then
  AC_LIBOBJ(snprintf)
  SNPRINTF_LIBS="-lm"
fi
AC_SUBST(SNPRINTF_LIBS)
])


dnl  EGG_TYPE_32BIT()
dnl
AC_DEFUN([EGG_TYPE_32BIT], [dnl
AC_CHECK_SIZEOF(unsigned int, 4)
if test "$ac_cv_sizeof_unsigned_int" = 4
then
  AC_DEFINE(UNSIGNED_INT32, 1,
            [Define this if an unsigned int is 32 bits.])
else
  AC_CHECK_SIZEOF(unsigned long, 4)
  if test "$ac_cv_sizeof_unsigned_long" = 4
  then
    AC_DEFINE(UNSIGNED_LONG32, 1,
              [Define this if an unsigned long is 32 bits.])
  fi
fi
])


dnl  EGG_VAR_SYS_ERRLIST()
dnl
AC_DEFUN([EGG_VAR_SYS_ERRLIST], [dnl
AC_CACHE_CHECK([for sys_errlist],
egg_cv_var_sys_errlist,
[AC_TRY_LINK([int *p;], [extern int sys_errlist; p = &sys_errlist;],
	    egg_cv_var_sys_errlist=yes, egg_cv_var_sys_errlist=no)
])
if test "$egg_cv_var_sys_errlist" = yes; then
  AC_DEFINE([HAVE_SYS_ERRLIST], 1,
           [Define if your system libraries have a sys_errlist variable.])
fi
])


dnl  EGG_CACHE_UNSET(CACHE-ID)
dnl
dnl  Unsets a certain cache item. Typically called before using
dnl  the AC_CACHE_*() macros.
AC_DEFUN([EGG_CACHE_UNSET], [dnl
  unset $1
])


dnl  EGG_REPLACE_IF_CHANGED(FILE-NAME, CONTENTS-CMDS, INIT-CMDS)
dnl
dnl  Replace FILE-NAME if the newly created contents differs from the existing
dnl  file contents.  Otherwise, leave the file allone.  This avoids needless
dnl  recompiles.
dnl
define(EGG_REPLACE_IF_CHANGED, [dnl
  AC_CONFIG_COMMANDS([$1], [
egg_replace_file="$1"
echo "creating $1"
$2
if test -f "$egg_replace_file" && cmp -s conftest.out $egg_replace_file
then
  echo "$1 is unchanged"
else
  mv conftest.out $egg_replace_file
fi
rm -f conftest.out], [$3])
])


dnl  AC_PROG_CC_WIN32()
dnl
dnl  FIXME: is this supposed to be checking for "windows.h"?
dnl
AC_DEFUN([AC_PROG_CC_WIN32], [
AC_MSG_CHECKING([how to access the Win32 API])
WIN32FLAGS=""
AC_TRY_COMPILE(,[
#ifndef WIN32
# ifndef _WIN32
#  error WIN32 or _WIN32 not defined
# endif
#endif], [
dnl found windows.h with the current config.
AC_MSG_RESULT([present by default])
], [
dnl try -mwin32
ac_compile_save="$ac_compile"
dnl we change CC so config.log looks correct
save_CC="$CC"
ac_compile="$ac_compile -mwin32"
CC="$CC -mwin32"
AC_TRY_COMPILE(,[
#ifndef WIN32
# ifndef _WIN32
#  error WIN32 or _WIN32 not defined
# endif
#endif], [
dnl found windows.h using -mwin32
AC_MSG_RESULT([found via -mwin32])
ac_compile="$ac_compile_save"
CC="$save_CC"
WIN32FLAGS="-mwin32"
], [
ac_compile="$ac_compile_save"
CC="$save_CC"
AC_MSG_RESULT([not found])
])
])
])


dnl  EGG_IPV6_OPTIONS()
dnl
AC_DEFUN([EGG_IPV6_OPTIONS], [dnl
AC_MSG_CHECKING(whether you enabled IPv6 support)
dnl dummy option for help string, next option is the real one
AC_ARG_ENABLE(ipv6,
              AC_HELP_STRING([--enable-ipv6],
                             [enable IPV6 support]),,)
AC_ARG_ENABLE(ipv6,
              AC_HELP_STRING([--disable-ipv6],
                             [disable IPV6 support]),
[ ac_cv_ipv6="$enableval"
  AC_MSG_RESULT($ac_cv_ipv6)
], [
  if test "$egg_cv_ipv6_supported" = yes
  then
    ac_cv_ipv6=yes
  else
    ac_cv_ipv6=no
  fi
  AC_MSG_RESULT((default) $ac_cv_ipv6)
])
if test "$ac_cv_ipv6" = yes
then
  AC_DEFINE(IPV6, 1,
            [Define if you want to enable IPv6 support.])
fi
])


dnl  EGG_TYPE_SOCKLEN_T
dnl
AC_DEFUN([EGG_TYPE_SOCKLEN_T], [dnl
AC_CACHE_CHECK(for socklen_t, egg_cv_var_socklen_t, [
AC_TRY_COMPILE([#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>
], [
socklen_t x;
x = 0;
], egg_cv_var_socklen_t=yes, egg_cv_var_socklen_t=no)
])
if test "$egg_cv_var_socklen_t" = no
then
  AC_DEFINE(socklen_t, unsigned,
            [Define to `unsigned' if something else doesn't define.])
fi
])


dnl  EGG_INADDR_LOOPBACK
dnl
AC_DEFUN([EGG_INADDR_LOOPBACK], [dnl
AC_MSG_CHECKING(for INADDR_LOOPBACK)
AC_CACHE_VAL(adns_cv_decl_inaddrloopback,[
 AC_TRY_COMPILE([
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 ],[
  INADDR_LOOPBACK;
 ],
 adns_cv_decl_inaddrloopback=yes,
 adns_cv_decl_inaddrloopback=no)])
if test "$adns_cv_decl_inaddrloopback" = yes
then
 AC_MSG_RESULT(found)
else
 AC_MSG_RESULT([not in standard headers, urgh...])
 AC_CHECK_HEADER(rpc/types.h,[
  AC_DEFINE(HAVEUSE_RPCTYPES_H, 1,
            [Define if we want to include rpc/types.h. Crap BSDs put INADDR_LOOPBACK there.])
 ],[
  AC_MSG_ERROR([cannot find INADDR_LOOPBACK or rpc/types.h])
 ])
fi
])


dnl  EGG_IPV6_SUPPORTED
dnl
AC_DEFUN([EGG_IPV6_SUPPORTED], [dnl
AC_MSG_CHECKING(for kernel IPv6 support)
AC_CACHE_VAL(egg_cv_ipv6_supported,[
 AC_TRY_RUN([
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

int main()
{
    struct sockaddr_in6 sin6;
    int s = socket(AF_INET6, SOCK_STREAM, 0);
    if (s != -1)
	close(s);
    return s == -1;
}
], egg_cv_ipv6_supported=yes, egg_cv_ipv6_supported=no,
egg_cv_ipv6_supported=no)])
if test "$egg_cv_ipv6_supported" = yes
then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi
])


dnl  EGG_DEFINE_VERSION_NUM
dnl
AC_DEFUN([EGG_DEFINE_VERSION_NUM], [dnl
egg_version_num=`echo $VERSION | $AWK 'BEGIN {FS = "."} {printf("%d%02d%02d00", [$]1, [$]2, [$]3)}'`
AC_DEFINE_UNQUOTED(VERSION_NUM, $egg_version_num,
                   [Define version number.])
])


dnl  EGG_GNU_GETTEXT
dnl
AC_DEFUN([EGG_GNU_GETTEXT], [dnl
AC_MSG_CHECKING(for avaible translations)
ALL_LINGUAS=""
cd po   
for LOC in `ls *.po 2> /dev/null`
do
  ALL_LINGUAS="$ALL_LINGUAS `echo $LOC | $AWK 'BEGIN {FS = "."} {printf("%s", [$]1)}'`"
done
cd ..
AC_MSG_RESULT($ALL_LINGUAS)

AM_GNU_GETTEXT(, need-ngettext)
])


dnl  EGG_LIBTOOL
dnl
AC_DEFUN([EGG_LIBTOOL], [dnl

AC_LIBTOOL_WIN32_DLL
AC_LIBLTDL_CONVENIENCE(libltdl)
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_SUBST(INCLTDL)
AC_SUBST(LIBLTDL)
AC_SUBST(LIBTOOL_DEPS)

if test "$enable_shared" = no
then
  egg_static_build=yes
else
  egg_static_build=no
fi

# HACK: This is needed for libltdl's configure script
egg_aux_dir=`CDPATH=:; cd $ac_aux_dir && pwd`
ac_configure_args="$ac_configure_args --with-auxdir=$egg_aux_dir"
])


dnl EGG_WITH_EFENCE
dnl
AC_DEFUN([EGG_WITH_EFENCE], [dnl
AC_REQUIRE([AM_WITH_MPATROL])

AC_ARG_WITH(efence,
            AC_HELP_STRING([--with-efence],
                           [build with the efence library]),
                           egg_with_efence="$withval", egg_with_efence=no)
if test "$egg_with_efence" = yes
then
  if test "$am_with_mpatrol" = 0
  then
    AC_CHECK_LIB(efence, malloc, egg_efence_libs="-lefence",
                 AC_MSG_ERROR(efence library not installed correctly))
    LIBS="$egg_efence_libs $LIBS"
  else
    AC_MSG_ERROR(enabling efence and mpatrol at the same time is evil!)
  fi
fi

])


dnl  EGG_DEBUG_OPTIONS
dnl
AC_DEFUN([EGG_DEBUG_OPTIONS], [dnl
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--disable-debug],
                             [disable debug support]),
                             debug="$enableval", debug=yes)
if test "$debug" = yes
then
  EGG_DEBUG="-DDEBUG"
  CFLAGS="$CFLAGS -g3"
fi
AC_SUBST(EGG_DEBUG)

AM_WITH_MPATROL(no)
EGG_WITH_EFENCE

])

dnl
dnl EGG_CHECK_MODULES()
dnl
AC_DEFUN([EGG_CHECK_MODULES],[
  esyscmd(./acconfig/config.stubs ./modules/)
])

dnl
dnl EGG_MODULE_START(name, help, enabled)
dnl
AC_DEFUN([EGG_MODULE_START],[
  EGG_MOD_NAME=$1
  EGG_MOD_DIR=modules/$1/

  AC_ARG_ENABLE([$1], AC_HELP_STRING(
    [--disable-$1], [disable $2]),
    [
      EGG_MOD_ENABLED="$enableval"
    ], [
      EGG_MOD_ENABLED=$3
  ])

  AC_CONFIG_FILES(modules/$1/Makefile)

  if test "$EGG_MOD_ENABLED" = "yes"; then
    AC_MSG_NOTICE([Configuring module $EGG_MOD_NAME...])
])

dnl
dnl EGG_MODULE_END()
dnl
AC_DEFUN([EGG_MODULE_END],[
  fi

  if test "$EGG_MOD_ENABLED" = "yes"; then
    EGG_MODULES_ENABLED="$EGG_MOD_NAME $EGG_MODULES_ENABLED"
  else
    EGG_MODULES_DISABLED="$EGG_MOD_NAME $EGG_MODULES_DISABLED"
  fi
])

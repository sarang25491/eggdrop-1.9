dnl acinclude.m4
dnl   macros autoconf uses when building configure from configure.in
dnl
dnl $Id: acinclude.m4,v 1.29 2003/03/08 07:32:49 tothwolf Exp $
dnl


dnl  EGG_MSG_CONFIGURE_START()
dnl
AC_DEFUN(EGG_MSG_CONFIGURE_START, [dnl
AC_MSG_RESULT([
This is Eggdrop's GNU configure script.
It's going to run a bunch of strange tests to hopefully
make your compile work without much twiddling.
])
])


dnl  EGG_MSG_CONFIGURE_END()
dnl
AC_DEFUN(EGG_MSG_CONFIGURE_END, [dnl
AC_MSG_RESULT([
------------------------------------------------------------------------
Configuration:

  Source code location:       $srcdir
  Compiler:                   $CC
  Compiler flags:             $CFLAGS
  Host System Type:           $host
  Install path:               $prefix
  Compress module:            $egg_compress
  Tcl module:                 $egg_tclscript
  Perl module:                $egg_perlscript
  Javascript module:          $egg_javascript

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
AC_DEFUN(EGG_CHECK_CC, [dnl
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
AC_DEFUN(EGG_CHECK_CCPIPE, [dnl
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
AC_DEFUN(EGG_CHECK_CFLAGS_WALL, [dnl
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
AC_DEFUN(EGG_PROG_AWK, [dnl
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
AC_DEFUN(EGG_PROG_BASENAME, [dnl
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
AC_DEFUN(EGG_DISABLE_CC_OPTIMIZATION, [dnl
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
AC_DEFUN(EGG_CHECK_OS, [dnl
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
AC_DEFUN(EGG_CHECK_LIBS, [dnl
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
AC_DEFUN(EGG_C_LONG_LONG, [dnl
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
AC_DEFUN(EGG_FUNC_C99_VSNPRINTF, [dnl
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
AC_DEFUN(EGG_FUNC_GETOPT_LONG, [dnl
AC_CHECK_FUNCS(getopt_long , ,
             [AC_LIBOBJ(getopt)
              AC_LIBOBJ(getopt1)])
])


dnl  EGG_REPLACE_SNPRINTF
dnl
AC_DEFUN(EGG_REPLACE_SNPRINTF, [dnl
if test "$egg_replace_snprintf" = yes
then
  AC_LIBOBJ(snprintf)
  SNPRINTF_LIBS="-lm"
fi
AC_SUBST(SNPRINTF_LIBS)
])


dnl  EGG_TYPE_32BIT()
dnl
AC_DEFUN(EGG_TYPE_32BIT, [dnl
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


dnl  EGG_TCL_WITH_OPTIONS()
dnl
AC_DEFUN(EGG_TCL_WITH_OPTIONS, [dnl
# oohh new configure --variables for those with multiple tcl libs
AC_ARG_WITH(tcllib, 
            AC_HELP_STRING([--with-tcllib=PATH],
                           [full path to tcl library]),
            tcllibname="$withval")
AC_ARG_WITH(tclinc,
            AC_HELP_STRING([--with-tclinc=PATH],
                           [full path to tcl header]),
            tclincname="$withval")

WARN=0
# Make sure either both or neither $tcllibname and $tclincname are set
if test "${tcllibname+set}" = set
then
  if test "${tclincname+set}" != set
  then
    WARN=1
    tcllibname=""
    TCLLIB=""
    TCLINC=""
  fi
else
  if test "${tclincname+set}" = set
  then
    WARN=1
    tclincname=""
    TCLLIB=""
    TCLINC=""
  fi
fi
if test "$WARN" = 1
then
  AC_MSG_WARN([

  You must specify both --with-tcllib and --with-tclinc for them to work.
  configure will now attempt to autodetect both the Tcl library and header...

])
fi
])


dnl  EGG_TCL_ENV()
dnl
AC_DEFUN(EGG_TCL_ENV, [dnl
WARN=0
# Make sure either both or neither $TCLLIB and $TCLINC are set
if test "${TCLLIB+set}" = set
then
  if test "${TCLINC+set}" != set
  then
    WARN=1
    WVAR1=TCLLIB
    WVAR2=TCLINC
    TCLLIB=""
  fi
else
  if test "${TCLINC+set}" = set
  then
    WARN=1
    WVAR1=TCLINC
    WVAR2=TCLLIB
    TCLINC=""
  fi
fi
if test "$WARN" = 1
then
  AC_MSG_WARN([

  Environment variable $WVAR1 was set, but I did not detect ${WVAR2}.
  Please set both TCLLIB and TCLINC correctly if you wish to use them.
  configure will now attempt to autodetect both the Tcl library and header...

])
fi
])


dnl  EGG_TCL_WITH_TCLLIB()
dnl
AC_DEFUN(EGG_TCL_WITH_TCLLIB, [dnl
# Look for Tcl library: if $tcllibname is set, check there first
if test "${tcllibname+set}" = set
then
  if test -f "$tcllibname" && test -r "$tcllibname"
  then
    TCLLIB=`echo $tcllibname | sed 's%/[[^/]][[^/]]*$%%'`
    TCLLIBFN=`$BASENAME $tcllibname | cut -c4-`
    TCLLIBEXT=".`echo $TCLLIBFN | $AWK '{j=split([$]1, i, "."); print i[[j]]}'`"
    TCLLIBFNS=`$BASENAME $tcllibname $TCLLIBEXT | cut -c4-`
  else
    AC_MSG_WARN([

  The file '$tcllibname' given to option --with-tcllib is not valid.
  configure will now attempt to autodetect both the Tcl library and header...

])
    tcllibname=""
    tclincname=""
    TCLLIB=""
    TCLLIBFN=""
    TCLINC=""
    TCLINCFN=""
  fi
fi
])


dnl  EGG_TCL_WITH_TCLINC()
dnl
AC_DEFUN(EGG_TCL_WITH_TCLINC, [dnl
# Look for Tcl header: if $tclincname is set, check there first
if test "${tclincname+set}" = set
then
  if test -f "$tclincname" && test -r "$tclincname"
  then
    TCLINC=`echo $tclincname | sed 's%/[[^/]][[^/]]*$%%'`
    TCLINCFN=`$BASENAME $tclincname`
  else
    AC_MSG_WARN([

  The file '$tclincname' given to option --with-tclinc is not valid.
  configure will now attempt to autodetect both the Tcl library and header...

])
    tcllibname=""
    tclincname=""
    TCLLIB=""
    TCLLIBFN=""
    TCLINC=""
    TCLINCFN=""
  fi
fi
])


dnl  EGG_TCL_FIND_LIBRARY()
dnl
AC_DEFUN(EGG_TCL_FIND_LIBRARY, [dnl
# Look for Tcl library: if $TCLLIB is set, check there first
if test "${TCLLIBFN+set}" != set
then
  if test "${TCLLIB+set}" = set
  then
    if test -d "$TCLLIB"
    then
      for tcllibfns in $tcllibnames
      do
        for tcllibext in $tcllibextensions
        do
          if test -r "$TCLLIB/lib$tcllibfns$tcllibext"
          then
            TCLLIBFN="$tcllibfns$tcllibext"
            TCLLIBEXT="$tcllibext"
            TCLLIBFNS="$tcllibfns"
            break 2
          fi
        done
      done
    fi
    if test "${TCLLIBFN+set}" != set
    then
      AC_MSG_WARN([

  Environment variable TCLLIB was set, but incorrect.
  Please set both TCLLIB and TCLINC correctly if you wish to use them.
  configure will now attempt to autodetect both the Tcl library and header...

])
      TCLLIB=""
      TCLLIBFN=""
      TCLINC=""
      TCLINCFN=""
    fi
  fi
fi
])


dnl  EGG_TCL_FIND_HEADER()
dnl
AC_DEFUN(EGG_TCL_FIND_HEADER, [dnl
# Look for Tcl header: if $TCLINC is set, check there first
if test "${TCLINCFN+set}" != set
then
  if test "${TCLINC+set}" = set
  then
    if test -d "$TCLINC"
    then
      for tclheaderfn in $tclheadernames
      do
        if test -r "$TCLINC/$tclheaderfn"
        then
          TCLINCFN="$tclheaderfn"
          break
        fi
      done
    fi
    if test "${TCLINCFN+set}" != set
    then
      AC_MSG_WARN([

  Environment variable TCLINC was set, but incorrect.
  Please set both TCLLIB and TCLINC correctly if you wish to use them.
  configure will now attempt to autodetect both the Tcl library and header...

])
      TCLLIB=""
      TCLLIBFN=""
      TCLINC=""
      TCLINCFN=""
    fi
  fi
fi
])


dnl  EGG_TCL_CHECK_LIBRARY()
dnl
AC_DEFUN(EGG_TCL_CHECK_LIBRARY, [dnl
AC_MSG_CHECKING(for Tcl library)

# Attempt autodetect for $TCLLIBFN if it's not set
if test "${TCLLIBFN+set}" = set
then
  AC_MSG_RESULT(using $TCLLIB/lib$TCLLIBFN)
else
  for tcllibfns in $tcllibnames
  do
    for tcllibext in $tcllibextensions
    do
      for tcllibpath in $tcllibpaths
      do
        if test -r "$tcllibpath/lib$tcllibfns$tcllibext"
        then
          AC_MSG_RESULT(found $tcllibpath/lib$tcllibfns$tcllibext)
          TCLLIB="$tcllibpath"
          TCLLIBFN="$tcllibfns$tcllibext"
          TCLLIBEXT="$tcllibext"
          TCLLIBFNS="$tcllibfns"
          break 3
        fi
      done
    done
  done
fi

# Show if $TCLLIBFN wasn't found
if test "${TCLLIBFN+set}" != set
then
  AC_MSG_RESULT(not found)
fi
AC_SUBST(TCLLIB)
AC_SUBST(TCLLIBFN)
])


dnl  EGG_TCL_CHECK_HEADER()
dnl
AC_DEFUN(EGG_TCL_CHECK_HEADER, [dnl
AC_MSG_CHECKING(for Tcl header)

# Attempt autodetect for $TCLINCFN if it's not set
if test "${TCLINCFN+set}" = set
then
  AC_MSG_RESULT(using $TCLINC/$TCLINCFN)
else
  for tclheaderpath in $tclheaderpaths
  do
    for tclheaderfn in $tclheadernames
    do
      if test -r "$tclheaderpath/$tclheaderfn"
      then
        AC_MSG_RESULT(found $tclheaderpath/$tclheaderfn)
        TCLINC="$tclheaderpath"
        TCLINCFN="$tclheaderfn"
        break 2
      fi
    done
  done
  # FreeBSD hack ...
  if test "${TCLINCFN+set}" != set
  then
    for tcllibfns in $tcllibnames
    do
      for tclheaderpath in $tclheaderpaths
      do
        for tclheaderfn in $tclheadernames
        do
          if test -r "$tclheaderpath/$tcllibfns/$tclheaderfn"
          then
            AC_MSG_RESULT(found $tclheaderpath/$tcllibfns/$tclheaderfn)
            TCLINC="$tclheaderpath/$tcllibfns"
            TCLINCFN="$tclheaderfn"
            break 3
          fi
        done
      done
    done
  fi
fi

# Show if $TCLINCFN wasn't found
if test "${TCLINCFN+set}" != set
then
  AC_MSG_RESULT(not found)
fi
AC_SUBST(TCLINC)
AC_SUBST(TCLINCFN)
])


dnl  EGG_CACHE_UNSET(CACHE-ID)
dnl
dnl  Unsets a certain cache item. Typically called before using
dnl  the AC_CACHE_*() macros.
AC_DEFUN(EGG_CACHE_UNSET, [dnl
  unset $1
])


dnl  EGG_TCL_DETECT_CHANGE()
dnl
dnl  Detect whether the tcl system has changed since our last
dnl  configure run. Set egg_tcl_changed accordingly.
dnl
dnl  Tcl related feature and version checks should re-run their
dnl  checks as soon as egg_tcl_changed is set to "yes".
AC_DEFUN(EGG_TCL_DETECT_CHANGE, [dnl
  AC_MSG_CHECKING(whether the tcl system has changed)
  egg_tcl_changed=yes
  egg_tcl_id="$TCLLIB:$TCLLIBFN:$TCLINC:$TCLINCFN"
  if test "$egg_tcl_id" != :::
  then
    egg_tcl_cached=yes
    AC_CACHE_VAL(egg_cv_var_tcl_id, [dnl
      egg_cv_var_tcl_id="$egg_tcl_id"
      egg_tcl_cached=no
    ])
    if test "$egg_tcl_cached" = yes
    then
      if test "$egg_cv_var_tcl_id" = "$egg_tcl_id"
      then
        egg_tcl_changed=no
      else
        egg_cv_var_tcl_id="$egg_tcl_id"
      fi
    fi
  fi
  if test "$egg_tcl_changed" = yes
  then
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
])


dnl  EGG_TCL_CHECK_VERSION()
dnl
AC_DEFUN(EGG_TCL_CHECK_VERSION, [dnl
# Both TCLLIBFN & TCLINCFN must be set, or we bail
TCL_FOUND=0
if test "${TCLLIBFN+set}" = set && test "${TCLINCFN+set}" = set
then
  TCL_FOUND=1

  # Check Tcl's version
  if test "$egg_tcl_changed" = yes
  then
    EGG_CACHE_UNSET(egg_cv_var_tcl_version)
  fi
  AC_MSG_CHECKING(for Tcl version)
  AC_CACHE_VAL(egg_cv_var_tcl_version, [dnl
    egg_cv_var_tcl_version=`grep TCL_VERSION $TCLINC/$TCLINCFN | head -1 | $AWK '{gsub(/\"/, "", [$]3); print [$]3}'`
  ])

  if test "${egg_cv_var_tcl_version+set}" = set
  then
    AC_MSG_RESULT($egg_cv_var_tcl_version)
  else
    AC_MSG_RESULT(not found)
    TCL_FOUND=0
  fi

  # Check Tcl's patch level (if avaliable)
  if test "$egg_tcl_changed" = yes
  then
    EGG_CACHE_UNSET(egg_cv_var_tcl_patch_level)
  fi
  AC_MSG_CHECKING(for Tcl patch level)
  AC_CACHE_VAL(egg_cv_var_tcl_patch_level, [dnl
    eval "egg_cv_var_tcl_patch_level=`grep TCL_PATCH_LEVEL $TCLINC/$TCLINCFN | head -1 | $AWK '{gsub(/\"/, "", [$]3); print [$]3}'`"
  ])

  if test "${egg_cv_var_tcl_patch_level+set}" = set
  then
    AC_MSG_RESULT($egg_cv_var_tcl_patch_level)
  else
    egg_cv_var_tcl_patch_level=unknown
    AC_MSG_RESULT(unknown)
  fi
fi

# Check if we found Tcl's version
if test "$TCL_FOUND" = 0
then
  AC_MSG_ERROR([

  I can't find Tcl on this system.

  Eggdrop requires Tcl to compile.  If you already have Tcl installed
  on this system, and I just wasn't looking in the right place for it,
  set the environment variables TCLLIB and TCLINC so I will know where
  to find 'libtcl.a' (or 'libtcl.so') and 'tcl.h' (respectively). Then
  run 'configure' again.

  Read the README file if you don't know what Tcl is or how to get it
  and install it.

])
fi
])


dnl  EGG_TCL_CHECK_PRE80()
dnl
AC_DEFUN(EGG_TCL_CHECK_PRE80, [dnl
# Is this version of Tcl too old for us to use ?
TCL_VER_PRE80=`echo $egg_cv_var_tcl_version | $AWK '{split([$]1, i, "."); if (i[[1]] < 8) print "yes"; else print "no"}'`
if test "$TCL_VER_PRE80" = yes
then
  AC_MSG_ERROR([

  Your Tcl version is much too old for Eggdrop to use.
  I suggest you download and complie a more recent version.
  The most reliable current version is $tclrecommendver and
  can be downloaded from $tclrecommendsite

])
fi
])


dnl  EGG_TCL_TESTLIBS()
dnl
AC_DEFUN(EGG_TCL_TESTLIBS, [dnl
# Set variables for Tcl library tests
TCL_TEST_LIB="$TCLLIBFNS"
TCL_TEST_OTHERLIBS="-L$TCLLIB $EGG_MATH_LIB"

if test "${ac_cv_lib_pthread+set}" = set
then
  TCL_TEST_OTHERLIBS="$TCL_TEST_OTHERLIBS $ac_cv_lib_pthread"
fi
])

dnl  EGG_TCL_CHECK_THREADS()
dnl
AC_DEFUN(EGG_TCL_CHECK_THREADS, [dnl

if test "$egg_tcl_changed" = yes
then
  EGG_CACHE_UNSET(egg_cv_var_tcl_threaded)
fi

# Check for TclpFinalizeThreadData()
AC_CHECK_LIB($TCL_TEST_LIB, TclpFinalizeThreadData, egg_cv_var_tcl_threaded=yes, egg_cv_var_tcl_threaded=no, $TCL_TEST_OTHERLIBS)

if test "$egg_cv_var_tcl_threaded" = yes
then
  # Add pthread library to $LIBS if we need it
  if test "${ac_cv_lib_pthread+set}" = set
  then
    LIBS="$ac_cv_lib_pthread $LIBS"
  fi
fi
])


dnl  EGG_TCL_LIB_REQS()
dnl
AC_DEFUN(EGG_TCL_LIB_REQS, [dnl
AC_REQUIRE([EGG_LIBTOOL])

if test "$EGG_CYGWIN" = yes
then
  TCL_REQS="$TCLLIB/lib$TCLLIBFN"
  TCL_LIBS="-L$TCLLIB -l$TCLLIBFNS $EGG_MATH_LIB"
else
if test ! "$TCLLIBEXT" = ".a"
then
  TCL_REQS="$TCLLIB/lib$TCLLIBFN"
  TCL_LIBS="-L$TCLLIB -l$TCLLIBFNS $EGG_MATH_LIB"
else
  # Error and ask for a static build for unshared Tcl library
  if test "$egg_static_build" = no
  then
    AC_MSG_ERROR([

  Your Tcl library is not a shared lib.
  You have to run configure with the --disable-shared parameter to force a
  static build.

])
  fi
fi
fi
AC_SUBST(TCL_REQS)
AC_SUBST(TCL_LIBS)
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


dnl  EGG_TCL_LUSH()
dnl
AC_DEFUN(EGG_TCL_LUSH, [dnl
    EGG_REPLACE_IF_CHANGED(lush.h, [
cat > conftest.out << EGGEOF
/* Ignore me but do not erase me.  I am a kludge. */

#include "$egg_tclinc/$egg_tclincfn"
EGGEOF], [
    egg_tclinc="$TCLINC"
    egg_tclincfn="$TCLINCFN"])
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
AC_DEFUN(EGG_IPV6_OPTIONS, [dnl
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
AC_DEFUN(EGG_TYPE_SOCKLEN_T, [dnl
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
AC_DEFUN(EGG_INADDR_LOOPBACK, [dnl
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
AC_DEFUN(EGG_IPV6_SUPPORTED, [dnl
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
AC_DEFUN(EGG_DEFINE_VERSION_NUM, [dnl
egg_version_num=`echo $VERSION | $AWK 'BEGIN {FS = "."} {printf("%d%02d%02d00", [$]1, [$]2, [$]3)}'`
AC_DEFINE_UNQUOTED(VERSION_NUM, $egg_version_num,
                   [Define version number.])
])


dnl  EGG_GNU_GETTEXT
dnl
AC_DEFUN(EGG_GNU_GETTEXT, [dnl
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
AC_DEFUN(EGG_LIBTOOL, [dnl
AC_DISABLE_FAST_INSTALL
AC_DISABLE_STATIC
AC_LIBTOOL_WIN32_DLL
AC_LIBLTDL_CONVENIENCE
AC_SUBST(INCLTDL)
AC_SUBST(LIBLTDL)
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

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
AC_DEFUN(EGG_WITH_EFENCE, [dnl
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
AC_DEFUN(EGG_DEBUG_OPTIONS, [dnl
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


dnl  EGG_LTLIBOBJS
dnl
AC_DEFUN(EGG_LTLIBOBJS, [dnl

AC_CONFIG_COMMANDS_PRE(
              [LIBOBJS=`echo "$LIBOBJS" |
                        sed 's,\.[[^.]]* ,$U&,g;s,\.[[^.]]*$,$U&,'`
               LTLIBOBJS=`echo "$LIBOBJS" |
                          sed 's,\.[[^.]]* ,.lo ,g;s,\.[[^.]]*$,.lo,'`
               AC_SUBST(LTLIBOBJS)])
])


dnl  EGG_COMPRESS_MODULE
dnl
AC_DEFUN(EGG_COMPRESS_MODULE, [dnl

egg_compress=no

AC_CHECK_LIB(z, gzopen, ZLIB="-lz")
AC_CHECK_HEADER(zlib.h)

# Disable the module if either the header file or the library
# are missing.
if test "${ZLIB+set}" != set
then
  AC_MSG_WARN([

  Your system does not provide a working zlib compression library. The
  compress module will therefore be disabled.

])
else
  if test "$ac_cv_header_zlib_h" != yes
  then
  AC_MSG_WARN([

  Your system does not provide the necessary zlib header files. The
  compress module will therefore be disabled.

])
  else
    egg_compress=yes
    AC_FUNC_MMAP
    AC_SUBST(ZLIB)
  fi
fi

AM_CONDITIONAL(EGG_COMPRESS, test "$egg_compress" = yes)
])

# FIXME: is it worth to make this macro more anal? Yes it is!
dnl  EGG_PERLSCRIPT_MODULE
dnl
AC_DEFUN(EGG_PERLSCRIPT_MODULE, [dnl

AC_ARG_WITH(perlscript,
            AC_HELP_STRING([--without-perlscript],
                           [build without the perl module]),
                           egg_with_perlscript="$withval", egg_with_perlscript=yes)
egg_perlscript=no

if test "$egg_with_perlscript" = yes
then

AC_PATH_PROG(perlcmd, perl)
PERL_LDFLAGS=`$perlcmd -MExtUtils::Embed -e ldopts 2>/dev/null`
if test "${PERL_LDFLAGS+set}" != set
then
  AC_MSG_WARN([

  Your system does not provide a working perl environment. The
  perlscript module will therefore be disabled.
  
])
else
  PERL_CCFLAGS=`$perlcmd -MExtUtils::Embed -e ccopts 2>/dev/null`
  egg_perlscript=yes
  AC_SUBST(PERL_LDFLAGS)
  AC_SUBST(PERL_CCFLAGS)
fi

fi
AM_CONDITIONAL(EGG_PERLSCRIPT, test "$egg_perlscript" = yes)
])

dnl  EGG_TCLSCRIPT_MODULE
dnl
AC_DEFUN(EGG_TCLSCRIPT_MODULE, [dnl

egg_tclscript=no

# Is this version of Tcl too old for tclscript to use ?
TCL_VER_PRE80=`echo $egg_cv_var_tcl_version | $AWK '{split([$]1, i, "."); if (i[[1]] < 8) print "yes"; else print "no"}'`
if test "$TCL_VER_PRE80" = yes
then
  AC_MSG_WARN([

  Your system does not provide tcl 8.0 or later. The
  tclscript module will therefore be disabled.

])
else
  egg_tclscript=yes
fi

AM_CONDITIONAL(EGG_TCLSCRIPT, test "$egg_tclscript" = yes)
])

dnl  EGG_JAVASCRIPT_MODULE
dnl
AC_DEFUN(EGG_JAVASCRIPT_MODULE, [dnl

egg_javascript=no

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
else
  if test "$ac_cv_header_jsapi_h" != yes
  then
    AC_MSG_WARN([

  Your system does not provide the necessary javascript header files. The
  javascript module will therefore be disabled.

])
  else
    egg_javascript=yes
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

AM_CONDITIONAL(EGG_JAVASCRIPT, test "$egg_javascript" = yes)
])

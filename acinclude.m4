dnl acinclude.m4
dnl   macros autoconf uses when building configure from configure.in
dnl
dnl $Id: acinclude.m4,v 1.8 2001/10/19 01:55:04 tothwolf Exp $
dnl


dnl  EGG_MSG_CONFIGURE_START()
dnl
AC_DEFUN(EGG_MSG_CONFIGURE_START, [dnl
AC_MSG_RESULT()
AC_MSG_RESULT(This is eggdrop's GNU configure script.)
AC_MSG_RESULT(It's going to run a bunch of strange tests to hopefully)
AC_MSG_RESULT(make your compile work without much twiddling.)
AC_MSG_RESULT()
])


dnl  EGG_MSG_CONFIGURE_END()
dnl
AC_DEFUN(EGG_MSG_CONFIGURE_END, [dnl
AC_MSG_RESULT()
cat << EOF
------------------------------------------------------------------------
Configuration:

  Source code location:       ${srcdir}
  Compiler:                   ${CC}
  Compiler flags:             ${CFLAGS}
  Host System Type:           ${host}
  Install path:               ${prefix}

See config.h for further configuration information.
------------------------------------------------------------------------
EOF
AC_MSG_RESULT()
AC_MSG_RESULT(Configure is done.)
AC_MSG_RESULT()
AC_MSG_RESULT([Type 'make' to create the bot.])
AC_MSG_RESULT()
])


dnl  EGG_CHECK_CC()
dnl
dnl  FIXME: make a better test
dnl
AC_DEFUN(EGG_CHECK_CC, [dnl
if test "${cross_compiling-x}" = "x"
then
  cat << 'EOF' >&2
configure: error:

  This system does not appear to have a working C compiler.
  A working C compiler is required to compile eggdrop.

EOF
  exit 1
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
    if test "$egg_cv_var_ccpipe" = "yes"
    then
      CC="$CC -pipe"
    fi
  fi
fi
])


dnl  EGG_PROG_STRIP()
dnl
AC_DEFUN(EGG_PROG_STRIP, [dnl
AC_CHECK_PROG(STRIP, strip, strip)
if test "${STRIP-x}" = "x"
then
  STRIP=touch
fi
])


dnl  EGG_PROG_AWK()
dnl
AC_DEFUN(EGG_PROG_AWK, [dnl
# awk is needed for Tcl library and header checks, and eggdrop version subst
AC_PROG_AWK
if test "${AWK-x}" = "x"
then
  cat << 'EOF' >&2
configure: error:

  This system seems to lack a working 'awk' command.
  A working 'awk' command is required to compile eggdrop.

EOF
  exit 1
fi
])


dnl  EGG_PROG_BASENAME()
dnl
AC_DEFUN(EGG_PROG_BASENAME, [dnl
# basename is needed for Tcl library and header checks
AC_CHECK_PROG(BASENAME, basename, basename)
if test "${BASENAME-x}" = "x"
then
  cat << 'EOF' >&2
configure: error:

  This system seems to lack a working 'basename' command.
  A working 'basename' command is required to compile eggdrop.

EOF
  exit 1
fi
])


dnl  EGG_CHECK_OS()
dnl
dnl  FIXME/NOTICE:
dnl    This function is obsolete. Any NEW code/checks should be written
dnl    as individual tests that will be checked on ALL operating systems.
dnl
AC_DEFUN(EGG_CHECK_OS, [dnl
AC_REQUIRE([AC_CANONICAL_HOST])

IRIX=no
DEFAULT_MAKE=static

AC_CACHE_CHECK(system type, egg_cv_var_system_type, egg_cv_var_system_type=`$EGG_UNAME -s`)
AC_CACHE_CHECK(system release, egg_cv_var_system_release, egg_cv_var_system_release=`$EGG_UNAME -r`)

case $host_os in
  cygwin)
    case "`echo $egg_cv_var_system_release | cut -c 1-3`" in
      1.*)
	AC_PROG_CC_WIN32
	CC="$CC $WIN32FLAGS"
        AC_MSG_CHECKING(for /usr/lib/binmode.o)
        if test -r /usr/lib/binmode.o
        then
          AC_MSG_RESULT(yes)
          LIBS="$LIBS /usr/lib/binmode.o"
        else
          AC_MSG_RESULT(no)
          AC_MSG_WARN(Make sure the directory eggdrop is installed into is mounted in binary mode.)
        fi
      ;;
      *)
        AC_MSG_WARN(Make sure the directory eggdrop is installed into is mounted in binary mode.)
      ;;
    esac
  ;;
  hpux*)
    AC_DEFINE(HPUX_HACKS, 1,
              [Define if running on hpux that supports dynamic linking])
    case $host_os in
    hpux10)
      AC_DEFINE(HPUX10_HACKS, 1,
                [Define if running on hpux 10.x])
    ;;
    esac
  ;;
  irix*)
    IRIX=yes
  ;;
  osf*)
    AC_DEFINE(STOP_UAC, 1,
              [Define if running on OSF/1 platform])
  ;;
  next*)
    AC_DEFINE(BORGCUBES, 1,
              [Define if running on NeXT Step])
  ;;
esac

])


dnl  EGG_CHECK_LIBS()
dnl
AC_DEFUN(EGG_CHECK_LIBS, [dnl
# FIXME: this needs to be fixed so that it works on IRIX
if test "$IRIX" = "yes"
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
egg_cv_have_long_long="yes",
egg_cv_have_long_long="no",
egg_cv_have_long_long="cross")])
if test "$egg_cv_have_long_long" = "yes"
then
  AC_DEFINE(HAVE_LONG_LONG, 1,
	    [Define if system supports long long])
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
egg_cv_have_c99_vsnprintf="yes",
egg_cv_have_c99_vsnprintf="no",
egg_cv_have_c99_vsnprintf="cross")])
if test "$egg_cv_have_c99_vsnprintf" = "yes"
then
  AC_DEFINE(HAVE_C99_VSNPRINTF, 1,
            [Define if vsnprintf is C99 compliant])
  egg_replace_snprintf=yes
fi
])


dnl  EGG_REPLACE_SNPRINTF
dnl
AC_DEFUN(EGG_REPLACE_SNPRINTF, [dnl
if test "${egg_replace_snprintf-x}" = "yes"
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
            [Define this if an unsigned int is 32 bits])
else
  AC_CHECK_SIZEOF(unsigned long, 4)
  if test "$ac_cv_sizeof_unsigned_long" = 4
  then
    AC_DEFINE(UNSIGNED_LONG32, 1,
              [Define this if an unsigned long is 32 bits])
  fi
fi
])


dnl  EGG_CYGWIN()
dnl
dnl  Check for Cygwin support.
AC_DEFUN(EGG_CYGWIN, [dnl
AC_CYGWIN
if test "$ac_cv_cygwin" = "yes"
then
  AC_DEFINE(CYGWIN_HACKS, 1,
            [Define if running under cygwin])
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
if test ! "${tcllibname-x}" = "x"
then
  if test "${tclincname-x}" = "x"
  then
    WARN=1
    tcllibname=""
    TCLLIB=""
    TCLINC=""
  fi
else
  if test ! "${tclincname-x}" = "x"
  then
    WARN=1
    tclincname=""
    TCLLIB=""
    TCLINC=""
  fi
fi
if test "$WARN" = 1
then
  cat << 'EOF' >&2
configure: warning:

  You must specify both --with-tcllib and --with-tclinc for them to work.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
fi
])


dnl  EGG_TCL_ENV()
dnl
AC_DEFUN(EGG_TCL_ENV, [dnl
WARN=0
# Make sure either both or neither $TCLLIB and $TCLINC are set
if test ! "${TCLLIB-x}" = "x"
then
  if test "${TCLINC-x}" = "x"
  then
    WARN=1
    WVAR1=TCLLIB
    WVAR2=TCLINC
    TCLLIB=""
  fi
else
  if test ! "${TCLINC-x}" = "x"
  then
    WARN=1
    WVAR1=TCLINC
    WVAR2=TCLLIB
    TCLINC=""
  fi
fi
if test "$WARN" = 1
then
  cat << EOF >&2
configure: warning:

  Environment variable $WVAR1 was set, but I did not detect ${WVAR2}.
  Please set both TCLLIB and TCLINC correctly if you wish to use them.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
fi
])


dnl  EGG_TCL_WITH_TCLLIB()
dnl
AC_DEFUN(EGG_TCL_WITH_TCLLIB, [dnl
# Look for Tcl library: if $tcllibname is set, check there first
if test ! "${tcllibname-x}" = "x"
then
  if test -f "$tcllibname" && test -r "$tcllibname"
  then
    TCLLIB=`echo $tcllibname | sed 's%/[[^/]][[^/]]*$%%'`
    TCLLIBFN=`$BASENAME $tcllibname | cut -c4-`
    TCLLIBEXT=".`echo $TCLLIBFN | $AWK '{j=split([$]1, i, "."); print i[[j]]}'`"
    TCLLIBFNS=`$BASENAME $tcllibname $TCLLIBEXT | cut -c4-`
  else
    cat << EOF >&2
configure: warning:

  The file '$tcllibname' given to option --with-tcllib is not valid.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
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
if test ! "${tclincname-x}" = "x"
then
  if test -f "$tclincname" && test -r "$tclincname"
  then
    TCLINC=`echo $tclincname | sed 's%/[[^/]][[^/]]*$%%'`
    TCLINCFN=`$BASENAME $tclincname`
  else
    cat << EOF >&2
configure: warning:

  The file '$tclincname' given to option --with-tclinc is not valid.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
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
if test "${TCLLIBFN-x}" = "x"
then
  if test ! "${TCLLIB-x}" = "x"
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
    if test "${TCLLIBFN-x}" = "x"
    then
      cat << 'EOF' >&2
configure: warning:

  Environment variable TCLLIB was set, but incorrect.
  Please set both TCLLIB and TCLINC correctly if you wish to use them.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
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
if test "${TCLINCFN-x}" = "x"
then
  if test ! "${TCLINC-x}" = "x"
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
    if test "${TCLINCFN-x}" = "x"
    then
      cat << 'EOF' >&2
configure: warning:

  Environment variable TCLINC was set, but incorrect.
  Please set both TCLLIB and TCLINC correctly if you wish to use them.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
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
if test ! "${TCLLIBFN-x}" = "x"
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
if test "${TCLLIBFN-x}" = "x"
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
if test ! "${TCLINCFN-x}" = "x"
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
  if test "${TCLINCFN-x}" = "x"
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
if test "${TCLINCFN-x}" = "x"
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
  if test ! "$egg_tcl_id" = ":::"
  then
    egg_tcl_cached=yes
    AC_CACHE_VAL(egg_cv_var_tcl_id, [dnl
      egg_cv_var_tcl_id="$egg_tcl_id"
      egg_tcl_cached=no
    ])
    if test "$egg_tcl_cached" = "yes"
    then
      if test "${egg_cv_var_tcl_id-x}" = "${egg_tcl_id-x}"
      then
        egg_tcl_changed=no
      else
        egg_cv_var_tcl_id="$egg_tcl_id"
      fi
    fi
  fi
  if test "$egg_tcl_changed" = "yes"
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
if test ! "${TCLLIBFN-x}" = "x" && test ! "${TCLINCFN-x}" = "x"
then
  TCL_FOUND=1

  # Check Tcl's version
  if test "$egg_tcl_changed" = "yes"
  then
    EGG_CACHE_UNSET(egg_cv_var_tcl_version)
  fi
  AC_MSG_CHECKING(for Tcl version)
  AC_CACHE_VAL(egg_cv_var_tcl_version, [dnl
    egg_cv_var_tcl_version=`grep TCL_VERSION $TCLINC/$TCLINCFN | head -1 | $AWK '{gsub(/\"/, "", [$]3); print [$]3}'`
  ])

  if test ! "${egg_cv_var_tcl_version-x}" = "x"
  then
    AC_MSG_RESULT($egg_cv_var_tcl_version)
  else
    AC_MSG_RESULT(not found)
    TCL_FOUND=0
  fi

  # Check Tcl's patch level (if avaliable)
  if test "$egg_tcl_changed" = "yes"
  then
    EGG_CACHE_UNSET(egg_cv_var_tcl_patch_level)
  fi
  AC_MSG_CHECKING(for Tcl patch level)
  AC_CACHE_VAL(egg_cv_var_tcl_patch_level, [dnl
    eval "egg_cv_var_tcl_patch_level=`grep TCL_PATCH_LEVEL $TCLINC/$TCLINCFN | head -1 | $AWK '{gsub(/\"/, "", [$]3); print [$]3}'`"
  ])

  if test ! "${egg_cv_var_tcl_patch_level-x}" = "x"
  then
    AC_MSG_RESULT($egg_cv_var_tcl_patch_level)
  else
    egg_cv_var_tcl_patch_level="unknown"
    AC_MSG_RESULT(unknown)
  fi
fi

# Check if we found Tcl's version
if test "$TCL_FOUND" = 0
then
  cat << 'EOF' >&2
configure: error:

  I can't find Tcl on this system.

  Eggdrop requires Tcl to compile.  If you already have Tcl installed
  on this system, and I just wasn't looking in the right place for it,
  set the environment variables TCLLIB and TCLINC so I will know where
  to find 'libtcl.a' (or 'libtcl.so') and 'tcl.h' (respectively). Then
  run 'configure' again.

  Read the README file if you don't know what Tcl is or how to get it
  and install it.

EOF
  exit 1
fi
])


dnl  EGG_TCL_CHECK_PRE70()
dnl
AC_DEFUN(EGG_TCL_CHECK_PRE70, [dnl
# Is this version of Tcl too old for us to use ?
TCL_VER_PRE70=`echo $egg_cv_var_tcl_version | $AWK '{split([$]1, i, "."); if (i[[1]] < 7) print "yes"; else print "no"}'`
if test "$TCL_VER_PRE70" = "yes"
then
  cat << EOF >&2
configure: error:

  Your Tcl version is much too old for eggdrop to use.
  I suggest you download and complie a more recent version.
  The most reliable current version is $tclrecommendver and
  can be downloaded from $tclrecommendsite

EOF
  exit 1
fi
])


dnl  EGG_TCL_CHECK_PRE75()
dnl
AC_DEFUN(EGG_TCL_CHECK_PRE75, [dnl
# Are we using a pre 7.5 Tcl version ?
TCL_VER_PRE75=`echo $egg_cv_var_tcl_version | $AWK '{split([$]1, i, "."); if (((i[[1]] == 7) && (i[[2]] < 5)) || (i[[1]] < 7)) print "yes"; else print "no"}'`
if test "$TCL_VER_PRE75" = "yes"
then
  AC_DEFINE(HAVE_PRE7_5_TCL, 1,
            [Define for pre Tcl 7.5 compatibility])
fi
])


dnl  EGG_TCL_TESTLIBS()
dnl
AC_DEFUN(EGG_TCL_TESTLIBS, [dnl
# Set variables for Tcl library tests
TCL_TEST_LIB="$TCLLIBFNS"
TCL_TEST_OTHERLIBS="-L$TCLLIB $EGG_MATH_LIB"

if test ! "${ac_cv_lib_pthread-x}" = "x"
then
  TCL_TEST_OTHERLIBS="$TCL_TEST_OTHERLIBS $ac_cv_lib_pthread"
fi
])


dnl  EGG_TCL_CHECK_FREE()
dnl
AC_DEFUN(EGG_TCL_CHECK_FREE, [dnl
if test "$egg_tcl_changed" = "yes"
then
  EGG_CACHE_UNSET(egg_cv_var_tcl_free)
fi

# Check for Tcl_Free()
AC_CHECK_LIB($TCL_TEST_LIB, Tcl_Free, egg_cv_var_tcl_free=yes, egg_cv_var_tcl_free=no, $TCL_TEST_OTHERLIBS)

if test "$egg_cv_var_tcl_free" = "yes"
then
  AC_DEFINE(HAVE_TCL_FREE, 1,
            [Define for Tcl that has Tcl_Free() (7.5p1 and later)])
fi
])


dnl  EGG_TCL_THREADS_OPTIONS()
dnl
AC_DEFUN(EGG_TCL_THREADS_OPTIONS, [dnl
AC_ARG_ENABLE(tcl-threads,
              AC_HELP_STRING([--disable-tcl-threads],
                             [disable threaded tcl support if detected (ignore this option unless you know what you are doing)]),
              enable_tcl_threads="$enableval",
              enable_tcl_threads=yes)
])


dnl  EGG_TCL_CHECK_THREADS()
dnl
AC_DEFUN(EGG_TCL_CHECK_THREADS, [dnl
if test "$egg_tcl_changed" = "yes"
then
  EGG_CACHE_UNSET(egg_cv_var_tcl_threaded)
fi

# Check for TclpFinalizeThreadData()
AC_CHECK_LIB($TCL_TEST_LIB, TclpFinalizeThreadData, egg_cv_var_tcl_threaded=yes, egg_cv_var_tcl_threaded=no, $TCL_TEST_OTHERLIBS)

if test "$egg_cv_var_tcl_threaded" = "yes"
then
  if test "$enable_tcl_threads" = "no"
  then

    cat << 'EOF' >&2
configure: warning:

  You have disabled threads support on a system with a threaded Tcl library.
  Tcl features that rely on scheduled events may not function properly.

EOF
  else
    AC_DEFINE(HAVE_TCL_THREADS, 1,
              [Define for Tcl that has threads])
  fi

  # Add pthread library to $LIBS if we need it
  if test ! "${ac_cv_lib_pthread-x}" = "x"
  then
    LIBS="$ac_cv_lib_pthread $LIBS"
  fi
fi
])


dnl  EGG_TCL_LIB_REQS()
dnl
AC_DEFUN(EGG_TCL_LIB_REQS, [dnl
if test "$ac_cv_cygwin" = "yes"
then
  TCL_REQS="$TCLLIB/lib$TCLLIBFN"
  TCL_LIBS="-L$TCLLIB -l$TCLLIBFNS $EGG_MATH_LIB"
else
if test ! "$TCLLIBEXT" = ".a"
then
  TCL_REQS="$TCLLIB/lib$TCLLIBFN"
  TCL_LIBS="-L$TCLLIB -l$TCLLIBFNS $EGG_MATH_LIB"
else
## FIXME: this needs to be changed so that it will error and exit saying
##        you have to run ./configure --disable-shared
  # Set default make as static for unshared Tcl library
  if test ! "$DEFAULT_MAKE" = "static"
  then
    cat << 'EOF' >&2
configure: warning:

  Your Tcl library is not a shared lib.
  configure will now set default make type to static...

EOF
    DEFAULT_MAKE=static
    AC_SUBST(DEFAULT_MAKE)
  fi

  # Are we using a pre 7.4 Tcl version ?
  TCL_VER_PRE74=`echo $egg_cv_var_tcl_version | $AWK '{split([$]1, i, "."); if (((i[[1]] == 7) && (i[[2]] < 4)) || (i[[1]] < 7)) print "yes"; else print "no"}'`
  if test "$TCL_VER_PRE74" = "no"
  then

    # Was the --with-tcllib option given ?
    if test ! "${tcllibname-x}" = "x"
    then
      TCL_REQS="$TCLLIB/lib$TCLLIBFN"
      TCL_LIBS="$TCLLIB/lib$TCLLIBFN $EGG_MATH_LIB"
    else
      TCL_REQS="$TCLLIB/lib$TCLLIBFN"
      TCL_LIBS="-L$TCLLIB -l$TCLLIBFNS $EGG_MATH_LIB"
    fi
  else
    cat << EOF >&2
configure: warning:

  Your Tcl version ($egg_cv_var_tcl_version) is older then 7.4.
  There are known problems, but we will attempt to work around them.

EOF
    TCL_REQS="libtcle.a"
    TCL_LIBS="-L`pwd` -ltcle $EGG_MATH_LIB"
  fi
fi
fi
AC_SUBST(TCL_REQS)
AC_SUBST(TCL_LIBS)
])


dnl  EGG_REPLACE_IF_CHANGED(FILE-NAME, CONTENTS-CMDS, INIT-CMDS)
dnl
dnl  Replace FILE-NAME if the newly created contents differs from the existing
dnl  file contents.  Otherwise leave the file allone.  This avoids needless
dnl  recompiles.
dnl
define(EGG_REPLACE_IF_CHANGED, [dnl
  AC_OUTPUT_COMMANDS([
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
AC_DEFUN([AC_PROG_CC_WIN32], [
AC_MSG_CHECKING([how to access the Win32 API])
WIN32FLAGS=
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
  if test "$egg_cv_ipv6_supported" = "yes"
  then
    ac_cv_ipv6=yes
  else
    ac_cv_ipv6=no
  fi
  AC_MSG_RESULT((default) $ac_cv_ipv6)
])
if test "$ac_cv_ipv6" = "yes"
then
  AC_DEFINE(IPV6, 1,
            [Define if you want IPv6 support])
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
if test "$egg_cv_var_socklen_t" = "no"
then
  AC_DEFINE(socklen_t, unsigned,
            [Define to `unsigned' if something else doesn't define])
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
if test "$adns_cv_decl_inaddrloopback" = "yes"
then
 AC_MSG_RESULT(found)
else
 AC_MSG_RESULT([not in standard headers, urgh...])
 AC_CHECK_HEADER(rpc/types.h,[
  AC_DEFINE(HAVEUSE_RPCTYPES_H, 1,
            [Define if we want to include rpc/types.h.  Crap BSDs put INADDR_LOOPBACK there.])
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
if test "$egg_cv_ipv6_supported" = "yes"
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
                   [Define version number])
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

AM_GNU_GETTEXT
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
AM_PROG_LIBTOOL

if test "x$enable_shared" = "xno"
then
  AC_DEFINE_UNQUOTED(STATIC, 1, Define if build is static)
fi
])dnl


dnl  EGG_DEBUG_OPTIONS
dnl
AC_DEFUN(EGG_DEBUG_OPTIONS, [dnl
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--disable-debug],
                             [disable debug support]),
                             debug="$enableval", debug=yes)
if test "$debug" = "yes"
then
  # FIXME: this should be done along with `--with-efence'
  AC_CHECK_LIB(efence, malloc)
  EGG_DEBUG="-DDEBUG"
fi
AC_SUBST(EGG_DEBUG)
])


dnl  EGG_COMPRESS_MODULE
dnl
AC_DEFUN(EGG_COMPRESS_MODULE, [dnl

egg_compress=no

AC_CHECK_LIB(z, gzopen, ZLIB="-lz", )
AC_CHECK_HEADER(zlib.h)

# Disable the module if either the header file or the library
# are missing.
if test "${ZLIB-x}" = "x"
then
  cat << 'EOF' >&2
configure: warning:

  Your system does not provide a working zlib compression library. The
  compress module will therefore be disabled.

EOF
else
  if test ! "${ac_cv_header_zlib_h}" = "yes"
  then
    cat << 'EOF' >&2
configure: warning:

  Your system does not provide the necessary zlib header files. The
  compress module will therefore be disabled.

EOF
  else
    egg_compress=yes
    AC_FUNC_MMAP
    AC_SUBST(ZLIB)
  fi
fi

AM_CONDITIONAL(EGG_COMPRESS, test "$egg_compress" = "yes")
])

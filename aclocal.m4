dnl aclocal.m4
dnl   macros autoconf uses when building configure from configure.in
dnl
dnl $Id: aclocal.m4,v 1.54 2001/08/22 00:41:57 ite Exp $
dnl


dnl  EGG_MSG_CONFIGURE_START()
dnl
AC_DEFUN(EGG_MSG_CONFIGURE_START, [dnl
AC_MSG_RESULT()
AC_MSG_RESULT(This is eggdrop's GNU configure script.)
AC_MSG_RESULT(It's going to run a bunch of strange tests to hopefully)
AC_MSG_RESULT(make your compile work without much twiddling.)
AC_MSG_RESULT()
])dnl


dnl  EGG_MSG_CONFIGURE_END()
dnl
AC_DEFUN(EGG_MSG_CONFIGURE_END, [dnl
AC_MSG_RESULT()
AC_MSG_RESULT(Configure is done.)
AC_MSG_RESULT()
AC_MSG_RESULT(Type 'make config' to configure the modules. Or 'make iconfig' to)
AC_MSG_RESULT(interactively choose which modules to compile.)
AC_MSG_RESULT()
if test -f "./$EGGEXEC"
then
  AC_MSG_RESULT([After that, type 'make clean' and then 'make' to create the bot.])
else
  AC_MSG_RESULT([After that, type 'make' to create the bot.])
fi
AC_MSG_RESULT()
])dnl


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
])dnl


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
      AC_TRY_COMPILE(,, egg_cv_var_ccpipe="yes", egg_cv_var_ccpipe="no")
      CC="$ac_old_CC"
    ])
    if test "$egg_cv_var_ccpipe" = "yes"
    then
      CC="$CC -pipe"
    fi
  fi
fi
])dnl


dnl  EGG_PROG_STRIP()
dnl
AC_DEFUN(EGG_PROG_STRIP, [dnl
AC_CHECK_PROG(STRIP, strip, strip)
if test "${STRIP-x}" = "x"
then
  STRIP=touch
fi
])dnl


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
])dnl


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
])dnl


dnl  EGG_CHECK_OS()
dnl
dnl  FIXME/NOTICE:
dnl    This function is obsolete. Any NEW code/checks should be written
dnl    as individual tests that will be checked on ALL operating systems.
dnl
AC_DEFUN(EGG_CHECK_OS, [dnl
LINUX=no
IRIX=no
SUNOS=no
HPUX=no
MOD_CC="$CC"
MOD_LD="$CC"
MOD_STRIP="$STRIP"
SHLIB_CC="$CC"
SHLIB_LD="$CC"
SHLIB_STRIP="$STRIP"
NEED_DL=1
DEFAULT_MAKE=debug
MOD_EXT=so

AC_CACHE_CHECK(system type, egg_cv_var_system_type, egg_cv_var_system_type=`$UNAME -s`)
AC_CACHE_CHECK(system release, egg_cv_var_system_release, egg_cv_var_system_release=`$UNAME -r`)

case "$egg_cv_var_system_type" in
  BSD/OS)
    case "`echo $egg_cv_var_system_release | cut -d . -f 1`" in
      2)
        NEED_DL=0
        DEFAULT_MAKE=static
      ;;
      3)
        MOD_CC=shlicc
        MOD_LD=shlicc
        MOD_STRIP="$STRIP -d"
        SHLIB_LD="shlicc -r"
        SHLIB_STRIP=touch
        AC_DEFINE(MODULES_OK)dnl
      ;;
      *)
        CFLAGS="$CFLAGS -Wall"
        MOD_LD="$CC"
        MOD_STRIP="$STRIP -d"
        SHLIB_CC="$CC -export-dynamic -fPIC"
        SHLIB_LD="$CC -shared -nostartfiles"
        AC_DEFINE(MODULES_OK)dnl
      ;;
    esac
  ;;
  CYGWIN*)
    case "`echo $egg_cv_var_system_release | cut -c 1-3`" in
      1.*)
        NEED_DL=0
        SHLIB_LD="$CC -shared"
	AC_PROG_CC_WIN32
	CC="$CC $WIN32FLAGS"
        MOD_CC="$CC"
        MOD_LD="$CC"
        AC_MSG_CHECKING(for /usr/lib/binmode.o)
        if test -r /usr/lib/binmode.o
        then
          AC_MSG_RESULT(yes)
          LIBS="$LIBS /usr/lib/binmode.o"
        else
          AC_MSG_RESULT(no)
          AC_MSG_WARN(Make sure the directory eggdrop is installed into is mounted in binary mode.)
        fi
        MOD_EXT=dll
        AC_DEFINE(MODULES_OK)dnl
      ;;
      *)
        NEED_DL=0
        DEFAULT_MAKE=static
        AC_MSG_WARN(Make sure the directory eggdrop is installed into is mounted in binary mode.)
      ;;
    esac
  ;;
  HP-UX)
    HPUX=yes
    MOD_LD="$CC -fPIC -shared"
    SHLIB_CC="$CC -fPIC"
    SHLIB_LD="ld -b"
    NEED_DL=0
    AC_DEFINE(MODULES_OK)dnl
    AC_DEFINE(HPUX_HACKS)dnl
    if test "`echo $egg_cv_var_system_release | cut -d . -f 2`" = "10"
    then
      AC_DEFINE(HPUX10_HACKS)dnl
    fi
  ;;
  dell)
    AC_MSG_RESULT(Dell SVR4)
    SHLIB_STRIP=touch
    NEED_DL=0
    MOD_LD="$CC -lelf -lucb"
  ;;
  IRIX)
    SHLIB_LD="ld -n32 -shared -rdata_shared"
    IRIX=yes
    SHLIB_STRIP=touch
    NEED_DL=0
    DEFAULT_MAKE=static
  ;;
  Ultrix)
    NEED_DL=0
    SHLIB_STRIP=touch
    DEFAULT_MAKE=static
    SHELL=/bin/sh5
  ;;
  SINIX*)
    NEED_DL=0
    SHLIB_STRIP=touch
    DEFAULT_MAKE=static
    SHLIB_CC="cc -G"
  ;;
  BeOS)
    NEED_DL=0
    SHLIB_STRIP=strip
    DEFAULT_MAKE=static
  ;;
  Linux)
    LINUX=yes
    CFLAGS="$CFLAGS -Wall"
    MOD_LD="$CC"
    SHLIB_CC="$CC -fPIC"
    SHLIB_LD="$CC -shared -nostartfiles"
    AC_DEFINE(MODULES_OK)dnl
  ;;
  Lynx)
    NEED_DL=0
    DEFAULT_MAKE=static
    SHLIB_STRIP=strip
  ;;
  QNX)
    NEED_DL=0
    DEFAULT_MAKE=static
    SHLIB_LD="ld -shared"
    SHLIB_STRIP=strip
  ;;
  OSF1)
    case "`echo $egg_cv_var_system_release | cut -d . -f 1`" in
      V*)
        # FIXME: we should check this in a seperate test
        # Digital OSF uses an ancient version of gawk
        if test "$AWK" = "gawk"
        then
          AWK=awk
        fi
        MOD_CC=cc
        MOD_LD=cc
        SHLIB_CC=cc
        SHLIB_LD="ld -shared -expect_unresolved \"'*'\""
        SHLIB_STRIP=touch
        AC_DEFINE(MODULES_OK)dnl
      ;;
      1.0|1.1|1.2)
        SHLIB_LD="ld -R -export $@:"
        AC_DEFINE(MODULES_OK)dnl
        AC_DEFINE(OSF1_HACKS)dnl
      ;;
      1.*)
        SHLIB_CC="$CC -fpic"
        SHLIB_LD="ld -shared"
        AC_DEFINE(MODULES_OK)dnl
        AC_DEFINE(OSF1_HACKS)dnl
      ;;
      *)
        NEED_DL=0
        DEFAULT_MAKE=static
      ;;
    esac
    AC_DEFINE(STOP_UAC)dnl
  ;;
  SunOS)
    if test "`echo $egg_cv_var_system_release | cut -d . -f 1`" = "5"
    then
      # Solaris
      if test -n "$GCC"
      then
        SHLIB_CC="$CC -fPIC"
        SHLIB_LD="$CC"
      else
        SHLIB_CC="$CC -KPIC"
        SHLIB_LD="$CC -G -z text"
      fi
    else
      # SunOS 4
      SUNOS=yes
      SHLIB_LD=ld
      SHLIB_CC="$CC -PIC"
      AC_DEFINE(DLOPEN_1)dnl
    fi
    AC_DEFINE(MODULES_OK)dnl
  ;;
  *BSD)
    # FreeBSD/OpenBSD/NetBSD
    SHLIB_CC="$CC -fPIC"
    SHLIB_LD="ld -Bshareable -x"
    AC_DEFINE(MODULES_OK)dnl
  ;;
  *)
    AC_MSG_CHECKING(if system is Mach based)
    if test -r /mach
    then
      AC_MSG_RESULT(yes)
      NEED_DL=0
      DEFAULT_MAKE=static
      AC_DEFINE(BORGCUBES)dnl
    else
      AC_MSG_RESULT(no)
      AC_MSG_CHECKING(if system is QNX)
      if test -r /cmds
      then
        AC_MSG_RESULT(yes)
        SHLIB_STRIP=touch
        NEED_DL=0
        DEFAULT_MAKE=static
      else
        AC_MSG_RESULT(no)
        AC_MSG_RESULT(Something unknown!)
        AC_MSG_RESULT([If you get dynamic modules to work, be sure to let the devel team know HOW :)])
        NEED_DL=0
        DEFAULT_MAKE=static
      fi
    fi
  ;;
esac
AC_SUBST(MOD_LD)dnl
AC_SUBST(MOD_CC)dnl
AC_SUBST(MOD_STRIP)dnl
AC_SUBST(SHLIB_LD)dnl
AC_SUBST(SHLIB_CC)dnl
AC_SUBST(SHLIB_STRIP)dnl
AC_SUBST(DEFAULT_MAKE)dnl
AC_SUBST(MOD_EXT)dnl
AC_DEFINE_UNQUOTED(EGG_MOD_EXT, "$MOD_EXT")dnl
])dnl


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
  AC_CHECK_LIB(dl, dlopen)
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
  if test "$SUNOS" = "yes"
  then
    # For suns without yp or something like that
    AC_CHECK_LIB(dl, main)
  else
    if test "$HPUX" = "yes"
    then
      AC_CHECK_LIB(dld, shl_load)
    fi
  fi
fi
])dnl


dnl  EGG_CHECK_FUNC_VSPRINTF()
dnl
AC_DEFUN(EGG_CHECK_FUNC_VSPRINTF, [dnl
AC_CHECK_FUNCS(vsprintf)
if test "$ac_cv_func_vsprintf" = "no"
then
  cat << 'EOF' >&2
configure: error:

  Your system does not have the sprintf/vsprintf libraries.
  These are required to compile almost anything.  Sorry.

EOF
  exit 1
fi
])dnl


dnl  EGG_HEADER_STDC()
dnl
AC_DEFUN(EGG_HEADER_STDC, [dnl
if test "$ac_cv_header_stdc" = "no"
then
  cat << 'EOF' >&2
configure: error:

  Your system must support ANSI C Header files.

EOF
  exit 1
fi
])dnl


dnl  EGG_CHECK_LIBSAFE_SSCANF()
dnl
AC_DEFUN(EGG_CHECK_LIBSAFE_SSCANF, [dnl
AC_CACHE_CHECK(for broken libsafe sscanf, egg_cv_var_libsafe_sscanf, [dnl
  AC_TRY_RUN([
#include <stdio.h>

int main()
{
  char *src = "0x001,guppyism\n";
  char dst[10];
  int idx;
  if (sscanf(src, "0x%x,%10c", &idx, dst) == 1)
    exit(1);
  return 0;
}
], egg_cv_var_libsafe_sscanf="no", egg_cv_var_libsafe_sscanf="yes",
egg_cv_var_libsafe_sscanf="no")
])
if test "$egg_cv_var_libsafe_sscanf" = "yes"
then
  AC_DEFINE(LIBSAFE_HACKS)dnl
fi
])dnl


dnl  EGG_CYGWIN()
dnl
dnl  Check for Cygwin support.
AC_DEFUN(EGG_CYGWIN, [dnl
AC_CYGWIN
if test "$ac_cv_cygwin" = "yes"
then
  AC_DEFINE(CYGWIN_HACKS)dnl
fi
])dnl


dnl  EGG_EXEEXT()
dnl
dnl  Test for executable suffix and define eggdrop's executable name
dnl  accordingly.
AC_DEFUN(EGG_EXEEXT, [dnl
EGGEXEC=eggdrop
AC_EXEEXT
if test ! "${EXEEXT-x}" = "x"
then
  EGGEXEC="eggdrop$EXEEXT"
fi
AC_SUBST(EGGEXEC)dnl
])dnl


dnl  EGG_TCL_ARG_WITH()
dnl
AC_DEFUN(EGG_TCL_ARG_WITH, [dnl
# oohh new configure --variables for those with multiple tcl libs
AC_ARG_WITH(tcllib, [  --with-tcllib=PATH      full path to tcl library], tcllibname="$withval")
AC_ARG_WITH(tclinc, [  --with-tclinc=PATH      full path to tcl header], tclincname="$withval")

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
])dnl


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
])dnl


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
])dnl


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
])dnl


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
])dnl


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
])dnl


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
AC_SUBST(TCLLIB)dnl
AC_SUBST(TCLLIBFN)dnl
])dnl


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
AC_SUBST(TCLINC)dnl
AC_SUBST(TCLINCFN)dnl
])dnl


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
])dnl


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
])dnl


dnl  EGG_TCL_CHECK_PRE75()
dnl
AC_DEFUN(EGG_TCL_CHECK_PRE75, [dnl
# Are we using a pre 7.5 Tcl version ?
TCL_VER_PRE75=`echo $egg_cv_var_tcl_version | $AWK '{split([$]1, i, "."); if (((i[[1]] == 7) && (i[[2]] < 5)) || (i[[1]] < 7)) print "yes"; else print "no"}'`
if test "$TCL_VER_PRE75" = "yes"
then
  AC_DEFINE(HAVE_PRE7_5_TCL)dnl
fi
])dnl


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
])dnl


dnl  EGG_TCL_CHECK_FREE()
dnl
AC_DEFUN(EGG_TCL_CHECK_FREE, [dnl
if test "$egg_tcl_changed" = "yes"
then
  EGG_CACHE_UNSET(egg_cv_var_tcl_free)
fi

# Check for Tcl_Free()
AC_CHECK_LIB($TCL_TEST_LIB, Tcl_Free, egg_cv_var_tcl_free="yes", egg_cv_var_tcl_free="no", $TCL_TEST_OTHERLIBS)

if test "$egg_cv_var_tcl_free" = "yes"
then
  AC_DEFINE(HAVE_TCL_FREE)dnl
fi
])dnl


dnl  EGG_TCL_ENABLE_THREADS()
dnl
AC_DEFUN(EGG_TCL_ENABLE_THREADS, [dnl
AC_ARG_ENABLE(tcl-threads,
[  --disable-tcl-threads   Disable threaded tcl support if detected. (Ignore this
                          option unless you know what you are doing)],
enable_tcl_threads="$enableval",
enable_tcl_threads=yes)
])dnl


dnl  EGG_TCL_CHECK_THREADS()
dnl
AC_DEFUN(EGG_TCL_CHECK_THREADS, [dnl
if test "$egg_tcl_changed" = "yes"
then
  EGG_CACHE_UNSET(egg_cv_var_tcl_threaded)
fi

# Check for TclpFinalizeThreadData()
AC_CHECK_LIB($TCL_TEST_LIB, TclpFinalizeThreadData, egg_cv_var_tcl_threaded="yes", egg_cv_var_tcl_threaded="no", $TCL_TEST_OTHERLIBS)

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
    AC_DEFINE(HAVE_TCL_THREADS)dnl
  fi

  # Add pthread library to $LIBS if we need it
  if test ! "${ac_cv_lib_pthread-x}" = "x"
  then
    LIBS="$ac_cv_lib_pthread $LIBS"
  fi
fi
])dnl


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

  # Set default make as static for unshared Tcl library
  if test ! "$DEFAULT_MAKE" = "static"
  then
    cat << 'EOF' >&2
configure: warning:

  Your Tcl library is not a shared lib.
  configure will now set default make type to static...

EOF
    DEFAULT_MAKE=static
    AC_SUBST(DEFAULT_MAKE)dnl
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
AC_SUBST(TCL_REQS)dnl
AC_SUBST(TCL_LIBS)dnl
])dnl


dnl  EGG_FUNC_DLOPEN()
dnl
AC_DEFUN(EGG_FUNC_DLOPEN, [dnl
if test "$NEED_DL" = 1 && test "$ac_cv_func_dlopen" = "no"
then
  if test "$LINUX" = "yes"
  then
    cat << 'EOF' >&2
configure: warning:

  Since you are on a Linux system, this has a known problem...
  I know a kludge for it,
EOF

    if test -r "/lib/libdl.so.1"
    then
      cat << 'EOF' >&2
  and you seem to have it, so we'll do that...

EOF
      AC_DEFINE(HAVE_DLOPEN)dnl
      LIBS="/lib/libdl.so.1 $LIBS"
    else
      cat << 'EOF' >&2
  which you DON'T seem to have... doh!
  perhaps you may still have the stuff lying around somewhere
  if you work out where it is, add it to your XLIBS= lines
  and #define HAVE_DLOPEN in config.h

  we'll proceed on anyway, but you probably won't be able
  to 'make eggdrop' but you might be able to make the
  static bot (I'll default your make to this version).

EOF
      DEFAULT_MAKE=static
    fi
  else
    cat << 'EOF' >&2
configure: warning:

  You don't seem to have libdl anywhere I can find it, this will
  prevent you from doing dynamic modules, I'll set your default
  make to static linking.

EOF
    DEFAULT_MAKE=static
  fi
fi
])dnl


dnl  EGG_SUBST_MOD_UPDIR()
dnl
dnl  Since module's Makefiles aren't generated by configure, some
dnl  paths in src/mod/Makefile.in take care of them. For correct
dnl  path "calculation", we need to keep absolute paths in mind
dnl  (which don't need a "../" pre-pended).
AC_DEFUN(EGG_SUBST_MOD_UPDIR, [dnl
case "$srcdir" in
  [[\\/]]* | ?:[[\\/]]*)
    MOD_UPDIR=""
  ;;
  *)
    MOD_UPDIR="../"
  ;;
esac
AC_SUBST(MOD_UPDIR)dnl
])dnl


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
rm -f conftest.out], [$3])dnl
])dnl


dnl  EGG_TCL_LUSH()
dnl
AC_DEFUN(EGG_TCL_LUSH, [dnl
    EGG_REPLACE_IF_CHANGED(lush.h, [
cat > conftest.out << EGGEOF
/* Ignore me but do not erase me.  I am a kludge. */

#include "$egg_tclinc/$egg_tclincfn"
EGGEOF], [
    egg_tclinc="$TCLINC"
    egg_tclincfn="$TCLINCFN"])dnl
])dnl


dnl  EGG_CATCH_MAKEFILE_REBUILD()
dnl
AC_DEFUN(EGG_CATCH_MAKEFILE_REBUILD, [dnl
  AC_OUTPUT_COMMANDS([
if test -f .modules
then
  $ac_given_srcdir/misc/modconfig --top_srcdir="$ac_given_srcdir" Makefile
fi])
])dnl


dnl  EGG_SAVE_PARAMETERS()
dnl
AC_DEFUN(EGG_SAVE_PARAMETERS, [dnl
  dnl  Normally, we shouldn't use this level as it's not intended for this
  dnl  type of code, but there's no other way to run it before the main
  dnl  parameter loop in configure.
  ifdef([AC_DIVERSION_NOTICE], [AC_DIVERT_PUSH(AC_DIVERSION_NOTICE)],
                              [AC_DIVERT_PUSH(NOTICE)])dnl
  egg_ac_parameters="[$]*"
  AC_DIVERT_POP()dnl to NORMAL
  AC_SUBST(egg_ac_parameters)dnl
])dnl

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
dnl


dnl  EGG_ENABLE_IPV6()
dnl
AC_DEFUN(EGG_ENABLE_IPV6, [dnl
AC_MSG_CHECKING(whether you enabled IPv6 support)
AC_ARG_ENABLE(ipv6,
[  --enable-ipv6           Enable IPV6 support.
  --disable-ipv6          Disable IPV6 support. ],
[ ac_cv_ipv6=$enableval
  AC_MSG_RESULT($ac_cv_ipv6)
], [
  if test "x$egg_cv_ipv6_supported" = "xyes"; then
    ac_cv_ipv6='yes'
  else
    ac_cv_ipv6='no'
  fi
  AC_MSG_RESULT((default) $ac_cv_ipv6)
])
if test "$ac_cv_ipv6" = "yes" ; then
        AC_DEFINE(IPV6)
fi
])dnl


dnl EGG_TYPE_SOCKLEN_T
dnl
AC_DEFUN(EGG_TYPE_SOCKLEN_T, [dnl
AC_CACHE_CHECK(for socklen_t, egg_cv_var_socklen_t, [
AC_TRY_COMPILE([#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>
], [
socklen_t x;
x = 0;
], egg_cv_var_socklen_t="yes", egg_cv_var_socklen_t="no")
])
if test "$egg_cv_var_socklen_t" = "no"
then
  AC_DEFINE(socklen_t, unsigned)dnl
fi
])dnl


dnl EGG_INADDR_LOOPBACK
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
if test "$adns_cv_decl_inaddrloopback" = yes; then
 AC_MSG_RESULT(found)
else
 AC_MSG_RESULT([not in standard headers, urgh...])
 AC_CHECK_HEADER(rpc/types.h,[
  AC_DEFINE(HAVEUSE_RPCTYPES_H)
 ],[
  AC_MSG_ERROR([cannot find INADDR_LOOPBACK or rpc/types.h])
 ])
fi
])dnl

dnl EGG_IPV6_SUPPORTED
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
if test "$egg_cv_ipv6_supported" = yes; then
 AC_MSG_RESULT(yes)
else
 AC_MSG_RESULT(no)
fi
])dnl

dnl EGG_DEFINE_VERSION_NUM
dnl
AC_DEFUN(EGG_DEFINE_VERSION_NUM, [dnl
egg_version_num=`echo $VERSION | $AWK 'BEGIN {FS = "."} {printf("%d%02d%02d00", [$]1, [$]2, [$]3)}'`
AC_DEFINE_UNQUOTED(VERSION_NUM, $egg_version_num)
])dnl

dnl EGG_GNU_GETTEXT
dnl
AC_DEFUN(EGG_GNU_GETTEXT, [dnl
AC_MSG_CHECKING(for avaible translations)
ALL_LINGUAS=""
cd po   
for LOC in `ls *.po 2> /dev/null`; do
  ALL_LINGUAS="$ALL_LINGUAS `echo $LOC | awk 'BEGIN {FS = "."} {printf("%s", [$]1)}'`"
done
cd ..
AC_MSG_RESULT($ALL_LINGUAS)

AM_GNU_GETTEXT
])dnl

#serial AM1

dnl From Bruno Haible.

AC_DEFUN([AM_LANGINFO_CODESET],
[
  AC_CACHE_CHECK([for nl_langinfo and CODESET], am_cv_langinfo_codeset,
    [AC_TRY_LINK([#include <langinfo.h>],
      [char* cs = nl_langinfo(CODESET);],
      am_cv_langinfo_codeset=yes,
      am_cv_langinfo_codeset=no)
    ])
  if test $am_cv_langinfo_codeset = yes; then
    AC_DEFINE(HAVE_LANGINFO_CODESET, 1,
      [Define if you have <langinfo.h> and nl_langinfo(CODESET).])
  fi
])
# Macro to add for using GNU gettext.
# Ulrich Drepper <drepper@cygnus.com>, 1995.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU General Public
# License but which still want to provide support for the GNU gettext
# functionality.
# Please note that the actual code of GNU gettext is covered by the GNU
# General Public License and is *not* in the public domain.

# serial 10

dnl Usage: AM_WITH_NLS([TOOLSYMBOL], [NEEDSYMBOL], [LIBDIR]).
dnl If TOOLSYMBOL is specified and is 'use-libtool', then a libtool library
dnl    $(top_builddir)/intl/libintl.la will be created (shared and/or static,
dnl    depending on --{enable,disable}-{shared,static} and on the presence of
dnl    AM-DISABLE-SHARED). Otherwise, a static library
dnl    $(top_builddir)/intl/libintl.a will be created.
dnl If NEEDSYMBOL is specified and is 'need-ngettext', then GNU gettext
dnl    implementations (in libc or libintl) without the ngettext() function
dnl    will be ignored.
dnl LIBDIR is used to find the intl libraries.  If empty,
dnl    the value `$(top_builddir)/intl/' is used.
dnl
dnl The result of the configuration is one of three cases:
dnl 1) GNU gettext, as included in the intl subdirectory, will be compiled
dnl    and used.
dnl    Catalog format: GNU --> install in $(datadir)
dnl    Catalog extension: .mo after installation, .gmo in source tree
dnl 2) GNU gettext has been found in the system's C library.
dnl    Catalog format: GNU --> install in $(datadir)
dnl    Catalog extension: .mo after installation, .gmo in source tree
dnl 3) No internationalization, always use English msgid.
dnl    Catalog format: none
dnl    Catalog extension: none
dnl The use of .gmo is historical (it was needed to avoid overwriting the
dnl GNU format catalogs when building on a platform with an X/Open gettext),
dnl but we keep it in order not to force irrelevant filename changes on the
dnl maintainers.
dnl
AC_DEFUN([AM_WITH_NLS],
  [AC_MSG_CHECKING([whether NLS is requested])
    dnl Default is enabled NLS
    AC_ARG_ENABLE(nls,
      [  --disable-nls           do not use Native Language Support],
      USE_NLS=$enableval, USE_NLS=yes)
    AC_MSG_RESULT($USE_NLS)
    AC_SUBST(USE_NLS)

    BUILD_INCLUDED_LIBINTL=no
    USE_INCLUDED_LIBINTL=no
    INTLLIBS=

    dnl If we use NLS figure out what method
    if test "$USE_NLS" = "yes"; then
      AC_DEFINE(ENABLE_NLS, 1,
        [Define to 1 if translation of program messages to the user's native language
   is requested.])
      AC_MSG_CHECKING([whether included gettext is requested])
      AC_ARG_WITH(included-gettext,
        [  --with-included-gettext use the GNU gettext library included here],
        nls_cv_force_use_gnu_gettext=$withval,
        nls_cv_force_use_gnu_gettext=no)
      AC_MSG_RESULT($nls_cv_force_use_gnu_gettext)

      nls_cv_use_gnu_gettext="$nls_cv_force_use_gnu_gettext"
      if test "$nls_cv_force_use_gnu_gettext" != "yes"; then
        dnl User does not insist on using GNU NLS library.  Figure out what
        dnl to use.  If GNU gettext is available we use this.  Else we have
        dnl to fall back to GNU NLS library.
	CATOBJEXT=NONE

        dnl Add a version number to the cache macros.
        define(gt_cv_func_gnugettext_libc, [gt_cv_func_gnugettext]ifelse([$2], need-ngettext, 2, 1)[_libc])
        define(gt_cv_func_gnugettext_libintl, [gt_cv_func_gnugettext]ifelse([$2], need-ngettext, 2, 1)[_libintl])

	AC_CHECK_HEADER(libintl.h,
	  [AC_CACHE_CHECK([for GNU gettext in libc], gt_cv_func_gnugettext_libc,
	    [AC_TRY_LINK([#include <libintl.h>
extern int _nl_msg_cat_cntr;],
	       [bindtextdomain ("", "");
return (int) gettext ("")]ifelse([$2], need-ngettext, [ + (int) ngettext ("", "", 0)], [])[ + _nl_msg_cat_cntr],
	       gt_cv_func_gnugettext_libc=yes,
	       gt_cv_func_gnugettext_libc=no)])

	   if test "$gt_cv_func_gnugettext_libc" != "yes"; then
	     AC_CACHE_CHECK([for GNU gettext in libintl],
	       gt_cv_func_gnugettext_libintl,
	       [gt_save_LIBS="$LIBS"
		LIBS="$LIBS -lintl $LIBICONV"
		AC_TRY_LINK([#include <libintl.h>
extern int _nl_msg_cat_cntr;],
		  [bindtextdomain ("", "");
return (int) gettext ("")]ifelse([$2], need-ngettext, [ + (int) ngettext ("", "", 0)], [])[ + _nl_msg_cat_cntr],
		  gt_cv_func_gnugettext_libintl=yes,
		  gt_cv_func_gnugettext_libintl=no)
		LIBS="$gt_save_LIBS"])
	   fi

	   dnl If an already present or preinstalled GNU gettext() is found,
	   dnl use it.  But if this macro is used in GNU gettext, and GNU
	   dnl gettext is already preinstalled in libintl, we update this
	   dnl libintl.  (Cf. the install rule in intl/Makefile.in.)
	   if test "$gt_cv_func_gnugettext_libc" = "yes" \
	      || { test "$gt_cv_func_gnugettext_libintl" = "yes" \
		   && test "$PACKAGE" != gettext; }; then
	     AC_DEFINE(HAVE_GETTEXT, 1,
               [Define if the GNU gettext() function is already present or preinstalled.])

	     if test "$gt_cv_func_gnugettext_libintl" = "yes"; then
	       dnl If iconv() is in a separate libiconv library, then anyone
	       dnl linking with libintl{.a,.so} also needs to link with
	       dnl libiconv.
	       INTLLIBS="-lintl $LIBICONV"
	     fi

	     gt_save_LIBS="$LIBS"
	     LIBS="$LIBS $INTLLIBS"
	     AC_CHECK_FUNCS(dcgettext)
	     LIBS="$gt_save_LIBS"

	     dnl Search for GNU msgfmt in the PATH.
	     AM_PATH_PROG_WITH_TEST(MSGFMT, msgfmt,
	       [$ac_dir/$ac_word --statistics /dev/null >/dev/null 2>&1], :)
	     AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)

	     dnl Search for GNU xgettext in the PATH.
	     AM_PATH_PROG_WITH_TEST(XGETTEXT, xgettext,
	       [$ac_dir/$ac_word --omit-header /dev/null >/dev/null 2>&1], :)

	     CATOBJEXT=.gmo
	   fi
	])

        if test "$CATOBJEXT" = "NONE"; then
	  dnl GNU gettext is not found in the C library.
	  dnl Fall back on GNU gettext library.
	  nls_cv_use_gnu_gettext=yes
        fi
      fi

      if test "$nls_cv_use_gnu_gettext" = "yes"; then
        dnl Mark actions used to generate GNU NLS library.
        INTLOBJS="\$(GETTOBJS)"
        AM_PATH_PROG_WITH_TEST(MSGFMT, msgfmt,
	  [$ac_dir/$ac_word --statistics /dev/null >/dev/null 2>&1], :)
        AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)
        AM_PATH_PROG_WITH_TEST(XGETTEXT, xgettext,
	  [$ac_dir/$ac_word --omit-header /dev/null >/dev/null 2>&1], :)
        AC_SUBST(MSGFMT)
	BUILD_INCLUDED_LIBINTL=yes
	USE_INCLUDED_LIBINTL=yes
        CATOBJEXT=.gmo
	INTLLIBS="ifelse([$3],[],\$(top_builddir)/intl,[$3])/libintl.ifelse([$1], use-libtool, [l], [])a $LIBICONV"
	LIBS=`echo " $LIBS " | sed -e 's/ -lintl / /' -e 's/^ //' -e 's/ $//'`
      fi

      dnl This could go away some day; the PATH_PROG_WITH_TEST already does it.
      dnl Test whether we really found GNU msgfmt.
      if test "$GMSGFMT" != ":"; then
	dnl If it is no GNU msgfmt we define it as : so that the
	dnl Makefiles still can work.
	if $GMSGFMT --statistics /dev/null >/dev/null 2>&1; then
	  : ;
	else
	  AC_MSG_RESULT(
	    [found msgfmt program is not GNU msgfmt; ignore it])
	  GMSGFMT=":"
	fi
      fi

      dnl This could go away some day; the PATH_PROG_WITH_TEST already does it.
      dnl Test whether we really found GNU xgettext.
      if test "$XGETTEXT" != ":"; then
	dnl If it is no GNU xgettext we define it as : so that the
	dnl Makefiles still can work.
	if $XGETTEXT --omit-header /dev/null >/dev/null 2>&1; then
	  : ;
	else
	  AC_MSG_RESULT(
	    [found xgettext program is not GNU xgettext; ignore it])
	  XGETTEXT=":"
	fi
      fi

      dnl We need to process the po/ directory.
      POSUB=po
    fi
    AC_OUTPUT_COMMANDS(
     [for ac_file in $CONFIG_FILES; do
        # Support "outfile[:infile[:infile...]]"
        case "$ac_file" in
          *:*) ac_file=`echo "$ac_file"|sed 's%:.*%%'` ;;
        esac
        # PO directories have a Makefile.in generated from Makefile.in.in.
        case "$ac_file" in */Makefile.in)
          # Adjust a relative srcdir.
          ac_dir=`echo "$ac_file"|sed 's%/[^/][^/]*$%%'`
          ac_dir_suffix="/`echo "$ac_dir"|sed 's%^\./%%'`"
          ac_dots=`echo "$ac_dir_suffix"|sed 's%/[^/]*%../%g'`
          # In autoconf-2.13 it is called $ac_given_srcdir.
          # In autoconf-2.50 it is called $srcdir.
          test -n "$ac_given_srcdir" || ac_given_srcdir="$srcdir"
          case "$ac_given_srcdir" in
            .)  top_srcdir=`echo $ac_dots|sed 's%/$%%'` ;;
            /*) top_srcdir="$ac_given_srcdir" ;;
            *)  top_srcdir="$ac_dots$ac_given_srcdir" ;;
          esac
          if test -f "$ac_given_srcdir/$ac_dir/POTFILES.in"; then
            rm -f "$ac_dir/POTFILES"
            test -n "$as_me" && echo "$as_me: creating $ac_dir/POTFILES" || echo "creating $ac_dir/POTFILES"
            sed -e "/^#/d" -e "/^[ 	]*\$/d" -e "s,.*,     $top_srcdir/& \\\\," -e "\$s/\(.*\) \\\\/\1/" < "$ac_given_srcdir/$ac_dir/POTFILES.in" > "$ac_dir/POTFILES"
            test -n "$as_me" && echo "$as_me: creating $ac_dir/Makefile" || echo "creating $ac_dir/Makefile"
            sed -e "/POTFILES =/r $ac_dir/POTFILES" "$ac_dir/Makefile.in" > "$ac_dir/Makefile"
          fi
          ;;
        esac
      done])


    dnl If this is used in GNU gettext we have to set BUILD_INCLUDED_LIBINTL
    dnl to 'yes' because some of the testsuite requires it.
    if test "$PACKAGE" = gettext; then
      BUILD_INCLUDED_LIBINTL=yes
    fi

    dnl intl/plural.c is generated from intl/plural.y. It requires bison,
    dnl because plural.y uses bison specific features. It requires at least
    dnl bison-1.26 because earlier versions generate a plural.c that doesn't
    dnl compile.
    dnl bison is only needed for the maintainer (who touches plural.y). But in
    dnl order to avoid separate Makefiles or --enable-maintainer-mode, we put
    dnl the rule in general Makefile. Now, some people carelessly touch the
    dnl files or have a broken "make" program, hence the plural.c rule will
    dnl sometimes fire. To avoid an error, defines BISON to ":" if it is not
    dnl present or too old.
    AC_CHECK_PROGS([INTLBISON], [bison])
    if test -z "$INTLBISON"; then
      ac_verc_fail=yes
    else
      dnl Found it, now check the version.
      AC_MSG_CHECKING([version of bison])
changequote(<<,>>)dnl
      ac_prog_version=`$INTLBISON --version 2>&1 | sed -n 's/^.*GNU Bison .* \([0-9]*\.[0-9.]*\).*$/\1/p'`
      case $ac_prog_version in
        '') ac_prog_version="v. ?.??, bad"; ac_verc_fail=yes;;
        1.2[6-9]* | 1.[3-9][0-9]* | [2-9].*)
changequote([,])dnl
           ac_prog_version="$ac_prog_version, ok"; ac_verc_fail=no;;
        *) ac_prog_version="$ac_prog_version, bad"; ac_verc_fail=yes;;
      esac
      AC_MSG_RESULT([$ac_prog_version])
    fi
    if test $ac_verc_fail = yes; then
      INTLBISON=:
    fi

    dnl These rules are solely for the distribution goal.  While doing this
    dnl we only have to keep exactly one list of the available catalogs
    dnl in configure.in.
    for lang in $ALL_LINGUAS; do
      GMOFILES="$GMOFILES $lang.gmo"
      POFILES="$POFILES $lang.po"
    done

    dnl Make all variables we use known to autoconf.
    AC_SUBST(BUILD_INCLUDED_LIBINTL)
    AC_SUBST(USE_INCLUDED_LIBINTL)
    AC_SUBST(CATALOGS)
    AC_SUBST(CATOBJEXT)
    AC_SUBST(GMOFILES)
    AC_SUBST(INTLLIBS)
    AC_SUBST(INTLOBJS)
    AC_SUBST(POFILES)
    AC_SUBST(POSUB)

    dnl For backward compatibility. Some configure.ins may be using this.
    nls_cv_header_intl=
    nls_cv_header_libgt=

    dnl For backward compatibility. Some Makefiles may be using this.
    DATADIRNAME=share
    AC_SUBST(DATADIRNAME)

    dnl For backward compatibility. Some Makefiles may be using this.
    INSTOBJEXT=.mo
    AC_SUBST(INSTOBJEXT)

    dnl For backward compatibility. Some Makefiles may be using this.
    GENCAT=gencat
    AC_SUBST(GENCAT)
  ])

dnl Usage: Just like AM_WITH_NLS, which see.
AC_DEFUN([AM_GNU_GETTEXT],
  [AC_REQUIRE([AC_PROG_MAKE_SET])dnl
   AC_REQUIRE([AC_PROG_CC])dnl
   AC_REQUIRE([AC_CANONICAL_HOST])dnl
   AC_REQUIRE([AC_PROG_RANLIB])dnl
   AC_REQUIRE([AC_ISC_POSIX])dnl
   AC_REQUIRE([AC_HEADER_STDC])dnl
   AC_REQUIRE([AC_C_CONST])dnl
   AC_REQUIRE([AC_C_INLINE])dnl
   AC_REQUIRE([AC_TYPE_OFF_T])dnl
   AC_REQUIRE([AC_TYPE_SIZE_T])dnl
   AC_REQUIRE([AC_FUNC_ALLOCA])dnl
   AC_REQUIRE([AC_FUNC_MMAP])dnl
   AC_REQUIRE([jm_GLIBC21])dnl

   AC_CHECK_HEADERS([argz.h limits.h locale.h nl_types.h malloc.h stddef.h \
stdlib.h string.h unistd.h sys/param.h])
   AC_CHECK_FUNCS([feof_unlocked fgets_unlocked getcwd getegid geteuid \
getgid getuid mempcpy munmap putenv setenv setlocale stpcpy strchr strcasecmp \
strdup strtoul tsearch __argz_count __argz_stringify __argz_next])

   AM_ICONV
   AM_LANGINFO_CODESET
   AM_LC_MESSAGES
   AM_WITH_NLS([$1],[$2],[$3])

   if test "x$CATOBJEXT" != "x"; then
     if test "x$ALL_LINGUAS" = "x"; then
       LINGUAS=
     else
       AC_MSG_CHECKING(for catalogs to be installed)
       NEW_LINGUAS=
       for presentlang in $ALL_LINGUAS; do
         useit=no
         for desiredlang in ${LINGUAS-$ALL_LINGUAS}; do
           # Use the presentlang catalog if desiredlang is
           #   a. equal to presentlang, or
           #   b. a variant of presentlang (because in this case,
           #      presentlang can be used as a fallback for messages
           #      which are not translated in the desiredlang catalog).
           case "$desiredlang" in
             "$presentlang"*) useit=yes;;
           esac
         done
         if test $useit = yes; then
           NEW_LINGUAS="$NEW_LINGUAS $presentlang"
         fi
       done
       LINGUAS=$NEW_LINGUAS
       AC_MSG_RESULT($LINGUAS)
     fi

     dnl Construct list of names of catalog files to be constructed.
     if test -n "$LINGUAS"; then
       for lang in $LINGUAS; do CATALOGS="$CATALOGS $lang$CATOBJEXT"; done
     fi
   fi

   dnl If the AC_CONFIG_AUX_DIR macro for autoconf is used we possibly
   dnl find the mkinstalldirs script in another subdir but $(top_srcdir).
   dnl Try to locate is.
   MKINSTALLDIRS=
   if test -n "$ac_aux_dir"; then
     MKINSTALLDIRS="$ac_aux_dir/mkinstalldirs"
   fi
   if test -z "$MKINSTALLDIRS"; then
     MKINSTALLDIRS="\$(top_srcdir)/mkinstalldirs"
   fi
   AC_SUBST(MKINSTALLDIRS)

   dnl Enable libtool support if the surrounding package wishes it.
   INTL_LIBTOOL_SUFFIX_PREFIX=ifelse([$1], use-libtool, [l], [])
   AC_SUBST(INTL_LIBTOOL_SUFFIX_PREFIX)
  ])
#serial 2

# Test for the GNU C Library, version 2.1 or newer.
# From Bruno Haible.

AC_DEFUN([jm_GLIBC21],
  [
    AC_CACHE_CHECK(whether we are using the GNU C Library 2.1 or newer,
      ac_cv_gnu_library_2_1,
      [AC_EGREP_CPP([Lucky GNU user],
	[
#include <features.h>
#ifdef __GNU_LIBRARY__
 #if (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 1) || (__GLIBC__ > 2)
  Lucky GNU user
 #endif
#endif
	],
	ac_cv_gnu_library_2_1=yes,
	ac_cv_gnu_library_2_1=no)
      ]
    )
    AC_SUBST(GLIBC21)
    GLIBC21="$ac_cv_gnu_library_2_1"
  ]
)
#serial AM2

dnl From Bruno Haible.

AC_DEFUN([AM_ICONV],
[
  dnl Some systems have iconv in libc, some have it in libiconv (OSF/1 and
  dnl those with the standalone portable GNU libiconv installed).

  AC_ARG_WITH([libiconv-prefix],
[  --with-libiconv-prefix=DIR  search for libiconv in DIR/include and DIR/lib], [
    for dir in `echo "$withval" | tr : ' '`; do
      if test -d $dir/include; then CPPFLAGS="$CPPFLAGS -I$dir/include"; fi
      if test -d $dir/lib; then LDFLAGS="$LDFLAGS -L$dir/lib"; fi
    done
   ])

  AC_CACHE_CHECK(for iconv, am_cv_func_iconv, [
    am_cv_func_iconv="no, consider installing GNU libiconv"
    am_cv_lib_iconv=no
    AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
      [iconv_t cd = iconv_open("","");
       iconv(cd,NULL,NULL,NULL,NULL);
       iconv_close(cd);],
      am_cv_func_iconv=yes)
    if test "$am_cv_func_iconv" != yes; then
      am_save_LIBS="$LIBS"
      LIBS="$LIBS -liconv"
      AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
        [iconv_t cd = iconv_open("","");
         iconv(cd,NULL,NULL,NULL,NULL);
         iconv_close(cd);],
        am_cv_lib_iconv=yes
        am_cv_func_iconv=yes)
      LIBS="$am_save_LIBS"
    fi
  ])
  if test "$am_cv_func_iconv" = yes; then
    AC_DEFINE(HAVE_ICONV, 1, [Define if you have the iconv() function.])
    AC_MSG_CHECKING([for iconv declaration])
    AC_CACHE_VAL(am_cv_proto_iconv, [
      AC_TRY_COMPILE([
#include <stdlib.h>
#include <iconv.h>
extern
#ifdef __cplusplus
"C"
#endif
#if defined(__STDC__) || defined(__cplusplus)
size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
#else
size_t iconv();
#endif
], [], am_cv_proto_iconv_arg1="", am_cv_proto_iconv_arg1="const")
      am_cv_proto_iconv="extern size_t iconv (iconv_t cd, $am_cv_proto_iconv_arg1 char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);"])
    am_cv_proto_iconv=`echo "[$]am_cv_proto_iconv" | tr -s ' ' | sed -e 's/( /(/'`
    AC_MSG_RESULT([$]{ac_t:-
         }[$]am_cv_proto_iconv)
    AC_DEFINE_UNQUOTED(ICONV_CONST, $am_cv_proto_iconv_arg1,
      [Define as const if the declaration of iconv() needs const.])
  fi
  LIBICONV=
  if test "$am_cv_lib_iconv" = yes; then
    LIBICONV="-liconv"
  fi
  AC_SUBST(LIBICONV)
])
#serial 1
# This test replaces the one in autoconf.
# Currently this macro should have the same name as the autoconf macro
# because gettext's gettext.m4 (distributed in the automake package)
# still uses it.  Otherwise, the use in gettext.m4 makes autoheader
# give these diagnostics:
#   configure.in:556: AC_TRY_COMPILE was called before AC_ISC_POSIX
#   configure.in:556: AC_TRY_RUN was called before AC_ISC_POSIX

undefine([AC_ISC_POSIX])

AC_DEFUN([AC_ISC_POSIX],
  [
    dnl This test replaces the obsolescent AC_ISC_POSIX kludge.
    AC_CHECK_LIB(cposix, strerror, [LIBS="$LIBS -lcposix"])
  ]
)
# Check whether LC_MESSAGES is available in <locale.h>.
# Ulrich Drepper <drepper@cygnus.com>, 1995.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU General Public
# License but which still want to provide support for the GNU gettext
# functionality.
# Please note that the actual code of GNU gettext is covered by the GNU
# General Public License and is *not* in the public domain.

# serial 2

AC_DEFUN([AM_LC_MESSAGES],
  [if test $ac_cv_header_locale_h = yes; then
    AC_CACHE_CHECK([for LC_MESSAGES], am_cv_val_LC_MESSAGES,
      [AC_TRY_LINK([#include <locale.h>], [return LC_MESSAGES],
       am_cv_val_LC_MESSAGES=yes, am_cv_val_LC_MESSAGES=no)])
    if test $am_cv_val_LC_MESSAGES = yes; then
      AC_DEFINE(HAVE_LC_MESSAGES, 1,
        [Define if your <locale.h> file defines LC_MESSAGES.])
    fi
  fi])
# Search path for a program which passes the given test.
# Ulrich Drepper <drepper@cygnus.com>, 1996.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU General Public
# License but which still want to provide support for the GNU gettext
# functionality.
# Please note that the actual code of GNU gettext is covered by the GNU
# General Public License and is *not* in the public domain.

# serial 2

dnl AM_PATH_PROG_WITH_TEST(VARIABLE, PROG-TO-CHECK-FOR,
dnl   TEST-PERFORMED-ON-FOUND_PROGRAM [, VALUE-IF-NOT-FOUND [, PATH]])
AC_DEFUN([AM_PATH_PROG_WITH_TEST],
[# Extract the first word of "$2", so it can be a program name with args.
set dummy $2; ac_word=[$]2
AC_MSG_CHECKING([for $ac_word])
AC_CACHE_VAL(ac_cv_path_$1,
[case "[$]$1" in
  /*)
  ac_cv_path_$1="[$]$1" # Let the user override the test with a path.
  ;;
  *)
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:"
  for ac_dir in ifelse([$5], , $PATH, [$5]); do
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/$ac_word; then
      if [$3]; then
	ac_cv_path_$1="$ac_dir/$ac_word"
	break
      fi
    fi
  done
  IFS="$ac_save_ifs"
dnl If no 4th arg is given, leave the cache variable unset,
dnl so AC_PATH_PROGS will keep looking.
ifelse([$4], , , [  test -z "[$]ac_cv_path_$1" && ac_cv_path_$1="$4"
])dnl
  ;;
esac])dnl
$1="$ac_cv_path_$1"
if test ifelse([$4], , [-n "[$]$1"], ["[$]$1" != "$4"]); then
  AC_MSG_RESULT([$]$1)
else
  AC_MSG_RESULT(no)
fi
AC_SUBST($1)dnl
])

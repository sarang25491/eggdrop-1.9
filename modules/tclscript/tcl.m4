#------------------------------------------------------------------------
# SC_PATH_TCLCONFIG --
#
#	Locate the tclConfig.sh file and perform a sanity check on
#	the Tcl compile flags
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-tcl=...
#
#	Defines the following vars:
#		TCL_BIN_DIR	Full path to the directory containing
#				the tclConfig.sh file
#------------------------------------------------------------------------

AC_DEFUN([SC_PATH_TCLCONFIG], [
    #
    # Ok, lets find the tcl configuration
    # First, look for one uninstalled.
    # the alternative search directory is invoked by --with-tcl
    #

    if test x"${no_tcl}" = x ; then
	# we reset no_tcl in case something fails here
	no_tcl=true
	AC_ARG_WITH(tcl, [  --with-tcl              directory containing tcl configuration (tclConfig.sh)], with_tclconfig=${withval})
	AC_MSG_CHECKING([for Tcl configuration])
	AC_CACHE_VAL(ac_cv_c_tclconfig,[

	    # First check to see if --with-tcl was specified.
	    if test x"${with_tclconfig}" != x ; then
		if test -f "${with_tclconfig}/tclConfig.sh" ; then
		    ac_cv_c_tclconfig=`(cd ${with_tclconfig}; pwd)`
		else
		    AC_MSG_ERROR([${with_tclconfig} directory doesn't contain tclConfig.sh])
		fi
	    fi

	    # then check for a private Tcl installation
	    if test x"${ac_cv_c_tclconfig}" = x ; then
		for i in \
			../tcl \
			`ls -dr ../tcl[[8-9]].[[0-9]]* 2>/dev/null` \
			../../tcl \
			`ls -dr ../../tcl[[8-9]].[[0-9]]* 2>/dev/null` \
			../../../tcl \
			`ls -dr ../../../tcl[[8-9]].[[0-9]]* 2>/dev/null` ; do
		    if test -f "$i/unix/tclConfig.sh" ; then
			ac_cv_c_tclconfig=`(cd $i/unix; pwd)`
			break
		    fi
		done
	    fi

	    # check in a few common install locations
	    if test x"${ac_cv_c_tclconfig}" = x ; then
		for i in `ls -d ${libdir} 2>/dev/null` \
			`ls -d /usr/local/lib 2>/dev/null` \
			`ls -dr /usr/local/lib/tcl?.? 2>/dev/null` \
			`ls -d /usr/contrib/lib 2>/dev/null` \
			`ls -d /usr/lib 2>/dev/null` \
			; do
		    if test -f "$i/tclConfig.sh" ; then
			ac_cv_c_tclconfig=`(cd $i; pwd)`
			break
		    fi
		done
	    fi

	    # check in a few other private locations
	    if test x"${ac_cv_c_tclconfig}" = x ; then
		for i in \
			${srcdir}/../tcl \
			`ls -dr ${srcdir}/../tcl[[8-9]].[[0-9]]* 2>/dev/null` ; do
		    if test -f "$i/unix/tclConfig.sh" ; then
		    ac_cv_c_tclconfig=`(cd $i/unix; pwd)`
		    break
		fi
		done
	    fi
	])

	if test x"${ac_cv_c_tclconfig}" = x ; then
	    TCL_BIN_DIR="# no Tcl configs found"
	    AC_MSG_WARN(Can't find Tcl configuration definitions)
	else
	    no_tcl=
	    TCL_BIN_DIR=${ac_cv_c_tclconfig}
	    AC_MSG_RESULT(found $TCL_BIN_DIR/tclConfig.sh)
	fi
    fi
])

#------------------------------------------------------------------------
# SC_PATH_TKCONFIG --
#
#	Locate the tkConfig.sh file
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-tk=...
#
#	Defines the following vars:
#		TK_BIN_DIR	Full path to the directory containing
#				the tkConfig.sh file
#------------------------------------------------------------------------

AC_DEFUN([SC_PATH_TKCONFIG], [
    #
    # Ok, lets find the tk configuration
    # First, look for one uninstalled.
    # the alternative search directory is invoked by --with-tk
    #

    if test x"${no_tk}" = x ; then
	# we reset no_tk in case something fails here
	no_tk=true
	AC_ARG_WITH(tk, [  --with-tk               directory containing tk configuration (tkConfig.sh)], with_tkconfig=${withval})
	AC_MSG_CHECKING([for Tk configuration])
	AC_CACHE_VAL(ac_cv_c_tkconfig,[

	    # First check to see if --with-tkconfig was specified.
	    if test x"${with_tkconfig}" != x ; then
		if test -f "${with_tkconfig}/tkConfig.sh" ; then
		    ac_cv_c_tkconfig=`(cd ${with_tkconfig}; pwd)`
		else
		    AC_MSG_ERROR([${with_tkconfig} directory doesn't contain tkConfig.sh])
		fi
	    fi

	    # then check for a private Tk library
	    if test x"${ac_cv_c_tkconfig}" = x ; then
		for i in \
			../tk \
			`ls -dr ../tk[[8-9]].[[0-9]]* 2>/dev/null` \
			../../tk \
			`ls -dr ../../tk[[8-9]].[[0-9]]* 2>/dev/null` \
			../../../tk \
			`ls -dr ../../../tk[[8-9]].[[0-9]]* 2>/dev/null` ; do
		    if test -f "$i/unix/tkConfig.sh" ; then
			ac_cv_c_tkconfig=`(cd $i/unix; pwd)`
			break
		    fi
		done
	    fi
	    # check in a few common install locations
	    if test x"${ac_cv_c_tkconfig}" = x ; then
		for i in `ls -d ${libdir} 2>/dev/null` \
			`ls -d /usr/local/lib 2>/dev/null` \
			`ls -d /usr/contrib/lib 2>/dev/null` \
			`ls -d /usr/lib 2>/dev/null` \
			; do
		    if test -f "$i/tkConfig.sh" ; then
			ac_cv_c_tkconfig=`(cd $i; pwd)`
			break
		    fi
		done
	    fi
	    # check in a few other private locations
	    if test x"${ac_cv_c_tkconfig}" = x ; then
		for i in \
			${srcdir}/../tk \
			`ls -dr ${srcdir}/../tk[[8-9]].[[0-9]]* 2>/dev/null` ; do
		    if test -f "$i/unix/tkConfig.sh" ; then
			ac_cv_c_tkconfig=`(cd $i/unix; pwd)`
			break
		    fi
		done
	    fi
	])
	if test x"${ac_cv_c_tkconfig}" = x ; then
	    TK_BIN_DIR="# no Tk configs found"
	    AC_MSG_WARN(Can't find Tk configuration definitions)
	    exit 0
	else
	    no_tk=
	    TK_BIN_DIR=${ac_cv_c_tkconfig}
	    AC_MSG_RESULT(found $TK_BIN_DIR/tkConfig.sh)
	fi
    fi

])

#------------------------------------------------------------------------
# SC_LOAD_TCLCONFIG --
#
#	Load the tclConfig.sh file
#
# Arguments:
#	
#	Requires the following vars to be set:
#		TCL_BIN_DIR
#
# Results:
#
#	Subst the following vars:
#		TCL_BIN_DIR
#		TCL_SRC_DIR
#		TCL_LIB_FILE
#
#------------------------------------------------------------------------

AC_DEFUN([SC_LOAD_TCLCONFIG], [
    AC_MSG_CHECKING([for existence of $TCL_BIN_DIR/tclConfig.sh])

    if test -f "$TCL_BIN_DIR/tclConfig.sh" ; then
        AC_MSG_RESULT([loading])
	. $TCL_BIN_DIR/tclConfig.sh
    else
        AC_MSG_RESULT([file not found])
    fi

    #
    # If the TCL_BIN_DIR is the build directory (not the install directory),
    # then set the common variable name to the value of the build variables.
    # For example, the variable TCL_LIB_SPEC will be set to the value
    # of TCL_BUILD_LIB_SPEC. An extension should make use of TCL_LIB_SPEC
    # instead of TCL_BUILD_LIB_SPEC since it will work with both an
    # installed and uninstalled version of Tcl.
    #

    if test -f $TCL_BIN_DIR/Makefile ; then
        TCL_LIB_SPEC=${TCL_BUILD_LIB_SPEC}
        TCL_STUB_LIB_SPEC=${TCL_BUILD_STUB_LIB_SPEC}
        TCL_STUB_LIB_PATH=${TCL_BUILD_STUB_LIB_PATH}
    fi

    #
    # eval is required to do the TCL_DBGX substitution
    #

    eval "TCL_LIB_FILE=\"${TCL_LIB_FILE}\""
    eval "TCL_LIB_FLAG=\"${TCL_LIB_FLAG}\""
    eval "TCL_LIB_SPEC=\"${TCL_LIB_SPEC}\""

    eval "TCL_STUB_LIB_FILE=\"${TCL_STUB_LIB_FILE}\""
    eval "TCL_STUB_LIB_FLAG=\"${TCL_STUB_LIB_FLAG}\""
    eval "TCL_STUB_LIB_SPEC=\"${TCL_STUB_LIB_SPEC}\""

    AC_SUBST(TCL_VERSION)
    AC_SUBST(TCL_BIN_DIR)
    AC_SUBST(TCL_SRC_DIR)

    AC_SUBST(TCL_LIB_FILE)
    AC_SUBST(TCL_LIB_FLAG)
    AC_SUBST(TCL_LIB_SPEC)

    AC_SUBST(TCL_STUB_LIB_FILE)
    AC_SUBST(TCL_STUB_LIB_FLAG)
    AC_SUBST(TCL_STUB_LIB_SPEC)
])

#------------------------------------------------------------------------
# SC_LOAD_TKCONFIG --
#
#	Load the tkConfig.sh file
#
# Arguments:
#	
#	Requires the following vars to be set:
#		TK_BIN_DIR
#
# Results:
#
#	Sets the following vars that should be in tkConfig.sh:
#		TK_BIN_DIR
#------------------------------------------------------------------------

AC_DEFUN([SC_LOAD_TKCONFIG], [
    AC_MSG_CHECKING([for existence of $TK_BIN_DIR/tkConfig.sh])

    if test -f "$TK_BIN_DIR/tkConfig.sh" ; then
        AC_MSG_RESULT([loading])
	. $TK_BIN_DIR/tkConfig.sh
    else
        AC_MSG_RESULT([could not find $TK_BIN_DIR/tkConfig.sh])
    fi

    AC_SUBST(TK_VERSION)
    AC_SUBST(TK_BIN_DIR)
    AC_SUBST(TK_SRC_DIR)
    AC_SUBST(TK_LIB_FILE)
])

#------------------------------------------------------------------------
# SC_ENABLE_SHARED --
#
#	Allows the building of shared libraries
#
# Arguments:
#	none
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-shared=yes|no
#
#	Defines the following vars:
#		STATIC_BUILD	Used for building import/export libraries
#				on Windows.
#
#	Sets the following vars:
#		SHARED_BUILD	Value of 1 or 0
#------------------------------------------------------------------------

AC_DEFUN([SC_ENABLE_SHARED], [
    AC_MSG_CHECKING([how to build libraries])
    AC_ARG_ENABLE(shared,
	[  --enable-shared         build and link with shared libraries [--enable-shared]],
	[tcl_ok=$enableval], [tcl_ok=yes])

    if test "${enable_shared+set}" = set; then
	enableval="$enable_shared"
	tcl_ok=$enableval
    else
	tcl_ok=yes
    fi

    if test "$tcl_ok" = "yes" ; then
	AC_MSG_RESULT([shared])
	SHARED_BUILD=1
    else
	AC_MSG_RESULT([static])
	SHARED_BUILD=0
	AC_DEFINE(STATIC_BUILD)
    fi
])

#------------------------------------------------------------------------
# SC_ENABLE_FRAMEWORK --
#
#	Allows the building of shared libraries into frameworks
#
# Arguments:
#	none
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-framework=yes|no
#
#	Sets the following vars:
#		FRAMEWORK_BUILD	Value of 1 or 0
#------------------------------------------------------------------------

AC_DEFUN([SC_ENABLE_FRAMEWORK], [
    AC_MSG_CHECKING([how to package libraries])
    AC_ARG_ENABLE(framework,
	[  --enable-framework      package shared libraries in frameworks [--disable-framework]],
	[tcl_ok=$enableval], [tcl_ok=no])

    if test "${enable_framework+set}" = set; then
	enableval="$enable_framework"
	tcl_ok=$enableval
    else
	tcl_ok=no
    fi

    if test "$tcl_ok" = "yes" ; then
	AC_MSG_RESULT([framework])
	FRAMEWORK_BUILD=1
	if test "${SHARED_BUILD}" = "0" ; then
	    AC_MSG_WARN("Frameworks can only be built if --enable-shared is yes")
	    FRAMEWORK_BUILD=0
	fi
    else
	AC_MSG_RESULT([standard shared library])
	FRAMEWORK_BUILD=0
    fi
])

#------------------------------------------------------------------------
# SC_ENABLE_THREADS --
#
#	Specify if thread support should be enabled
#
# Arguments:
#	none
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-threads
#
#	Sets the following vars:
#		THREADS_LIBS	Thread library(s)
#
#	Defines the following vars:
#		TCL_THREADS
#		_REENTRANT
#		_THREAD_SAFE
#
#------------------------------------------------------------------------

AC_DEFUN([SC_ENABLE_THREADS], [
    AC_MSG_CHECKING(for building with threads)
    AC_ARG_ENABLE(threads, [  --enable-threads        build with threads],
	[tcl_ok=$enableval], [tcl_ok=no])

    if test "$tcl_ok" = "yes"; then
	AC_MSG_RESULT(yes)
	TCL_THREADS=1
	AC_DEFINE(TCL_THREADS)
	# USE_THREAD_ALLOC tells us to try the special thread-based
	# allocator that significantly reduces lock contention
	AC_DEFINE(USE_THREAD_ALLOC)
	AC_DEFINE(_REENTRANT)
	AC_DEFINE(_THREAD_SAFE)
	AC_CHECK_LIB(pthread,pthread_mutex_init,tcl_ok=yes,tcl_ok=no)
	if test "$tcl_ok" = "no"; then
	    # Check a little harder for __pthread_mutex_init in the same
	    # library, as some systems hide it there until pthread.h is
	    # defined.  We could alternatively do an AC_TRY_COMPILE with
	    # pthread.h, but that will work with libpthread really doesn't
	    # exist, like AIX 4.2.  [Bug: 4359]
	    AC_CHECK_LIB(pthread,__pthread_mutex_init,tcl_ok=yes,tcl_ok=no)
	fi

	if test "$tcl_ok" = "yes"; then
	    # The space is needed
	    THREADS_LIBS=" -lpthread"
	else
	    AC_CHECK_LIB(pthreads,pthread_mutex_init,tcl_ok=yes,tcl_ok=no)
	    if test "$tcl_ok" = "yes"; then
		# The space is needed
		THREADS_LIBS=" -lpthreads"
	    else
		AC_CHECK_LIB(c,pthread_mutex_init,tcl_ok=yes,tcl_ok=no)
	    	if test "$tcl_ok" = "no"; then
		    AC_CHECK_LIB(c_r,pthread_mutex_init,tcl_ok=yes,tcl_ok=no)
		    if test "$tcl_ok" = "yes"; then
			# The space is needed
			THREADS_LIBS=" -pthread"
		    else
			TCL_THREADS=0
			AC_MSG_WARN("Don t know how to find pthread lib on your system - you must disable thread support or edit the LIBS in the Makefile...")
		    fi
	    	fi
	    fi
	fi

	# Does the pthread-implementation provide
	# 'pthread_attr_setstacksize' ?

	ac_saved_libs=$LIBS
	LIBS="$LIBS $THREADS_LIBS"
	AC_CHECK_FUNCS(pthread_attr_setstacksize)
	LIBS=$ac_saved_libs
	AC_CHECK_FUNCS(readdir_r)
    else
	TCL_THREADS=0
	AC_MSG_RESULT([no (default)])
    fi
    AC_SUBST(TCL_THREADS)
])

#------------------------------------------------------------------------
# SC_ENABLE_SYMBOLS --
#
#	Specify if debugging symbols should be used.
#	Memory (TCL_MEM_DEBUG) and compile (TCL_COMPILE_DEBUG) debugging
#	can also be enabled.
#
# Arguments:
#	none
#	
#	Requires the following vars to be set in the Makefile:
#		CFLAGS_DEBUG
#		CFLAGS_OPTIMIZE
#		LDFLAGS_DEBUG
#		LDFLAGS_OPTIMIZE
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-symbols
#
#	Defines the following vars:
#		CFLAGS_DEFAULT	Sets to $(CFLAGS_DEBUG) if true
#				Sets to $(CFLAGS_OPTIMIZE) if false
#		LDFLAGS_DEFAULT	Sets to $(LDFLAGS_DEBUG) if true
#				Sets to $(LDFLAGS_OPTIMIZE) if false
#		DBGX		Debug library extension
#
#------------------------------------------------------------------------

AC_DEFUN([SC_ENABLE_SYMBOLS], [
    AC_MSG_CHECKING([for build with symbols])
    AC_ARG_ENABLE(symbols, [  --enable-symbols        build with debugging symbols [--disable-symbols]],    [tcl_ok=$enableval], [tcl_ok=no])
# FIXME: Currently, LDFLAGS_DEFAULT is not used, it should work like CFLAGS_DEFAULT.
    if test "$tcl_ok" = "no"; then
	CFLAGS_DEFAULT='$(CFLAGS_OPTIMIZE)'
	LDFLAGS_DEFAULT='$(LDFLAGS_OPTIMIZE)'
	DBGX=""
	AC_MSG_RESULT([no])
    else
	CFLAGS_DEFAULT='$(CFLAGS_DEBUG)'
	LDFLAGS_DEFAULT='$(LDFLAGS_DEBUG)'
	DBGX=g
	if test "$tcl_ok" = "yes"; then
	    AC_MSG_RESULT([yes (standard debugging)])
	fi
    fi
    AC_SUBST(CFLAGS_DEFAULT)
    AC_SUBST(LDFLAGS_DEFAULT)

    if test "$tcl_ok" = "mem" -o "$tcl_ok" = "all"; then
	AC_DEFINE(TCL_MEM_DEBUG)
    fi

    if test "$tcl_ok" = "compile" -o "$tcl_ok" = "all"; then
	AC_DEFINE(TCL_COMPILE_DEBUG)
	AC_DEFINE(TCL_COMPILE_STATS)
    fi

    if test "$tcl_ok" != "yes" -a "$tcl_ok" != "no"; then
	if test "$tcl_ok" = "all"; then
	    AC_MSG_RESULT([enabled symbols mem compile debugging])
	else
	    AC_MSG_RESULT([enabled $tcl_ok debugging])
	fi
    fi
])

#------------------------------------------------------------------------
# SC_ENABLE_LANGINFO --
#
#	Allows use of modern nl_langinfo check for better l10n.
#	This is only relevant for Unix.
#
# Arguments:
#	none
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-langinfo=yes|no (default is yes)
#
#	Defines the following vars:
#		HAVE_LANGINFO	Triggers use of nl_langinfo if defined.
#
#------------------------------------------------------------------------

AC_DEFUN([SC_ENABLE_LANGINFO], [
    AC_ARG_ENABLE(langinfo,
	[  --enable-langinfo	  use nl_langinfo if possible to determine
			  encoding at startup, otherwise use old heuristic],
	[langinfo_ok=$enableval], [langinfo_ok=yes])

    HAVE_LANGINFO=0
    if test "$langinfo_ok" = "yes"; then
	if test "$langinfo_ok" = "yes"; then
	    AC_CHECK_HEADER(langinfo.h,[langinfo_ok=yes],[langinfo_ok=no])
	fi
    fi
    AC_MSG_CHECKING([whether to use nl_langinfo])
    if test "$langinfo_ok" = "yes"; then
	AC_TRY_COMPILE([#include <langinfo.h>],
		[nl_langinfo(CODESET);],[langinfo_ok=yes],[langinfo_ok=no])
	if test "$langinfo_ok" = "no"; then
	    langinfo_ok="no (could not compile with nl_langinfo)";
	fi
	if test "$langinfo_ok" = "yes"; then
	    AC_DEFINE(HAVE_LANGINFO)
	fi
    fi
    AC_MSG_RESULT([$langinfo_ok])
])

#--------------------------------------------------------------------
# SC_CONFIG_MANPAGES
#	
#	Decide whether to use symlinks for linking the manpages and
#	whether to compress the manpages after installation.
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--enable-man-symlinks
#		--enable-man-compression=PROG
#
#	Defines the following variable:
#
#	MKLINKS_FLAGS -		The apropriate flags for mkLinks
#				according to the user's selection.
#
#--------------------------------------------------------------------
AC_DEFUN([SC_CONFIG_MANPAGES], [

	AC_MSG_CHECKING([whether to use symlinks for manpages])
	AC_ARG_ENABLE(man-symlinks,
		[  --enable-man-symlinks   use symlinks for the manpages],
		test "$enableval" != "no" && MKLINKS_FLAGS="$MKLINKS_FLAGS --symlinks",
		enableval="no")
	AC_MSG_RESULT([$enableval])

	AC_MSG_CHECKING([compression for manpages])
	AC_ARG_ENABLE(man-compression,
		[  --enable-man-compression=PROG
                          compress the manpages with PROG],
		test "$enableval" = "yes" && echo && AC_MSG_ERROR([missing argument to --enable-man-compression])
		test "$enableval" != "no" && MKLINKS_FLAGS="$MKLINKS_FLAGS --compress $enableval",
		enableval="no")
	AC_MSG_RESULT([$enableval])

	AC_SUBST(MKLINKS_FLAGS)
])

#--------------------------------------------------------------------
# SC_SERIAL_PORT
#
#	Determine which interface to use to talk to the serial port.
#	Note that #include lines must begin in leftmost column for
#	some compilers to recognize them as preprocessor directives,
#	and some build environments have stdin not pointing at a
#	pseudo-terminal (usually /dev/null instead.)
#
# Arguments:
#	none
#	
# Results:
#
#	Defines only one of the following vars:
#		HAVE_SYS_MODEM_H
#		USE_TERMIOS
#		USE_TERMIO
#		USE_SGTTY
#
#--------------------------------------------------------------------

AC_DEFUN([SC_SERIAL_PORT], [
    AC_CHECK_HEADERS(sys/modem.h)
    AC_MSG_CHECKING([termios vs. termio vs. sgtty])
    AC_CACHE_VAL(tcl_cv_api_serial, [
    AC_TRY_RUN([
#include <termios.h>

int main() {
    struct termios t;
    if (tcgetattr(0, &t) == 0) {
	cfsetospeed(&t, 0);
	t.c_cflag |= PARENB | PARODD | CSIZE | CSTOPB;
	return 0;
    }
    return 1;
}], tcl_cv_api_serial=termios, tcl_cv_api_serial=no, tcl_cv_api_serial=no)
    if test $tcl_cv_api_serial = no ; then
	AC_TRY_RUN([
#include <termio.h>

int main() {
    struct termio t;
    if (ioctl(0, TCGETA, &t) == 0) {
	t.c_cflag |= CBAUD | PARENB | PARODD | CSIZE | CSTOPB;
	return 0;
    }
    return 1;
}], tcl_cv_api_serial=termio, tcl_cv_api_serial=no, tcl_cv_api_serial=no)
    fi
    if test $tcl_cv_api_serial = no ; then
	AC_TRY_RUN([
#include <sgtty.h>

int main() {
    struct sgttyb t;
    if (ioctl(0, TIOCGETP, &t) == 0) {
	t.sg_ospeed = 0;
	t.sg_flags |= ODDP | EVENP | RAW;
	return 0;
    }
    return 1;
}], tcl_cv_api_serial=sgtty, tcl_cv_api_serial=no, tcl_cv_api_serial=no)
    fi
    if test $tcl_cv_api_serial = no ; then
	AC_TRY_RUN([
#include <termios.h>
#include <errno.h>

int main() {
    struct termios t;
    if (tcgetattr(0, &t) == 0
	|| errno == ENOTTY || errno == ENXIO || errno == EINVAL) {
	cfsetospeed(&t, 0);
	t.c_cflag |= PARENB | PARODD | CSIZE | CSTOPB;
	return 0;
    }
    return 1;
}], tcl_cv_api_serial=termios, tcl_cv_api_serial=no, tcl_cv_api_serial=no)
    fi
    if test $tcl_cv_api_serial = no; then
	AC_TRY_RUN([
#include <termio.h>
#include <errno.h>

int main() {
    struct termio t;
    if (ioctl(0, TCGETA, &t) == 0
	|| errno == ENOTTY || errno == ENXIO || errno == EINVAL) {
	t.c_cflag |= CBAUD | PARENB | PARODD | CSIZE | CSTOPB;
	return 0;
    }
    return 1;
    }], tcl_cv_api_serial=termio, tcl_cv_api_serial=no, tcl_cv_api_serial=no)
    fi
    if test $tcl_cv_api_serial = no; then
	AC_TRY_RUN([
#include <sgtty.h>
#include <errno.h>

int main() {
    struct sgttyb t;
    if (ioctl(0, TIOCGETP, &t) == 0
	|| errno == ENOTTY || errno == ENXIO || errno == EINVAL) {
	t.sg_ospeed = 0;
	t.sg_flags |= ODDP | EVENP | RAW;
	return 0;
    }
    return 1;
}], tcl_cv_api_serial=sgtty, tcl_cv_api_serial=none, tcl_cv_api_serial=none)
    fi])
    case $tcl_cv_api_serial in
	termios) AC_DEFINE(USE_TERMIOS);;
	termio)  AC_DEFINE(USE_TERMIO);;
	sgtty)   AC_DEFINE(USE_SGTTY);;
    esac
    AC_MSG_RESULT($tcl_cv_api_serial)
])

#--------------------------------------------------------------------
# SC_MISSING_POSIX_HEADERS
#
#	Supply substitutes for missing POSIX header files.  Special
#	notes:
#	    - stdlib.h doesn't define strtol, strtoul, or
#	      strtod insome versions of SunOS
#	    - some versions of string.h don't declare procedures such
#	      as strstr
#
# Arguments:
#	none
#	
# Results:
#
#	Defines some of the following vars:
#		NO_DIRENT_H
#		NO_ERRNO_H
#		NO_VALUES_H
#		NO_LIMITS_H
#		NO_STDLIB_H
#		NO_STRING_H
#		NO_SYS_WAIT_H
#		NO_DLFCN_H
#		HAVE_UNISTD_H
#		HAVE_SYS_PARAM_H
#
#		HAVE_STRING_H ?
#
#--------------------------------------------------------------------

AC_DEFUN([SC_MISSING_POSIX_HEADERS], [
    AC_MSG_CHECKING(dirent.h)
    AC_TRY_LINK([#include <sys/types.h>
#include <dirent.h>], [
#ifndef _POSIX_SOURCE
#   ifdef __Lynx__
	/*
	 * Generate compilation error to make the test fail:  Lynx headers
	 * are only valid if really in the POSIX environment.
	 */

	missing_procedure();
#   endif
#endif
DIR *d;
struct dirent *entryPtr;
char *p;
d = opendir("foobar");
entryPtr = readdir(d);
p = entryPtr->d_name;
closedir(d);
], tcl_ok=yes, tcl_ok=no)

    if test $tcl_ok = no; then
	AC_DEFINE(NO_DIRENT_H)
    fi

    AC_MSG_RESULT($tcl_ok)
    AC_CHECK_HEADER(errno.h, , [AC_DEFINE(NO_ERRNO_H)])
    AC_CHECK_HEADER(float.h, , [AC_DEFINE(NO_FLOAT_H)])
    AC_CHECK_HEADER(values.h, , [AC_DEFINE(NO_VALUES_H)])
    AC_CHECK_HEADER(limits.h, , [AC_DEFINE(NO_LIMITS_H)])
    AC_CHECK_HEADER(stdlib.h, tcl_ok=1, tcl_ok=0)
    AC_EGREP_HEADER(strtol, stdlib.h, , tcl_ok=0)
    AC_EGREP_HEADER(strtoul, stdlib.h, , tcl_ok=0)
    AC_EGREP_HEADER(strtod, stdlib.h, , tcl_ok=0)
    if test $tcl_ok = 0; then
	AC_DEFINE(NO_STDLIB_H)
    fi
    AC_CHECK_HEADER(string.h, tcl_ok=1, tcl_ok=0)
    AC_EGREP_HEADER(strstr, string.h, , tcl_ok=0)
    AC_EGREP_HEADER(strerror, string.h, , tcl_ok=0)

    # See also memmove check below for a place where NO_STRING_H can be
    # set and why.

    if test $tcl_ok = 0; then
	AC_DEFINE(NO_STRING_H)
    fi

    AC_CHECK_HEADER(sys/wait.h, , [AC_DEFINE(NO_SYS_WAIT_H)])
    AC_CHECK_HEADER(dlfcn.h, , [AC_DEFINE(NO_DLFCN_H)])

    # OS/390 lacks sys/param.h (and doesn't need it, by chance).

    AC_HAVE_HEADERS(unistd.h sys/param.h)

])

#--------------------------------------------------------------------
# SC_PATH_X
#
#	Locate the X11 header files and the X11 library archive.  Try
#	the ac_path_x macro first, but if it doesn't find the X stuff
#	(e.g. because there's no xmkmf program) then check through
#	a list of possible directories.  Under some conditions the
#	autoconf macro will return an include directory that contains
#	no include files, so double-check its result just to be safe.
#
# Arguments:
#	none
#	
# Results:
#
#	Sets the the following vars:
#		XINCLUDES
#		XLIBSW
#
#--------------------------------------------------------------------

AC_DEFUN([SC_PATH_X], [
    AC_PATH_X
    not_really_there=""
    if test "$no_x" = ""; then
	if test "$x_includes" = ""; then
	    AC_TRY_CPP([#include <X11/XIntrinsic.h>], , not_really_there="yes")
	else
	    if test ! -r $x_includes/X11/Intrinsic.h; then
		not_really_there="yes"
	    fi
	fi
    fi
    if test "$no_x" = "yes" -o "$not_really_there" = "yes"; then
	AC_MSG_CHECKING(for X11 header files)
	found_xincludes="no"
	AC_TRY_CPP([#include <X11/Intrinsic.h>], found_xincludes="yes", found_xincludes="no")
	if test "$found_xincludes" = "no"; then
	    dirs="/usr/unsupported/include /usr/local/include /usr/X386/include /usr/X11R6/include /usr/X11R5/include /usr/include/X11R5 /usr/include/X11R4 /usr/openwin/include /usr/X11/include /usr/sww/include"
	    for i in $dirs ; do
		if test -r $i/X11/Intrinsic.h; then
		    AC_MSG_RESULT($i)
		    XINCLUDES=" -I$i"
		    found_xincludes="yes"
		    break
		fi
	    done
	fi
    else
	if test "$x_includes" != ""; then
	    XINCLUDES="-I$x_includes"
	    found_xincludes="yes"
	fi
    fi
    if test found_xincludes = "no"; then
	AC_MSG_RESULT(couldn't find any!)
    fi

    if test "$no_x" = yes; then
	AC_MSG_CHECKING(for X11 libraries)
	XLIBSW=nope
	dirs="/usr/unsupported/lib /usr/local/lib /usr/X386/lib /usr/X11R6/lib /usr/X11R5/lib /usr/lib/X11R5 /usr/lib/X11R4 /usr/openwin/lib /usr/X11/lib /usr/sww/X11/lib"
	for i in $dirs ; do
	    if test -r $i/libX11.a -o -r $i/libX11.so -o -r $i/libX11.sl; then
		AC_MSG_RESULT($i)
		XLIBSW="-L$i -lX11"
		x_libraries="$i"
		break
	    fi
	done
    else
	if test "$x_libraries" = ""; then
	    XLIBSW=-lX11
	else
	    XLIBSW="-L$x_libraries -lX11"
	fi
    fi
    if test "$XLIBSW" = nope ; then
	AC_CHECK_LIB(Xwindow, XCreateWindow, XLIBSW=-lXwindow)
    fi
    if test "$XLIBSW" = nope ; then
	AC_MSG_RESULT(couldn't find any!  Using -lX11.)
	XLIBSW=-lX11
    fi
])
#--------------------------------------------------------------------
# SC_BLOCKING_STYLE
#
#	The statements below check for systems where POSIX-style
#	non-blocking I/O (O_NONBLOCK) doesn't work or is unimplemented. 
#	On these systems (mostly older ones), use the old BSD-style
#	FIONBIO approach instead.
#
# Arguments:
#	none
#	
# Results:
#
#	Defines some of the following vars:
#		HAVE_SYS_IOCTL_H
#		HAVE_SYS_FILIO_H
#		USE_FIONBIO
#		O_NONBLOCK
#
#--------------------------------------------------------------------

AC_DEFUN([SC_BLOCKING_STYLE], [
    AC_CHECK_HEADERS(sys/ioctl.h)
    AC_CHECK_HEADERS(sys/filio.h)
    AC_MSG_CHECKING([FIONBIO vs. O_NONBLOCK for nonblocking I/O])
    if test -f /usr/lib/NextStep/software_version; then
	system=NEXTSTEP-`awk '/3/,/3/' /usr/lib/NextStep/software_version`
    else
	system=`uname -s`-`uname -r`
	if test "$?" -ne 0 ; then
	    system=unknown
	else
	    # Special check for weird MP-RAS system (uname returns weird
	    # results, and the version is kept in special file).
	
	    if test -r /etc/.relid -a "X`uname -n`" = "X`uname -s`" ; then
		system=MP-RAS-`awk '{print $3}' /etc/.relid'`
	    fi
	    if test "`uname -s`" = "AIX" ; then
		system=AIX-`uname -v`.`uname -r`
	    fi
	fi
    fi
    case $system in
	# There used to be code here to use FIONBIO under AIX.  However, it
	# was reported that FIONBIO doesn't work under AIX 3.2.5.  Since
	# using O_NONBLOCK seems fine under AIX 4.*, I removed the FIONBIO
	# code (JO, 5/31/97).

	OSF*)
	    AC_DEFINE(USE_FIONBIO)
	    AC_MSG_RESULT(FIONBIO)
	    ;;
	SunOS-4*)
	    AC_DEFINE(USE_FIONBIO)
	    AC_MSG_RESULT(FIONBIO)
	    ;;
	ULTRIX-4.*)
	    AC_DEFINE(USE_FIONBIO)
	    AC_MSG_RESULT(FIONBIO)
	    ;;
	*)
	    AC_MSG_RESULT(O_NONBLOCK)
	    ;;
    esac
])

#--------------------------------------------------------------------
# SC_TIME_HANLDER
#
#	Checks how the system deals with time.h, what time structures
#	are used on the system, and what fields the structures have.
#
# Arguments:
#	none
#	
# Results:
#
#	Defines some of the following vars:
#		USE_DELTA_FOR_TZ
#		HAVE_TM_GMTOFF
#		HAVE_TM_TZADJ
#		HAVE_TIMEZONE_VAR
#
#--------------------------------------------------------------------

AC_DEFUN([SC_TIME_HANDLER], [
    AC_CHECK_HEADERS(sys/time.h)
    AC_HEADER_TIME
    AC_STRUCT_TIMEZONE

    AC_CHECK_FUNCS(gmtime_r localtime_r)

    AC_MSG_CHECKING([tm_tzadj in struct tm])
    AC_CACHE_VAL(tcl_cv_member_tm_tzadj,
	AC_TRY_COMPILE([#include <time.h>], [struct tm tm; tm.tm_tzadj;],
	    tcl_cv_member_tm_tzadj=yes, tcl_cv_member_tm_tzadj=no))
    AC_MSG_RESULT($tcl_cv_member_tm_tzadj)
    if test $tcl_cv_member_tm_tzadj = yes ; then
	AC_DEFINE(HAVE_TM_TZADJ)
    fi

    AC_MSG_CHECKING([tm_gmtoff in struct tm])
    AC_CACHE_VAL(tcl_cv_member_tm_gmtoff,
	AC_TRY_COMPILE([#include <time.h>], [struct tm tm; tm.tm_gmtoff;],
	    tcl_cv_member_tm_gmtoff=yes, tcl_cv_member_tm_gmtoff=no))
    AC_MSG_RESULT($tcl_cv_member_tm_gmtoff)
    if test $tcl_cv_member_tm_gmtoff = yes ; then
	AC_DEFINE(HAVE_TM_GMTOFF)
    fi

    #
    # Its important to include time.h in this check, as some systems
    # (like convex) have timezone functions, etc.
    #
    AC_MSG_CHECKING([long timezone variable])
    AC_CACHE_VAL(tcl_cv_var_timezone,
	AC_TRY_COMPILE([#include <time.h>],
	    [extern long timezone;
	    timezone += 1;
	    exit (0);],
	    tcl_cv_timezone_long=yes, tcl_cv_timezone_long=no))
    AC_MSG_RESULT($tcl_cv_timezone_long)
    if test $tcl_cv_timezone_long = yes ; then
	AC_DEFINE(HAVE_TIMEZONE_VAR)
    else
	#
	# On some systems (eg IRIX 6.2), timezone is a time_t and not a long.
	#
	AC_MSG_CHECKING([time_t timezone variable])
	AC_CACHE_VAL(tcl_cv_timezone_time,
	    AC_TRY_COMPILE([#include <time.h>],
		[extern time_t timezone;
		timezone += 1;
		exit (0);],
		tcl_cv_timezone_time=yes, tcl_cv_timezone_time=no))
	AC_MSG_RESULT($tcl_cv_timezone_time)
	if test $tcl_cv_timezone_time = yes ; then
	    AC_DEFINE(HAVE_TIMEZONE_VAR)
	fi
    fi
])

#--------------------------------------------------------------------
# SC_TCL_LINK_LIBS
#
#	Search for the libraries needed to link the Tcl shell.
#	Things like the math library (-lm) and socket stuff (-lsocket vs.
#	-lnsl) are dealt with here.
#
# Arguments:
#	Requires the following vars to be set in the Makefile:
#		DL_LIBS
#		LIBS
#		MATH_LIBS
#	
# Results:
#
#	Subst's the following var:
#		TCL_LIBS
#		MATH_LIBS
#
#	Might append to the following vars:
#		LIBS
#
#	Might define the following vars:
#		HAVE_NET_ERRNO_H
#
#--------------------------------------------------------------------

AC_DEFUN([SC_TCL_LINK_LIBS], [
    #--------------------------------------------------------------------
    # On a few very rare systems, all of the libm.a stuff is
    # already in libc.a.  Set compiler flags accordingly.
    # Also, Linux requires the "ieee" library for math to work
    # right (and it must appear before "-lm").
    #--------------------------------------------------------------------

    AC_CHECK_FUNC(sin, MATH_LIBS="", MATH_LIBS="-lm")
    AC_CHECK_LIB(ieee, main, [MATH_LIBS="-lieee $MATH_LIBS"])

    #--------------------------------------------------------------------
    # Interactive UNIX requires -linet instead of -lsocket, plus it
    # needs net/errno.h to define the socket-related error codes.
    #--------------------------------------------------------------------

    AC_CHECK_LIB(inet, main, [LIBS="$LIBS -linet"])
    AC_CHECK_HEADER(net/errno.h, [AC_DEFINE(HAVE_NET_ERRNO_H)])

    #--------------------------------------------------------------------
    #	Check for the existence of the -lsocket and -lnsl libraries.
    #	The order here is important, so that they end up in the right
    #	order in the command line generated by make.  Here are some
    #	special considerations:
    #	1. Use "connect" and "accept" to check for -lsocket, and
    #	   "gethostbyname" to check for -lnsl.
    #	2. Use each function name only once:  can't redo a check because
    #	   autoconf caches the results of the last check and won't redo it.
    #	3. Use -lnsl and -lsocket only if they supply procedures that
    #	   aren't already present in the normal libraries.  This is because
    #	   IRIX 5.2 has libraries, but they aren't needed and they're
    #	   bogus:  they goof up name resolution if used.
    #	4. On some SVR4 systems, can't use -lsocket without -lnsl too.
    #	   To get around this problem, check for both libraries together
    #	   if -lsocket doesn't work by itself.
    #--------------------------------------------------------------------

    tcl_checkBoth=0
    AC_CHECK_FUNC(connect, tcl_checkSocket=0, tcl_checkSocket=1)
    if test "$tcl_checkSocket" = 1; then
	AC_CHECK_FUNC(setsockopt, , [AC_CHECK_LIB(socket, setsockopt,
	    LIBS="$LIBS -lsocket", tcl_checkBoth=1)])
    fi
    if test "$tcl_checkBoth" = 1; then
	tk_oldLibs=$LIBS
	LIBS="$LIBS -lsocket -lnsl"
	AC_CHECK_FUNC(accept, tcl_checkNsl=0, [LIBS=$tk_oldLibs])
    fi
    AC_CHECK_FUNC(gethostbyname, , [AC_CHECK_LIB(nsl, gethostbyname,
	    [LIBS="$LIBS -lnsl"])])
    
    # Don't perform the eval of the libraries here because DL_LIBS
    # won't be set until we call SC_CONFIG_CFLAGS

    TCL_LIBS='${DL_LIBS} ${LIBS} ${MATH_LIBS}'
    AC_SUBST(TCL_LIBS)
    AC_SUBST(MATH_LIBS)
])

#--------------------------------------------------------------------
# SC_TCL_EARLY_FLAGS
#
#	Check for what flags are needed to be passed so the correct OS
#	features are available.
#
# Arguments:
#	None
#	
# Results:
#
#	Might define the following vars:
#		_ISOC99_SOURCE
#		_LARGEFILE64_SOURCE
#
#--------------------------------------------------------------------

AC_DEFUN([SC_TCL_EARLY_FLAG],[
    AC_CACHE_VAL([tcl_cv_flag_]translit($1,[A-Z],[a-z]),
	AC_TRY_COMPILE([$2], $3, [tcl_cv_flag_]translit($1,[A-Z],[a-z])=no,
	    AC_TRY_COMPILE([[#define ]$1[ 1
]$2], $3,
		[tcl_cv_flag_]translit($1,[A-Z],[a-z])=yes,
		[tcl_cv_flag_]translit($1,[A-Z],[a-z])=no)))
    if test ["x${tcl_cv_flag_]translit($1,[A-Z],[a-z])[}" = "xyes"] ; then
	AC_DEFINE($1)
	tcl_flags="$tcl_flags $1"
    fi])

AC_DEFUN([SC_TCL_EARLY_FLAGS],[
    AC_MSG_CHECKING([for required early compiler flags])
    tcl_flags=""
    SC_TCL_EARLY_FLAG(_ISOC99_SOURCE,[#include <stdlib.h>],
	[char *p = (char *)strtoll; char *q = (char *)strtoull;])
    SC_TCL_EARLY_FLAG(_LARGEFILE64_SOURCE,[#include <sys/stat.h>],
	[struct stat64 buf; int i = stat64("/", &buf);])
    if test "x${tcl_flags}" = "x" ; then
	AC_MSG_RESULT(none)
    else
	AC_MSG_RESULT(${tcl_flags})
    fi])

#--------------------------------------------------------------------
# SC_TCL_64BIT_FLAGS
#
#	Check for what is defined in the way of 64-bit features.
#
# Arguments:
#	None
#	
# Results:
#
#	Might define the following vars:
#		TCL_WIDE_INT_IS_LONG
#		TCL_WIDE_INT_TYPE
#		HAVE_STRUCT_DIRENT64
#		HAVE_STRUCT_STAT64
#		HAVE_TYPE_OFF64_T
#
#--------------------------------------------------------------------

AC_DEFUN([SC_TCL_64BIT_FLAGS], [
    AC_MSG_CHECKING([for 64-bit integer type])
    AC_CACHE_VAL(tcl_cv_type_64bit,[
	tcl_cv_type_64bit=none
	# See if the compiler knows natively about __int64
	AC_TRY_COMPILE(,[__int64 value = (__int64) 0;],
	    tcl_type_64bit=__int64, tcl_type_64bit="long long")
	# See if we should use long anyway  Note that we substitute in the
	# type that is our current guess for a 64-bit type inside this check
	# program, so it should be modified only carefully...
	AC_TRY_RUN([#include <unistd.h>
	    int main() {exit(!(sizeof(]${tcl_type_64bit}[) > sizeof(long)));}
	    ], tcl_cv_type_64bit=${tcl_type_64bit},:,:)])
    if test "${tcl_cv_type_64bit}" = none ; then
	AC_DEFINE(TCL_WIDE_INT_IS_LONG)
	AC_MSG_RESULT(using long)
    else
	AC_DEFINE_UNQUOTED(TCL_WIDE_INT_TYPE,${tcl_cv_type_64bit})
	AC_MSG_RESULT(${tcl_cv_type_64bit})

	# Now check for auxiliary declarations
	AC_MSG_CHECKING([for struct dirent64])
	AC_CACHE_VAL(tcl_cv_struct_dirent64,[
	    AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/dirent.h>],[struct dirent64 p;],
		tcl_cv_struct_dirent64=yes,tcl_cv_struct_dirent64=no)])
	if test "x${tcl_cv_struct_dirent64}" = "xyes" ; then
	    AC_DEFINE(HAVE_STRUCT_DIRENT64)
	fi
	AC_MSG_RESULT(${tcl_cv_struct_dirent64})

	AC_MSG_CHECKING([for struct stat64])
	AC_CACHE_VAL(tcl_cv_struct_stat64,[
	    AC_TRY_COMPILE([#include <sys/stat.h>],[struct stat64 p;
],
		tcl_cv_struct_stat64=yes,tcl_cv_struct_stat64=no)])
	if test "x${tcl_cv_struct_stat64}" = "xyes" ; then
	    AC_DEFINE(HAVE_STRUCT_STAT64)
	fi
	AC_MSG_RESULT(${tcl_cv_struct_stat64})

	AC_MSG_CHECKING([for off64_t])
	AC_CACHE_VAL(tcl_cv_type_off64_t,[
	    AC_TRY_COMPILE([#include <sys/types.h>],[off64_t offset;
],
		tcl_cv_type_off64_t=yes,tcl_cv_type_off64_t=no)])
	if test "x${tcl_cv_type_off64_t}" = "xyes" ; then
	    AC_DEFINE(HAVE_TYPE_OFF64_T)
	fi
	AC_MSG_RESULT(${tcl_cv_type_off64_t})
    fi])

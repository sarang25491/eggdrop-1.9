#ifndef _EGG_CONFIG_H
#define _EGG_CONFIG_H
@TOP@
/* 
 * acconfig.h
 *   template file autoheader uses when building config.h.in
 * 
 * $Id: acconfig.h,v 1.11 2001/10/10 01:20:08 ite Exp $
 */

/* Define package name */
#undef PACKAGE

/* Define version string */
#undef VERSION

/* Define version number */
#undef VERSION_NUM

/* Define locale's resources path */
#undef LOCALEDIR

/* Define if running on hpux that supports dynamic linking  */
#undef HPUX_HACKS

/* Define if running on hpux 10.x  */
#undef HPUX10_HACKS

/* Define if running on OSF/1 platform.  */
#undef STOP_UAC

/* Define if running on NeXT Step  */
#undef BORGCUBES

/* Define if running under cygwin  */
#undef CYGWIN_HACKS

/* Define for pre Tcl 7.5 compat  */
#undef HAVE_PRE7_5_TCL

/* Define for Tcl that has Tcl_Free() (7.5p1 and later)  */
#undef HAVE_TCL_FREE

/* Define for Tcl that has threads  */
#undef HAVE_TCL_THREADS

/* Define if you want IPv6 support */
#undef IPV6

/* Define to `unsigned' if something else doesn't define.  */
#undef socklen_t

/* Define if we want to include rpc/types.h.  Crap BSDs put INADDR_LOOPBACK there. */
#undef HAVEUSE_RPCTYPES_H

#undef HAVE_GETTEXT
#undef HAVE_LC_MESSAGES
#undef HAVE_CATGETS
#undef ENABLE_NLS
#undef HAVE_STPCPY

@BOTTOM@

#endif /* !_EGG_CONFIG_H */

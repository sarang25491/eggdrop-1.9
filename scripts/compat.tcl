# compat.tcl
#   This script just quickly maps 1.6 Tcl functions to the new ones.
#
# $Id: compat.tcl,v 1.11 2003/12/17 00:58:02 wcc Exp $

proc puthelp {text {next ""}} {
	if {[string length $next]} {
		putserv -slow -next $text
	} else {
		putserv -slow $text
	}
}

proc putquick {text {next ""}} {
	if {[string length $next]} {
		putserv -quick -next $text
	} else {
		putserv -quick $text
	}
}


# action.fix.tcl
#
# Tothwolf  25May1999: cleanup
# Tothwolf  04Oct1999: changed proc names slightly
#
# $Id: action.fix.tcl,v 1.4 2001/10/21 06:04:43 stdarg Exp $

# fix for mirc dcc chat /me's
bind filt - "\001ACTION *\001" filt:dcc_action
proc filt:dcc_action {idx text} {
  global filt_string
  set filt_string ".me [string trim [join [lrange [split $text] 1 end]] \001]"
  return 0
}

# fix for telnet session /me's
bind filt - "/me *" filt:telnet_action
proc filt:telnet_action {idx text} {
  global filt_string
  set filt_string ".me [join [lrange [split $text] 1 end]]"
  return 0
}

# action.fix.tcl
#
# Tothwolf  25May1999: cleanup
# Tothwolf  04Oct1999: changed proc names slightly
# poptix    07Dec2001: handle irssi (and some others) "correct" messages for DCC CTCP
#
# $Id: action.fix.tcl,v 1.5 2001/12/08 16:58:34 ite Exp $

# fix for mirc dcc chat /me's
bind filt - "\001ACTION *\001" filt:dcc_action
bind filt - "CTCP_MESSAGE \001ACTION *\001" filt:dcc_action2
proc filt:dcc_action {idx text} {
  global filt_string
  set filt_string ".me [string trim [join [lrange [split $text] 1 end]] \001]"
  return 0
}

proc filt:dcc_action2 {idx text} {
  global filt_string
  set filt_string ".me [string trim [join [lrange [split $text] 2 end]] \001]"
  return 0
}

# fix for telnet session /me's
bind filt - "/me *" filt:telnet_action
proc filt:telnet_action {idx text} {
  global filt_string
  set filt_string ".me [join [lrange [split $text] 1 end]]"
  return 0
}

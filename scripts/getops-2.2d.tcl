
# Getops 2.2d

# $Id: getops-2.2d.tcl,v 1.2 1999/12/21 17:35:08 fabian Exp $

# This script is used for bots to request and give ops to each other. 
# For this to work, you'll need:

# - Bots must be linked in a botnet
# - Every bot that should be ops on your channels must load this script
# - Add all bots you wanna op with this one using the .+bot nick address
#   command. The "nick" should be exactly the botnet-nick of the other bot
# - Add the hostmasks that uniquely identify this bot on IRC
# - Add a global or channel +o flag on all bots to be opped
# - Do exactly the same on all other bots

# The security of this relies on the fact that the bot which wants to have
# ops must be 1) linked to the current botnet (which requires a password),
# 2) have an entry with +o on the bot that he wants ops from and 3) must match
# the hostmask that is stored on each bots userfile (so it is good to keep the
# hostmasks up-to-date).

# -----------------------------------------------------------------------------

# 2.2d by brainsick <brnsck@mail.earthlink.net>
#  - Undernet now handles keys differently.  It no longer gives the key on a
#    join, but instead gives it on an op, but eggdrop doesn't check for this.
#    getops-2.2d should now handle this correctly.  (This should be the final
#    fix to the key problems.)

# 2.2c by Progfou (Jean Christophe ANDRE <progfou@rumiko.info.unicaen.fr>)
#  - changed "Requested" to "Requesting" as it was a little confusing
#  - corrected the "I am not on chan..." problem with key request
#    (thanks to Kram |FL| and Gael for reporting it)
#  - removed more unnecessary check

# 2.2b by Progfou (Jean Christophe ANDRE <progfou@rumiko.info.unicaen.fr>)
#  - removed global +o in unknown bot test
#  - removed unnecessary checks due to previous unknown bot test

# 2.2a by Progfou (Jean Christophe ANDRE <progfou@rumiko.info.unicaen.fr>)
#  - removed Polish language!

# 2.2 by Cron (Arkadiusz Miskiewicz <misiek@zsz2.starachowice.pl>)
#  - works good (tested on eggdrop 1.3.11)
#  - asks from unknown (and bots without +bo) are ignored
#  - all messages in Polish language
#  - better response from script to users
#  - fixed several bugs

# 2.1 by Ernst
# - asks for ops right after joining the channel: no wait anymore
# - sets "need-op/need-invite/etc" config right after joining dynamic channels
# - fixed "You aren't +o" being replied when other bot isn't even on channel
# - better response from bots, in case something went wrong
#   (for example if bot is not recognized (hostmask) when asking for ops)
# - removed several no-more-used variables
# - added the information and description above

# 2.0.1 by beldin (1.3.x ONLY version)
# - actually, iso needed to be modded for 1.3 :P, and validchan is builtin
#   and I'll tidy a couple of functions up to

# 2.0 by DarkDruid
# - It'll work with dynamic channels(dan is a dork for making it not..)
# - It will also only ask one bot at a time for ops so there won't be any more
#   annoying mode floods, and it's more secure that way
# - I also took that annoying wallop and resynch stuff out :P
# - And I guess this will with with 1.3.x too

# Previously by The_O, dtM and poptix.

# -----------------------------------------------------------------------------

# [0/1] do you want GetOps to notice when some unknown (unauthorized) bot
#       sends request to your bot
set go_bot_unknown 1

# [0/1] do you want your bot to request to be unbanned if it becomes banned?
set go_bot_unban 1

# [0/1] do you want GetOps to notice the channel if there are no ops?
set go_cycle 0

# set this to the notice txt for the above (go_cycle)
set go_cycle_msg "Please part the channel so the bots can cycle!"

# -----------------------------------------------------------------------------

set bns ""
proc gain_entrance {what chan} {
 global botnick botname go_bot_unban go_cycle go_cycle_msg bns
 switch -exact $what {
  "limit" {
   foreach bs [lbots] {
    putbot $bs "gop limit $chan $botnick"
    putlog "GetOps: Requesting limit raise from $bs on $chan."
   }
  }
  "invite" {
   foreach bs [lbots] {
    putbot $bs "gop invite $chan $botnick"
    putlog "GetOps: Requesting invite from $bs for $chan."
   }  
  }
  "unban" {
   if {$go_bot_unban} {
    foreach bs [lbots] {
     putbot $bs "gop unban $chan $botname"
     putlog "GetOps: Requesting unban on $chan from $bs."
    }
   }
  }
  "key" {
   foreach bs [lbots] {
    putbot $bs "gop key $chan $botnick"
    putlog "GetOps: Requesting key on $chan from $bs."
   }
  }
  "op" {
   if {[hasops $chan]} {
    set bot [getbot $chan]
    if {$bot == ""} {
     set bns ""
     set bot [getbot $chan]
    }
    lappend bns "$bot"
    if {$bot != ""} {
     putbot $bot "gop op $chan $botnick"
     putlog "GetOps: Requesting ops from $bot on $chan"
    }
   } {
    if {$go_cycle} {
     putserv "NOTICE $chan :$go_cycle_msg"
    }
   }
  }
 }
}
proc hasops {chan} {
  foreach user [chanlist $chan] {
    if {[isop $user $chan]} {
      return 1
    }
  }
  return 0
}
proc getbot {chan} {
  global bns
  foreach bn [bots] {
    if {[lsearch $bns $bn] < 0} {
      if {([matchattr $bn o]) || ([matchattr $bn |o $chan])} {
        if {([onchan [hand2nick $bn $chan] $chan]) && ([isop [hand2nick $bn $chan] $chan])} {
          return $bn
          break
        }
      }
    }
  }
}

proc botnet_request {bot com args} {
 global botnick go_bot_unban go_bot_unknown
 set args [lindex $args 0]
 set subcom [lindex $args 0]
 set chan [string tolower [lindex $args 1]]
 set nick [lindex $args 2]

 if {[matchattr $bot b] == 0} {
  if { $go_bot_unknown == 1} {
   putlog "GetOps: Request ($subcom) from $bot - unknown bot (IGNORED)"
  }
  return 0
 }
 if {$subcom != "takekey"} {
  if {[validchan $chan] == 0} {
   putbot $bot "gop_resp I don't monitor $chan."
   return 0
  }
  if {[onchan $botnick $chan] == 0} {
   putbot $bot "gop_resp I am not on $chan."
   return 0
  }
 }

 switch -exact $subcom {
  "op" {
   if {[onchan $nick $chan] == 0} {
    putbot $bot "gop_resp You are not on $chan for me."
    return 1
   }
   set bothand [finduser $nick![getchanhost $nick $chan]]
   if {$bothand == "*"} {
    putlog "GetOps: $bot requested ops on $chan. Ident not recognized: $nick![getchanhost $nick $chan]."
    putbot $bot "gop_resp I don't recognize you on IRC (your ident: $nick![getchanhost $nick $chan])"
    return 1
   }
   if {[string tolower $bothand] == [string tolower $nick]} {
    putlog "GetOps: $bot requested ops on $chan."
   } {
    putlog "GetOps: $bot requested ops as $nick on $chan."
   }
   if {[iso $nick $chan] && [matchattr $bothand b]} {
    if {[botisop $chan]} {
     if {[isop $nick $chan] == 0} {
      putlog "GetOps: $nick asked for op on $chan."
      putbot $bot "gop_resp Opped $nick on $chan."
      pushmode $chan +o $nick
     }
    } {
     putbot $bot "gop_resp I am not +o on $chan."
    }
   } {
    putbot $bot "gop_resp You aren't +o in my userlist for $chan, sorry."
   }
   return 1
  }
  "unban" {
   if {$go_bot_unban} {
    putlog "GetOps: $bot requested that I unban him on $chan."
    foreach ban [chanbans $chan] {
     if {[string compare $nick $ban]} {
      set bug_1 [lindex $ban 0]
      pushmode $chan -b $bug_1
     }
    }
    return 1
   } {
    putlog "GetOps: Refused request to unban $bot ($nick) on $chan."
    putbot $bot "gop_resp Sorry, not accepting unban requests."
   }
  }
  "invite" {
   putlog "GetOps: $bot asked for an invite to $chan."
   putserv "invite $nick $chan"
   return 1
  }
  "limit" {
   putlog "GetOps: $bot asked for a limit raise on $chan."
   pushmode $chan +l [expr [llength [chanlist $chan]] + 1]
   return 1
  }
  "key" {
   putlog "GetOps: $bot requested the key on $chan."
   if {[string match *k* [lindex [getchanmode $chan] 0]]} {
    putbot $bot "gop takekey $chan [lindex [getchanmode $chan] 1]"
   } {
    putbot $bot "gop_resp There isn't a key on $chan!"
   }
   return 1
  }
  "takekey" {
   putlog "GetOps: $bot gave me the key to $chan! ($nick)"
   foreach channel [string tolower [channels]] {
    if {$chan == $channel} {
     putserv "JOIN $channel $nick"
     return 1
    }
   }
  }
  default {
   putlog "GetOps: ALERT! $bot sent fake 'gop' message! ($subcom)"
  }
 }
}

proc gop_resp {bot com msg} {
 putlog "GetOps: MSG from $bot: $msg"
 return 1
}

proc lbots {} {
 set unf ""
 foreach users [userlist b] {
  foreach bs [bots] {
   if {$users == $bs} {
    lappend unf $users
   }
  }
 }
 return $unf
}

# Returns list of bots in the botnet and +o in my userfile on that channel
proc lobots { channel } {
 set unf ""
 foreach users [userlist b] {
  if {[matchattr $users o|o $channel] == 0} { continue }
  foreach bs [bots] {
   if {$users == $bs} {	lappend unf $users }
  }
 }
 return $unf
}

proc iso {nick chan1} {
 return [matchattr [nick2hand $nick $chan1] o|o $chan1]
}

proc do_channels {} {
 global go_chanset
 foreach a [string tolower [channels]] {
  if {[info exist go_chanset($a)] == 0} {
   channel set $a need-op "gain_entrance op $a"
   channel set $a need-key "gain_entrance key $a"
   channel set $a need-invite "gain_entrance invite $a"
   channel set $a need-unban "gain_entrance unban $a"
   channel set $a need-limit "gain_entrance limit $a"
   set go_chanset($a) 1
  }
 }
 if {[string match "*do_channels*" [timers]] == 0} { timer 5 do_channels }
}

if {[string match "*do_channels*" [utimers]] == 0} {
 # Set things up one second after starting (dynamic things already loaded)
 utimer 1 do_channels
}

bind bot - gop botnet_request
bind bot - gop_resp gop_resp

# Ask for ops when joining a channel
bind join - * gop_join

# Get the key after getting opped
bind mode - "* +o" get_key

proc get_key { nick uhost hand chan mode whom } {
 global botnick
 if {$botnick == $whom} {
  puthelp "MODE $chan"
 }
 return 0
}

proc requestop { chan } {
 global botnick
 set chan [string tolower $chan]
 foreach thisbot [lobots $chan] {
  # Send request to all, because the bot does not have the channel info yet
  putbot $thisbot "gop op $chan $botnick"
  lappend askedbot $thisbot
 }
 if {[info exist askedbot]} {
  regsub -all " " $askedbot ", " askedbot
  putlog "GetOps: Requested Ops from $askedbot on $chan."
 } {
  putlog "GetOps: No bots to ask for ops on $chan."
 }
 return 0
}

proc gop_join { nick uhost hand chan } {
 global botnick
 # Check if it was me joining
 if {$nick != $botnick} { return 0 }
 # Adjust channel settings, if needed (e.g when a dynamic channel was added)
 do_channels
 set chan [string tolower $chan]
 # Wait 3 sec, because IRC-lag > Botnet-Lag
 utimer 3 "requestop $chan"
 return 0
}

set getops_loaded 1

putlog "GetOps v2.2d by brainsick, Progfou, Cron@irc.pl, dtM, The_O, DarkDruid & Ernst loaded."

# $Id: config.tcl,v 1.1 2003/02/14 20:55:01 stdarg Exp $
#
# This config file includes all possible options you can
# use to configure your bot properly.
# be sure that you know what you are doing!
#
# more detailed descriptions of all those settings can be found in
# doc/settings/


##### GENERAL STUFF #####

# the username the bot uses, this is only used if identd isn't working
# on the machine the bot is running on.
set username "lamest"

# who's running this bot?
set admin "Lamer <email: lamer@lamest.lame.org>"

# what IRC network are you on?  this is just info to share with others on
# your botnet, for human curiosity only.
set network "I.didnt.edit.my.config.file.net"

# what characters do you want to signify a command on the partyline?
set dcc_command_chars "./"

# what timezone is your bot in? The timezone string specifies the name of
# the timezone and must be three or more alphabetic characters.
#
# ex. Central European Time(UTC+1) would be "CET"
set timezone "EST"

# offset specifies the time value to be added to the local time to get
# Coordinated Universal Time (UTC aka GMT).  The offset is positive if the
# local timezone is west of the Prime Meridian and negative if it is east.
# The value(hours) must be between 0 and 24.
#
# ex. if the timezone is UTC+1 the offset is -1
set offset "5"

# If you dont want to use the timezone setting for scripting purpose
# only but instead everywhere possible (new) then uncomment the next line.
#set env(TZ) "$timezone $offset"

# if you're using virtual hosting (your machine has more than 1 IP), you
# may want to specify the particular IP to bind to.  you can specify
# by IP. if eggdrop has trouble detecting the hostname when it starts up,
# set my_ip. (it will let you know if it has trouble -- trust me.)
# my_ip will be used for IPv4 hosts, my_ip6 will be used for IPv6 hosts.
#set my_ip "99.99.0.0"
#set my_ip6 "3ffe:1337::1"

##### LOG FILES #####

# You can specify a limit on how many log files you can have.
# At midnight every day, the old log files are renamed and a new log file begins.
# By default, the old one is called "(logfilename).yesterday",
# and any logfiles before yesterday are erased.

# Events are logged by certain categories -- this way you can specify
# exactly what kind of events you want sent to various logfiles.  the
# events are:
#   m  private msgs/ctcps to the bot
#   k  kicks, bans, mode changes on the channel
#   j  joins, parts, netsplits on the channel
#   p  public chatter on the channel
#   s  server connects/disconnects/notices
#   b  information about bot linking and userfile sharing
#   c  commands people use (via msg or dcc)
#   x  file transfers and file-area commands
#   o  other: misc info, errors -- IMPORTANT STUFF
#   w  wallops: msgs between IRCops (be sure to set the bot +w)
# There are others, but you probably shouldn't log them, it'd be rather
# unethical ;)

# maximum number of logfiles to allow - this can be increased if needed
# (don't decrease this)
set max_logs 5

# maximum size of your logfiles, set this to 0 to disable.
# this only works if you have keep_all_logs 0 (OFF)
# this value is in KiloBytes, so '550' would mean cycle logs when
# it reaches the size of 550 KiloBytes.
set max_logsize 0

# write the logfiles and check the size every minute
# (if max_logsize is enabled) instead of every 5minutes as before.
# This could be good if you have had problem with the
# logfile filling your quota or hdd or if you log +p
# and publish it on the web and wants more uptodate info.
# If you are concerned with resources keep the default setting 0.
# (although I haven't noticed anything)
set quick_logs 0

# each logfile also belongs to a certain channel.  events of type 'k', 'j',
# and 'p' are logged to whatever channel they happened on.  most other
# events are currently logged to every channel.  you can make a logfile
# belong to all channels by assigning it to channel "*".  there are also
# five user-defined levels ('1'..'5') which are used by Tcl scripts.

# in 'eggdrop.log' put private msgs/ctcps, commands, misc info, and
# errors from any channel:
logfile mco * "logs/eggdrop.log"
# in 'lame.log' put joins, parts, kicks, bans, and mode changes from #lamest:
logfile jk #lamest "logs/lamest.log"

# [0/1] enable quiet saves? "Writing user file..." and "Writing channel file ..."
# will not be logged if this option is enabled.
set quiet_save 0

# this is the default console mode -- what masters will see automatically
# when they dcc chat with the bot (masters can alter their own console
# flags once they connect, though) -- it uses the same event flags as
# the log files
# (note that the console channel is automatically set to your "primary"
# channel -- the one you defined first in this file.  masters can change
# their console channel with the '.console' command, however.)
set console "mkcobxs"


##### FILES AND DIRECTORIES #####

# the userfile: where user records are stored
set userfile "LamestBot.user"

# the pidfile: where eggdrop saves its pid file to
# set pidfile "pid.$botnet_nick"

# where the help files can be found (and there are plenty)
set help_path "help/"

# where the text files can be found (used with various dump commands)
set text_path "text/"

# a good place to temporarily store files (i.e.: /tmp)
set temp_path "/tmp"

# the MOTD is displayed when people dcc chat to the bot.
# type '.help set motd' in DCC CHAT for tons of text substitutions
# that the bot can performed on the motd.
set motd "text/motd"

# the telnet banner is displayed when people first make a telnet
# connection to the bot. type '.help set motd' in DCC CHAT for tons of
# text substitutions that the bot can be performed on the telnet banner.
set telnet_banner "text/banner"

# Specifies what permissions the user, channel and notes files should be set
# to.  The octal values are the same as for the chmod system command.
#
#          u  g  o           u  g  o           u  g  o
#    0600  rw-------   0400  r--------   0200  -w-------    u - user
#    0660  rw-rw----   0440  r--r-----   0220  -w--w----    g - group
#    0666  rw-rw-rw-   0444  r--r--r--   0222  -w--w--w-    o - others
#
# Most users will want to leave the permissions set to 0600, to ensure
# maximum security.
set userfile_perm 0600

##### BOTNET #####

# you probably shouldn't deal with this until reading 'botnet.doc' or
# something.  you need to know what you're doing.

# if you want to use a different nickname on the botnet than you use on
# IRC, set it here:
#set botnet_nick "LlamaBot"

# what telnet port should this bot answer?
# NOTE: if you are running more than one bot on the same machine, you will
#   want to space the telnet ports at LEAST 5 apart... 10 is even better
# if you would rather have one port for the botnet, and one for normal
#   users, you will want something like this instead:
#listen 3333 bots
#listen 4444 users
# NOTE: there are more options listed for the listen command in
#   doc/tcl-commands.doc
listen 3333 all

# [0/1] This setting will drop telnet connections not matching a known host
# It greatly improves protection from IRCOPs, but makes it impossible
# for NOIRC bots to add hosts or have NEW as a valid login
set protect_telnet 0

# and a timeout value for ident lookups would help (seconds)
set ident_timeout 5

# How long (in seconds) should I wait for a connect (dcc chat, telnet,
# relay, etc) before it times out?
set connect_timeout 15

# number of messages / lines from a user on the partyline (dcc, telnet)  before
# they are considered to be flooding (and therefore get booted)
set dcc_flood_thr 3

# how many telnet connection attempt in how many seconds from the same
# host constitutes a flood?
set telnet_flood 5:60

# [0/1] apply telnet flood protection for everyone?
# set this to 0 if you want to exempt +f users from telnet flood protection
set paranoid_telnet_flood 1

# how long should I wait (seconds) before giving up on hostname/address
# lookup? (you might want to increase this if you are on a slow network).
set resolve_timeout 15


##### MORE ADVANCED STUFF #####

# are you behind a firewall?  uncomment this and specify your socks host
#set firewall "proxy:178"
# or, for a Sun "telnet passthru" firewall, set it this way
# (does anyone besides Sun use this?)
#set firewall "!sun-barr.ebay:3666"

# if you have a NAT firewall (you box has an IP in one of the following
# ranges: 192.168.0.0-192.168.255.255, 172.16.0.0-172.31.255.255,
# 10.0.0.0-10.255.255.255 and your firewall transparently changes your
# address to a unique address for your box.) or you have IP masquerading
# between you and the rest of the world, and /dcc chat,/ctcp chat or
# userfile shareing aren't working. Enter your outside IP here.
# Do not enter anything for my_ip.
#set nat_ip "127.0.0.1"

# if you want all dcc file transfers to use a particular portrange either
# because you're behind a firewall, or for other security reasons, set it
# here.
#set reserved_portrange 2010:2020

# [0/1] enables certain console/logging flags which can be used to see all
# information sent to and from the server, also botnet and share traffic.
# NOTE: This is a large security hole, allowing people to see passwords and
# other information they shouldn't otherwise really see. These flags are
# restricted to +n users only. Please choose your owners carefully when you
# enable this option.
set debug_output 0

# temporary ignores will last how many minutes?
set ignore_time 15

# this setting affects what part of the hour the 'hourly' calls occur
# on the bot, this includes such things as note notifying,
# You can change that here (for example, "15" means to
# notify every hour at 15 minutes past the hour)
# this now includes when the bot will save its userfile
set hourly_updates 00

# the following user(s) will ALWAYS have the owner (+n) flag (You really 
# should change this default value)
#set owner "---"

# who should I send a note to when I learn new users?
set notify_newusers "$owner"

# what flags should new users get as a default?
# check '.help whois' on the partyline (dcc chat, telnet) for tons of
# options.
set default_flags "hp"

# what user-defined fields should be displayed in a '.whois'?
# this will only be shown if the user has one of these xtra fields
# you might prefer to comment this out and use the userinfo1.0.tcl script
# which provides commands for changing all of these.
set whois_fields "url birthday"

# [0/1/2] allow people from other bots (in your bot-net) to boot people off
# your bot's party line?
# values:
#   0 - allow *no* outside boots
#   1 - allow boots from sharebots
#   2 - allow any boots
set remote_boots 2

# [0/1] if you don't want people to unlink your share bots from remote bots
# set this to 0
set share_unlinks 1

# [0/1] die on receiving a SIGHUP?
# The bot will save it's userfile when it receives a SIGHUP signal
# with either setting.
set die_on_sighup 0

# [0/1] die on receiving a SIGTERM?
# The bot will save it's userfile when it receives a SIGTERM signal
# with either setting.
set die_on_sigterm 1

# to enable the 'tcl' and 'set' command (let owners directly execute
# Tcl commands)? - a security risk!!
# If you select your owners wisely, you should be okay enabling these.
# to enable, comment these two lines out
# (In previous versions, this was enabled by default in eggdrop.h)
#unbind dcc n tcl *dcc:tcl
#unbind dcc n set *dcc:set

# comment the following line out to add the 'simul' command (owners can
# manipulate other people on the party line).
# Please select owners wisely! Use this command ethically!
#unbind dcc n simul *dcc:simul

# maximum number of dcc connections you will allow - you can increase this
# later, but never decrease it, 50 seems to be enough for everybody
set max_dcc 50

# [0/1] allow +d & +k users to use commands bound as -|- ?
set allow_dk_cmds 1

# If a bot connects which already seems to be connected, I wait
# dupwait_timeout seconds before I check again and then finally reject
# the bot. This is useful to stop hubs from rejecting bots that actually
# have already disconnected from another hub, but the disconnect information
# has not yet spread over the botnet due to lag.
set dupwait_timeout 5



########## MODULES ##########

# below are various settings for the modules available with eggdrop,
# PLEASE EDIT THEM CAREFULLY, READ THEM, even if you're an old hand
# at eggdrop, lots of things have changed slightly

# this is the directory to look for the modules in, if you run the
# bot in the compilation directories you will want to set this to ""
# if you use 'make install' (like all good kiddies do ;) this is a fine
# default, otherwise, use your head :)
set mod_path "modules/"


##### CHANNELS MODULE #####

# this next module provides channel related support for the bot, without
# it, it will just sit on irc, it can respond to msg & ctcp commands, but
# that's all
#loadmodule channels

# the chanfile: where dynamic channel settings are stored
set chanfile "LamestBot.chan"

# [0/1] expire bans/exempts/invites set by other opped bots on the channel?
# set force_expire 0

# [0/1] share user greets with other bots on the channel if sharing user data?
set share_greet 0

# [0/1] allow users to store an info line?
set use_info 1

# these settings are used as default values when you
# .+chan #chan or .tcl channel add #chan
# look in the section above for explanation on every option

set global_flood_chan 10:60
set global_flood_deop 3:10
set global_flood_kick 3:10
set global_flood_join 5:60
set global_flood_ctcp 3:60
set global_flood_nick 5:60

set global_aop_delay 5:30

set global_idle_kick 0
set global_chanmode "nt"
set global_stopnethack_mode 0
set global_revenge_mode 0
set global_ban_time 120
set global_exempt_time 60
set global_invite_time 60

set global_chanset {
        -autoop               -autovoice
        -bitch                +cycle
        +dontkickops          +dynamicbans
        +dynamicexempts       +dynamicinvites
        -enforcebans          +greet
        -inactive             -nodesynch
        -protectfriends       +protectops
        -revenge              -revengebot
        -secret               +statuslog
        +shared               +userinvites
        +userbans             +userexempts
        +honor-global-bans    +honor-global-invites
        +honor-global-exempts
}

##### SERVER MODULE #####

# this provides the core server support (removing this is equivalent to
# the old NO_IRC define)
#loadmodule server

# Uncomment and edit one of the folowing files for network specific
# features. 
source nettype/custom.server.conf
#source nettype/dalnet.server.conf
#source nettype/efnet.server.conf
#source nettype/hybridefnet.server.conf
#source nettype/ircnet.server.conf
#source nettype/undernet.server.conf

##### variables:
# the nick of the bot, that which it uses on IRC, and on the botnet
# unless you specify a separate botnet_nick
set nick "LamestBot"

set botnet_nick "LamestBot"

set myname "LamestBot"

# an alternative nick to use if the nick specified by 'set nick' is
# unavailable. All '?' characters will be replaced by a random number.
set altnick "Llamabot"

# what to display in the real-name field for the bot
set realname "/msg LamestBot hello"

# tcl code to run (if any) when first connecting to a server

bind event init-server event:init_server

proc event:init_server {type} {
  global botnick
  putserv -quick "MODE $botnick +i-ws"
}

# if no port is specified on a .jump, which port should I use?
set default_port 6667

# the server list -- the bot will start at the first server listed, and cycle
# through them whenever it's disconnected
# (please note: you need to change these servers to YOUR network's servers)

# server_add <host> [port] [pass] <-- port and pass are optional

server_add edit.your.config.file

# [0/1] if the bot's nickname is changed (for example, if the intended
# nickname is already in use) keep trying to get the nick back?
set keep_nick 1

# [0/1] if this is set, a leading '~' on user@hosts WON'T be stripped off
set strict_host 0

# [0/1] Squelch the error message when rejecting a DCC CHAT or SEND?
# Normally it tells the DCC user that the CHAT or SEND has been rejected
# because they don't have access, but sometimes IRC server operators
# detect bots that way.
set quiet_reject 1

# answer HOW MANY stacked ctcps at once
set answer_ctcp 3

# setting any of the following with how many == 0 will turn them off
# how many msgs in how many seconds from the same host constitutes a flood?
set flood_msg 5:60
# how many CTCPs in how many seconds?
set flood_ctcp 3:60

# number of seconds to wait between each server connect (0 = no wait)
# useful for preventing ircu throttling
# setting this too low could make your server admins *very* unhappy
set server_cycle_wait 60

# how many seconds to wait for a response when connecting to a server
# before giving up and moving on?
set server_timeout 60

# [0/1] check for stoned servers? (i.e. Where the server connection has
# died, but eggdrop hasn't been notified yet).
set check_stoned 1

# maximum number of lines to queue to the server.
# if you're going to dump large chunks of text to people over irc,  you
# will probably want to raise this -- most people are fine at 300 though
set max_queue_msg 300

# [0/1] trigger bindings for ignored users?
set trigger_on_ignore 0

# [0/1/2] do you want the bot to optimize the kicking queues? Set to 2 if you
# want the bot to change queues if somebody parts or changes nickname.
# ATTENTION: Setting 2 is very CPU intensive
set optimize_kicks 1

# If your network supports more recipients per command then 1, you can
# change this behavior here. Set this to the number of recipients per
# command, or set this to 0 for unlimited.
set stack_limit 4


##### CTCP MODULE #####

# this provides the normal ctcp replies that you'd expect *RECOMMENDED*
#loadmodule ctcp

# several variables exist to better blend your egg in.  they are
# ctcp_version, ctcp_finger, and ctcp_userinfo.  you can use set
# to set them to values you like.

# [0/1/2] 0: normal behavior. 1: bot ignores all CTCPs, except for CTCP
# CHATs & PINGs requested by +o flag users. 2: bot doesn't answer more
# than C CTCPs in S seconds.
# C/S are defined by the set flood_ctcp C:S (cf. server module)
set ctcp_mode 0


##### IRC MODULE #####

# this module provides ALL NORMAL IRC INTERACTION, if you want the normal
# join & maintain channels stuff, this is the module.
#loadmodule irc

# Uncomment and edit one of the folowing files for network specific
# features. 
source nettype/custom.irc.conf
#source nettype/dalnet.irc.conf
#source nettype/efnet.irc.conf
#source nettype/hybridefnet.irc.conf
#source nettype/ircnet.irc.conf
#source nettype/undernet.irc.conf

# [0/1] define this if you want to bounce all server bans
set bounce_bans 1

# [0/1] define this if you want to bounce all the server modes
set bounce_modes 0

# [0/1] let users introduce themselves to the bot via 'hello'?
set learn_users 0

# time (in seconds) to wait for someone to return from a netsplit
set wait_split 600

# time (in seconds) that someone must have been off-channel before
# re-displaying their info
set wait_info 180

# this is the maximum number of bytes to send in the arguments to mode's
# sent to the server, most servers default this to 200, so it should
# be sufficient
set mode_buf_length 200

# many irc ops check for bots that respond to 'hello'.  you can change this
# to another word by uncommenting the following two lines, and changing
# "myword" to the word you want to use instead of 'hello' (it must be a
# single word)
# novice users are not expected to understand what these two lines do; they
# are just here to help you.  for more information on 'bind', check the file
# 'tcl-commands.doc'
#unbind msg - hello *msg:hello
#bind msg - myword *msg:hello

# Many takeover attempts occur due to lame users blindy /msg ident'n to
# the bot without checking if the bot is the bot.
# We now unbind this command by default to discourage them
#unbind msg ident *msg:ident

# If you or your users use many different hosts and wants to
# be able to add it by /msg'ing you need to remove the
# unbind ident line above or bind it to another word.
#bind msg - myidentword *msg:ident

# [0/1] If you are so lame you want the bot to display peoples info lines, even
# when you are too lazy to add their chanrecs to a channel, set this to 1
# *NOTE* This means *every* user with an info line will have their info
# display on EVERY channel they join (provided they have been gone longer than
# wait_info)
set no_chanrec_info 0


##### TRANSFER MODULE #####

# uncomment this line to load the transfer module, this provides
# dcc send/get support and bot userfile transfer support (not sharing)
#loadmodule transfer

##### variables:
# set maximum number of simultaneous downloads to allow for each user
set max_dloads 3

# set the block size for dcc transfers (ircII uses 512 bytes, but admits
# that may be too small -- 1024 is standard these days)
# set this to 0 to use turbo-dcc (recommended)
set dcc_block 0

# [0/1] copy files into the /tmp directory before sending them?  this is
# useful on most systems for file stability.  (someone could move a file
# around while it's being downloaded, and mess up the transfer.)  but if
# your directories are NFS mounted, it's a pain, and you'll want to set
# this to 0. If you are low on disk space, you may want to set this to 0.
set copy_to_tmp 1

# time (in seconds) that a dcc file transfer can remain inactive
# before being timed out
set xfer_timeout 30


##### SHARE MODULE #####

# this provides the userfile sharing support
# (this requires the channels & transfer modules)
#loadmodule share

##### variables:
# [0/1] When two bots get disconnected this flag allows them to create
# a resync buffer which saves all changes done to the userfile during
# the disconnect. So, when they reconnect, they will not have to transfer
# the complete user file, but instead, just send the resync buffer.
# If you have problems with this feature please tell us. Take a look at
# doc/BUG-REPORT first though.
#set allow_resync 0

# this specifies how long to hold another bots resync data for before
# flushing it
#set resync_time 900

# [0/1] when sharing user lists, DONT ACCEPT global flag changes from other bots?
# NOTE: the bot will still send changes made on the bot, it just wont accept
# any global flag changes from other bots
#set private_global 0

# when sharing user lists, if private_global isn't set, which global flag
# changes from other bots should be ignored ?
#set private_globals "mnot"

# [0/1] when sharing user lists, DON'T ACCEPT any userfile changes from other
# bots?
# NOTE: paranoid people should use this feature on their hub bot - this
# will force all +host/+user/chpass/etc. changes to be made via the hub
#set private_user 0

# [0/1] this setting makes the bot discard it's own bot records in favor of
# the ones sent by the hub. Note: This only works with hubs that are v1.5.1
# _or higher_.
#set override_bots 0


##### COMPRESS MODULE #####

# The compress module provides support for file compression. This allows the
# bot to transfer compressed user files and therefore save a significant
# amount of bandwidth, especially on very active hubs.
#loadmodule compress

# [0/1] allow compressed sending of user files. The user files
# are compressed with the compression level defined in `compress_level'.
#set share_compressed 1

# [0-9] default compression level used.
#set compress_level 9


##### FILESYSTEM MODULE #####

# uncomment this line to load the file system module, this provides
# an area within the bot where you can store files
#loadmodule filesys

# this is the 'root' directory for the file system (set it to "" if you
# don't want a file system)
set files_path "/home/mydir/filesys"

# if you want to allow uploads, set this to the directory uploads should be
# put into
set incoming_path "/home/mydir/filesys/incoming"

# [0/1] alternately, you can set this, and uploads will go to the current
# directory that a user is in
set upload_to_pwd 0

# eggdrop creates a '.filedb' file in each subdirectory of your dcc area,
# to keep track of its own file system info -- if you can't do that (like
# if the dcc path isn't owned by yours) or you just don't want it to do
# that, specify a path here where you'd like all the database files to
# be stored instead (otherwise, just leave it blank)
set filedb_path ""

# set maximum number of people that can be in the file area at once
# (0 to make it effectively infinite)
set max_file_users 20

# maximum allowable file size that will be received, in K
# (default is 1024K = 1M). 0 makes it effectively infinite.
set max_filesize 1024


##### NOTES MODULE #####

# this provides support for storing of notes for users from each other
# notes between currently online users is supported in the core, this is
# only for storing the notes for later retrieval, direct user->user notes
# are built-in
#loadmodule notes

# the notefile: where private notes between users are stored
set notefile "LamestBot.notes"

# maximum number of notes to allow to be stored for each user
# (to prevent flooding)
set max_notes 50

# time (in days) to let stored notes live before expiring them
set note_life 60

# [0/1] allow users to specify a forwarding address for forwarding notes
# to another bot
set allow_fwd 0

# [0/1] set this to 1 if you want the bot to let people know hourly if they
# have any notes
set notify_users 1

# [0/1] set this to 1 if you want the bot to let people know on join if they
# have any notes
set notify_onjoin 1

##### CONSOLE MODULE #####

# this module provides storage of console settings when you exit the bot
# (or .store)
#loadmodule console

##### variables:
# [0/1] save users console settings automatically? (otherwise they have to use
# .store)
set console_autosave 0

# [0-99999] if a user doesn't have any console settings saved, which channel
# do you want them automatically put on?
set force_channel 0

# [0/1] display a user's global info line when they join a botnet channel?
set info_party 0


##### UPTIME MODULE #####

# this module reports uptime statistics to http://uptime.eggheads.org
# go look and see what your uptime is! (it will show up after 9 hours or so)
# (this requires the server module)
#loadmodule uptime


##### SCRIPTS #####

# these are some commonly loaded (and needed) scripts.
#source scripts/alltools.tcl
#source scripts/action.fix.tcl

# use this for tcl and eggdrop downwards compatibility
#source scripts/compat.tcl

# This script provides many useful minor informational commands
# (like setting user's URLs, email address, etc). You can modify
# it to add extra entries, you might also want to modify help/userinfo.help
# and help/msg/userinfo.help to change the help files.
#source scripts/userinfo.tcl
loadhelp userinfo.help

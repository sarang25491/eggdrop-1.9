/*
 * main.c --
 *
 *	core event handling
 *	signal handling
 *	command line arguments
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
/*
 * The author (Robey Pointer) can be reached at:  robey@netcom.com
 * NOTE: Robey is no longer working on this code, there is a discussion
 * list available at eggheads@eggheads.org.
 */

#ifndef lint
static const char rcsid[] = "$Id: main.c,v 1.150 2003/06/08 03:21:23 stdarg Exp $";
#endif

#include <unistd.h>
#include <eggdrop/eggdrop.h>
#include "main.h"
#include "core_config.h"
#include <fcntl.h>
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <setjmp.h>
#include <locale.h>
#include <ctype.h>
#include <ltdl.h>

#ifdef STOP_UAC				/* osf/1 complains a lot */
#include <sys/sysinfo.h>
#define UAC_NOPRINT    0x00000001	/* Don't report unaligned fixups */
#endif
/* Some systems have a working sys/wait.h even though configure will
 * decide it's not bsd compatable.  Oh well.
 */

#include "bg.h"
#include "core_binds.h"
#include "logfile.h"

#ifdef CYGWIN_HACKS
#include <windows.h>
#endif

#ifndef _POSIX_SOURCE
/* Solaris needs this */
#define _POSIX_SOURCE 1
#endif

char	egg_version[1024] = VERSION;
int	egg_numver = VERSION_NUM;

eggdrop_t *egg = NULL;		/* Eggdrop's context */

int	default_flags = 0;	/* Default user flags and */
int	default_uflags = 0;	/* Default userdefinied flags for people
				   who say 'hello' or for .adduser */

int	backgrd = 1;		/* Run in the background? */
int	con_chan = 0;		/* Foreground: constantly display channel
				   stats? */
int	term_z = 0;		/* Foreground: use the terminal as a party
				   line? */
char	configfile[121] = "config.xml"; /* Name of the config file */
char	helpdir[121] = "help/";	/* Directory of help files (if used) */
char	textdir[121] = "text/";	/* Directory for text files that get dumped */
time_t	online_since;		/* Unix-time that the bot loaded up */
int	make_userfile = 0;	/* Using bot in make-userfile mode? (first
				   user to 'hello' becomes master) */
char	pid_file[120];		/* Name of the file for the pid to be
				   stored in */
int	save_users_at = 0;	/* How many minutes past the hour to
				   save the userfile? */
char	version[81];		/* Version info (long form) */
char	ver[41];		/* Version info (short form) */
int	use_stderr = 1;		/* Send stuff to stderr instead of logfiles? */
int	do_restart = 0;		/* .restart has been called, restart asap */
int	die_on_sighup = 0;	/* die if bot receives SIGHUP */
int	die_on_sigterm = 1;	/* die if bot receives SIGTERM */
int	resolve_timeout = 15;	/* hostname/address lookup timeout */
char	quit_msg[1024];		/* quit message */
time_t	now;			/* duh, now :) */
egg_timeval_t egg_timeval_now;	/* Same thing, but seconds and microseconds. */

void fatal(const char *s, int recoverable)
{
  putlog(LOG_MISC, "*", "* %s", s);
  flushlogs();
  unlink(pid_file);
  if (!recoverable) {
    bg_send_quit(BG_ABORT);
    exit(1);
  }
}

static void got_bus(int z)
{
  fatal("BUS ERROR -- CRASHING!", 1);
#ifdef SA_RESETHAND
  kill(getpid(), SIGBUS);
#else
  bg_send_quit(BG_ABORT);
  exit(1);
#endif
}

static void got_segv(int z)
{
  fatal("SEGMENT VIOLATION -- CRASHING!", 1);
#ifdef SA_RESETHAND
  kill(getpid(), SIGSEGV);
#else
  bg_send_quit(BG_ABORT);
  exit(1);
#endif
}

static void got_fpe(int z)
{
  fatal("FLOATING POINT ERROR -- CRASHING!", 0);
}

static void got_term(int z)
{
  check_bind_event("sigterm");
  if (die_on_sigterm) {
    fatal("TERMINATE SIGNAL -- SIGNING OFF", 0);
  } else {
    putlog(LOG_MISC, "*", "RECEIVED TERMINATE SIGNAL (IGNORING)");
  }
}

static void got_quit(int z)
{
  check_bind_event("sigquit");
  putlog(LOG_MISC, "*", "RECEIVED QUIT SIGNAL (IGNORING)");
  return;
}

static void got_hup(int z)
{
  check_bind_event("sighup");
  if (die_on_sighup) {
    fatal("HANGUP SIGNAL -- SIGNING OFF", 0);
  } else
    putlog(LOG_MISC, "*", "Received HUP signal: rehashing...");
  do_restart = -2;
  return;
}

/* Got ILL signal
 */
static void got_ill(int z)
{
  check_bind_event("sigill");
}


static void print_version(void)
{
  char x[1024];
    
  strlcpy(x, egg_version, sizeof x);
  printf("%s\n", version);
}

static void print_help(void)
{
  print_version ();
  printf("\n\
Usage: %s [OPTIONS]... [FILE]\n", PACKAGE);
  printf(_("\n\
  -h, --help                 Print help and exit\n\
  -v, --version              Print version and exit\n\
  -n, --foreground           Don't go into the background (default=off)\n\
  -c  --channel-stats        (with -n) Display channel stats every 10 seconds\n\
  -t  --terminal             (with -n) Use terminal to simulate dcc-chat\n\
  -m  --make-userfile        Userfile creation mode\n\
  FILE  optional config filename (default 'config.xml')\n"));
  printf("\n");
}

static void do_args(int argc, char * const *argv)
{
  int c;
  
  optarg = 0;
  optind = 1;
  opterr = 1;
  optopt = '?';
	  
  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
      { "help",       0, NULL, 'h' },
      { "version",    0, NULL, 'v' },
      { "load-module",    1, NULL, 'p' },
      { "foreground",      0, NULL, 'n' },
      { "channel-stats",      0, NULL, 'c' },
      { "terminal",   0, NULL, 't' },
      { "make-userfile",      0, NULL, 'm' },
      { NULL, 0, NULL, 0 }
    };
	  
    c = getopt_long (argc, argv, "hvp:nctm", long_options, &option_index);

    if (c == -1) break;
    
    switch (c) {
      case 'n':
	backgrd = 0;
	break;

      case 'c':
	con_chan = 1;
	term_z = 0;
	break;

      case 't':
	con_chan = 0;
	term_z = 1;
        break;

      case 'm':
	make_userfile = 1;
        break;

      case 'v':
	print_version();
	bg_send_quit(BG_ABORT);
	exit(0);
        break; /* this should never be reached */

      case 'h':
	print_help();
	bg_send_quit(BG_ABORT);
	exit(0);
	break; /* this should never be reached */

      case 0: /* Long option with no short option */
      case '?':       /* Invalid option.  */
	/* getopt_long() already printed an error message.  */
	bg_send_quit(BG_ABORT);
	exit(1);

      default:        /* bug: option not considered.  */
	fprintf (stderr, "%s: option unknown: %c\n", PACKAGE, c);
	bg_send_quit(BG_ABORT);
	exit(1);
	break; /* this should never be reached */
    }
  }
  if (optind < argc)
    strlcpy(configfile, argv[optind], sizeof configfile);
}

/* Timer info */
static int		lastmin = 99;
static struct tm	nowtm;

/* Called once a second.
 */
static int core_secondly()
{
  static int cnt = 0;
  int miltime;

  check_bind_secondly();
  cnt++;
  memcpy(&nowtm, localtime(&now), sizeof(struct tm));
  if (nowtm.tm_min != lastmin) {
    int i = 0;

    /* Once a minute */
    lastmin = (lastmin + 1) % 60;
    check_bind_event("minutely");
    check_bind_time(&nowtm);
    /* In case for some reason more than 1 min has passed: */
    while (nowtm.tm_min != lastmin) {
      /* Timer drift, dammit */
      putlog(LOG_DEBUG, "*", "timer: drift (lastmin=%d, now=%d)", lastmin,
             nowtm.tm_min);
      i++;
      lastmin = (lastmin + 1) % 60;
      check_bind_event("minutely");
    }
    if (i > 1)
      putlog(LOG_MISC, "*", "(!) timer drift -- spun %d minutes", i);
    miltime = (nowtm.tm_hour * 100) + (nowtm.tm_min);
    if (((int) (nowtm.tm_min / 5) * 5) == (nowtm.tm_min)) {	/* 5 min */
	    check_bind_event("5minutely");
      if (!miltime) {	/* At midnight */
	char s[25];

	strlcpy(s, ctime(&now), sizeof s);
	putlog(LOG_ALL, "*", "--- %.11s%s", s, s + 20);
	check_bind_event("backup");
	check_bind_event("daily");
      }
    }
  }
  return(0);
}

void core_party_init();
void core_config_init();
//void telnet_init();

int file_check(const char *filename)
{
	struct stat sbuf = {0};
	int r;

	r = stat(filename, &sbuf);
	if (r) return(-1);
	errno = 0;
	if (sbuf.st_mode & (S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)) return(-1);
	return(0);
}

int main(int argc, char **argv)
{
  int xx, i;
  char s[25];
  FILE *f;
  struct sigaction sv;
  egg_timeval_t howlong;
  int timeout;
  void *config_root, *entry;
  char *modname, *scriptname;

#ifdef DEBUG
  /* Make sure it can write core, if you make debug. Else it's pretty
   * useless (dw)
   */
  {
#include <sys/resource.h>
    struct rlimit cdlim;

    cdlim.rlim_cur = RLIM_INFINITY;
    cdlim.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &cdlim);
  }
#endif

#ifdef ENABLE_NLS
  setlocale(LC_MESSAGES, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
#endif

	/* Initialize ltdl. */
	LTDL_SET_PRELOADED_SYMBOLS();
	if (lt_dlinit()) {
		printf("Fatal error initializing ltdl: %s\n", lt_dlerror());
		return(-1);
	}

  /* Version info! */
  snprintf(ver, sizeof ver, "%s v%s", PACKAGE, egg_version);
  snprintf(version, sizeof version, "Eggdrop v%s (C) 1997 Robey Pointer (C) 2003 Eggheads",
	       egg_version);
  /* Now add on the patchlevel (for Tcl) */
  sprintf(&egg_version[strlen(egg_version)], " %u", egg_numver);
#ifdef STOP_UAC
  {
    int nvpair[2];

    nvpair[0] = SSIN_UACPROC;
    nvpair[1] = UAC_NOPRINT;
    setsysinfo(SSI_NVPAIRS, (char *) nvpair, 1, NULL, 0);
  }
#endif

  /* Set up error traps: */
  sv.sa_handler = got_bus;
  sigemptyset(&sv.sa_mask);
#ifdef SA_RESETHAND
  sv.sa_flags = SA_RESETHAND;
#else
  sv.sa_flags = 0;
#endif
  sigaction(SIGBUS, &sv, NULL);
  sv.sa_handler = got_segv;
  sigaction(SIGSEGV, &sv, NULL);
#ifdef SA_RESETHAND
  sv.sa_flags = 0;
#endif
  sv.sa_handler = got_fpe;
  sigaction(SIGFPE, &sv, NULL);
  sv.sa_handler = got_term;
  sigaction(SIGTERM, &sv, NULL);
  sv.sa_handler = got_hup;
  sigaction(SIGHUP, &sv, NULL);
  sv.sa_handler = got_quit;
  sigaction(SIGQUIT, &sv, NULL);
  sv.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sv, NULL);
  sv.sa_handler = got_ill;
  sigaction(SIGILL, &sv, NULL);

  /* Initialize variables and stuff */
  timer_update_now(&egg_timeval_now);
  now = egg_timeval_now.sec;
  memcpy(&nowtm, localtime(&now), sizeof(struct tm));
  lastmin = nowtm.tm_min;
  srandom(now % (getpid() + getppid()));

  do_args(argc, argv);

  printf("\n%s\n", version);
  printf("   *** WARNING: Do NOT run this DEVELOPMENT version for any purpose other than testing.\n\n");

  /* Don't allow eggdrop to run as root */
  if (((int) getuid() == 0) || ((int) geteuid() == 0))
    fatal(_("ERROR: Eggdrop will not run as root!"), 0);

	if (file_check(configfile)) {
		if (errno) perror(configfile);
		else fprintf(stderr, "ERROR\n\nCheck file permissions on your config file!\nMake sure other groups and users cannot read/write it.\n");
		exit(-1);
	}

  egg = eggdrop_new();
  config_init();
  core_config_init(configfile);
  egglog_init();
  logfile_init();
  script_init();
  user_init();
  if (core_config.userfile) user_load(core_config.userfile);
  partyline_init();
  core_party_init();
  //telnet_init();
  module_init();
  egg_net_init();
  core_binds_init();

  if (backgrd)
    bg_prepare_split();

  strlcpy(s, ctime(&now), sizeof s);
  strcpy(&s[11], &s[20]);
  putlog(LOG_ALL, "*", "--- Loading %s (%s)", ver, s);

	/* Put the module directory in the ltdl search path. */
	if (core_config.module_path) module_add_dir(core_config.module_path);

	/* Scan the autoload section of config. */
	config_root = config_get_root("eggdrop");
	for (i = 0; (entry = config_exists(config_root, "eggdrop", 0, "autoload", 0, "module", i, NULL)); i++) {
		modname = NULL;
		config_get_str(&modname, entry, NULL);
		module_load(modname);
	}
	for (i = 0; (entry = config_exists(config_root, "eggdrop", 0, "autoload", 0, "script", i, NULL)); i++) {
		scriptname = NULL;
		config_get_str(&scriptname, entry, NULL);
		script_load(scriptname);
	}



  if (!pid_file[0])
    snprintf(pid_file, sizeof pid_file, "pid.%s", core_config.botname);

  /* Check for pre-existing eggdrop! */
  f = fopen(pid_file, "r");
  if (f != NULL) {
    fgets(s, 10, f);
    xx = atoi(s);
    kill(xx, SIGCHLD);		/* Meaningless kill to determine if pid
				   is used */
    if (errno != ESRCH) {
      printf(_("I detect %s already running from this directory.\n"), core_config.botname);
      printf(_("If this is incorrect, erase the %s\n"), pid_file);
      bg_send_quit(BG_ABORT);
      exit(1);
    }
  }

  /* Move into background? */
  if (backgrd) {
#ifndef CYGWIN_HACKS
    bg_do_split();
  } else {			/* !backgrd */
#endif
    xx = getpid();
    if (xx != 0) {
      FILE *fp;

      /* Write pid to file */
      unlink(pid_file);
      fp = fopen(pid_file, "w");
      if (fp != NULL) {
        fprintf(fp, "%u\n", xx);
        if (fflush(fp)) {
	  /* Let the bot live since this doesn't appear to be a botchk */
	  printf(_("* Warning!  Could not write %s file!\n"), pid_file);
	  fclose(fp);
	  unlink(pid_file);
        } else
 	  fclose(fp);
      } else
        printf(_("* Warning!  Could not write %s file!\n"), pid_file);
#ifdef CYGWIN_HACKS
      printf("Launched into the background  (pid: %d)\n\n", xx);
#endif
    }
  }

  use_stderr = 0;		/* Stop writing to stderr now */
  if (backgrd) {
    /* Ok, try to disassociate from controlling terminal (finger cross) */
#if HAVE_SETPGID && !defined(CYGWIN_HACKS)
    setpgid(0, 0);
#endif
    /* Tcl wants the stdin, stdout and stderr file handles kept open. */
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
#ifdef CYGWIN_HACKS
    FreeConsole();
#endif
  }

  online_since = now;
  howlong.sec = 1;
  howlong.usec = 0;
  timer_create_repeater(&howlong, "main loop", core_secondly);

  putlog(LOG_DEBUG, "*", "main: entering loop");
	while (1) {
		timer_update_now(&egg_timeval_now);
		now = egg_timeval_now.sec;
		random(); /* Woop, lets really jumble things */
		timer_run();

		if (timer_get_shortest(&howlong)) {
			timeout = 1000;
		}
		else {
			timeout = howlong.sec * 1000 + howlong.usec / 1000;
		}
		sockbuf_update_all(timeout);
		garbage_run();
	}
}

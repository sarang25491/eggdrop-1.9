/*
 * main.c --
 *
 *	core event handling
 *	signal handling
 *	command line arguments
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Eggheads Development Team
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
static const char rcsid[] = "$Id: main.c,v 1.155 2003/12/16 03:26:42 wcc Exp $";
#endif

#include <ctype.h>
#include <unistd.h>
#include <eggdrop/eggdrop.h>
#include "main.h"
#include "core_config.h"
#include "bg.h"
#include "core_binds.h"
#include "logfile.h"
#include <fcntl.h>

#ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else
#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else
#    include <time.h>
#  endif
#endif

#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <setjmp.h>
#include <locale.h>
#include <ctype.h>
#include <ltdl.h>

#ifdef STOP_UAC /* OSF/1 complains a lot */
#  include <sys/sysinfo.h>
#  define UAC_NOPRINT 0x00000001 /* Don't report unaligned fixups */
#endif

#ifdef CYGWIN_HACKS
#  include <windows.h>
#endif

#ifndef _POSIX_SOURCE
#  define _POSIX_SOURCE 1 /* Solaris needs this */
#endif

eggdrop_t *egg = NULL;	/* Eggdrop's context */

int backgrd = 1;	/* Run in the background? */
int con_chan = 0;	/* Foreground: constantly display channel stats? */
int term_z = 0;		/* Foreground: use the terminal as a partyline? */
int make_userfile = 0;	/* Start bot in make-userfile mode? */

char configfile[121] = "config.xml";	/* Name of the config file */
char helpdir[121] = "help/";		/* Directory of help files (if used) */
char textdir[121] = "text/";		/* Directory for text files that get dumped */
char pid_file[120];			/* Name of Eggdrop's pid file */

time_t online_since;		/* Time the bot was started */
time_t now;			/* Current time */
egg_timeval_t egg_timeval_now;	/* Current time in seconds and microseconds. */

int use_stderr = 1;		/* Send stuff to stderr instead of logfiles? */

char version[81];

static int lastmin = 99;
static struct tm nowtm;

void core_party_init();
void core_config_init();

void fatal(const char *s, int recoverable)
{
	putlog(LOG_MISC, "*", "Fatal: %s", s);
	flushlogs();
	unlink(pid_file);
	if (!recoverable) {
		bg_send_quit(BG_ABORT);
		exit(1);
	}
}

static void got_bus(int z)
{
	fatal("BUS ERROR.", 1);
#ifdef SA_RESETHAND
	kill(getpid(), SIGBUS);
#else
	bg_send_quit(BG_ABORT);
	exit(1);
#endif
}

static void got_segv(int z)
{
	fatal("SEGMENT VIOLATION.", 1);
#ifdef SA_RESETHAND
	kill(getpid(), SIGSEGV);
#else
	bg_send_quit(BG_ABORT);
	exit(1);
#endif
}

static void got_fpe(int z)
{
	fatal("FLOATING POINT ERROR.", 0);
}

static void got_term(int z)
{
	eggdrop_event("sigterm");
	if (core_config.die_on_sigterm) {
		putlog(LOG_MISC, "*", "Saving user file...");
		user_save(core_config.userfile);
		putlog(LOG_MISC, "*", "Saving config file...");
		core_config_save();
		putlog(LOG_MISC, "*", "Bot shutting down: received TERMINATE signal.");
		flushlogs();
		unlink(pid_file);
		exit(0);
	}
	else putlog(LOG_MISC, "*", "Received TERMINATE signal (ignoring).");
}

static void got_quit(int z)
{
	eggdrop_event("sigquit");
	putlog(LOG_MISC, "*", "Received QUIT signal (ignoring).");
	return;
}

static void got_hup(int z)
{
	eggdrop_event("sighup");
	putlog(LOG_MISC, "*", "Received HUP signal (ignoring).");
	return;
}

static void got_ill(int z)
{
	eggdrop_event("sigill");
	putlog(LOG_MISC, "*", "Received ILL signal (ignoring).");
}


static void print_version(void)
{
	printf("%s\n", version);
}

static void print_help(const char **argv)
{
	print_version();
	printf("\nUsage: %s [OPTIONS]... [FILE]\n", argv[0]);
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

static void do_args(int argc, const char **argv)
{
	int c;

	optarg = 0;
	optind = 1;
	opterr = 1;
	optopt = '?';

	while (1) {
		int option_index = 0;

		static struct option long_options[] = {
			{"help", 0, NULL, 'h'},
			{"version", 0, NULL, 'v'},
			{"load-module", 1, NULL, 'p'},
			{"foreground", 0, NULL, 'n'},
			{"channel-stats", 0, NULL, 'c'},
			{"terminal", 0, NULL, 't'},
			{"make-userfile", 0, NULL, 'm'},
			{NULL, 0, NULL, 0}
		};

		c = getopt_long(argc, argv, "hvp:nctm", long_options, &option_index);
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
				print_help(argv);
				bg_send_quit(BG_ABORT);
				exit(0);
				break; /* this should never be reached */
			case 0: /* Long option with no short option */
			case '?': /* Invalid option. */
				/* getopt_long() already printed an error message. */
				bg_send_quit(BG_ABORT);
				exit(1);
			default: /* bug: option not considered.  */
				fprintf(stderr, "%s: option unknown: %c\n", argv[0], c);
				bg_send_quit(BG_ABORT);
				exit(1);
				break; /* this should never be reached */
		}
	}
	if (optind < argc) strlcpy(configfile, argv[optind], sizeof configfile);
}

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
		eggdrop_event("minutely");
		check_bind_time(&nowtm);
		/* In case for some reason more than 1 min has passed: */
		while (nowtm.tm_min != lastmin) {
			/* Timer drift, dammit */
			putlog(LOG_DEBUG, "*", "timer: drift (lastmin=%d, now=%d)", lastmin, nowtm.tm_min);
			i++;
			lastmin = (lastmin + 1) % 60;
			eggdrop_event("minutely");
		}
		if (i > 1) putlog(LOG_MISC, "*", "Warning: timer drift (%d minutes).", i);

		miltime = (nowtm.tm_hour * 100) + (nowtm.tm_min);
		if (((int) (nowtm.tm_min / 5) * 5) == (nowtm.tm_min)) { /* 5 minutes */
			eggdrop_event("5minutely");
			if (!miltime) { /* At midnight */
				char s[25];

				strlcpy(s, ctime(&now), sizeof s);
				putlog(LOG_ALL, "*", "--- %.11s%s", s, s + 20);
				eggdrop_event("backup");
				eggdrop_event("daily");
			}
		}
	}
	return(0);
}


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
	int xx, i, timeout;
	char s[25], *modname, *scriptname;
	FILE *f;
	struct sigaction sv;
	egg_timeval_t howlong;
	void *config_root, *entry;

#ifdef DEBUG
	{
#  include <sys/resource.h>
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

	snprintf(version, sizeof version, "Eggdrop v%s (C) 1997 Robey Pointer (C) 2004 Eggheads Development Team", VERSION);

#ifdef STOP_UAC
	{
		int nvpair[2];

		nvpair[0] = SSIN_UACPROC;
		nvpair[1] = UAC_NOPRINT;
		setsysinfo(SSI_NVPAIRS, (char *) nvpair, 1, NULL, 0);
	}
#endif

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

	timer_update_now(&egg_timeval_now);
	now = egg_timeval_now.sec;
	memcpy(&nowtm, localtime(&now), sizeof(struct tm));
	lastmin = nowtm.tm_min;

	srandom(now % (getpid() + getppid()));

	do_args(argc, argv);
	printf("\n%s\n", version);
	printf("WARNING: Do NOT run this DEVELOPMENT version for any purpose other than testing.\n\n");

	if (((int) getuid() == 0) || ((int) geteuid() == 0)) fatal(_("Eggdrop will not run as root!"), 0);

	if (file_check(configfile)) {
		if (errno) perror(configfile);
		else fprintf(stderr, "ERROR\n\nCheck file permissions on your config file!\nMake sure other groups and users cannot read/write it.\n");
		exit(-1);
	}

	egg = eggdrop_new();
	eggdrop_init();
	core_config_init(configfile);
	logfile_init();
	if (core_config.userfile) user_load(core_config.userfile);
	core_party_init();
	core_binds_init();

	if (make_userfile) {
		user_t *owner;
		char handle[512], password[12];
		int len;

		if (user_count() != 0) {
			printf("\n\n\nYou are trying to create a new userfile, but the old one still exists (%s)!\n\nPlease remove the userfile, or do not pass the -m option.\n\n", core_config.userfile);
			exit(1);
		}

		printf("\n\n\nHello! I see you are creating a new userfile.\n\n");
		printf("Let's create the owner account.\n\n");
		do {
			printf("Enter the owner handle: ");
			fflush(stdout);
			fgets(handle, sizeof(handle), stdin);
			for (len = 0; handle[len]; len++) {
				if (!ispunct(handle[len]) && !isalnum(handle[len])) break;
			}
			if (len == 0) printf("Come on, enter a real handle.\n\n");
		} while (len <= 0);
		handle[len] = 0;
		owner = user_new(handle);
		if (!owner) {
			printf("Failed to create user record! Out of memory?\n\n");
			exit(1);
		}
		user_rand_pass(password, sizeof(password));
		user_set_pass(owner, password);
		user_set_flag_str(owner, NULL, "+n");
		printf("Your owner handle is '%s' and your password is '%s' (without the quotes).\n\n", handle, password);
		memset(password, 0, sizeof(password));
		str_redup(&core_config.owner, handle);
		core_config_save();
		user_save(core_config.userfile);
	}

	if (backgrd) bg_prepare_split();

	strlcpy(s, ctime(&now), sizeof s);
	strcpy(&s[11], &s[20]);
	putlog(LOG_ALL, "*", "Loading Eggdrop %s (%s)", VERSION, s);
	snprintf(pid_file, sizeof pid_file, "pid.%s", core_config.botname);

	f = fopen(pid_file, "r");
	if (f != NULL) {
		fgets(s, 10, f);
		xx = atoi(s);
		kill(xx, SIGCHLD); /* Meaningless kill to determine if pid is used */
		if (errno != ESRCH) {
			printf(_("I detect %s already running from this directory.\n"), core_config.botname);
			printf(_("If this is incorrect, please erase '%s'.\n"), pid_file);
			bg_send_quit(BG_ABORT);
			exit(1);
		}
	}

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

	if (backgrd) { /* Move into background? */
#ifndef CYGWIN_HACKS
		bg_do_split();
	}
	else {
#endif
		xx = getpid();
		if (xx != 0) {
			FILE *fp;

			unlink(pid_file);
			fp = fopen(pid_file, "w");
			if (fp != NULL) {
				fprintf(fp, "%u\n", xx);
				if (fflush(fp)) {
					/* Let the bot live since this doesn't appear to be a botchk */
					printf("WARNING: Could not write pid file '%s'.\n", pid_file);
					fclose(fp);
					unlink(pid_file);
				}
				else fclose(fp);
			}
			else printf("WARNING: Could not write pid file '%s'.\n", pid_file);
#ifdef CYGWIN_HACKS
			printf("Launched into the background  (pid: %d)\n\n", xx);
#endif
		}
	}

	use_stderr = 0; /* Stop writing to stderr now */

	if (backgrd) {
		/* Try to disassociate from controlling terminal (finger cross) */
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

	putlog(LOG_DEBUG, "*", "Entering main loop.");
	while (1) {
		timer_update_now(&egg_timeval_now);
		now = egg_timeval_now.sec;
		random(); /* Woop, lets really jumble things */
		timer_run();
		if (timer_get_shortest(&howlong)) timeout = 1000;
		else timeout = howlong.sec * 1000 + howlong.usec / 1000;
		sockbuf_update_all(timeout);
		garbage_run();
	}
}

/* main.c
 *
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

#ifndef lint
static const char rcsid[] = "$Id: main.c,v 1.182 2004/07/04 23:55:35 darko Exp $";
#endif

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <setjmp.h>
#include <locale.h>
#include <ctype.h>
#include <ltdl.h>

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

#include <eggdrop/eggdrop.h>
#include "lib/compat/compat.h"
#include "debug.h"
#include "core_config.h"
#include "core_party.h"
#include "core_binds.h"
#include "logfile.h"
#include "terminal.h"
#include "bg.h"
#include "main.h"

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

#define RUNMODE_NORMAL		1
#define RUNMODE_SHUTDOWN	2
#define RUNMODE_RESTART		3

int runmode = 1;	/* The most important variable ever!  	*/
int backgrd = 1;	/* Run in the background? 		*/
int make_userfile = 0;	/* Start bot in make-userfile mode? 	*/
int terminal_mode = 0;	/* Terminal mode			*/

/* XXX: remove this feature (but not yet) */
int debug_run = 0;	/* Just init and shutdown		*/

const char *configfile = NULL;	/* Name of the config file */
char pid_file[512];		/* Name of Eggdrop's pid file */

time_t online_since;		/* Time the bot was started */
time_t now;			/* Current time */
egg_timeval_t egg_timeval_now;	/* Current time in seconds and microseconds. */

int use_stderr = 1;		/* Send stuff to stderr instead of logfiles? */

char version[81];

static int lastmin = 99;
static struct tm nowtm;

void fatal(const char *errmsg)
{
	putlog(LOG_MISC, "*", "Fatal: %s", errmsg);
	flushlogs();
	exit(1);
}

static void got_bus(int z)
{
	fatal("BUS ERROR.");
}

static void got_segv(int z)
{
	fatal("SEGMENT VIOLATION.");
}

static void got_fpe(int z)
{
	fatal("FLOATING POINT ERROR.");
}

static void got_term(int z)
{
	eggdrop_event("sigterm");
	if (core_config.die_on_sigterm) core_shutdown(SHUTDOWN_SIGTERM, NULL, NULL);
	else putlog(LOG_MISC, "*", _("Received TERM signal (ignoring)."));
}

static void got_quit(int z)
{
	eggdrop_event("sigquit");
	putlog(LOG_MISC, "*", _("Received QUIT signal (ignoring)."));
	return;
}

static void got_hup(int z)
{
	eggdrop_event("sighup");
	putlog(LOG_MISC, "*", _("Received HUP signal (ignoring)."));
	return;
}

static void print_version(void)
{
	printf("%s\n", version);
}

static void print_help(char *const *argv)
{
	print_version();
	printf(_("\nUsage: %s [OPTIONS]... [FILE]\n"), argv[0]);
	printf(_("\n\
  -h, --help		   Print help and exit\n\
  -v, --version		Print version and exit\n\
  -n, --foreground	    Don't go into the background (default=off)\n\
  -c  --channel-stats	 (with -n) Display channel stats every 10 seconds\n\
  -t  --terminal		(with -n) Use terminal to simulate dcc-chat\n\
  -m  --make-userfile	 Userfile creation mode\n\
  -d  --debug		  Do a debugging dry-run\n\
  FILE  optional config filename (default 'config.xml')\n"));
	printf("\n");
}

static void do_args(int argc, char **argv)
{
	int c;

	optarg = 0;
	optind = 1;
	opterr = 1;
	optopt = '?';

	while (1) {
		int option_index = 0;

		static struct option long_options[] = {
			{ "help", 0, NULL, 'h' },
			{ "version", 0, NULL, 'v' },
			{ "load-module", 1, NULL, 'p' },
			{ "foreground", 0, NULL, 'n' },
			{ "terminal", 0, NULL, 't' },
			{ "make-userfile", 0, NULL, 'm' },
			{ "debug", 0, NULL, 'd' },
			{NULL, 0, NULL, 0}
		};

		c = getopt_long(argc, argv, "hvp:nctmd", long_options, &option_index);
		if (c == -1) break;
		switch (c) {
			case 'n':
				backgrd = 0;
				break;
			case 't':
				terminal_mode = 1;
				break;
			case 'm':
				make_userfile = 1;
				break;
			case 'd':
				backgrd = 0;
				debug_run = 1;
				break;
			case 'v':
				print_version();
				exit(0);
				break; /* this should never be reached */
			case 'h':
				print_help(argv);
				exit(0);
				break; /* this should never be reached */
			case 0: /* Long option with no short option */
			case '?': /* Invalid option. */
				/* getopt_long() already printed an error message. */
				exit(1);
			default: /* bug: option not considered.  */
				fprintf(stderr, _("%s: option unknown: %c\n"), argv[0], c);
				exit(1);
				break; /* this should never be reached */
		}
	}
	if (optind < argc) {
		configfile = argv[optind++];
		eggdrop_set_params(argv+optind, argc-optind);
	}
	else configfile = "config.xml";
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
		if (i > 1) putlog(LOG_MISC, "*", _("Warning: timer drift (%d minutes)."), i);

		miltime = (nowtm.tm_hour * 100) + (nowtm.tm_min);
		if (nowtm.tm_min % 5 == 0) { /* 5 minutes */
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

static int create_userfile(void)
{
	user_t *owner;
	char handle[512], password[12];
	int len;
	
	printf("\n");
	if (user_count() != 0) {
		printf("You are trying to create a new userfile, but the old one still exists (%s)!\n", core_config.userfile);
		printf("Please remove the userfile, or do not pass the -m option.\n\n");
		return(-1);
	}
	
	printf("Hello! I see you are creating a new userfile.\n\n");
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
		return(-1);
	}
	
	user_rand_pass(password, sizeof(password));
	user_set_pass(owner, password);
	user_set_flags_str(owner, NULL, "+n");
	printf("Your owner handle is '%s' and your password is '%s' (without the quotes).\n\n", handle, password);
	memset(password, 0, sizeof(password));
	str_redup(&core_config.owner, handle);
	core_config_save();	
	user_save(core_config.userfile);
	
	return(0);
}

static void init_signals(void)
{
	struct sigaction sv;


	sv.sa_handler = got_bus;
	sigemptyset(&sv.sa_mask);
#ifdef SA_RESETHAND
	sv.sa_flags = SA_RESETHAND;
#else
	sv.sa_flags = 0;
#endif
	sigaction(SIGBUS, &sv, NULL);

	sv.sa_handler = got_segv;
	//sigaction(SIGSEGV, &sv, NULL);
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
}

static void core_shutdown_or_restart()
{
	/* notifiy shutdown listeners */
	check_bind_shutdown();

	putlog(LOG_MISC, "*", _("Saving user file..."));
	user_save(core_config.userfile);

	putlog(LOG_MISC, "*", _("Saving config file..."));
	core_config_save();
	
	/* destroy terminal */
	if (terminal_mode && runmode != RUNMODE_RESTART)
		terminal_shutdown();

	/* unload core help */
	help_unload_by_module("core");

	/* shutdown core binds */
	core_binds_shutdown();

	/* shutdown logging */ 
	logfile_shutdown();
	
	/* shutdown libeggdrop */
	eggdrop_shutdown();

	/* force a garbage run */	
	garbage_run();

	/* just remove pid file if didn't restart */ 
	if (runmode != RUNMODE_RESTART)
		unlink(pid_file);
}

int main(int argc, char **argv)
{
	int timeout;
	
#ifdef DEBUG
	{
#  include <sys/resource.h>
		struct rlimit cdlim;

		cdlim.rlim_cur = RLIM_INFINITY;
		cdlim.rlim_max = RLIM_INFINITY;
		setrlimit(RLIMIT_CORE, &cdlim);
	}
#endif

	init_signals();
	
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

	timer_update_now(&egg_timeval_now);
	now = egg_timeval_now.sec;
	memcpy(&nowtm, localtime(&now), sizeof(struct tm));
	lastmin = nowtm.tm_min;

	srandom(now % (getpid() + getppid()));

	do_args(argc, argv);
	printf("\n%s\n", version);
	printf("WARNING: Do NOT run this DEVELOPMENT version for any purpose other than testing.\n\n");

	/* we may not run as root */
	if (((int) getuid() == 0) || ((int) geteuid() == 0)) {
		fprintf(stderr, "ERROR\n");
		fprintf(stderr, "\tEggdrop will not run as root!\n\n");
		return -1;
	}

	init_flag_map();

	/* config file may not be world read/writeable */
	if (file_check(configfile)) {
		fprintf(stderr, "ERROR\n");
		if (errno) {
			fprintf(stderr, "\tFailed to load config file '%s': %s\n\n", configfile, strerror(errno));
		} else {
			fprintf(stderr, "\tCheck file permissions on your config file '%s'!\n", configfile);
			fprintf(stderr, "\tMake sure other groups and users cannot read/write it.\n\n");
		}
		return -1;
	}
	
	/* set uptime */
	online_since = now;
	
	do {
		int tid;
		egg_timeval_t howlong;
		
		/* default select timeout is 1 sec */
		howlong.sec  = 1;
		howlong.usec = 0;

		tid = timer_create_repeater(&howlong, "main loop", core_secondly);

		/* init core */
		if (core_init() != 0)
			break;

		/* set normal running mode */
		runmode = RUNMODE_NORMAL;

		putlog(LOG_DEBUG, "*", "Entering main loop.");

		/* main loop */		
		while (!debug_run && runmode == RUNMODE_NORMAL) {
			/* Update the time. */
			timer_update_now(&egg_timeval_now);
			now = egg_timeval_now.sec;

			/* Run any expired timers. */
			timer_run();

			/* Figure out how long to wait for activity. */
			if (timer_get_shortest(&howlong)) timeout = 1000;
			else timeout = howlong.sec * 1000 + howlong.usec / 1000;

			/* Wait and monitor sockets. */
			sockbuf_update_all(timeout);

			/* Run any scheduled cleanups. */
			garbage_run();
		}

		/* shutdown secondly timer */
		timer_destroy(tid);

		/* Save user file, config file, ... */
		core_shutdown_or_restart();					

		/* the only chance to loop again is that running = 2, meaning
		   we have a restart */
	} while (runmode == RUNMODE_RESTART);

	if (debug_run)
		mem_dbg_stats();

	return 0;
}

int core_init()
{		
	int i;
	char *name;
	void *config_root, *entry;
	char datetime[25];
	static int lockfd = -1;
	struct flock lock;

	/* init libeggdrop */
	eggdrop_init();

	/* load config */	
	if (core_config_init(configfile) != 0)
		return (-1);

	/* init logging */
	logfile_init();	

	/* did the user specify -m? */
	if (make_userfile) {
		if (create_userfile()) return(-1);
		make_userfile = 0;
	}

	/* init background mode and pid file */
	if (runmode != RUNMODE_RESTART) {
		if (backgrd) bg_begin_split();
	}

	/* Check lock file. */
	if (core_config.lockfile && lockfd == -1) {
		lockfd = open(core_config.lockfile, O_WRONLY | O_CREAT, S_IWUSR);
		if (lockfd < 0) {
			fatal("Could not open lock file!");
		}
		memset(&lock, 0, sizeof(lock));
		lock.l_type = F_WRLCK;
		if (fcntl(lockfd, F_SETLK, &lock) != 0) {
			fatal("Lock file is already locked! Is eggdrop already running?");
		}
	}

	/* just issue this "Loading Eggdrop" message if we are not
	 * restarting */
	if (runmode != RUNMODE_RESTART) {	
		strlcpy(datetime, ctime(&now), sizeof(datetime));
		strcpy(&datetime[11], &datetime[20]);
		putlog(LOG_ALL, "*", "Loading Eggdrop %s (%s)", VERSION, datetime);
	}	

	/* load userlist */
	if (core_config.userfile)
		user_load(core_config.userfile);

	/* init core bindings */
	core_binds_init();

	/* init core partyline */
	core_party_init();

	/* Load core help */
	help_load_by_module ("core");

	/* Put the module directory in the ltdl search path. */
	if (core_config.module_path)
		module_add_dir(core_config.module_path);

	/* Scan the autoload section of config. */
	config_root = config_get_root("eggdrop");
	for (i = 0; (entry = config_exists(config_root, "eggdrop", 0, "autoload", 0, "module", i, NULL)); i++) {
		name = NULL;
		config_get_str(&name, entry, NULL);
		module_load(name);
	}
	for (i = 0; (entry = config_exists(config_root, "eggdrop", 0, "autoload", 0, "script", i, NULL)); i++) {
		name = NULL;
		config_get_str(&name, entry, NULL);
		script_load(name);
	}
		
	/* notify init listeners */
	check_bind_init();
	
	/* start terminal */
	if (runmode != RUNMODE_RESTART) {
		if (backgrd) bg_finish_split();
		if (terminal_mode) terminal_init();
	}

	return (0);
}

int core_restart(const char *nick)
{
	putlog(LOG_MISC, "*", "Restarting...");
	runmode = RUNMODE_RESTART;
	return (0);
}

int core_shutdown(int how, const char *nick, const char *reason)
{
	if (how == SHUTDOWN_SIGTERM)
		putlog(LOG_MISC, "*", ("Received TERM signal, shutting down."));	
	else if (how != SHUTDOWN_RESTART)
		putlog(LOG_MISC, "*", ("Shutdown requested by %s: %s"), nick,
				(reason) ? reason : _("No reason"));

	/* check how to shut down */
	switch (how) {

		case (SHUTDOWN_RESTART):
			runmode = RUNMODE_RESTART;
			break;
			
		case (SHUTDOWN_GRACEFULL):
			runmode = RUNMODE_SHUTDOWN;
			break;

		/* These two cases are treaded special because something
		 * unusual happened and we're better off if we exit() NOW 
		 * than doing a gracefull shutdown with freeing all
  		 * memory, closing all sockets, ... */
		case (SHUTDOWN_HARD):
		case (SHUTDOWN_SIGTERM):
			runmode = RUNMODE_SHUTDOWN;
			core_shutdown_or_restart();
			exit(0);
			break;
			
	}

	return BIND_RET_LOG;
}

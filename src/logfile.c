/* logfile.c: logging
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Eggheads Development Team
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
 *
 */

#ifndef lint
static const char rcsid[] = "$Id: logfile.c,v 1.48 2004/09/26 09:42:09 stdarg Exp $";
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <eggdrop/eggdrop.h>
#include "core_config.h"
#include "terminal.h"					/* TERMINAL_NICK	*/
#include "logfile.h"


static logfile_t *logfiles = NULL;
static int nlogfiles = 0;

extern int backgrd, use_stderr, terminal_mode;
int terminal_enabled = 0;
extern time_t now;

static int logfile_minutely();
static int logfile_5minutely();
static int logfile_cycle();
static void check_logsizes();
static void flushlog(logfile_t *log, char *timestamp);

/* Bind for the log table. The core does logging to files and the partyline, but
 * modules may implement sql logging or whatever. */
static int on_putlog(int flags, const char *chan, const char *text, int len);

static script_command_t log_script_cmds[] = {
	{"", "logfile", logfile_add, NULL, 3, "sss", "modes chan filename", SCRIPT_STRING, 0},	/* DDD	*/
	{0}
};

void logfile_init(void)
{
	void *root, *node;
	char *filename, *chname, *mask;
	int i;

	script_create_commands(log_script_cmds);
	bind_add_simple("log", NULL, NULL, on_putlog);
	bind_add_simple("event", NULL, "minutely", logfile_minutely);
	bind_add_simple("event", NULL, "5minutely", logfile_5minutely);

	root = config_get_root("eggdrop");
	node = config_lookup_section(root, "eggdrop.logging.logfiles", 0, NULL);
	for (i = 0; ; i++) {
		config_get_str(&filename, node, "logfile", i, "filename", 0, NULL);
		config_get_str(&chname, node, "logfile", i, "channel", 0, NULL);
		config_get_str(&mask, node, "logfile", i, "mask", 0, NULL);
		if (!filename || !chname || !mask) break;
		logfile_add(mask, chname, filename);
	}
}

void logfile_shutdown(void)
{
	void *root, *node;
	int i;

	flushlogs();
	
	root = config_get_root("eggdrop");
	node = config_lookup_section(root, "eggdrop.logging.logfiles", 0, NULL);
	for (i = 0; i < nlogfiles; i++) {
		config_set_str(logfiles[i].filename, node, "logfile", i, "filename", 0, NULL);
		config_set_str(logfiles[i].chname, node, "logfile", i, "channel", 0, NULL);
		config_set_str("*", node, "logfile", i, "mask", 0, NULL);
	}

	bind_rem_simple("log", NULL, NULL, on_putlog);
	bind_rem_simple("event", NULL, "minutely", logfile_minutely);
	bind_rem_simple("event", NULL, "5minutely", logfile_5minutely);
	script_delete_commands(log_script_cmds);
}

static int logfile_minutely()
{
	struct tm *nowtm;
	int miltime;

	if (core_config.logging.quick) {
		flushlogs();
		check_logsizes();
	}

	nowtm = localtime(&now);
	miltime = 100 * nowtm->tm_hour + nowtm->tm_min;

	if (miltime == core_config.logging.switch_at) logfile_cycle();

	return(0);
}

static int logfile_5minutely()
{
	if (!core_config.logging.quick) {
		flushlogs();
		check_logsizes();
	}
	return(0);
}

static int logfile_cycle()
{
	logfile_t *log;
	int i;
	char suffix[32];
	char *newfname;

	putlog(LOG_MISC, "*", _("Cycling logfiles..."));
	flushlogs();

	/* Determine suffix for cycled logfiles. */
	if (core_config.logging.keep_all) {
		strftime(suffix, 32, core_config.logging.suffix, localtime(&now));
	}

	for (i = nlogfiles - 1; i >= 0; i--) {
		log = &logfiles[i];

		fclose(log->fp);

		if (core_config.logging.keep_all) newfname = egg_mprintf("%s%s", log->filename, suffix);
		else newfname = egg_mprintf("%s.yesterday", log->filename);

		unlink(newfname);
		movefile(log->filename, newfname);
		free(newfname);

		log->fp = fopen(log->filename, "a");
		if (!log->fp) logfile_del(log->filename);
	}
	
	return(0);
}

char *logfile_add(char *modes, char *chan, char *fname)
{
	FILE *fp;
	logfile_t *log;

	/* Get rid of any duplicates. */
	logfile_del(fname);

	/* Test the filename. */
	fp = fopen(fname, "a");
	if (!fp) return("");
	
	logfiles = realloc(logfiles, (nlogfiles + 1) * sizeof(*logfiles));
			
	log = &logfiles[nlogfiles++];
	log->filename = strdup(fname);
	log->chname = strdup(chan);
	log->last_msg = strdup("");
	log->mask = LOG_ALL;
	log->fp = fp;

	return (log->filename);
}

int logfile_del(char *filename)
{
	logfile_t *log;
	int i;

	log = NULL;
	for (i = 0; i < nlogfiles; i++) {
		log = &logfiles[i];
		if (!strcmp(log->filename, filename)) break;
		log = NULL;
	}

	if (log == NULL) return(-1);
		
	if (log->fp) {
		flushlog(log, timer_get_timestamp());
		fclose(log->fp);
	}

	if (log->last_msg) free(log->last_msg);
	if (log->filename) free(log->filename);

	if (nlogfiles == 1) {
		free(logfiles);
		logfiles = NULL;
	} else {		
		memmove(logfiles + i, logfiles + i + 1, (nlogfiles - i - 1) * sizeof(logfile_t));
		logfiles = realloc(logfiles, (nlogfiles - 1) * sizeof(logfile_t));
	}

	nlogfiles--;

	return(0);
}

static int on_putlog(int flags, const char *chan, const char *text, int len)
{
	char *ts;
	int i;

	ts = timer_get_timestamp();
	for (i = nlogfiles - 1; i >= 0; i--) {
		logfile_t *log = &logfiles[i];
		
		/* If this log is disabled, skip it */
		if (log->state != LOG_STATE_ENABLED)
			continue;

		/* If this log doesn't match, skip it. */
		if (!(log->mask & flags)) {
			continue;
		}

		if (chan[0] != '*' && log->chname[0] != '*' && irccmp(chan, log->chname)) continue;
		
		/* If it's a repeat message, don't write it again. */
		if (log->last_msg && !strcasecmp(text, log->last_msg)) {
			log->repeats++;
			continue;
		}


		/* If there was a repeated message, write the count. */
		if (log->repeats) {
			fprintf(log->fp, "%s", ts);
			fprintf(log->fp, _("Last message repeated %d time(s).\n"), log->repeats);
			log->repeats = 0;
		}

		/* Save this msg to check for repeats next time. */
		str_redup(&log->last_msg, text);

		if (log->fp == NULL) {
			if (log->fname == NULL) {
				char buf[1024];
				time_t now;

				now = time(NULL);
				strftime(buf, sizeof(buf), log->filename, localtime(&now));
				log->fname = strdup(buf);
			}	

			log->fp = fopen(log->fname, "a+");
			if (log->fp == NULL) {
				log->state = LOG_STATE_DISABLED;
				putlog(LOG_MISC, "*", _("Failed to open log file: %s"), log->fname);
				putlog(LOG_MISC, "*", _("  Check if directory (if any) exists and is read- and writeable."));
				continue;
			}
		}

		/* Now output to the file. */
		fprintf(log->fp, "%s%s\n", ts, text);
	}

	if (!backgrd || use_stderr) {
	
		if (terminal_mode) {
			/* check if HQ is on console. If yes we disable
			 * output to stdout since otherwise everything would
			 * be printed out twice. */		
			if (!terminal_enabled) {
				terminal_enabled = (
					partymember_lookup_nick (TERMINAL_NICK) != NULL);
			}
			if (terminal_enabled)
				return 0;

		}
		
		fprintf (stdout, "%s %s%s\n", chan, ts, text);
	}
		
	return(0);
}

static void check_logsizes()
{
	int size, i;
	char *newfname;

	if (core_config.logging.keep_all || core_config.logging.max_size <= 0) return;

	for (i = 0; i < nlogfiles; i++) {
		logfile_t *log = &logfiles[i];
		
		size = ftell(log->fp) / 1024; /* Size in kilobytes. */
		if (size < core_config.logging.max_size) continue;

		/* It's too big. */
		putlog(LOG_MISC, "*", _("Cycling logfile %s: over max-logsize (%d kilobytes)."), log->filename, size);
		fclose(log->fp);

		newfname = egg_mprintf("%s.yesterday", log->filename);
		unlink(newfname);
		movefile(log->filename, newfname);
		free(newfname);
	}
}

static void flushlog(logfile_t *log, char *timestamp)
{
	if (log->repeats) {
		fprintf(log->fp, "%s", timestamp);
		fprintf(log->fp, _("Last message repeated %d time(s).\n"), log->repeats);
		log->repeats = 0;
		realloc_strcpy(log->last_msg, "");
	}
	fflush(log->fp);
}

/* Flush the logfiles to disk
 */
void flushlogs()
{
	char *ts;
	int i;

	ts = timer_get_timestamp();
	for (i = 0; i < nlogfiles; i++) {
		flushlog(&logfiles[i], ts);
	}
}

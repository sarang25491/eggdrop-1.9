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
static const char rcsid[] = "$Id: logfile.c,v 1.39 2004/06/15 11:54:33 wingman Exp $";
#endif

#include <eggdrop/eggdrop.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "core_config.h"
#include "logfile.h"

typedef struct log_b {
	struct log_b *next;
	char *filename;
	int mask;
	char *chname;
	char *last_msg;
	int repeats;
	int flags;
	FILE *fp;
} log_t;

extern int backgrd, use_stderr;
static int terminal_enabled = 0;
extern time_t now;

static log_t *log_list_head = NULL; /* Linked list of logfiles. */

static int logfile_minutely();
static int logfile_5minutely();
static int logfile_cycle();
static void check_logsizes();
static void flushlog(log_t *log, char *timestamp);

/* Bind for the log table. The core does logging to files and the partyline, but
 * modules may implement sql logging or whatever. */
static int on_putlog(int flags, const char *chan, const char *text, int len);

static script_command_t log_script_cmds[] = {
	{"", "logfile", logfile_add, NULL, 3, "sss", "modes chan filename", SCRIPT_STRING, 0},
	{0}
};

static bind_list_t log_binds[] = {
	{NULL, NULL, on_putlog},
	{0}
};

static bind_list_t log_events[] = {
	{NULL, "minutely", logfile_minutely},
	{NULL, "5minutely", logfile_5minutely},
	{0}
};

void logfile_init()
{
	script_create_commands(log_script_cmds);
	bind_add_list("log", log_binds);
	bind_add_list("event", log_events);
}

static int get_timestamp(char *t)
{
	time_t now2 = time(NULL);

	/* Calculate timestamp. */
	strftime(t, 32, "[%H:%M] ", localtime(&now2));
	return(0);
}

static int logfile_minutely()
{
	struct tm *nowtm;
	int miltime;

	if (core_config.quick_logs) {
		flushlogs();
		check_logsizes();
	}

	nowtm = localtime(&now);
	miltime = 100 * nowtm->tm_hour + nowtm->tm_min;

	if (miltime == core_config.switch_logfiles_at) logfile_cycle();

	return(0);
}

static int logfile_5minutely()
{
	if (!core_config.quick_logs) {
		flushlogs();
		check_logsizes();
	}
	return(0);
}

static int logfile_cycle()
{
	log_t *log, *prev;
	char suffix[32];
	char *newfname;

	putlog(LOG_MISC, "*", _("Cycling logfiles..."));
	flushlogs();

	/* Determine suffix for cycled logfiles. */
	if (core_config.keep_all_logs) {
		strftime(suffix, 32, core_config.logfile_suffix, localtime(&now));
	}

	prev = NULL;
	for (log = log_list_head; log; log = log->next) {
		fclose(log->fp);

		if (core_config.keep_all_logs) newfname = egg_mprintf("%s%s", log->filename, suffix);
		else newfname = egg_mprintf("%s.yesterday", log->filename);

		unlink(newfname);
		movefile(log->filename, newfname);
		free(newfname);

		log->fp = fopen(log->filename, "a");
		if (!log->fp) {
			logfile_del(log->filename);
			if (prev) log = prev;
			else log = log_list_head;
		}
		else prev = log;
	}
	return(0);
}

char *logfile_add(char *modes, char *chan, char *fname)
{
	FILE *fp;
	log_t *log;

	/* Get rid of any duplicates. */
	logfile_del(fname);

	/* Test the filename. */
	fp = fopen(fname, "a");
	if (!fp) return("");

	log = (log_t *)calloc(1, sizeof(*log));
	log->filename = strdup(fname);
	log->chname = strdup(chan);
	log->last_msg = strdup("");
	log->mask = LOG_ALL;
	log->fp = fp;

	log->next = log_list_head;
	log_list_head = log;

	return(log->filename);
}

int logfile_del(char *filename)
{
	log_t *log, *prev;
	char timestamp[32];

	prev = NULL;
	for (log = log_list_head; log; log = log->next) {
		if (!strcmp(log->filename, filename)) break;
		prev = log;
	}
	if (!log) return(1);
	if (prev) prev->next = log->next;
	else log_list_head = log->next;
	if (log->fp) {
		get_timestamp(timestamp);
		flushlog(log, timestamp);
		fclose(log->fp);
	}
	free(log->last_msg);
	free(log->filename);
	free(log);
	return(0);
}

static int on_putlog(int flags, const char *chan, const char *text, int len)
{
	log_t *log;
	char timestamp[32];

	get_timestamp(timestamp);
	for (log = log_list_head; log; log = log->next) {
		/* If this log doesn't match, skip it. */
		if (!(log->mask & flags)) continue;
		if (chan[0] != '*' && log->chname[0] != '*' && irccmp(chan, log->chname)) continue;

		
		/* If it's a repeat message, don't write it again. */
		if (!strcasecmp(text, log->last_msg)) {
			log->repeats++;
			continue;
		}

		/* If there was a repeated message, write the count. */
		if (log->repeats) {
			fprintf(log->fp, "%s", timestamp);
			fprintf(log->fp, _("Last message repeated %d time(s).\n"), log->repeats);
			log->repeats = 0;
		}

		/* Save this msg to check for repeats next time. */
		str_redup(&log->last_msg, text);

		/* Now output to the file. */
		fprintf(log->fp, "%s%s\n", timestamp, text);
	}

	if (!backgrd || use_stderr) {
	
		if (partyline_terminal_mode) {
			/* check if HQ is on console. If yes we disable
			 * output to stdout since otherwise everything would
			 * be printed out twice. */		
			if (!terminal_enabled) {
				terminal_enabled = (
					partymember_lookup_nick (PARTY_TERMINAL_NICK) != NULL);
			}
			if (terminal_enabled)
				return 0;

		}
		
		fprintf (stdout, "%s%s\n", timestamp, text);
	}
		
	return(0);
}

static void check_logsizes()
{
	int size;
	char *newfname;
	log_t *log;

	if (core_config.keep_all_logs || core_config.max_logsize <= 0) return;

	for (log = log_list_head; log; log = log->next) {
		size = ftell(log->fp) / 1024; /* Size in kilobytes. */
		if (size < core_config.max_logsize) continue;

		/* It's too big. */
		putlog(LOG_MISC, "*", _("Cycling logfile %s: over max-logsize (%d kilobytes)."), log->filename, size);
		fclose(log->fp);

		newfname = egg_mprintf("%s.yesterday", log->filename);
		unlink(newfname);
		movefile(log->filename, newfname);
		free(newfname);
	}
}

static void flushlog(log_t *log, char *timestamp)
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
	char timestamp[32];
	log_t *log;

	get_timestamp(timestamp);
	for (log = log_list_head; log; log = log->next) {
		flushlog(log, timestamp);
	}
}

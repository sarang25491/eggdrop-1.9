#include "main.h"

#include "modules.h" /* add_hook() */

#include "lib/egglib/msprintf.h"

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

extern int use_stderr; /* From main.c, while we're starting eggdrop. */
extern int term_z;
extern int backgrd;
extern int con_chan;
extern time_t now;

extern int dcc_total;
extern struct dcc_t *dcc;

static int cycle_at = 300; /* Military time where we cycle logfiles. */
static int keep_all_logs = 0; /* Keep all logs? */
static int max_logsize = 0; /* Max log size in kilobytes. */
static int quick_logs = 0; /* Check size more often? */
static char *logfile_suffix = NULL; /* Suffix for old logfiles. */

static log_t *log_list_head = NULL; /* Linked list of logfiles. */

static int logfile_minutely();
static int logfile_5minutely();
static int logfile_cycle();
static void check_logsizes();
static void flushlog(log_t *log, char *timestamp);

/* Functions for accessing the logfiles via scripts. */
static int script_putlog(void *cdata, char *text);
static int script_putloglev(char *level, char *chan, char *text);

static script_command_t log_script_cmds[] = {
	{"", "putlog", script_putlog, (void *)LOG_MISC, 1, "s", "text", SCRIPT_INTEGER, SCRIPT_PASS_CDATA},
	{"", "putcmdlog", script_putlog, (void *)LOG_CMDS, 1, "s", "text", SCRIPT_INTEGER, SCRIPT_PASS_CDATA},
	{"", "putxferlog", script_putlog, (void *)LOG_FILES, 1, "s", "text", SCRIPT_INTEGER, SCRIPT_PASS_CDATA},
	{"", "putloglev", script_putloglev, NULL, 3, "sss", "level channel text", SCRIPT_INTEGER, 0},
	{"", "logfile", (Function) logfile_add, NULL, 3, "sss", "modes channel filename", SCRIPT_STRING, 0},
	{"", "stoplog", logfile_del, NULL, 1, "s", "filename", SCRIPT_INTEGER, 0},
	{0}
};

static script_str_t log_script_strings[] = {
	{"", "logfile-suffix", &logfile_suffix},
	{0}
};

static script_int_t log_script_ints[] = {
	{"", "max-logsize", &max_logsize},
	{"", "switch-logfiles-at", &cycle_at},
	{"", "keep-all-logs", &keep_all_logs},
	{"", "quick-logs", &quick_logs},
	{0}
};

void logfile_init()
{
	logfile_suffix = strdup(".%d%b%Y");
	script_create_cmd_table(log_script_cmds);
	script_link_int_table(log_script_ints);
	script_link_str_table(log_script_strings);
	add_hook(HOOK_MINUTELY, logfile_minutely);
	add_hook(HOOK_5MINUTELY, logfile_5minutely);
}

static int get_timestamp(char *t)
{
	/* Calculate timestamp. */
	strftime(t, 32, "[%H:%M] ", localtime(&now));
	return(0);
}

static int logfile_minutely()
{
	struct tm *nowtm;
	int miltime;

	if (quick_logs) {
		flushlogs();
		check_logsizes();
	}

	nowtm = localtime(&now);
	miltime = 100 * nowtm->tm_hour + nowtm->tm_min;

	if (miltime == cycle_at) logfile_cycle();

	return(0);
}

static int logfile_5minutely()
{
	if (!quick_logs) {
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

	putlog(LOG_MISC, "*", _("Cycling logfiles"));
	flushlogs();

	/* Determine suffix for cycled logfiles. */
	if (keep_all_logs) {
		strftime(suffix, 32, logfile_suffix, localtime(&now));
	}

	prev = NULL;
	for (log = log_list_head; log; log = log->next) {
		fclose(log->fp);

		if (keep_all_logs) newfname = msprintf("%s%s", log->filename, suffix);
		else newfname = msprintf("%s.yesterday", log->filename);

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

static int script_putlog(void *cdata, char *text)
{
	return putlog((int) cdata, "*", "%s", text);
}

static int script_putloglev(char *level, char *chan, char *text)
{
	int lev = 0;

	lev = logmodes(level);
	if (!lev) return(-1);
	return putlog(lev, chan, "%s", text);
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
	log->mask = logmodes(modes);
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

/* Log something
 * putlog(level,channel_name,format,...);
 */
int putlog EGG_VARARGS_DEF(int, arg1)
{
  int i, type, len;
  log_t *log;
  char *format, *chname, *out;
  char timestamp[32];
  va_list va;


	len = 128;
	out = (char *)malloc(len);
	while (1) {
		type = EGG_VARARGS_START(int, arg1, va);
		chname = va_arg(va, char *);
		format = va_arg(va, char *);
		i = vsnprintf(out, len, format, va);
		if (i > -1 && i < len) break; /* Done. */
		if (i > len) len = i+1; /* Exact amount. */
		else len *= 2; /* Just guessing. */
		out = (char *)realloc(out, len);
	}
	len = i;

  va_end(va);

	get_timestamp(timestamp);
	for (log = log_list_head; log; log = log->next) {
		/* If this log doesn't match, skip it. */
		if (!(log->mask & type)) continue;
		if (chname[0] != '*' && log->chname[0] != '*' && irccmp(chname, log->chname)) continue;

		/* If it's a repeat message, don't write it again. */
		if (!strcasecmp(out, log->last_msg)) {
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
		realloc_strcpy(log->last_msg, out);

		/* Now output to the file. */
		fprintf(log->fp, "%s%s\n", timestamp, out);
	}

  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_CHAT) && (dcc[i].u.chat->con_flags & type)) {
      if ((chname[0] == '*') || (dcc[i].u.chat->con_chan[0] == '*') ||
	  (!irccmp(chname, dcc[i].u.chat->con_chan)))
	dprintf(i, "%s%s\n", timestamp, out);
    }
  if ((!backgrd) && (!con_chan) && (!term_z))
    dprintf(DP_STDOUT, "%s%s\n", timestamp, out);
  else if ((type & LOG_MISC) && use_stderr) {
    dprintf(DP_STDERR, "%s%s\n", timestamp, out);
  }

  free(out);
  return(len);
}

static void check_logsizes()
{
	int size;
	char *newfname;
	log_t *log;

	if (keep_all_logs || max_logsize <= 0) return;

	for (log = log_list_head; log; log = log->next) {
		size = ftell(log->fp) / 1024; /* Size in kilobytes. */
		if (size < max_logsize) continue;

		/* It's too big. */
		putlog(LOG_MISC, "*", _("Cycling logfile %s, over max-logsize (%d kilobytes)"), log->filename, size);
		fflush(log->fp);
		fclose(log->fp);

		newfname = msprintf("%s.yesterday", log->filename);
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

#include <stdarg.h>
#include <eggdrop/eggdrop.h>

/* When does pid wrap around? This lets pids get up to 99999. */
#define PID_WRAPAROUND	100000

/* Flags for partyline members. */
#define PARTY_ECHO	1
#define PARTY_DELETED	2

static char *partyline_command_chars = NULL;

static bind_table_t *BT_cmd = NULL;	/* Commands. */
static bind_table_t *BT_party_out = NULL;	/* Text sent to a user. */

static int on_putlog(int flags, const char *chan, const char *text, int len);
static int partymember_cleanup(void *client_data);

extern int partychan_init();
extern int partymember_init();

static bind_list_t log_binds[] = {
	//{"", on_putlog},
	{0}
};

int partyline_init()
{
	partychan_init();
	partymember_init();
	str_redup(&partyline_command_chars, "./");
	BT_cmd = bind_table_add("party", 5, "PsUss", MATCH_PARTIAL, 0);
	BT_party_out = bind_table_add("party_out", 5, "isUsi", MATCH_NONE, 0);
	bind_add_list("log", log_binds);
	return(0);
}

/* 1 if the text starts with a recognized command character or 0 if not */
int partyline_is_command(const char *text)
{
	return strchr(partyline_command_chars, *text) ? 1 : 0;
}

/* Process any input from someone on the partyline (not bots, just users).
 * Everything is either a command or public text. Private messages and all that
 * are handled via commands. */
int partyline_on_input(partychan_t *chan, partymember_t *p, const char *text, int len)
{
	if (!p || p->flags & PARTY_DELETED || !len) return(0);
	if (!chan) chan = p->channels[0];

	/* See if we should interpret it as a command. */
	if (strchr(partyline_command_chars, *text)) {
		text++;
		len--;
		/* If there are 2 command chars in a row, then it's not a
		 * command. We skip the command char and treat it as text. */
		if (*text != *(text-1)) {
			const char *space;

			space = strchr(text, ' ');
			if (space) {
				char *cmd;

				len = space-text;
				cmd = malloc(len+1);
				memcpy(cmd, text, len);
				cmd[len] = 0;
				partyline_on_command(chan, p, cmd, space+1);
				free(cmd);
			}
			else {
				partyline_on_command(chan, p, text, NULL);
			}
			return(0);
		}
	}
	/* It's not a command, so it's public text. */
	partychan_msg(chan, p, text, len);
	return(0);
}

/* Processes input as a command. The first word is the command, and the rest
 * is the argument. */
int partyline_on_command(partychan_t *chan, partymember_t *p, const char *cmd, const char *text)
{
	int r;

	if (!chan || !p) return(-1);

	r = bind_check(BT_cmd, cmd, p, p->nick, p->user, cmd, text);
	return(r);
}

int partyline_update_info(partymember_t *p, const char *ident, const char *host)
{
	if (!p) return(-1);
	if (ident) str_redup(&p->ident, ident);
	if (host) str_redup(&p->host, host);
	return(0);
}

#if 0
int partyline_write_all_but(int pid, const char *text, int len)
{
	partymember_t *p;

	if (len < 0) len = strlen(text);
	for (p = partyhead; p; p = p->next) {
		if (p->pid != pid) partyline_write_internal(p, text, len);
	}
	return(0);
}

int partyline_printf(int pid, const char *fmt, ...)
{
	va_list args;
	char *ptr, buf[1024];
	int len;

	va_start(args, fmt);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, fmt, args);
	va_end(args);

	partyline_write(pid, ptr, len);

	if (ptr != buf) free(ptr);
	return(0);
}

int partyline_printf_chan_but(int cid, int pid, const char *fmt, ...)
{
	va_list args;
	char *ptr, buf[1024];
	int len;

	va_start(args, fmt);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, fmt, args);
	va_end(args);

	partyline_write_chan_but(cid, pid, ptr, len);

	if (ptr != buf) free(ptr);
	return(0);
}

int partyline_printf_all_but(int pid, const char *fmt, ...)
{
	va_list args;
	char *ptr, buf[1024];
	int len;

	va_start(args, fmt);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, fmt, args);
	va_end(args);

	partyline_write_all_but(pid, ptr, len);

	if (ptr != buf) free(ptr);
	return(0);
}

/* Logging stuff. */
static int on_putlog(int flags, const char *chan, const char *text, int len)
{
	char *ptr, buf[1024];
	int buflen;
	partymember_t *p;

	ptr = egg_msprintf(buf, sizeof(buf), &buflen, "%s\n", text);
	partyline_write_all_but(-1, ptr, buflen);
	if (ptr != buf) free(ptr);
	return(0);
}
#endif

#include <eggdrop/eggdrop.h>
#include "ircparty.h"

static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len);
static int on_nick(void *client_data, partymember_t *src, const char *oldnick, const char *newnick);
static int on_quit(void *client_data, partymember_t *src, const char *text, int len);
static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
static int on_join(void *client_data, partychan_t *chan, partymember_t *src);
static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);

partyline_event_t irc_party_handler = {
	on_privmsg,
	on_nick,
	on_quit,
	on_chanmsg,
	on_join,
	on_part
};

static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len)
{
	irc_session_t *session = client_data;

	if (src) egg_iprintf(session->idx, ":%s!%s@%s PRIVMSG %s :%s\r\n", src->nick, src->ident, src->host, dest->nick, text);
	else egg_iprintf(session->idx, "NOTICE %s\r\n", text);
	return(0);
}

static int on_nick(void *client_data, partymember_t *src, const char *oldnick, const char *newnick)
{
	irc_session_t *session = client_data;

	egg_iprintf(session->idx, ":%s NICK %s\n", oldnick, newnick);
	return(0);
}

static int on_quit(void *client_data, partymember_t *src, const char *text, int len)
{
	irc_session_t *session = client_data;

	egg_iprintf(session->idx, ":%s!%s@%s QUIT :%s\n", src->nick, src->ident, src->host, text);
	if (src == session->party) sockbuf_delete(session->idx);

	return(0);
}

static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	irc_session_t *session = client_data;

	if (src) {
		if (src != session->party) egg_iprintf(session->idx, ":%s!%s@%s PRIVMSG #%s :%s\r\n", src->nick, src->ident, src->host, chan->name, text);
	}
	else egg_iprintf(session->idx, ":* PRIVMSG #%s :%s\r\n", chan->name, text);
	return(0);
}

static int on_join(void *client_data, partychan_t *chan, partymember_t *src)
{
	irc_session_t *session = client_data;

	egg_iprintf(session->idx, ":%s!%s@%s JOIN #%s\r\n", src->nick, src->ident, src->host, chan->name);
	return(0);
}

static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	irc_session_t *session = client_data;

	egg_iprintf(session->idx, ":%s!%s@%s PART #%s :%s\r\n", src->nick, src->ident, src->host, chan->name, text);
	return(0);
}

#include <eggdrop/eggdrop.h>
#include "telnetparty.h"

static int on_write(void *client_data, partymember_t *dest, const char *text, int len);
static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len);
static int on_chanwrite(void *client_data, partychan_t *chan, const char *text, int len);
static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
static int on_join(void *client_data, partychan_t *chan, partymember_t *src);
static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len);
static int on_delete(void *client_data);

partyline_event_t telnet_party_handler = {
	on_write,
	on_privmsg,
	on_chanwrite,
	on_chanmsg,
	on_join, on_part,
	on_delete
};

static int on_write(void *client_data, partymember_t *dest, const char *text, int len)
{
	telnet_session_t *session = client_data;

	egg_iprintf(session->idx, "%s\r\n", text);
	return(0);
}

static int on_privmsg(void *client_data, partymember_t *dest, partymember_t *src, const char *text, int len)
{
	telnet_session_t *session = client_data;

	egg_iprintf(session->idx, "[%s] %s\r\n", src->nick, text);
	return(0);
}

static int on_chanwrite(void *client_data, partychan_t *chan, const char *text, int len)
{
	telnet_session_t *session = client_data;

	egg_iprintf(session->idx, "%s %s\r\n", chan->name, text);
	return(0);
}

static int on_chanmsg(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	telnet_session_t *session = client_data;

	egg_iprintf(session->idx, "%s <%s> %s\r\n", chan->name, src->nick, text);
	return(0);
}

static int on_join(void *client_data, partychan_t *chan, partymember_t *src)
{
	telnet_session_t *session = client_data;

	egg_iprintf(session->idx, "%s %s (%s@%s) has joined the channel\r\n", chan->name, src->nick, src->ident, src->host);
	return(0);
}

static int on_part(void *client_data, partychan_t *chan, partymember_t *src, const char *text, int len)
{
	telnet_session_t *session = client_data;

	/* If we're the ones leaving, delete our channel handler too. */
	if (session->party == src) {
		partychan_part_handler(chan, &telnet_party_handler, client_data);
	}

	egg_iprintf(session->idx, "%s %s (%s@%s) has left the channel: %s\r\n", chan->name, src->nick, src->ident, src->host, text);
	return(0);
}

static int on_delete(void *client_data)
{
	telnet_session_t *session = client_data;

	sockbuf_delete(session->idx);
	return(0);
}

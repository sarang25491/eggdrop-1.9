#include "server.h"

static int match_botnick(const char *nick)
{
	return current_server.nick && !(current_server.strcmp)(current_server.nick, nick);
}

void *server_get_api()
{
	static egg_server_api_t api;

	api.major = EGG_SERVER_API_MAJOR;
	api.minor = EGG_SERVER_API_MINOR;

	/* General stuff. */
	api.match_botnick = match_botnick;

	/* Output functions. */
	api.printserv = printserv;
	api.queue_append = queue_append;
	api.queue_unlink = queue_unlink;
	api.queue_entry_from_text = queue_entry_from_text;
	api.queue_entry_to_text = queue_entry_to_text;
	api.queue_entry_cleanup = queue_entry_cleanup;
	api.queue_get_by_priority = queue_get_by_priority;
	api.dequeue_messages = dequeue_messages;

	/* Channel functions. */
	api.channel_lookup = channel_lookup;
	api.channel_add = channel_add;
	api.channel_set = channel_set;
	api.channel_get = channel_get;
	api.channel_get_int = channel_get_int;

	return(&api);
}

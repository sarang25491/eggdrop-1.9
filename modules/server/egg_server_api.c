#include "server.h"

void *server_get_api()
{
	static egg_server_api_t api;

	api.major = EGG_SERVER_API_MAJOR;
	api.minor = EGG_SERVER_API_MINOR;

	/* Output functions. */
	api.printserv = printserv;
	api.queue_append = queue_append;
	api.queue_unlink = queue_unlink;
	api.queue_entry_from_text = queue_entry_from_text;
	api.queue_entry_to_text = queue_entry_to_text;
	api.queue_entry_cleanup = queue_entry_cleanup;
	api.queue_get_by_priority = queue_get_by_priority;
	api.dequeue_messages = dequeue_messages;
#if 0
	/* Channel functions. */
	int (*channel_list)(const char ***chans);
	int (*channel_list_members)(const char *chan, const char ***members);
} egg_server_api_t;

#endif
	return(&api);
}

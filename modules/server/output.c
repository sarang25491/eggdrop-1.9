#define MAKING_SERVER
#include "lib/eggdrop/module.h"
#include "server.h"
#include "output.h"

extern eggdrop_t *egg;
extern current_server_t current_server;

/* Sends formatted output to the currently connected server. */
int printserv(int priority, const char *format, ...)
{
	char buf[512], *ptr;
	int len;
	va_list args;

	va_start(args, format);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, format, args);
	va_end(args);

	if (len > 510) {
		ptr[508] = '\r';
		ptr[509] = '\n';
		ptr[510] = 0;
		len = 510;
	}

//	if (priority == SERVER_NOQUEUE) {
		sockbuf_write(current_server.idx, ptr, len);
		putlog(LOG_MISC, "*", "sent '%s' to server", ptr);
		if (len > 0 && ptr[len-1] != '\n') {
			buf[0] = '\r';
			buf[1] = '\n';
			sockbuf_write(current_server.idx, buf, 2);
		}
/*	}
	else {
		queue_server(priority, ptr, len);
	}
	*/
	if (ptr != buf) free(ptr);
	return(len);
}

void queue_server(int priority, char *text, int len)
{
	printserv(SERVER_NOQUEUE, "%s", text);
}

void dequeue_messages()
{
}

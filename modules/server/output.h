#ifndef _OUTPUT_H_
#define _OUTPUT_H_

/* Server output priorities. */
#define SERVER_NOQUEUE	1
#define SERVER_MODE	2
#define SERVER_NORMAL	3
#define SERVER_HELP	4
#define SERVER_NEXT	8

int printserv(int priority, const char *format, ...);
void queue_server(int priority, char *text, int len);
void dequeue_messages();

#endif

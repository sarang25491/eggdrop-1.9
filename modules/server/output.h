#ifndef _OUTPUT_H_
#define _OUTPUT_H_

/* Server output priorities. */
#define SERVER_NOQUEUE	1
#define SERVER_QUICK	2
#define SERVER_NORMAL	3
#define SERVER_SLOW	4
#define SERVER_NEXT	8

int printserv(int priority, const char *format, ...);
void dequeue_messages();

#endif

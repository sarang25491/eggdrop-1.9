#ifndef _OUTPUT_H_
#define _OUTPUT_H_

/* Server output priorities. */
#define SERVER_NOQUEUE	1
#define SERVER_QUICK	2
#define SERVER_NORMAL	3
#define SERVER_SLOW	4
#define SERVER_NEXT	8

typedef struct queue_entry {
	struct queue_entry *next, *prev;
	int id;
	irc_msg_t msg;
} queue_entry_t;

typedef struct {
	queue_entry_t *queue_head, *queue_tail;
	int len;
	int next_id;
} queue_t;

int printserv(int priority, const char *format, ...);
queue_entry_t *queue_new(char *text);
void queue_append(queue_t *queue, queue_entry_t *q);
void queue_unlink(queue_t *queue, queue_entry_t *q);
void queue_entry_from_text(queue_entry_t *q, char *text);
void queue_entry_to_text(queue_entry_t *q, char *text, int *remaining);
void queue_entry_cleanup(queue_entry_t *q);
queue_t *queue_get_by_priority(int priority);

void dequeue_messages();

#endif

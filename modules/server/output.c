#if 0
#define MAKING_SERVER
#include "lib/eggdrop/module.h"
#include "server.h"
#include "output.h"
#endif
#include <eggdrop/eggdrop.h>
#include "server.h"
#include "parse.h"
#include "output.h"

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

/* We have 6 queues, one for each combo of priority and 'next'. */
static queue_t output_queues[6] = {{0}, {0}, {0}, {0}, {0}, {0}};

/* And one to store unused queue entries. */
static queue_t unused_queue = {0};

static void queue_server(int priority, char *text, int len);

/* Sends formatted output to the currently connected server. */
int printserv(int priority, const char *format, ...)
{
	char buf[1024], *ptr;
	int len;
	va_list args;

	va_start(args, format);
	ptr = egg_mvsprintf(buf, sizeof(buf), &len, format, args);
	va_end(args);

	if (len > 510) len = 510;

	if (len < 2 || ptr[len-1] != '\n') {
		ptr[len++] = '\r';
		ptr[len++] = '\n';
	}
	ptr[len] = 0;

	if ((priority & (~SERVER_NEXT)) == SERVER_NOQUEUE) {
		sockbuf_write(current_server.idx, ptr, len);
	}
	else {
		queue_server(priority, ptr, len);
	}

	if (ptr != buf) free(ptr);
	return(len);
}

static void queue_append(queue_t *queue, queue_entry_t *q)
{
	q->next = NULL;
	q->prev = queue->queue_tail;
	if (queue->queue_tail) queue->queue_tail->next = q;
	else queue->queue_head = q;
	queue->queue_tail = q;

	queue->len++;
}

static void queue_unlink(queue_t *queue, queue_entry_t *q)
{
	if (q->next) q->next->prev = q->prev;
	else queue->queue_tail = q->prev;

	if (q->prev) q->prev->next = q->next;
	else queue->queue_head = q->next;

	queue->len--;
}

static void queue_server(int priority, char *text, int len)
{
	queue_t *queue;
	queue_entry_t *q;
	int i, qnum;
	irc_msg_t *msg;

	qnum = priority & (~SERVER_NEXT);
	if (qnum < SERVER_QUICK || qnum > SERVER_SLOW) qnum = SERVER_SLOW;
	qnum -= SERVER_QUICK;
	if (priority & SERVER_NEXT) qnum += 3;
	queue = output_queues+qnum;

	if (unused_queue.queue_head) {
		q = unused_queue.queue_head;
		queue_unlink(&unused_queue, q);
	}
	else {
		q = calloc(1, sizeof(*q));
	}

	/* Parse the irc message. */
	msg = &q->msg;
	irc_msg_parse(text, msg);

	/* Copy each item ('text' param belongs to caller). */
	if (msg->prefix) msg->prefix = strdup(msg->prefix);
	if (msg->cmd) msg->cmd = strdup(msg->cmd);
	for (i = 0; i < msg->nargs; i++) {
		msg->args[i] = strdup(msg->args[i]);
	}

	/* Ok, put it in the queue. */
	q->id = queue->next_id++;
	queue_append(queue, q);
}

static void append_str(char **dest, int *remaining, const char *src)
{
	int len;

	if (!src || *remaining <= 0) return;

	len = strlen(src);
	if (len > *remaining) len = *remaining;

	memcpy(*dest, src, len);
	*remaining -= len;
	*dest += len;
}

void dequeue_messages()
{
	queue_entry_t *q;
	queue_t *queue = NULL;
	irc_msg_t *msg;
	int i, remaining, len;
	char *text, buf[1024];

	/* Choose which queue to select from. */
	for (i = 0; i < 3; i++) {
		if (output_queues[i+3].queue_head) {
			queue = output_queues+i+3;
			break;
		}
		if (output_queues[i].queue_head) {
			queue = output_queues+i;
			break;
		}
	}

	/* No messages to dequeue? */
	if (!queue) return;

	q = queue->queue_head;

	/* Remove it from the queue. */
	queue_unlink(queue, q);

	/* Construct an irc message out of it. */
	remaining = sizeof(buf);
	text = buf;
	msg = &q->msg;
	if (msg->prefix) {
		append_str(&text, &remaining, msg->prefix);
		append_str(&text, &remaining, " ");
		free(msg->prefix);
	}
	if (msg->cmd) {
		append_str(&text, &remaining, msg->cmd);
		free(msg->cmd);
	}

	/* Add the args (except for last one). */
	msg->nargs--;
	for (i = 0; i < msg->nargs; i++) {
		append_str(&text, &remaining, " ");
		append_str(&text, &remaining, msg->args[i]);
		free(msg->args[i]);
	}
	msg->nargs++;

	/* If the last arg has a space in it, put a : before it. */
	if (i < msg->nargs) {
		if (strchr(msg->args[i], ' ')) append_str(&text, &remaining, " :");
		else append_str(&text, &remaining, " ");
		append_str(&text, &remaining, msg->args[i]);
		free(msg->args[i]);
	}

	/* Now we're done with q, so free anything we need to free and store it
	 * in the unused queue. */
	irc_msg_cleanup(msg);
	queue_append(&unused_queue, q);

	/* Make sure last 2 chars are \r\n. */
	if (remaining < 2) remaining = 2;
	len = sizeof(buf) - remaining;
	buf[len++] = '\r';
	buf[len++] = '\n';
	sockbuf_write(current_server.idx, buf, len);
}

queue_entry_t *queue_lookup(int priority, int num)
{
}

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
#include "binds.h"

char *global_output_string = NULL;

/* We have 6 queues, one for each combo of priority and 'next'. */
static queue_t output_queues[6] = {{0}, {0}, {0}, {0}, {0}, {0}};

/* And one to store unused queue entries. */
static queue_t unused_queue = {0};

static void queue_server(int priority, char *text, int len);

/* Little utility function to help append to a fixed buffer. */
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

static void do_output(const char *text, int len)
{
	int r;

	if (global_output_string) {
		free(global_output_string);
		global_output_string = NULL;
	}
	r = bind_check(BT_server_output, text, text);
	if (r & BIND_RET_BREAK) return;

	if (global_output_string) {
		len = strlen(global_output_string);
		if (len > 0) sockbuf_write(current_server.idx, global_output_string, len);
	}
	else sockbuf_write(current_server.idx, text, len);
}

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
		do_output(ptr, len);
	}
	else {
		queue_server(priority, ptr, len);
	}

	if (ptr != buf) free(ptr);
	return(len);
}

queue_entry_t *queue_new(char *text)
{
	queue_entry_t *q;

	/* Either get one from the unused_queue, or allocate a new one. */
	if (unused_queue.queue_head) {
		q = unused_queue.queue_head;
		queue_unlink(&unused_queue, q);
	}
	else {
		q = calloc(1, sizeof(*q));
	}

	/* Parse the text into the queue entry. */
	queue_entry_from_text(q, text);

	return(q);
}

void queue_append(queue_t *queue, queue_entry_t *q)
{
	q->next = NULL;
	q->prev = queue->queue_tail;
	if (queue->queue_tail) queue->queue_tail->next = q;
	else queue->queue_head = q;
	queue->queue_tail = q;

	queue->len++;
}

void queue_unlink(queue_t *queue, queue_entry_t *q)
{
	if (q->next) q->next->prev = q->prev;
	else queue->queue_tail = q->prev;

	if (q->prev) q->prev->next = q->next;
	else queue->queue_head = q->next;

	queue->len--;
}

void queue_entry_from_text(queue_entry_t *q, char *text)
{
	irc_msg_t *msg;
	int i;

	/* Parse the irc message. */
	msg = &q->msg;
	irc_msg_parse(text, msg);

	/* Copy each item ('text' param belongs to caller). */
	if (msg->prefix) msg->prefix = strdup(msg->prefix);
	if (msg->cmd) msg->cmd = strdup(msg->cmd);
	for (i = 0; i < msg->nargs; i++) {
		msg->args[i] = strdup(msg->args[i]);
	}
}

void queue_entry_cleanup(queue_entry_t *q)
{
	irc_msg_t *msg;
	int i;

	msg = &q->msg;
	if (msg->prefix) free(msg->prefix);
	if (msg->cmd) free(msg->cmd);

	for (i = 0; i < msg->nargs; i++) {
		free(msg->args[i]);
	}
	memset(msg, 0, sizeof(*msg));
}

void queue_entry_to_text(queue_entry_t *q, char *text, int *remaining)
{
	irc_msg_t *msg;
	int i;

	msg = &q->msg;
	if (msg->prefix) {
		append_str(&text, remaining, msg->prefix);
		append_str(&text, remaining, " ");
	}
	if (msg->cmd) {
		append_str(&text, remaining, msg->cmd);
	}

	/* Add the args (except for last one). */
	msg->nargs--;
	for (i = 0; i < msg->nargs; i++) {
		append_str(&text, remaining, " ");
		append_str(&text, remaining, msg->args[i]);
	}
	msg->nargs++;

	/* If the last arg has a space in it, put a : before it. */
	if (i < msg->nargs) {
		if (strchr(msg->args[i], ' ')) append_str(&text, remaining, " :");
		else append_str(&text, remaining, " ");
		append_str(&text, remaining, msg->args[i]);
	}
}

queue_t *queue_get_by_priority(int priority)
{
	int qnum;

	qnum = priority & (~SERVER_NEXT);
	if (qnum < SERVER_QUICK || qnum > SERVER_SLOW) qnum = SERVER_SLOW;
	qnum -= SERVER_QUICK;
	if (priority & SERVER_NEXT) qnum += 3;

	return output_queues+qnum;
}

static void queue_server(int priority, char *text, int len)
{
	queue_t *queue;
	queue_entry_t *q;

	queue = queue_get_by_priority(priority);

	q = queue_new(text);

	/* Ok, put it in the queue. */
	q->id = queue->next_id++;
	queue_append(queue, q);
}

void dequeue_messages()
{
	queue_entry_t *q;
	queue_t *queue = NULL;
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
	text = buf;
	remaining = sizeof(buf);
	queue_entry_to_text(q, text, &remaining);

	/* Now we're done with q, so free anything we need to free and store it
	 * in the unused queue. */
	queue_entry_cleanup(q);
	queue_append(&unused_queue, q);

	/* Make sure last 2 chars are \r\n and there's a space left for \0. */
	if (remaining < 3) remaining = 3;
	len = sizeof(buf) - remaining;
	buf[len++] = '\r';
	buf[len++] = '\n';
	buf[len] = 0;

	do_output(buf, len);
}

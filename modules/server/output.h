#ifndef _OUTPUT_H_
#define _OUTPUT_H_

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

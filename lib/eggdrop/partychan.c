#include <stdarg.h>
#include <eggdrop/eggdrop.h>

/* When does cid wrap around? This lets cids get up to 99999. */
#define CID_WRAPAROUND	100000

/* Flags for partyline members. */
#define PARTY_DELETED	1

static hash_table_t *cid_ht = NULL;
static partychan_t *partychan_head = NULL;
static int g_cid = 0;	/* Keep track of next available cid. */

int partychan_init()
{
	cid_ht = hash_table_create(NULL, NULL, 13, HASH_TABLE_INTS);
	return(0);
}

int partychan_get_cid()
{
	int cid;

	while (partychan_lookup_cid(g_cid)) {
		g_cid++;
		if (g_cid >= CID_WRAPAROUND) g_cid = 0;
	}
	cid = g_cid++;
	if (g_cid >= CID_WRAPAROUND) g_cid = 0;
	return(cid);
}

partychan_t *partychan_new(int cid, const char *name)
{
	partychan_t *chan;

	chan = calloc(1, sizeof(*chan));
	if (cid == -1) cid = partychan_get_cid();
	chan->cid = cid;
	chan->name = strdup(name);
	chan->next = partychan_head;
	if (partychan_head) partychan_head->prev = chan;
	partychan_head = chan;
	return(chan);
}

int partychan_delete(partychan_t *chan)
{
	return(0);
}

static int partychan_cleanup(partychan_t *chan)
{
	int i;
	int dirty = 0;

	for (i = 0; i < chan->nmembers; i++) {
		if (chan->members[i].flags & PARTY_DELETED) {
			memcpy(chan->members+i, chan->members+i+1, sizeof(*chan->members) * (chan->nmembers-i-1));
			chan->nmembers--;
			dirty++;
			i--;
		}
	}
	if (dirty) chan->members = realloc(chan->members, sizeof(*chan->members) * chan->nmembers);

	dirty = 0;
	for (i = 0; i < chan->nhandlers; i++) {
		if (chan->handlers[i].flags & PARTY_DELETED) {
			memcpy(chan->handlers+i, chan->handlers+i+1, sizeof(*chan->handlers) * (chan->nhandlers-i-1));
			chan->nhandlers--;
			dirty++;
			i--;
		}
	}
	if (dirty) chan->handlers = realloc(chan->handlers, sizeof(*chan->handlers) * chan->nhandlers);

	return(0);
}

partychan_t *partychan_lookup_cid(int cid)
{
	partychan_t *chan = NULL;

	hash_table_find(cid_ht, (void *)cid, &chan);
	if (chan && chan->flags & PARTY_DELETED) chan = NULL;
	return(chan);
}

partychan_t *partychan_lookup_name(const char *name)
{
	partychan_t *chan = NULL;

	for (chan = partychan_head; chan; chan = chan->next) {
		if (chan->flags & PARTY_DELETED) continue;
		if (!strcasecmp(name, chan->name)) break;
	}
	return(chan);
}

partychan_t *partychan_get_default(partymember_t *p)
{
	if (p->nchannels > 0) return(p->channels[p->nchannels-1]);
	return(NULL);
}

int partychan_join_name(const char *chan, partymember_t *p)
{
	partychan_t *chanptr;

	if (!chan) return(-1);
	chanptr = partychan_lookup_name(chan);
	if (!chanptr) chanptr = partychan_new(-1, chan);
	return partychan_join(chanptr, p);
}

int partychan_join_cid(int cid, partymember_t *p)
{
	partychan_t *chanptr;

	chanptr = partychan_lookup_cid(cid);
	return partychan_join(chanptr, p);
}

int partychan_join(partychan_t *chan, partymember_t *p)
{
	int i;

	if (!chan || !p) return(-1);

	/* Add the member. */
	chan->members = realloc(chan->members, sizeof(*chan->members) * (chan->nmembers+1));
	chan->members[chan->nmembers].p = p;
	chan->members[chan->nmembers].flags = 0;
	chan->nmembers++;

	/* Now add the channel to the member's list. */
	p->channels = realloc(p->channels, sizeof(chan) * (p->nchannels+1));
	p->channels[p->nchannels] = chan;
	p->nchannels++;

	/* Send out the join event to the members. */
	for (i = 0; i < chan->nmembers; i++) {
		if (chan->members[i].flags & PARTY_DELETED) continue;
		(chan->members[i].p->handler->on_join)(chan->members[i].p->client_data, chan, p);
	}

	/* Send out the join event to the listeners. */
	for (i = 0; i < chan->nhandlers; i++) {
		if (chan->handlers[i].flags & PARTY_DELETED) continue;
		(chan->handlers[i].handler->on_join)(chan->handlers[i].client_data, chan, p);
	}
	return(0);
}

int partychan_part_name(const char *chan, partymember_t *p, const char *text)
{
	partychan_t *chanptr;

	if (!chan) return(-1);
	chanptr = partychan_lookup_name(chan);
	return partychan_part(chanptr, p, text);
}

int partychan_part_cid(int cid, partymember_t *p, const char *text)
{
	partychan_t *chanptr;

	chanptr = partychan_lookup_cid(cid);
	return partychan_part(chanptr, p, text);
}

int partychan_part(partychan_t *chan, partymember_t *p, const char *text)
{
	int i, len;

	if (!chan || !p) return(-1);

	/* Remove the channel entry from the member. */
	for (i = 0; i < p->nchannels; i++) {
		if (p->channels[i] == chan) {
			memcpy(p->channels+i, p->channels+i+1, sizeof(chan) * (p->nchannels-i-1));
			p->nchannels--;
			break;
		}
	}

	/* Mark the member to be deleted. */
	for (i = 0; i < chan->nmembers; i++) {
		if (chan->members[i].flags & PARTY_DELETED) continue;
		if (chan->members[i].p == p) {
			chan->members[i].flags |= PARTY_DELETED;
			garbage_add((garbage_proc_t)partychan_cleanup, chan, GARBAGE_ONCE);
			break;
		}
	}

	len = strlen(text);

	/* Send out the part event to the members. */
	for (i = 0; i < chan->nmembers; i++) {
		if (chan->members[i].flags & PARTY_DELETED) continue;
		(chan->members[i].p->handler->on_part)(chan->members[i].p->client_data, chan, p, text, len);
	}

	/* Send out the part event to the listeners. */
	len = strlen(text);
	for (i = 0; i < chan->nhandlers; i++) {
		if (chan->handlers[i].flags & PARTY_DELETED) continue;
		(chan->handlers[i].handler->on_part)(chan->handlers[i].client_data, chan, p, text, len);
	}
	return(0);
}

int partychan_join_handler(partychan_t *chan, partyline_event_t *handler, void *client_data)
{
	/* Add the handler (if there is one). */
	chan->handlers = realloc(chan->handlers, sizeof(*chan->handlers) * (chan->nhandlers+1));
	chan->handlers[chan->nhandlers].handler = handler;
	chan->handlers[chan->nhandlers].client_data = client_data;
	chan->handlers[chan->nhandlers].flags = 0;
	chan->nhandlers++;
	return(0);
}

int partychan_part_handler(partychan_t *chan, partyline_event_t *handler, void *client_data)
{
	int i;

	if (!chan) return(-1);

	/* Mark the handler for later deletion. */
	for (i = 0; i < chan->nhandlers; i++) {
		if (chan->handlers[i].handler == handler && chan->handlers[i].client_data == client_data) {
			chan->handlers[i].flags |= PARTY_DELETED;
			break;
		}
	}
	return(0);
}

int partychan_msg_name(const char *name, partymember_t *src, const char *text, int len)
{
	partychan_t *chan;

	chan = partychan_lookup_name(name);
	return partychan_msg(chan, src, text, len);
}

int partychan_msg_cid(int cid, partymember_t *src, const char *text, int len)
{
	partychan_t *chan;

	chan = partychan_lookup_cid(cid);
	return partychan_msg(chan, src, text, len);
}

int partychan_msg(partychan_t *chan, partymember_t *src, const char *text, int len)
{
	int i;

	if (!chan || chan->flags & PARTY_DELETED) return(-1);

	if (len < 0) len = strlen(text);

	for (i = 0; i < chan->nmembers; i++) {
		if (chan->members[i].flags & PARTY_DELETED) continue;
		(chan->members[i].p->handler->on_chanmsg)(chan->members[i].p->client_data, chan, src, text, len);
	}

	for (i = 0; i < chan->nhandlers; i++) {
		if (chan->handlers[i].flags & PARTY_DELETED) continue;
		(chan->handlers[i].handler->on_chanmsg)(chan->handlers[i].client_data, chan, src, text, len);
	}
	return(0);
}

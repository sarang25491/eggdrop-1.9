/* partychan.c: partyline channels
 *
 * Copyright (C) 2003, 2004 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef lint
static const char rcsid[] = "$Id: partychan.c,v 1.23 2006/11/14 14:51:23 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

/* When does cid wrap around? This lets cids get up to 99999. */
#define CID_WRAPAROUND 100000

static hash_table_t *cid_ht = NULL;
static partychan_t *partychan_head = NULL;
static int g_cid = 0; /* Keep track of next available cid. */
static partymember_common_t *common_list_head = NULL;

/* Some bind tables for partyline channels. */
static bind_table_t *BT_partyjoin = NULL,
	*BT_partypart = NULL,
	*BT_partypub = NULL;

int partychan_init(void)
{
	cid_ht = hash_table_create(NULL, NULL, 13, HASH_TABLE_INTS);

	/* The first 3 args for each bind are:
	 * channel name, channel id, partier */
	BT_partyjoin = bind_table_add(BTN_PARTYLINE_JOIN, 3, "siP", MATCH_NONE, BIND_STACKABLE);	/* DDD	*/
	BT_partypart = bind_table_add(BTN_PARTYLINE_PART, 5, "siPsi", MATCH_NONE, BIND_STACKABLE);	/* DDD	*/
	BT_partypub = bind_table_add(BTN_PARTYLINE_PUBLIC, 5, "siPsi", MATCH_NONE, BIND_STACKABLE);	/* DDD	*/
	return(0);
}

int partychan_shutdown(void)
{
	partychan_t *chan, *next;
	partymember_common_t *common, *next_common;

	if (partychan_head != NULL) {
		for (chan = partychan_head; chan; ) {
			next = chan->next;
			partychan_delete(chan);
			chan = next;
		}
		partychan_head = NULL;
	}

	if (common_list_head) {
		for (common = common_list_head; common; ) {
			next_common = common->next;

			free(common->members);
			free(common);

			common = next_common;
		} 
		common_list_head = NULL;
	}

	bind_table_del(BT_partypub);
	bind_table_del(BT_partypart);
	bind_table_del(BT_partyjoin);
	
	/* flush any pending partychan deletes */
	garbage_run();

	hash_table_delete(cid_ht);

	return (0);
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

static int partychan_cleanup(partychan_t *chan)
{
	int i;

	if (chan->flags & PARTY_DELETED) {
		/* Following is bulky code to resolve recursive referencing.
		   Don't bother reading it (unless hunting for bugs).
		   All it does is searches through a memeber list of a channel
		   and then updates each member so to NOT contain said channel in it's
		   channel list. Eh, sounds funky even in English. */
		   
		for (i = 0; i < chan->nmembers; i++) {
			int j;
			for (j = 0; j < chan->members[i].p->nchannels; j++) {
				if (chan == chan->members[i].p->channels[j]) {
					if (j + 1 < chan->members[i].p->nchannels)
						memmove(chan->members[i].p->channels[j], chan->members[i].p->channels[j+1],
								(chan->members[i].p->nchannels-j-1) * sizeof(*chan));
					chan->members[i].p->nchannels--;
					break;
				}
			}
		}

		if (chan->prev) {
			chan->prev->next = chan->next;
			if (chan->next)
				chan->next->prev = chan->prev;
		}
		else if (chan->next)
			chan->next->prev = NULL;

		free(chan->name);
		free(chan->members);
		free(chan);
		return(0);
	}

	for (i = 0; i < chan->nmembers; i++) {
		if (chan->members[i].flags & PARTY_DELETED) {
			memmove(chan->members+i, chan->members+i+1, sizeof(*chan->members) * (chan->nmembers-i-1));
			chan->nmembers--;
			i--;
		}
	}

	return(0);
}

void partychan_delete(partychan_t *chan)
{
	chan->flags |= PARTY_DELETED;
	hash_table_remove(cid_ht, (void *)chan->cid, NULL);
	garbage_add((garbage_proc_t)partychan_cleanup, chan, GARBAGE_ONCE);
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

int partychan_ison_name(const char *chan, partymember_t *p)
{
	partychan_t *chanptr;

	if (!chan) return(0);
	chanptr = partychan_lookup_name(chan);
	if (!chanptr) return(0);
	return partychan_ison(chanptr, p);
}

int partychan_ison(partychan_t *chan, partymember_t *p)
{
	int i;

	egg_assert_val (chan != NULL, 0);
	egg_assert_val (p != NULL, 0);
	
	for (i = 0; i < chan->nmembers; i++) {
		if (chan->members[i].flags & PARTY_DELETED) continue;
		if (chan->members[i].p == p) return(1);
	}
		
	return(0);
}

int partychan_join_name(const char *chan, partymember_t *p, int linking)
{
	partychan_t *chanptr;

	if (!chan) return(-1);
	chanptr = partychan_lookup_name(chan);
	if (!chanptr) chanptr = partychan_new(-1, chan);
	return partychan_join(chanptr, p, linking);
}

int partychan_join_cid(int cid, partymember_t *p, int linking)
{
	partychan_t *chanptr;

	chanptr = partychan_lookup_cid(cid);
	return partychan_join(chanptr, p, linking);
}

int partychan_join(partychan_t *chan, partymember_t *p, int linking)
{
	partychan_member_t *mem;
	int i;

	if (!chan || !p) return(-1);

	/* Check to see if he's already on. We put this here just to save some
	 * redundancy in partyline modules. */
	for (i = 0; i < chan->nmembers; i++) {
		if (chan->members[i].flags & PARTY_DELETED) continue;
		if (chan->members[i].p == p) return(-1);
	}

	/* Add the member. */
	chan->members = realloc(chan->members, sizeof(*chan->members) * (chan->nmembers+1));
	chan->members[chan->nmembers].p = p;
	chan->members[chan->nmembers].flags = 0;
	chan->nmembers++;

	/* Now add the channel to the member's list. */
	p->channels = realloc(p->channels, sizeof(chan) * (p->nchannels+1));
	p->channels[p->nchannels] = chan;
	p->nchannels++;

	/* Trigger the partyline channel join bind. */
	bind_check(BT_partyjoin, NULL, NULL, chan->name, chan->cid, p);

	/* Send out the join event to the members. */
	for (i = 0; i < chan->nmembers; i++) {
		mem = chan->members+i;
		if (mem->flags & PARTY_DELETED || mem->p->flags & PARTY_DELETED) continue;
		if (mem->p->handler && mem->p->handler->on_join)
			(mem->p->handler->on_join)(mem->p->client_data, chan, p, linking);
	}

	botnet_member_join(chan, p, linking);

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
	partychan_member_t *mem;

	if (!chan || !p) return(-1);

	/* Remove the channel entry from the member. */
	if (!(p->flags & PARTY_DELETED)) for (i = 0; i < p->nchannels; i++) {
		if (p->channels[i] == chan) {
			memmove(p->channels+i, p->channels+i+1, sizeof(chan) * (p->nchannels-i-1));
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

	/* If the member is already deleted, then the quit event has been
	 * fired already. */
	if (p->flags & PARTY_DELETED) return(0);

	if (!text) text = "";
	len = strlen(text);

	/* Trigger the partyline channel part bind. */
	bind_check(BT_partypart, NULL, NULL, chan->name, chan->cid, p, text, len);

	if (p->handler->on_part) (p->handler->on_part)(p->client_data, chan, p, text, len);

	/* Send out the part event to the members. */
	for (i = 0; i < chan->nmembers; i++) {
		mem = chan->members+i;
		if (mem->flags & PARTY_DELETED || mem->p->flags & PARTY_DELETED) continue;
		if (mem->p->handler && mem->p->handler->on_part)
			(mem->p->handler->on_part)(mem->p->client_data, chan, p, text, len);
	}

	botnet_member_part(chan, p, text, len);

	return(0);
}

static int chan_msg(partychan_t *chan, partymember_t *src, const char *text, int len, int local)
{
	partychan_member_t *mem;
	int i;

	if (!chan || chan->flags & PARTY_DELETED) return(-1);

	if (len < 0) len = strlen(text);

	/* Trigger the partyline channel pub bind. */
	bind_check(BT_partypub, NULL, NULL, chan->name, chan->cid, src, text, len);

	for (i = 0; i < chan->nmembers; i++) {
		mem = chan->members+i;
		if (mem->flags & PARTY_DELETED || mem->p->flags & PARTY_DELETED) continue;
		if (mem->p->handler && mem->p->handler->on_chanmsg)
			(mem->p->handler->on_chanmsg)(mem->p->client_data, chan, src, text, len);
	}

	if (!local) botnet_chanmsg(chan, src, text, len);

	return(0);
}

int partychan_msg_name(const char *name, partymember_t *src, const char *text, int len)
{
	partychan_t *chan;

	chan = partychan_lookup_name(name);
	return chan_msg(chan, src, text, len, 0);
}

int partychan_msg_cid(int cid, partymember_t *src, const char *text, int len)
{
	partychan_t *chan;

	chan = partychan_lookup_cid(cid);
	return chan_msg(chan, src, text, len, 0);
}

int partychan_msg(partychan_t *chan, partymember_t *src, const char *text, int len)
{
	return chan_msg(chan, src, text, len, 0);
}

int localchan_msg(partychan_t *chan, partymember_t *src, const char *text, int len)
{
	return chan_msg(chan, src, text, len, 1);
}

/* Build a list of local members on the same channels as p. */
partymember_common_t *partychan_get_common(partymember_t *p)
{
	partymember_common_t *common;
	partychan_t *chan;
	partymember_t *mem;
	int i, j;

	if (common_list_head) {
		common = common_list_head;
		common_list_head = common_list_head->next;
	}
	else {
		common = calloc(1, sizeof(*common));
	}

	common->len = 0;
	for (i = 0; i < p->nchannels; i++) {
		chan = p->channels[i];
		if (chan->flags & PARTY_DELETED) continue;
		for (j = 0; j < chan->nmembers; j++) {
			if (chan->members[j].flags & PARTY_DELETED) continue;
			mem = chan->members[j].p;
			if (mem->flags & (PARTY_DELETED | PARTY_SELECTED)) continue;
			if (mem->bot) continue;
			mem->flags |= PARTY_SELECTED;
			if (common->len >= common->max) {
				common->max = common->len + 10;
				common->members = realloc(common->members, sizeof(*common->members) * common->max);
			}
			common->members[common->len] = mem;
			common->len++;
		}
	}
	for (i = 0; i < common->len; i++) common->members[i]->flags &= ~PARTY_SELECTED;
	return(common);
}

int partychan_free_common(partymember_common_t *common)
{
	common->len = 0;
	common->next = common_list_head;
	common_list_head = common;
	return(0);
}

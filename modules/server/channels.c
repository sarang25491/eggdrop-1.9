/* channels.c: channel support
 *
 * Copyright (C) 2002, 2003, 2004 Eggheads Development Team
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
static const char rcsid[] = "$Id: channels.c,v 1.35 2005/03/03 18:45:26 stdarg Exp $";
 #endif

#include "server.h"

channel_t *channel_head = NULL;
int nchannels = 0;

void channel_init()
{
	channel_head = NULL;
	nchannels = 0;
	channel_load(server_config.chanfile);
}

/* Reset everything when we disconnect from the server. */
void channel_reset()
{
	channel_t *chan;

	/* Clear out channel list. */
	for (chan = channel_head; chan; chan = chan->next) {
		channel_free_online(chan);
	}
	uhost_cache_reset();
}

void channel_destroy()
{
	channel_reset();
	channel_save(server_config.chanfile);
}

void channel_free(channel_t *chan)
{
	/* Free online things. */
	channel_free_online(chan);

	/* Delete settings. */
	if (chan->name) free(chan->name);
	if (chan->settings) xml_node_delete(chan->settings);

	/* Unlink. */
	if (chan->prev) chan->prev->next = chan->next;
	else channel_head = chan->next;
	if (chan->next) chan->next->prev = chan->prev;
}

/* Find or create channel when passed 0 or 1 */
channel_t *channel_probe(const char *chan_name, int create)
{
	channel_t *chan, *prev;
	int i;

	prev = NULL;
	for (chan = channel_head; chan; chan = chan->next) {
		if (!strcasecmp(chan->name, chan_name)) return(chan);
		prev = chan;
	}
	if (!create) return(NULL);

	/* Create a new channel. */
	nchannels++;
	chan = calloc(1, sizeof(*chan));
	chan->name = strdup(chan_name);
	chan->settings = xml_node_new();
	chan->settings->name = strdup("settings");

	if (current_server.type1modes) {
		chan->nlists = strlen(current_server.type1modes);
		chan->lists = calloc(chan->nlists, sizeof(*chan->lists));
		for (i = 0; i < chan->nlists; i++) {
			chan->lists[i] = calloc(1, sizeof(*chan->lists[i]));
			chan->lists[i]->type = current_server.type1modes[i];
		}
	}

	if (current_server.type2modes) {
		chan->nargs = strlen(current_server.type2modes);
		chan->args = calloc(chan->nargs, sizeof(*chan->args));
		for (i = 0; i < chan->nargs; i++) {
			chan->args[i].type = current_server.type2modes[i];
		}
	}

	/* Link to list. */
	if (prev) prev->next = chan;
	else channel_head = chan;
	chan->prev = prev;
	return(chan);
}

/* Find a channel if it exists. */
channel_t *channel_lookup(const char *chan_name)
{
	return channel_probe(chan_name, 0);
}

/* Add a channel to our static list. */
channel_t *channel_add(const char *name)
{
	channel_t *chan;

	chan = channel_probe(name, 1);
	chan->flags |= CHANNEL_STATIC;
	return(chan);
}

/* Remove a channel from our static list. */
int channel_remove(const char *name)
{
	channel_t *chan;

	chan = channel_probe(name, 0);
	if (!chan) return(-1);
	chan->flags &= ~CHANNEL_STATIC;
	return(0);
}

channel_mask_list_t *channel_get_mask_list(channel_t *chan, char type)
{
	int i;

	for (i = 0; i < chan->nlists; i++) {
		if (chan->lists[i]->type == type)
			return(chan->lists[i]);
	}
	return(NULL);
}

void channel_add_mask(channel_t *chan, char type, const char *mask, const char *set_by, int time)
{
	channel_mask_list_t *l;
	channel_mask_t *m;

	l = channel_get_mask_list(chan, type);

	if (!l) return;

	m = calloc(1, sizeof(*m));
	m->mask = strdup(mask);
	m->next = l->head;
	if (set_by) m->set_by = strdup(set_by);
	m->time = time;
	m->last_used = 0;

	l->head = m;
	l->len++;
}

void channel_del_mask(channel_t *chan, char type, const char *mask)
{
	channel_mask_list_t *l;
	channel_mask_t *m, *prev;

	l = channel_get_mask_list(chan, type);
	prev = NULL;
	for (m = l->head; m; m = m->next) {
		if (m->mask && !strcasecmp(m->mask, mask)) {
			free(m->mask);
			if (m->set_by) free(m->set_by);
			if (prev) prev->next = m->next;
			else l->head = m->next;
			l->len--;
			free(m);
			break;
		}
	}
}

int channel_mode(const char *chan_name, const char *nick, char *buf)
{
	channel_t *chan;
	channel_member_t *m;
	flags_t *flags = NULL;

	buf[0] = 0;
	chan = channel_probe(chan_name, 0);
	if (!chan) return(-1);
	if (!nick) flags = &chan->mode;
	else {
		for (m = chan->member_head; m; m = m->next) {
			if (!(current_server.strcmp)(nick, m->nick)) {
				flags = &m->mode;
				break;
			}
		}
		if (!flags) return(-2);
	}
	flag_to_str(flags, buf);
	return(0);
}

int channel_mode_arg(const char *chan_name, int type, const char **value)
{
	int i;
	channel_t *chan;

	chan = channel_probe(chan_name, 0);
	if (!chan) return(-1);
	for (i = 0; i < chan->nargs; i++) {
		if (chan->args[i].type == type) {
			*value = chan->args[i].value;
			return(0);
		}
	}
	return(-2);
}

int channel_set(channel_t *chan, const char *value, ...)
{
	va_list args;
	xml_node_t *node;
	char *setting, *oldvalue;
	int r;

	va_start(args, value);
	node = xml_node_vlookup(chan->settings, args, 1);
	va_end(args);
	setting = xml_node_fullname(node);
	xml_node_get_str(&oldvalue, node, NULL);
	r = bind_check(BT_chanset, NULL, setting, chan->name, setting, oldvalue, value);
	free(setting);
	if (!(r & BIND_RET_BREAK)) xml_node_set_str(value, node, NULL);
	return(0);
}

int channel_get(channel_t *chan, char **strptr, ...)
{
	va_list args;
	xml_node_t *node;

	va_start(args, strptr);
	node = xml_node_vlookup(chan->settings, args, 0);
	va_end(args);
	return xml_node_get_str(strptr, node, NULL);
}

int channel_get_int(channel_t *chan, int *intptr, ...)
{
	va_list args;
	xml_node_t *node;

	va_start(args, intptr);
	node = xml_node_vlookup(chan->settings, args, 0);
	va_end(args);
	return xml_node_get_int(intptr, node, NULL);
}

xml_node_t *channel_get_node(channel_t *chan, ...)
{
	va_list args;
	xml_node_t *node;

	va_start(args, chan);
	node = xml_node_vlookup(chan->settings, args, 0);
	va_end(args);
	return(node);
}

int channel_load(const char *fname)
{
	xml_node_t *root = xml_parse_file(fname);
	xml_node_t *chan_node, *settings;
	channel_t *chan;
	char *name;
	int flags;

	if (!root) {
		putlog(LOG_MISC, "*", "Could not load channel file '%s': %s", fname, xml_last_error());
		return(-1);
	}

	chan_node = xml_node_lookup(root, 0, "channel", 0, 0);
	for (; chan_node; chan_node = chan_node->next_sibling) {
		xml_node_get_vars(chan_node, "sin", "name", &name, "flags", &flags, "settings", &settings);
		if (!name) continue;
		chan = channel_probe(name, 1);
		if (settings) {
			xml_node_unlink(settings);
			xml_node_delete(chan->settings);
			chan->settings = settings;
		}
		chan->flags = flags;
	}
	return(0);
}

int channel_save(const char *fname)
{
	xml_node_t *root = xml_node_new();
	xml_node_t *chan_node;
	channel_t *chan;

	if (!fname) fname = "channels.xml";
	root->name = strdup("channels");
	for (chan = channel_head; chan; chan = chan->next) {
		if (!(chan->flags & CHANNEL_STATIC)) continue;

		chan_node = xml_node_new();
		chan_node->name = strdup("channel");
		xml_node_set_str(chan->name, chan_node, "name", 0, 0);
		xml_node_set_int(chan->flags, chan_node, "flags", 0, 0);
		xml_node_append(chan_node, chan->settings);
		xml_node_append(root, chan_node);
	}
	xml_save_file(fname, root, XML_INDENT);
	for (chan = channel_head; chan; chan = chan->next) {
		if (chan->settings) xml_node_unlink(chan->settings);
	}
	xml_node_delete(root);
	return(0);
}

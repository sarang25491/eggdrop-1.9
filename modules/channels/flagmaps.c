/*
 * flagmaps.c --
 */
/*
 * Copyright (C) 2002, 2003 Eggheads Development Team
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

/* FIXME: #include mess
#ifndef lint
static const char rcsid[] = "$Id: flagmaps.c,v 1.6 2003/02/12 08:42:22 wcc Exp $";
#endif
*/

typedef struct {
	int flagval;
	char *name;
} channel_flag_map_t;

static channel_flag_map_t normal_flag_map[] = {
	{CHAN_ENFORCEBANS, "enforcebans"},
	{CHAN_DYNAMICBANS, "dynamicbans"},
	{CHAN_OPONJOIN, "autoop"},
	{CHAN_NODESYNCH, "nodesynch"},
	{CHAN_GREET, "greet"},
	{CHAN_DONTKICKOPS, "dontkickops"},
	{CHAN_INACTIVE, "inactive"},
	{CHAN_LOGSTATUS, "statuslog"},
	{CHAN_SECRET, "secret"},
	{CHAN_SHARED, "shared"},
	{CHAN_AUTOVOICE, "autovoice"},
	{CHAN_CYCLE, "cycle"},
	{CHAN_HONORGLOBALBANS, "honor-global-bans"},
	{0, 0}
};

static channel_flag_map_t stupid_ircnet_flag_map[] = {
	{CHAN_DYNAMICEXEMPTS, "dynamicexempts"},
	{CHAN_DYNAMICINVITES, "dynamicinvites"},
	{CHAN_HONORGLOBALEXEMPTS, "honor-global-exempts"},
	{CHAN_HONORGLOBALINVITES, "honor-global-invites"},
	{0, 0}
};

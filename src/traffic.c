/*
 * traffic.c --
 */
/*
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
/*
 * $Id: traffic.c,v 1.5 2003/12/11 00:49:11 wcc Exp $
 */

#include "main.h"
#include "dccutil.h"			/* dprintf		*/
#include "modules.h"			/* add_hook()		*/
#include "traffic.h"			/* prototypes		*/

#ifndef lint
static const char rcsid[] = "$Id: traffic.c,v 1.5 2003/12/11 00:49:11 wcc Exp $";
#endif

egg_traffic_t traffic;			/* traffic information	*/

/* converts bytes to a human readable string
 */
static char traffictxt[80]; 
static char *btos(unsigned long  bytes)
{
  char unit[10];
  float xbytes;

  sprintf(unit, "Bytes");
  xbytes = bytes;
  if (xbytes > 1024.0) {
    sprintf(unit, "KBytes");
    xbytes = xbytes / 1024.0;
  }
  if (xbytes > 1024.0) {
    sprintf(unit, "MBytes");
    xbytes = xbytes / 1024.0;
  }
  if (xbytes > 1024.0) {
    sprintf(unit, "GBytes");
    xbytes = xbytes / 1024.0;
  }
  if (xbytes > 1024.0) {
    sprintf(unit, "TBytes");
    xbytes = xbytes / 1024.0;
  }
  if (bytes > 1024)
    sprintf(traffictxt, "%.2f %s", xbytes, unit);
  else
    sprintf(traffictxt, "%lu Bytes", bytes);

  return traffictxt;
}

/* update outgoing traffic stats (used in main.c loop)
 */
void traffic_update_out(struct dcc_table *type, int size)
{
        if (!type->name)                /* sanity check */
                return;

        if (!strncmp(type->name, "BOT", 3))
                traffic.out_today.bn += size;
        else if (!strcmp(type->name, "SERVER"))
                traffic.out_today.irc += size;
        else if (!strncmp(type->name, "CHAT", 4))
                traffic.out_today.dcc += size;
        else if (!strncmp(type->name, "FILES", 5))
                traffic.out_today.dcc += size;
        else
                traffic.out_today.unknown += size;
}

/* update incoming traffic stats (used in main.c loop)
 */
void traffic_update_in(struct dcc_table *type, int size)
{
	if (!type->name)		/* sanity check	*/ 
		return;

	if (!strncmp(type->name, "BOT", 3))
		traffic.in_today.bn += size;
	else if (!strcmp(type->name, "SERVER"))
		traffic.in_today.irc += size;
	else if (!strncmp(type->name, "CHAT", 4))
		traffic.in_today.dcc += size;
	else if (!strncmp(type->name, "FILES", 5))
		traffic.in_today.dcc += size;
	else
		traffic.in_today.unknown += size;
}

/* init our traffic stats
 */
void traffic_init()
{
	add_hook(HOOK_DAILY, (Function) traffic_reset);
}

/* resets our traffic stats (called in main.c)
 */
void traffic_reset()
{
	traffic.out_total.irc += traffic.out_today.irc;
	traffic.out_total.bn += traffic.out_today.bn;
	traffic.out_total.dcc += traffic.out_today.dcc;
	traffic.out_total.filesys += traffic.out_today.filesys;
	traffic.out_total.unknown += traffic.out_today.unknown;

	traffic.in_total.irc += traffic.in_today.irc;
	traffic.in_total.bn += traffic.in_today.bn;
	traffic.in_total.dcc += traffic.in_today.dcc;
	traffic.in_total.filesys += traffic.in_today.filesys;
	traffic.in_total.unknown += traffic.in_today.unknown;

	memset(&traffic.out_today, 0, sizeof(traffic.out_today));
	memset(&traffic.in_today, 0, sizeof(traffic.in_today));
}

/* traffic dcc command 
 */
int cmd_traffic(struct userrec *u, int idx, char *par)
{
  unsigned long itmp, itmp2;

  dprintf(idx, "Traffic since last restart\n");
  dprintf(idx, "==========================\n");
      
  /* IRC */
  if (traffic.out_total.irc > 0 || traffic.in_total.irc > 0 ||
      traffic.out_today.irc > 0 || traffic.in_today.irc > 0) {
    dprintf(idx, "IRC:\n");
    dprintf(idx, "  out: %s", btos(traffic.out_total.irc
			      + traffic.out_today.irc));
    dprintf(idx, " (%s today)\n", btos(traffic.out_today.irc));
    dprintf(idx, "   in: %s", btos(traffic.in_total.irc
			      + traffic.in_today.irc));
    dprintf(idx, " (%s today)\n", btos(traffic.in_today.irc));
  }
 
  /*
  if (traffic.out_total.bn > 0 || traffic.in_total.bn > 0 ||
      traffic.out_today.bn > 0 || traffic.in_today.bn > 0) {
    dprintf(idx, "Botnet:\n");
    dprintf(idx, "  out: %s", btos(traffic.out_total.bn
			      + traffic.out_today.bn));
    dprintf(idx, " (%s today)\n", btos(traffic.out_today.bn));
    dprintf(idx, "   in: %s", btos(traffic.in_total.bn + traffic.in_today.bn));
    dprintf(idx, " (%s today)\n", btos(traffic.in_today.bn));
  }
  */

  /* Partyline */
  if (traffic.out_total.dcc > 0 || traffic.in_total.dcc > 0 ||
      traffic.out_today.dcc > 0  || traffic.in_today.dcc > 0) {
    itmp = traffic.out_total.dcc + traffic.out_today.dcc;
    itmp2 = traffic.out_today.dcc;

    dprintf(idx, "Partyline:\n");
    dprintf(idx, "  out: %s", btos(itmp));
    dprintf(idx, " (%s today)\n", btos(itmp2));
    dprintf(idx, "   in: %s", btos(traffic.in_total.dcc +
            traffic.in_today.dcc));
    dprintf(idx, " (%s today)\n", btos(traffic.in_today.dcc));
  }

  /* unknown */
  if (traffic.out_total.unknown > 0 || traffic.out_today.unknown > 0) {
    dprintf(idx, "Misc:\n");
    dprintf(idx, "  out: %s", btos(traffic.out_total.unknown +
            traffic.out_today.unknown));
    dprintf(idx, " (%s today)\n", btos(traffic.out_today.unknown));
    dprintf(idx, "   in: %s", btos(traffic.in_total.unknown +
            traffic.in_today.unknown));
    dprintf(idx, " (%s today)\n", btos(traffic.in_today.unknown));
  }

  dprintf(idx, "---\n");
  dprintf(idx, "Total:\n");

  /* compute total */
  itmp = traffic.out_total.irc + traffic.out_total.bn + 
         traffic.out_total.dcc + traffic.out_total.unknown +
         traffic.out_today.irc + traffic.out_today.bn +
         traffic.out_today.dcc + traffic.out_today.unknown;

  itmp2 = traffic.out_today.irc + traffic.out_today.bn +
          traffic.out_today.dcc + traffic.out_today.unknown;

  dprintf(idx, "  out: %s", btos(itmp));
  dprintf(idx, " (%s today)\n", btos(itmp2));
  dprintf(idx, "   in: %s", btos(traffic.in_total.irc + traffic.in_total.bn +
          traffic.in_total.dcc + traffic.in_total.unknown +
          traffic.in_today.irc + traffic.in_today.bn + traffic.in_today.dcc +
          traffic.in_today.unknown));

  dprintf(idx, " (%s today)\n", btos(traffic.in_today.irc +
          traffic.in_today.bn + traffic.in_today.dcc +
          traffic.in_today.unknown));

  return 1;
}

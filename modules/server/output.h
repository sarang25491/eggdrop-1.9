/* output.h: header for output.c
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: output.h,v 1.7 2003/12/18 06:50:47 wcc Exp $
 */

#ifndef _EGG_MOD_SERVER_OUTPUT_H_
#define _EGG_MOD_SERVER_OUTPUT_H_

int printserv(int priority, const char *format, ...);
queue_entry_t *queue_new(char *text);
void queue_append(queue_t *queue, queue_entry_t *q);
void queue_unlink(queue_t *queue, queue_entry_t *q);
void queue_entry_from_text(queue_entry_t *q, char *text);
void queue_entry_to_text(queue_entry_t *q, char *text, int *remaining);
void queue_entry_cleanup(queue_entry_t *q);
queue_t *queue_get_by_priority(int priority);
void dequeue_messages();

#endif /* !_EGG_MOD_SERVER_OUTPUT_H_ */

/* script.h: header for script.c
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Eggheads Development Team
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
 *
 * $Id: script.h,v 1.20 2006/11/14 14:51:23 sven Exp $
 */

#ifndef _EGG_SCRIPT_H_
#define _EGG_SCRIPT_H_

BEGIN_C_DECLS

/* Script events that get recorded in the script journal. */
enum
{
	SCRIPT_EVENT_LOAD_SCRIPT = 0,
	SCRIPT_EVENT_LINK_VAR,
	SCRIPT_EVENT_UNLINK_VAR,
	SCRIPT_EVENT_CREATE_CMD,
	SCRIPT_EVENT_DELETE_CMD,
	SCRIPT_EVENT_MAX
};

/* Script implementation return codes. */
enum
{
	SCRIPT_OK = 0,
	SCRIPT_ERR_NOT_RESPONSIBLE,
	SCRIPT_ERR_CODE,
};

/* Byte-arrays are simply strings with their length given explicitly, so it
 * can have embedded NULs. */
typedef struct byte_array_b {
	unsigned char *bytes;
	int len;
	int do_free;
} byte_array_t;

/* Flags for commands. */
/* SCRIPT_PASS_CDATA means your callback wants its (void *)client_data passed
 * as its *first* arg.
 *
 * SCRIPT_PASS_RETVAL will pass a (script_var_t *)retval so that you can return
 * complex types.
 *
 * SCRIPT_PASS_COUNT will pass the number of script arguments you're getting.
 *
 * SCRIPT_PASS_ARRAY will pass the arguments as an array, with argc and argv.
 *
 * SCRIPT_VAR_ARGS means you accept variable number of args. The nargs field
 * of the command struct is the minimum number required, and the strlen of your
 * syntax field is the max number. Arguments that weren't given will be filled
 * in with 0.
 *
 * SCRIPT_VAR_FRONT means the variable args come from the front instead of the
 * back. This is useful for flags and stuff.
 */
#define SCRIPT_PASS_CDATA	1
#define SCRIPT_PASS_RETVAL	2
#define SCRIPT_PASS_COUNT	4
#define SCRIPT_PASS_ARRAY	8
#define SCRIPT_VAR_ARGS		16
#define SCRIPT_VAR_FRONT	32

/* Flags for callbacks. */
/* SCRIPT_CALLBACK_ONCE means the callback will be automatically deleted after
 *  its first use.
 */
#define SCRIPT_CALLBACK_ONCE 1

/* Flags for variables. */
/* SCRIPT_FREE means we will call free(retval->value) (i.e. free a malloc'd
 * string or array base ptr.)
 * SCRIPT_FREE_VAR means we will call free(retval) (you may need this if you
 * build a complex list structure of (script_var_t *)'s).
 * SCRIPT_ARRAY means we interpret retval->value as an array of length
 * retval->len, of whatever basic type is specified in retval->type (flags
 * are ignored).
 * SCRIPT_ERROR means the interpreter will raise an error exception if it knows
 * how, with retval->value as the error information. Otherwise, it is ignored.
 */
#define SCRIPT_FREE	256
#define SCRIPT_FREE_VAR	512
#define SCRIPT_ARRAY	1024
#define SCRIPT_ERROR	2048
#define SCRIPT_READONLY	4096

/* Types for variables. */
#define SCRIPT_STRING		    's'
#define SCRIPT_STRING_LIST	'S'
#define SCRIPT_INTEGER		  'i'
#define SCRIPT_UNSIGNED		  'u'
#define SCRIPT_POINTER		  'p'
#define SCRIPT_CALLBACK		  'c'
#define SCRIPT_USER		      'U'
#define SCRIPT_BOT          'B'
#define SCRIPT_PARTIER		  'P'
#define SCRIPT_BYTES		    'b'
#define SCRIPT_VAR		      'v'
#define SCRIPT_TYPE_MASK	  255

typedef struct script_callback_b {
	int (*callback)();
	void *callback_data;
	void *delete_data;
	char *syntax;
	char *name;
	int flags;
	event_owner_t *owner;
} script_callback_t;

typedef struct script_var_b {
	int type;	/* Type of variable (int, str, etc). */
	void *value;	/* Value (needs to be cast to right type). */
	int len;	/* Length of string or array (when appropriate). */
} script_var_t;

struct script_linked_var_b;

typedef struct script_var_callbacks_b {
	int (*on_read)(struct script_linked_var_b *linked_var, script_var_t *newvalue);
	int (*on_write)(struct script_linked_var_b *linked_var, script_var_t *newvalue);
	void *client_data;
} script_var_callbacks_t;

typedef struct script_linked_var_b {
	char *class;
	char *name;
	void *value;
	int type;
	script_var_callbacks_t *callbacks;
} script_linked_var_t;

typedef struct script_command_b {
	char *class; /* General class of the function. E.g. "server". */
	char *name; /* Specific name of the function. E.g. "putserv". */
	void *callback; /* Pointer to the callback function. */
	void *client_data; /* Whatever private data you want passed back. */
	int nargs; /* Number of arguments the script wants. */
	char *syntax; /* Argument types. */
	char *syntax_error; /* Error to print when called incorrectly. */
	int retval_type; /* Limited return value type, for simple stuff. */
	int flags;
} script_command_t;

struct script_module_b;

typedef struct script_args_b {
	struct script_module_b *module;
	void *client_data;
	int len;
} script_args_t;

typedef struct {
	char *class;
	char *name;
	int (*callback)(void *client_data, script_args_t *args, script_var_t *retval);
	void *client_data;
} script_raw_command_t;

extern int script_init(void);
extern int script_shutdown(void);
extern int script_remove_events_by_owner(egg_module_t *module, void *script);

extern int script_load(char *filename);
extern int script_link_vars(script_linked_var_t *table);
extern int script_unlink_vars(script_linked_var_t *table);
extern int script_create_raw_commands(script_raw_command_t *table);
extern int script_delete_raw_commands(script_raw_command_t *table);
extern int script_create_commands(script_command_t *table);
extern int script_delete_commands(script_command_t *table);
extern int script_get_arg(script_args_t *args, int num, script_var_t *var, int type);

extern script_var_t *script_string(char *str, int len);
extern script_var_t *script_dynamic_string(char *str, int len);
extern script_var_t *script_copy_string(char *str, int len);
extern script_var_t *script_int(int val);
extern script_var_t *script_list(int nitems, ...);
extern int script_list_append(script_var_t *list, script_var_t *item);

/* Interface definition for scripting modules. */

typedef struct script_module_b {
	char *name;
	void *client_data;

	int (*load_script)(void *client_data, char *filename);
	int (*link_var)(void *client_data, script_linked_var_t *linked_var);
	int (*unlink_var)(void *client_data, script_linked_var_t *linked_var);
	int (*create_command)(void *client_data, script_raw_command_t *cmd);
	int (*delete_command)(void *client_data, script_raw_command_t *cmd);
	int (*get_arg)(void *client_data, script_args_t *args, int num, script_var_t *var, int type);
} script_module_t;

typedef int dns_function_t(const char *query, int timeout, dns_callback_t callback, void *data, event_owner_t *event);

extern int script_register_module(script_module_t *module);
extern int script_unregister_module(script_module_t *module);
extern int script_playback(script_module_t *module);
extern int script_linked_var_on_write(script_linked_var_t *var, script_var_t *newval);
extern int script_dns_query(dns_function_t *function, const char *host, script_callback_t *callback, char *text, int len);

END_C_DECLS

#endif /* !_EGG_SCRIPT_H_ */

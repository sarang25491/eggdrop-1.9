/* script.c: functions for scripting modules
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
 */

#ifndef lint
static const char rcsid[] = "$Id: script.c,v 1.27 2007/04/14 15:21:12 sven Exp $";
#endif

#include <eggdrop/eggdrop.h>

#define EVENT_VAR 1
#define EVENT_CMD 2

typedef struct {
	int event;
	void *data;
	void *key;
} journal_event_t;

typedef struct {
	script_callback_t *callback;
	int id;
	unsigned char *text;
	int len;
} script_dns_callback_data_t;

static journal_event_t *journal_events = NULL;
static int njournal_events = 0;

static script_module_t **script_modules = NULL;
static int nscript_modules = 0;

static void *journal_del(int event, void *key);
static void journal_add(int event, void *data, void *key);

static script_command_t script_cmds[] = {
	{"", "loadscript", script_load, NULL, 1, "s", "filename", SCRIPT_INTEGER, 0},
	{0}
};

int script_init(void)
{
	script_create_commands(script_cmds);

	return(0);
}

int script_shutdown(void)
{
	int i;

	script_delete_commands(script_cmds);

	/* XXX: this may be the wrong place. This should be handled by module
 	 * XXX: dependencies. E.g. if someone unloads "script", e.g. "tclscript" 
	 * XXX: should be magicly unloaded too */
	if (nscript_modules > 0) {
		for (i = nscript_modules - 1; i >= 0; i--) {
			script_unregister_module(script_modules[i]);
		}
	}
	if (script_modules) free(script_modules); script_modules = NULL;

	/* XXX: this shouldn't be necessary too if everything gets unloaded. */
	if (njournal_events > 0) {
		for (i = njournal_events - 1; i >= 0; i--) {
			journal_del(
				journal_events[i].event,
				journal_events[i].key);
		}
	}
	if (journal_events) free(journal_events); journal_events = NULL;

	return (0);
}

/*!
 * \brief Delete all asyncronous events of a module or script.
 *
 * This functions kills all asyncronous events of a given module or script
 * by calling the *_delete_by_owner() functions of all event types.
 *
 * \param module The module whose bots should be deleted.
 * \param script The script whose bots should be deleted. NULL matches everything.
 *
 * \return The number of deleted events.
 *
 * \bug Not all event types have a *_delete_by_owner() function yet.
 *      sockets, socketfilters and scripting functions are missing.
 */

int script_remove_events_by_owner(egg_module_t *module, void *script)
{
	int removed = 0;

	removed += botnet_delete_by_owner(module, script);
	removed += partymember_delete_by_owner(module, script);
	removed += kill_binds_by_owner(module, script);
	removed += timer_destroy_by_owner(module, script);
	removed += egg_dns_cancel_by_owner(module, script);
	removed += egg_ident_cancel_by_owner(module, script);
	return removed;
}

/* Called by scripting modules to register themselves. */
int script_register_module(script_module_t *module)
{
	script_modules = realloc(script_modules, sizeof(*script_modules) * (nscript_modules+1));
	script_modules[nscript_modules] = module;
	nscript_modules++;
	return(0);
}

/* Called by scripting modules to unregister themselves. */
int script_unregister_module(script_module_t *module)
{
	int i;

	for (i = 0; i < nscript_modules; i++) {
		if (script_modules[i] == module) break;
	}
	if (i == nscript_modules) return(-1);
	memmove(script_modules+i, script_modules+i+1, sizeof(script_module_t) * (nscript_modules-i-1));
	nscript_modules--;
	return(0);
}

/* Called by scripting modules to receive the full list of linked variables
	and functions. */
int script_playback(script_module_t *module)
{
	int i;

	for (i = 0; i < njournal_events; i++) {
		switch (journal_events[i].event) {
			case EVENT_VAR:
				module->link_var(module->client_data, journal_events[i].data);
				break;
			case EVENT_CMD:
				module->create_command(module->client_data, journal_events[i].data);
				break;
		}
	}
	return(0);
}

/* Add an event to our internal journal. */
static void journal_add(int event, void *data, void *key)
{
	journal_events = realloc(journal_events, sizeof(*journal_events) * (njournal_events+1));
	journal_events[njournal_events].event = event;
	journal_events[njournal_events].data = data;
	journal_events[njournal_events].key = key;
	njournal_events++;
}

/* Cancel an event from the journal. */
static void *journal_del(int event, void *key)
{
	int i;
	void *data = NULL;

	for (i = 0; i < njournal_events; i++) {
		if (journal_events[i].event == event && journal_events[i].key == key) break;
	}
	if (i < njournal_events) {
		data = journal_events[i].data;
		memmove(journal_events+i, journal_events+i+1, sizeof(*journal_events) * (njournal_events-i-1));
		njournal_events--;
	}

	return(data);
}

/* We shall provide a handy default linked var write handler. */

int script_linked_var_on_write(script_linked_var_t *var, script_var_t *newval)
{
	int r;
	char **charptr;

	/* See if the owner provides a write callback. */
	if (var->callbacks && var->callbacks->on_write) {
		r = (var->callbacks->on_write)(var, newval);
		return(r);
	}

	/* Provide default handling for strings and ints. */
	switch (var->type & SCRIPT_TYPE_MASK) {
		case SCRIPT_UNSIGNED:
		case SCRIPT_INTEGER:
			*(int *)(var->value) = (int) newval->value;
			break;
		case SCRIPT_STRING:
			charptr = (char **)var->value;
			if (*charptr) free(*charptr);
			if (newval->type & SCRIPT_FREE) *charptr = newval->value;
			else *charptr = strdup(newval->value);
			break;
	}
	return(0);
}

/* We shall provide a handy callback interface, to reduce the amount of
	work in other places. */

static int my_command_handler(void *client_data, script_args_t *args, script_var_t *retval)
{
	script_command_t *cmd = client_data;
	void *static_argstack[20], *static_free_args[20];
	void **argstack, **free_args;
	script_callback_t **callbacks, *static_callbacks[20];
	int argstack_len, nfree_args, ncallbacks;
	char *syntax;
	int i, skip, nopts, err;
	void *simple_retval;
	script_var_t var;
	void *(*callback)();

	/* Check if there is an argument count error. */
	if (cmd->flags & SCRIPT_VAR_ARGS) err = (cmd->nargs > args->len);
	else err = (cmd->nargs != args->len);

	if (err) {
		retval->type = SCRIPT_STRING | SCRIPT_ERROR;
		retval->value = cmd->syntax_error;
		return(-1);
	}

	/* Get space for the argument conversion.
		We'll try to use stack space instead of a calloc(). */
	if (args->len+3 > 20) {
		argstack = calloc(args->len+3, sizeof(void *));
		free_args = calloc(args->len, sizeof(void *));
		callbacks = calloc(args->len, sizeof(*callbacks));
	}
	else {
		memset(static_argstack, 0, sizeof(static_argstack));
		argstack = static_argstack;
		free_args = static_free_args;
		callbacks = static_callbacks;
	}
	argstack_len = 3;
	nfree_args = 0;
	ncallbacks = 0;

	/* Figure out how many args to skip. */
	syntax = cmd->syntax;
	if (cmd->flags & SCRIPT_VAR_FRONT) {
		skip = strlen(syntax) - args->len;
		if (skip < 0) skip = 0;
		argstack_len += skip;
		syntax += skip;
	}
	else skip = 0;

	/* Now start converting arguments according to the command's syntax
		string. */
	for (i = 0; i < args->len; i++) {
		err = script_get_arg(args, i, &var, *syntax);
		if (err) {
			retval->value = cmd->syntax_error;
			retval->type = SCRIPT_STRING | SCRIPT_ERROR;
			goto cleanup_args;
		}
		if (var.type & SCRIPT_FREE) free_args[nfree_args++] = var.value;
		else if (*syntax == SCRIPT_CALLBACK) callbacks[ncallbacks++] = var.value;
		argstack[argstack_len++] = var.value;
		syntax++;
	}

	/* Calculate the optional args we want for the callback.
		This is why we saved space for 3 args earlier on. */
	nopts = 3;
	if (cmd->flags & SCRIPT_PASS_COUNT) {
		nopts--;
		argstack[nopts] = (void *)(argstack_len - 3 - skip);
	}
	if (cmd->flags & SCRIPT_PASS_RETVAL) {
		nopts--;
		argstack[nopts] = retval;
	}
	if (cmd->flags & SCRIPT_PASS_CDATA) {
		nopts--;
		argstack[nopts] = cmd->client_data;
	}

	/* Adjust the base of the argument stack. */
	argstack += nopts;
	argstack_len -= nopts;

	/* Execute the callback. */
	callback = (void *(*)())cmd->callback;
	if (cmd->flags & SCRIPT_PASS_ARRAY) {
		simple_retval = callback(argstack_len, argstack);
	}
	else {
		simple_retval = callback(argstack[0], argstack[1],
			argstack[2], argstack[3], argstack[4], argstack[5],
			argstack[6], argstack[7], argstack[8], argstack[9]);
	}

	argstack -= nopts;
	argstack_len += nopts;

	/* Process the return value. */
	if (!(cmd->flags & SCRIPT_PASS_RETVAL)) {
		retval->type = cmd->retval_type;
		retval->len = -1;
		retval->value = simple_retval;
	}

cleanup_args:
	for (i = 0; i < nfree_args; i++) {
		if (free_args[i]) free(free_args[i]);
	}
	if (err) {
		for (i = 0; i < ncallbacks; i++) {
			if (callbacks[i] && callbacks[i]->owner && callbacks[i]->owner->on_delete) callbacks[i]->owner->on_delete(callbacks[i]->owner, callbacks[i]);
		}
	}
	if (argstack != static_argstack) {
		free(argstack);
		free(free_args);
		free(callbacks);
	}

	return(0);
}

/* Ahh, the client scripting interface. */
int script_load(char *filename)
{
	int i, ret;

	egg_assert_val (filename != NULL, 0);

	/* check if we have at least one script module loaded. */
	if (nscript_modules == 0) {
		putlog (LOG_MISC, "*", _("Failed to load script '%s': no script modules loaded."),
			filename);
		return 0;
	}

	for (i = 0; i < nscript_modules; i++) {
		ret = script_modules[i]->load_script(script_modules[i]->client_data, filename);

		switch (ret) {

			/* The current module was responsible and everything worked fine
			 */
			case (SCRIPT_OK):
				putlog (LOG_MISC, "*", _("Script loaded: %s"), filename);
				return 1;
	
			/* The current module is not responsible for this type of
			 * script. */
			case (SCRIPT_ERR_NOT_RESPONSIBLE):
				break;

			/* Code error, script implementation (hopefully) put a log
			 * message for a detailed error description. */
			case (SCRIPT_ERR_CODE):
				return 0;

		}
	}

	putlog (LOG_MISC, "*", _("Failed to load script '%s': no appropiate script module."), filename);
	
	return 0;
}

int script_link_vars(script_linked_var_t *table)
{
	int i;

	while (table->class && table->name) {
		journal_add(EVENT_VAR, table, table);
		for (i = 0; i < nscript_modules; i++) {
			script_modules[i]->link_var(script_modules[i]->client_data, table);
		}
		table++;
	}
	return(0);
}

int script_unlink_vars(script_linked_var_t *table)
{
	int i;

	while (table->class && table->name) {
		journal_del(EVENT_VAR, table);
		for (i = 0; i < nscript_modules; i++) {
			script_modules[i]->unlink_var(script_modules[i]->client_data, table);
		}
		table++;
	}
	return(0);
}

int script_create_raw_commands(script_raw_command_t *table)
{
	int i;

	while (table->class && table->name) {
		journal_add(EVENT_CMD, table, table);
		for (i = 0; i < nscript_modules; i++) {
			script_modules[i]->create_command(script_modules[i]->client_data, table);
		}
		table++;
	}
	return(0);
}

int script_delete_raw_commands(script_raw_command_t *table)
{
	int i;

	while (table->class && table->name) {
		journal_del(EVENT_CMD, table);
		for (i = 0; i < nscript_modules; i++) {
			script_modules[i]->delete_command(script_modules[i]->client_data, table);
		}
		table++;
	}
	return(0);
}

int script_create_commands(script_command_t *table)
{
	int i;
	script_raw_command_t *cmd;

	while (table->class && table->name) {
		cmd = malloc(sizeof(*cmd));
		cmd->class = table->class;
		cmd->name = table->name;
		cmd->callback = my_command_handler;
		cmd->client_data = table;

		journal_add(EVENT_CMD, cmd, table);
		for (i = 0; i < nscript_modules; i++) {
			script_modules[i]->create_command(script_modules[i]->client_data, cmd);
		}
		table++;
	}
	return(0);
}

int script_delete_commands(script_command_t *table)
{
	int i;
	script_raw_command_t *cmd;

	for (; table->class && table->name; table++) {
		cmd = journal_del(EVENT_CMD, table);
		if (!cmd)
			continue;

		for (i = 0; i < nscript_modules; i++) {
			script_modules[i]->delete_command(script_modules[i]->client_data, cmd);
		}

		free(cmd);
	}
	return(0);
}

int script_get_arg(script_args_t *args, int num, script_var_t *var, int type)
{
	script_module_t *module;

	module = args->module;
	return module->get_arg(module->client_data, args, num, var, type);
}

script_var_t *script_string(char *str, int len)
{
	script_var_t *var = malloc(sizeof(*var));

	var->type = SCRIPT_STRING | SCRIPT_FREE_VAR;
	if (!str) {
		str = "";
		len = 0;
	}
	else if (len < 0) len = strlen(str);
	var->value = str;
	var->len = len;
	return(var);
}

script_var_t *script_dynamic_string(char *str, int len)
{	script_var_t *var = script_string(str, len);
	var->type |= SCRIPT_FREE;
	return(var);
}

script_var_t *script_copy_string(char *str, int len)
{
	char *copy;

	if (!str) {
		str = "";
		len = 0;
	}
	else if (len < 0) len = strlen(str);
	copy = malloc(len+1);
	memcpy(copy, str, len);
	copy[len] = 0;
	return script_dynamic_string(copy, len);
}

script_var_t *script_int(int val)
{
	script_var_t *var = malloc(sizeof(*var));
	var->type = SCRIPT_INTEGER | SCRIPT_FREE_VAR;
	var->value = (void *)val;
	return(var);
}

script_var_t *script_list(int nitems, ...)
{
	script_var_t *list;

	list = malloc(sizeof(*list));
	list->type = SCRIPT_ARRAY | SCRIPT_FREE | SCRIPT_VAR | SCRIPT_FREE_VAR;
	list->len = nitems;
	if (nitems > 0) {
		list->value = malloc(nitems * sizeof(script_var_t *));
		memmove(list->value, &nitems + 1, nitems * sizeof(script_var_t *));
	}
	else list->value = NULL;
	return(list);
}

int script_list_append(script_var_t *list, script_var_t *item)
{
	list->value = realloc(list->value, sizeof(item) * (list->len+1));
	((script_var_t **)list->value)[list->len] = item;
	list->len++;
	return(0);
}

static int script_dns_delete(struct event_owner_b *event, void *client_data)
{
	script_dns_callback_data_t *data = client_data;

	if (data->callback->owner && data->callback->owner->on_delete) data->callback->owner->on_delete(data->callback->owner, data->callback);
	if (data->text) free(data->text);
	free(data);

	free(event);

	return 51896; /* Why do these callbacks have to return something? */
}

static int script_dns_callback(void *client_data, const char *query, char **result)
{
	static unsigned char no_data = 0;
	byte_array_t bytes;
	script_dns_callback_data_t *data = client_data;

	bytes.bytes = data->text;
	bytes.len = data->len;
	bytes.do_free = 0;

	if (!bytes.bytes) bytes.bytes = &no_data;
	if (bytes.len <= 0) bytes.len = strlen((char *) bytes.bytes);

	data->callback->callback(data->callback, data->id, query, result, &bytes);

	return 0; /* what exactly should this function return? the return value is always ignored */
}

int script_dns_query(dns_function_t *function, const char *host, script_callback_t *callback, char *text, int len)
{
	int id;
	event_owner_t *event;
	script_dns_callback_data_t *data;

	event = malloc(sizeof(*event));
	memcpy(event, callback->owner, sizeof(*event));
	event->on_delete = script_dns_delete;

	callback->syntax = strdup("isSb");
	data = malloc(sizeof(*data));
	data->callback = callback;
	data->id = -1;
	data->text = text;
	data->len = len;
	id = function(host, -1, script_dns_callback, data, event);
	if (id == -1) {
		/* the query was cached, the callback has already been called and data has been freed */
		return -1;
	}
	data->id = id;
	return id;
}


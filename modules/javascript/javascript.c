/*
 * javascript.c --
 */
/*
 * Copyright (C) 2002 Eggheads Development Team
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
static const char rcsid[] = "$Id: javascript.c,v 1.1 2002/05/06 09:39:50 stdarg Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsapi.h"

#include "lib/eggdrop/module.h"
#include "lib/egglib/mstack.h"
#include "lib/egglib/msprintf.h"
#include <eggdrop/eggdrop.h>

#define MODULE_NAME "javascript"

static eggdrop_t *egg = NULL;

/* Data we need for a tcl callback. */
typedef struct {
	JSContext *cx;
	JSObject *obj;
	JSFunction *command;
	char *name;
} my_callback_cd_t;

static jsval c_to_js_var(JSContext *cx, script_var_t *v);
static int js_to_c_var(JSContext *cx, JSObject *obj, jsval val, script_var_t *var, int type);

static JSRuntime *global_js_runtime;
static JSContext *global_js_context;
static JSObject  *global_js_object;

static char *error_logfile = NULL;

/* Load a JS script. */
static int my_load_script(registry_entry_t *entry, char *fname)
{
	int result;
	int len;

	/* Check the filename and make sure it ends in .tcl */
	len = strlen(fname);
	if (len < 3 || fname[len-1] != 's' || fname[len-2] != 'j' || fname[len-3] != '.') {
		/* Nope, let someone else load it. */
		return(0);
	}

	/* result = Tcl_EvalFile(ginterp, fname); */
	return(0);
}

/* This function handles trace callbacks from Tcl. */
static int my_linked_var_get()
{
}

static int my_linked_var_set()
{
	script_linked_var_t *linked_var = (script_linked_var_t *)client_data;

	//tcl_to_c_var(irp, obj, &newvalue, linked_var->type);

	/* If they give a callback, then let them handle it. Otherwise, we
		do some default handling for strings and ints. */
	if (linked_var->callbacks && linked_var->callbacks->on_write) {
		int r;

		r = (linked_var->callbacks->on_write)(linked_var, &newvalue);
		return(r);
	}
	switch (linked_var->type & SCRIPT_TYPE_MASK) {
		case SCRIPT_UNSIGNED:
		case SCRIPT_INTEGER:
			/* linked_var->value is a pointer to an int/uint */
			*(int *)(linked_var->value) = (int) newvalue.value;
			break;
		case SCRIPT_STRING: {
			/* linked_var->value is a pointer to a (char *) */
			char **charptr = (char **)(linked_var->value);

			/* Free the old value. */
			if (*charptr) free(*charptr);

			/* If we copied the string (> 8.0) then just use the copy. */
			if (newvalue.type & SCRIPT_FREE) *charptr = newvalue.value;
			else *charptr = strdup(newvalue.value);
			break;
		}
		default:
			return(0);
	}
}

/* This function creates the JS <-> C variable linkage on reads, writes, and unsets. */
static int my_link_var(void *ignore, script_linked_var_t *var)
{
	char *varname;
	JSObject *obj;

	if (var->class && strlen(var->class)) varname = msprintf("%s(%s)", var->class, var->name);
	else varname = strdup(var->name);

	set_linked_var(var);
	obj = JS_DefineObject(global_js_context, global_js_object,
		varname, NULL, NULL, JSPROP_PERMANENT | JSPROP_EXPORTED);
	JS_SetPrivate(global_js_context, obj, var);

	free(varname);
	return(0);
}

/* This function deletes JS <-> C variable links. */
static int my_unlink_var(void *ignore, script_linked_var_t *var)
{
	char *varname;

	if (var->class && strlen(var->class)) varname = msprintf("%s(%s)", var->class, var->name);
	else varname = strdup(var->name);

	JS_DeleteProperty(global_js_context, global_js_object, varname);

	free(varname);
	return(0);
}

/*
static void log_error_message(Tcl_Interp *myinterp)
{
	FILE *fp;
	char *errmsg;
	time_t timenow;

	errmsg = Tcl_GetStringResult(myinterp);
	putlog(LOG_MISC, "*", "Tcl Error: %s", errmsg);

	if (!error_logfile || !error_logfile[0]) return;

	timenow = time(NULL);
	fp = fopen(error_logfile, "a");
	if (!fp) putlog(LOG_MISC, "*", "Error opening Tcl error log (%s)!", error_logfile);
	else {
		errmsg = Tcl_GetVar(myinterp, "errorInfo", TCL_GLOBAL_ONLY);
		fprintf(fp, "%s", asctime(localtime(&timenow)));
		fprintf(fp, "%s\n\n", errmsg);
		fclose(fp);
	}
}
*/

/* When you use a script_callback_t's callback() function, this gets executed.
	It converts the C variables to JS variables and executes the JS script.
*/
static int my_js_callbacker(script_callback_t *me, ...)
{
	jsval *argv, result = 0;
	script_var_t var;
	my_callback_cd_t *cd; /* My callback client data */
	int i, n, retval;
	void **al;

	/* This struct contains the interp and the obj command. */
	cd = (my_callback_cd_t *)me->callback_data;

	al = (void **)&me;
	al++;
	if (me->syntax) n = strlen(me->syntax);
	else n = 0;
	for (i = 0; i < n; i++) {
		var.type = me->syntax[i];
		var.value = al[i];
		var.len = -1;
		argv[i] = c_to_js_var(cd->mycx, &var);
	}

	n = JS_CallFunction(cd->mycx, cd->myobj, cd->command, n, argv, &result);
	if (n) {
		int32 intval;

		if (JS_ValueToInt32(cx, val, &intval)) retval = intval;
	}

	/* If it's a one-time callback, delete it. */
	if (me->flags & SCRIPT_CALLBACK_ONCE) me->del(me);

	return(retval);
}

/* This implements the delete() member of JS script callbacks. */
static int my_tcl_cb_delete(script_callback_t *me)
{
	my_callback_cd_t *cd;

	cd = (my_callback_cd_t *)me->callback_data;
	free(cd->name);
	if (me->syntax) free(me->syntax);
	free(cd);
	free(me);
	return(0);
}

/* Create JS commands with a C callback. */
static int my_create_cmd(void *ignore, script_command_t *info)
{
	char *cmdname;
	JSFunction *func;
	JSObject *obj;

	if (info->class && strlen(info->class)) {
		cmdname = msprintf("%s_%s", info->class, info->name);
	}
	else {
		cmdname = strdup(info->name);
	}
	func = JS_NewFunction(global_js_context, my_command_handler, 0, 0, NULL, cmdname);
	free(cmdname);

	obj = JS_GetFunctionObject(func);
	JS_SetPrivate(global_js_context, obj, info);

	return(0);
}

/* Delete JS commands. */
static int my_delete_cmd(void *ignore, script_command_t *info)
{
	char *cmdname;

	if (info->class && strlen(info->class)) {
		cmdname = msprintf("%s_%s", info->class, info->name);
	}
	else {
		cmdname = strdup(info->name);
	}
	/* Don't know how to delete commands yet! */
	free(cmdname);

	return(0);
}

/* Convert a C variable to a Tcl variable. */
static jsval c_to_js_var(JSContext *cx, script_var_t *v)
{
	jsval result;

	result = JS_VOID;

	/* If it's an array, we call ourselves recursively. */
	if (v->type & SCRIPT_ARRAY) {
		JSObject *js_array;
		jsval element;
		int i;

		js_array = JS_NewArrayObject(cx, 0, NULL);

		/* If it's an array of script_var_t's, then it's easy. */
		if ((v->type & SCRIPT_TYPE_MASK) == SCRIPT_VAR) {
			script_var_t **v_list;

			v_list = (script_var_t **)v->value;
			for (i = 0; i < v->len; i++) {
				element = c_to_js_var(cx, v_list[i]);
				JS_SetElement(cx, js_array, i, &element);
			}
		}
		else {
			/* Otherwise, we have to turn them into fake script_var_t's. */
			script_var_t v_sub;
			void **values;

			v_sub.type = v->type & (~SCRIPT_ARRAY);
			v_sub.len = -1;
			values = (void **)v->value;
			for (i = 0; i < v->len; i++) {
				v_sub.value = values[i];
				element = c_to_js_var(cx, &v_sub);
				JS_SetElement(cx, js_array, i, &element);
			}
		}
		/* Whew */
		if (v->type & SCRIPT_FREE) free(v->value);
		if (v->type & SCRIPT_FREE_VAR) free(v);
		result = OBJECT_TO_JSVAL(js_array);
		return(result);
	}

	/* Here is where we handle the basic types. */
	switch (v->type & SCRIPT_TYPE_MASK) {
		case SCRIPT_INTEGER:
		case SCRIPT_UNSIGNED:
			result = INT_TO_JSVAL(((int) v->value));
			break;
		case SCRIPT_STRING:
		case SCRIPT_BYTES: {
			char *str = v->value;

			if (!str) str = "";
			if (v->len == -1) v->len = strlen(str);
			result = STRING_TO_JSVAL(JS_NewStringCopyN(cx, str, v->len));

			if (v->value && v->type & SCRIPT_FREE) free((char *)v->value);
			break;
		}
		case SCRIPT_POINTER: {
			char str[32];

			sprintf(str, "#%u", (unsigned int) v->value);
			result = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, str));
			break;
		}
		case SCRIPT_USER: {
			/* An eggdrop user record (struct userrec *). */
			char *handle;
			struct userrec *u = (struct userrec *)v->value;

			if (u) handle = u->handle;
			else handle = "*";
			result = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, handle));
			break;
		}
		default:
			/* Default: just pass a string with an error message. */
			result = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "Unsupported type"));
	}
	if (v->type & SCRIPT_FREE_VAR) free(v);
	return(result);
}

/* Here we convert JS variables into C variables.
	When we return, var->type may have the SCRIPT_FREE bit set, in which case
	you should free(var->value) when you're done with it. */
static int js_to_c_var(JSContext *cx, JSObject *obj, jsval val, script_var_t *var, int type)
{
	int err = 0;

	var->type = type;
	var->len = -1;
	var->value = NULL;

	switch (type) {
		case SCRIPT_STRING: {
			JSString *str;
			int len;

			str = JS_ValueToString(cx, val);
			var->value = JS_GetStringBytes(str);
			var->len = JS_GetStringLength(str);
			break;
		}
		case SCRIPT_BYTES: {
			byte_array_t *byte_array;
			JSString *str;

			byte_array = (byte_array_t *)malloc(sizeof(*byte_array));

			str = JS_ValueToString(cx, val);
			byte_array->bytes = JS_GetStringBytes(str);
			byte_array->len = JS_GetStringLength(str);

			var->value = byte_array;
			var->type |= SCRIPT_FREE;
			break;
		}
		case SCRIPT_UNSIGNED:
		case SCRIPT_INTEGER: {
			int32 intval;

			err = !JS_ValueToInt32(cx, val, &intval);
			var->value = (void *)intval;
			break;
		}
		case SCRIPT_CALLBACK: {
			JSFunction *func;
			script_callback_t *cback; /* Callback struct */
			my_callback_cd_t *cdata; /* Our client data */

			func = JS_ValueToFunction(cx, val);
			if (!func) {
				err = 1;
				break;
			}

			cback = (script_callback_t *)calloc(1, sizeof(*cback));
			cdata = (my_callback_cd_t *)calloc(1, sizeof(*cdata));
			cback->callback = (Function) my_js_callbacker;
			cback->callback_data = (void *)cdata;
			cback->del = (Function) my_js_cb_delete;
			cback->name = strdup(JS_GetFunctionName(func));

			cdata->mycx = cx;
			cdata->myobj = obj;
			cdata->command = func;

			var->value = cback;
			break;
		}
		case SCRIPT_USER: {
			struct userrec *u;
			char *handle;

			handle = JS_ValueToString(cx, val);
			u = get_user_by_handle(userlist, handle);
			var->value = u;
			if (!u) {
				JS_ReportError(cx, "User not found: %s", handle);
				err = 1;
			}
			break;
		}
		default: {
			char vartype[2];

			vartype[0] = type;
			vartype[1] = 0;
			JS_ReportError(myinterp, "Cannot convert JS object to unknown variable type '%s'", vartype);
			err = 1;
		}
	}

	return(err);
}

/* This function cleans up temporary buffers we used when calling
	a C command. */
static int my_argument_cleanup(mstack_t *args, mstack_t *bufs, mstack_t *cbacks, int err)
{
	int i;

	for (i = 0; i < bufs->len; i++) {
		free((void *)bufs->stack[i]);
	}
	if (err) {
		for (i = 0; i < cbacks->len; i++) {
			script_callback_t *cback;

			cback = (script_callback_t *)cbacks->stack[i];
			cback->del(cback);
		}
	}
	mstack_destroy(cbacks);
	mstack_destroy(bufs);
	mstack_destroy(args);
	return(0);
}

/* This handles all the JS commands we create. It converts the JS arguments
	to C variables, calls the C callback function, then converts
	the return value to a JS variable. */
static int my_command_handler(JSContext *cx, JSObject *obj, int argc, jsval *argv, jsval *rval)
{
	script_command_t *cmd;
	script_var_t retval, var;
	mstack_t *args, *bufs, *cbacks;
	void **al; /* Argument list to pass to the callback*/
	int nopts; /* Number of optional args we're passing. */
	int simple_retval; /* Return value for simple commands. */
	int i, err, skip;
	char *syntax;

	/* Get the associated command structure stored in the object. */
	cmd = (script_command_t *)JS_GetPrivate(cx, obj);

	/* Check for proper number of args. */
	if (cmd->flags & SCRIPT_VAR_ARGS) i = (cmd->nargs <= (objc-1));
	else i = (cmd->nargs < 0 || cmd->nargs == (objc-1));

	if (!i) {
		JS_ReportError(cx, "Wrong number of arguments: %s", cmd->syntax_error);
		return JS_FALSE;
	}

	/* We want space for at least 10 args. */
	args = mstack_new(2*objc+10);

	/* Reserve space for 3 optional args. */
	mstack_push(args, NULL);
	mstack_push(args, NULL);
	mstack_push(args, NULL);

	/* Have some space for buffers too. */
	bufs = mstack_new(objc);

	/* And callbacks. */
	cbacks = mstack_new(objc);

	/* Parse arguments. */
	syntax = cmd->syntax;
	if (cmd->flags & SCRIPT_VAR_FRONT) {
		/* See how many args to skip. */
		skip = strlen(syntax) - (objc-1);
		if (skip < 0) skip = 0;
		for (i = 0; i < skip; i++) mstack_push(args, NULL);
		syntax += skip;
	}
	else skip = 0;

	err = TCL_OK;
	for (i = 0; i < argc; i++) {
		err = js_to_c_var(cx, obj, argv[i], &var, *syntax);

		if (err) break;

		if (var.type & SCRIPT_FREE) mstack_push(bufs, var.value);
		else if (*syntax == SCRIPT_CALLBACK) mstack_push(cbacks, var.value);

		mstack_push(args, var.value);
		syntax++;
	}

	if (err) {
		my_argument_cleanup(args, bufs, cbacks, 1);
		JS_ReportError(cx, "Invalid arguments, should be: %s", cmd->syntax_error);
		return JS_FALSE;
	}

	memset(&retval, 0, sizeof(retval));

	al = args->stack; /* Argument list shortcut name. */
	nopts = 0;
	if (cmd->flags & SCRIPT_PASS_COUNT) {
		al[2-nopts] = (void *)(args->len - 3 - skip);
		nopts++;
	}
	if (cmd->flags & SCRIPT_PASS_RETVAL) {
		al[2-nopts] = &retval;
		nopts++;
	}
	if (cmd->flags & SCRIPT_PASS_CDATA) {
		al[2-nopts] = cmd->client_data;
		nopts++;
	}
	al += (3-nopts);
	args->len -= (3-nopts);

	if (cmd->flags & SCRIPT_PASS_ARRAY) {
		simple_retval = cmd->callback(args->len, al);
	}
	else {
		simple_retval = cmd->callback(al[0], al[1], al[2], al[3], al[4]);
	}

	if (!(cmd->flags & SCRIPT_PASS_RETVAL)) {
		retval.type = cmd->retval_type;
		retval.len = -1;
		retval.value = (void *)simple_retval;
	}

	err = retval.type & SCRIPT_ERROR;
	*rval = c_to_tcl_var(myinterp, &retval);

	my_argument_cleanup(args, bufs, cbacks, 0);

	if (err) return JS_FALSE;
	return JS_TRUE;
}

static int javascript_init()
{
	JSVersion version;
	JSObject  *it;
	JSClass global_class = {"global", 0, 
		JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,
		JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub
	};
	JSBool builtins;

	/* Create a new runtime environment. */
	global_js_runtime = JS_NewRuntime(0x100000);
	if (!global_js_runtime) return(1);

	/* Create a new context. */
	global_js_context = JS_NewContext(global_js_runtime, 0x1000);
	if (!global_js_context) return(1);

	/* Create the global object here */
	globl_js_object = JS_NewObject(global_js_context, &global_class, NULL, NULL);

	/* Initialize the built-in JS objects and the global object */
	builtins = JS_InitStandardClasses(global_js_context, global_js_object);
}

static registry_simple_chain_t my_functions[] = {
	{"script", NULL, 0}, /* First arg gives our class */
	{"load script", my_load_script, 2}, /* name, ptr, nargs */
	{"link var", my_link_var, 2},
	{"unlink var", my_unlink_var, 2},
	{"create cmd", my_create_cmd, 2},
	{"delete cmd", my_delete_cmd, 2},
	{0}
};

static Function journal_table[] = {
	(Function)1,	/* Version */
	(Function)5,	/* Number of functions */
	my_load_script,
	my_link_var,
	my_unlink_var,
	my_create_cmd,
	my_delete_cmd
};

static Function journal_playback;
static void *journal_playback_h;

/* Here we process the dcc console .tcl command. */
static int cmd_js(struct userrec *u, int idx, char *text)
{
	char *str;
	static int curline = 1;

	if (!isowner(dcc[idx].nick)) {
		dprintf(idx, _("You must be a permanent owner (defined in the config file) to use this command.\n"));
		return(BIND_RET_LOG);
	}

	retval = JS_EvaluateScript(global_js_context, global_js_object,
			text, strlen(text), "console", curline++);
	if (retval) {
		dprintf(idx, "JS Error: unknown for now\n");
	}
	else {
		str = JSVAL_TO_STRING(js_rval);
		dprintf(idx, "JS: %s\n", str);
	}
	return(0);
}

static void javascript_report(int idx, int details)
{
	JSVersion version;
	const char *version_str;

	version = JS_GetVersion(global_js_context);
	version_str = JS_VersionToString(version);

	if (!details) {
		dprintf(idx, "Using JavaScript version %s\n", version_str);
		return;
	}

	dprintf(idx, "    Using JavaScript version %s\n", version_str);
}

static cmd_t dcc_commands[] = {
	{"js", 	"n", 	(Function) cmd_js,	NULL},
	{0}
};

EXPORT_SCOPE char *javascript_LTX_start();
static char *javascript_close();

static Function javascript_table[] = {
	(Function) javascript_LTX_start,
	(Function) javascript_close,
	(Function) 0,
	(Function) javascript_report
};

char *javascript_LTX_start(eggdrop_t *eggdrop)
{
	bind_table_t *dcc_table;

	egg = eggdrop;

	javascript_init();

	module_register("javascript", javascript_table, 1, 2);
	if (!module_depend("javascript", "eggdrop", 107, 0)) {
		module_undepend("javascript");
		return "This module requires eggdrop1.7.0 or later";
	}

	error_logfile = strdup("logs/javascript_errors.log");

	registry_add_simple_chains(my_functions);
	registry_lookup("script", "playback", &journal_playback, &journal_playback_h);
	if (journal_playback) journal_playback(journal_playback_h, journal_table);

	dcc_table = find_bind_table2("dcc");
	if (dcc_table) add_builtins2(dcc_table, dcc_commands);

	return(NULL);
}

static char *javascript_close()
{
	bind_table_t *dcc_table;

	/* When tcl is gone from the core, this will be uncommented. */
	/* Tcl_DeleteInterp(ginterp); */

	dcc_table = find_bind_table2("dcc");
	if (dcc_table) rem_builtins2(dcc_table, dcc_commands);

	module_undepend("javascript");
	return(NULL);
}

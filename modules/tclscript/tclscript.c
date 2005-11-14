/* tclscript.c: Tcl scripting support
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
static const char rcsid[] = "$Id: tclscript.c,v 1.48 2005/11/14 04:44:43 wcc Exp $";
#endif

#include <string.h>
#include <stdlib.h>

#include <tcl.h>

#include <eggdrop/eggdrop.h>

#if (((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 1)) || (TCL_MAJOR_VERSION > 8))
#  define USE_TCL_BYTE_ARRAYS
#endif

/* Data we need for a tcl callback. */
typedef struct {
	Tcl_Interp *myinterp;
	Tcl_Obj *command;
	char *name;
} my_callback_cd_t;

/* Data we need to process command arguments. */
typedef struct {
	Tcl_Interp *irp;
	Tcl_Obj *CONST *objv;
} my_args_data_t;

static int my_command_handler(ClientData client_data, Tcl_Interp *myinterp, int objc, Tcl_Obj *CONST objv[]);
static Tcl_Obj *c_to_tcl_var(Tcl_Interp *myinterp, script_var_t *v);
static int tcl_to_c_var(Tcl_Interp *myinterp, Tcl_Obj *obj, script_var_t *var, int type);

/* Implementation of the script module interface. */
static int my_load_script(void *ignore, char *fname);
static int my_link_var(void *ignore, script_linked_var_t *var);
static int my_unlink_var(void *ignore, script_linked_var_t *var);
static int my_create_command(void *ignore, script_raw_command_t *info);
static int my_delete_command(void *ignore, script_raw_command_t *info);
static int my_get_arg(void *ignore, script_args_t *args, int num, script_var_t *var, int type);

static script_module_t my_script_interface = {
	"Tcl", NULL,
	my_load_script,
	my_link_var, my_unlink_var,
	my_create_command, my_delete_command,
	my_get_arg
};

static Tcl_Interp *ginterp; /* Our global interpreter. */

static char *error_logfile = NULL;

#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION < 2)
/* We emulate the Tcl_SetVar2Ex and Tcl_GetVar2Ex functions for 8.0 and 8.1. */
static Tcl_Obj *Tcl_SetVar2Ex(Tcl_Interp *myinterp, const char *name1, const char *name2, Tcl_Obj *newval, int flags) {
	Tcl_Obj *part1, *part2, *obj;

	part1 = Tcl_NewStringObj((char *)name1, -1);
	part2 = Tcl_NewStringObj((char *)name2, -1);
	obj = Tcl_ObjSetVar2(myinterp, part1, part2, newval, flags);
	Tcl_DecrRefCount(part1);
	Tcl_DecrRefCount(part2);
	return(obj);
}

static Tcl_Obj *Tcl_GetVar2Ex(Tcl_Interp *myinterp, const char *name1, const char *name2, int flags) {
	Tcl_Obj *part1, *part2, *obj;

	part1 = Tcl_NewStringObj((char *)name1, -1);
	part2 = Tcl_NewStringObj((char *)name2, -1);
	obj = Tcl_ObjGetVar2(myinterp, part1, part2, flags);
	Tcl_DecrRefCount(part1);
	Tcl_DecrRefCount(part2);
	return(obj);
}
#endif

/* Load a tcl script. */
static int my_load_script(void *ignore, char *fname)
{
	int result;
	int len;

	/* Check the filename and make sure it ends in .tcl */
	len = strlen(fname);
	if (len < 4 || fname[len-1] != 'l' || fname[len-2] != 'c' || fname[len-3] != 't' || fname[len-4] != '.') {
		/* Nope, let someone else load it. */
		return SCRIPT_ERR_NOT_RESPONSIBLE;
	}

	result = Tcl_EvalFile (ginterp, fname);
	if (result != TCL_OK) {
		putlog (LOG_MISC, "*", _("Failed to load script %s: %s"),
				fname, Tcl_GetStringResult (ginterp));
		return SCRIPT_ERR_CODE;
	}

	return SCRIPT_OK; 
}

/* This updates the value of a linked variable (c to tcl). */
static void set_linked_var(script_linked_var_t *var, script_var_t *val)
{
	Tcl_Obj *obj;
	script_var_t script_var;

	if (!val || !val->type) {
		script_var.type = var->type & SCRIPT_TYPE_MASK;
		script_var.len = -1;
		script_var.value = *(void **)var->value;
		val = &script_var;
	}

	obj = c_to_tcl_var(ginterp, val);

	if (var->class && strlen(var->class)) {
		Tcl_SetVar2Ex(ginterp, var->class, var->name, obj, TCL_GLOBAL_ONLY);
	}
	else {
		Tcl_SetVar2Ex(ginterp, var->name, NULL, obj, TCL_GLOBAL_ONLY);
	}
}

/* This function handles trace callbacks from Tcl. */
static char *my_trace_callback(ClientData client_data, Tcl_Interp *irp, char *name1, char *name2, int flags)
{
	script_linked_var_t *linked_var = (script_linked_var_t *)client_data;
	script_var_t newvalue = {0};

	if (flags & TCL_INTERP_DESTROYED) return(NULL);

	if (flags & TCL_TRACE_READS) {
		if (linked_var->callbacks && linked_var->callbacks->on_read) {
			script_var_t newvalue = {0};
			int r = (linked_var->callbacks->on_read)(linked_var, &newvalue);
			if (r) return(NULL);
		}
		set_linked_var(linked_var, &newvalue);
	}
	else if (flags & TCL_TRACE_WRITES) {
		Tcl_Obj *obj;

		if (linked_var->type & SCRIPT_READONLY) return("read only variable");

		obj = Tcl_GetVar2Ex(irp, name1, name2, 0);
		if (!obj) return("Error setting variable");

		tcl_to_c_var(irp, obj, &newvalue, linked_var->type);
		script_linked_var_on_write(linked_var, &newvalue);
	}
	else if (flags & TCL_TRACE_UNSETS) {
		/* If someone unsets a variable, we'll just reset it. */
		if (flags & TCL_TRACE_DESTROYED) my_link_var(NULL, linked_var);
		else set_linked_var(linked_var, NULL);
		return("read only variable");
	}
	return(NULL);
}

/* This function creates the Tcl <-> C variable linkage on reads, writes, and unsets. */
static int my_link_var(void *ignore, script_linked_var_t *var)
{
	char *varname;

	if (var->class && strlen(var->class)) varname = egg_mprintf("%s(%s)", var->class, var->name);
	else varname = strdup(var->name);

	set_linked_var(var, NULL);
	Tcl_TraceVar(ginterp, varname, TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS, (Tcl_VarTraceProc *)my_trace_callback, var);

	free(varname);
	return(0);
}

/* This function deletes Tcl <-> C variable links. */
static int my_unlink_var(void *ignore, script_linked_var_t *var)
{
	char *varname;

	if (var->class && strlen(var->class)) varname = egg_mprintf("%s(%s)", var->class, var->name);
	else varname = strdup(var->name);

	Tcl_UntraceVar(ginterp, varname, TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS, (Tcl_VarTraceProc *)my_trace_callback, var);

	free(varname);
	return(0);
}

static void log_error_message(Tcl_Interp *myinterp)
{
	FILE *fp;
	const char *errmsg;
	time_t timenow;

	errmsg = Tcl_GetStringResult(myinterp);
	putlog(LOG_MISC, "*", "Tcl Error: %s", errmsg);

	if (!error_logfile || !error_logfile[0]) return;

	timenow = time(NULL);
	fp = fopen(error_logfile, "a");
	if (!fp) putlog(LOG_MISC, "*", _("Error opening Tcl error log (%s)!"), error_logfile);
	else {
		errmsg = Tcl_GetVar(myinterp, "errorInfo", TCL_GLOBAL_ONLY);
		fprintf(fp, "%s", asctime(localtime(&timenow)));
		fprintf(fp, "%s\n\n", errmsg);
		fclose(fp);
	}
}

/* When you use a script_callback_t's callback() function, this gets executed.
	It converts the C variables to Tcl variables and executes the Tcl script. */
static int my_tcl_callbacker(script_callback_t *me, ...)
{
	Tcl_Obj *arg, *final_command, *result;
	Tcl_Interp *interp;
	script_var_t var;
	my_callback_cd_t *cd; /* My callback client data */
	int i, n, retval;
	va_list va;

	/* This struct contains the interp and the obj command. */
	cd = (my_callback_cd_t *)me->callback_data;

	/* Get a copy of the command, then append args. */
	final_command = Tcl_DuplicateObj(cd->command);

	if (me->syntax) n = strlen(me->syntax);
	else n = 0;

	va_start(va, me);
	for (i = 0; i < n; i++) {
		var.type = me->syntax[i];
		if (var.type == SCRIPT_INTEGER || var.type == SCRIPT_UNSIGNED) var.value = (void *) (va_arg(va, int));
		else var.value = va_arg(va, void *);
		var.len = -1;
		arg = c_to_tcl_var(cd->myinterp, &var);
		Tcl_ListObjAppendElement(cd->myinterp, final_command, arg);
	}
	va_end(va);

	interp = cd->myinterp;

	/* If it's a one-time callback, delete it. */
	if (me->flags & SCRIPT_CALLBACK_ONCE) me->del(me);

#ifdef USE_TCL_BYTE_ARRAYS
	n = Tcl_EvalObjEx(interp, final_command, TCL_EVAL_GLOBAL);
#else
	n = Tcl_GlobalEvalObj(interp, final_command);
#endif

	if (n == TCL_OK) {
		result = Tcl_GetObjResult(interp);
		Tcl_GetIntFromObj(interp, result, &retval);
	}
	else {
		log_error_message(interp);
		Tcl_BackgroundError(interp);
	}

	/* Clear any errors or stray messages. */
	Tcl_ResetResult(interp);

	return(retval);
}

/* This implements the delete() member of Tcl script callbacks. */
static int my_tcl_cb_delete(script_callback_t *me)
{
	my_callback_cd_t *cd;

	cd = (my_callback_cd_t *)me->callback_data;
	Tcl_DecrRefCount(cd->command);
	if (cd->name) free(cd->name);
	if (me->syntax) free(me->syntax);
	if (me->name) free(me->name);
	free(cd);
	free(me);
	return(0);
}

/* Create Tcl commands with a C callback. */
static int my_create_command(void *ignore, script_raw_command_t *info)
{
	char *cmdname;

	if (info->class && strlen(info->class)) {
		cmdname = egg_mprintf("%s_%s", info->class, info->name);
	}
	else {
		cmdname = strdup(info->name);
	}
	Tcl_CreateObjCommand(ginterp, cmdname, my_command_handler, (ClientData) info, NULL);
	free(cmdname);

	return(0);
}

/* Delete Tcl commands. */
static int my_delete_command(void *ignore, script_raw_command_t *info)
{
	char *cmdname;

	if (info->class && strlen(info->class)) {
		cmdname = egg_mprintf("%s_%s", info->class, info->name);
	}
	else {
		cmdname = strdup(info->name);
	}
	Tcl_DeleteCommand(ginterp, cmdname);
	free(cmdname);

	return(0);
}

/* Convert a C variable to a Tcl variable. */
static Tcl_Obj *c_to_tcl_var(Tcl_Interp *myinterp, script_var_t *v)
{
	Tcl_Obj *result;

	result = NULL;
	/* If it's an array, we call ourselves recursively. */
	if (v->type & SCRIPT_ARRAY) {
		Tcl_Obj *element;
		int i;

		result = Tcl_NewListObj(0, NULL);
		/* If it's an array of script_var_t's, then it's easy. */
		if ((v->type & SCRIPT_TYPE_MASK) == SCRIPT_VAR) {
			script_var_t **v_list;

			v_list = (script_var_t **)v->value;
			for (i = 0; i < v->len; i++) {
				element = c_to_tcl_var(myinterp, v_list[i]);
				Tcl_ListObjAppendElement(myinterp, result, element);
			}
		}
		else {
			/* Otherwise, we have to turn them into fake script_var_t's. */
			script_var_t v_sub;
			void **values;

			values = (void **)v->value;
			for (i = 0; i < v->len; i++) {
				v_sub.type = v->type & (~SCRIPT_ARRAY);
				v_sub.value = values[i];
				v_sub.len = -1;
				element = c_to_tcl_var(myinterp, &v_sub);
				Tcl_ListObjAppendElement(myinterp, result, element);
			}
		}
		/* Whew */
		if (v->type & SCRIPT_FREE) free(v->value);
		if (v->type & SCRIPT_FREE_VAR) free(v);
		return(result);
	}

	/* Here is where we handle the basic types. */
	switch (v->type & SCRIPT_TYPE_MASK) {
		case SCRIPT_INTEGER:
		case SCRIPT_UNSIGNED:
			result = Tcl_NewIntObj((int) v->value);
			break;
		case SCRIPT_STRING: {
			char *str = v->value;

			if (!str) str = "";
			if (v->len == -1) v->len = strlen(str);
#ifdef USE_TCL_BYTE_ARRAYS
			result = Tcl_NewByteArrayObj(str, v->len);
#else
			result = Tcl_NewStringObj(str, v->len);
#endif
			if (v->value && v->type & SCRIPT_FREE) free(v->value);
			break;
		}
		case SCRIPT_STRING_LIST: {
			char **str = v->value;
			Tcl_Obj *element;

			result = Tcl_NewListObj(0, NULL);
			while (str && *str) {
#ifdef USE_TCL_BYTE_ARRAYS
				element = Tcl_NewByteArrayObj(*str, strlen(*str));
#else
				element = Tcl_NewStringObj(*str, strlen(*str));
#endif
				Tcl_ListObjAppendElement(myinterp, result, element);
				str++;
			}
			break;
		}
		case SCRIPT_BYTES: {
			byte_array_t *bytes = v->value;
#ifdef USE_TCL_BYTE_ARRAYS
			result = Tcl_NewByteArrayObj(bytes->bytes, bytes->len);
#else
			result = Tcl_NewStringObj(bytes->bytes, bytes->len);
#endif
			if (bytes->do_free) free(bytes->bytes);
			if (v->type & SCRIPT_FREE) free(bytes);
			break;
		}
		case SCRIPT_POINTER: {
			char str[32];

			sprintf(str, "#%u", (unsigned int) v->value);
			result = Tcl_NewStringObj(str, -1);
			break;
		}
		case SCRIPT_PARTIER: {
			partymember_t *p = v->value;
			int pid;

			if (p) pid = p->pid;
			else pid = -1;
			result = Tcl_NewIntObj(pid);
			break;
		}
		case SCRIPT_USER: {
			/* An eggdrop user record (struct userrec *). */
			char *handle;
			user_t *u = v->value;

			if (u) handle = u->handle;
			else handle = "*";
			result = Tcl_NewStringObj(handle, -1);
			break;
		}
		default:
			/* Default: just pass a string with an error message. */
			result = Tcl_NewStringObj("unsupported type", -1);
	}
	if (v->type & SCRIPT_FREE_VAR) free(v);
	return(result);
}

/* Here we convert Tcl variables into C variables.
	When we return, var->type may have the SCRIPT_FREE bit set, in which case
	you should free(var->value) when you're done with it. */
static int tcl_to_c_var(Tcl_Interp *myinterp, Tcl_Obj *obj, script_var_t *var, int type)
{
	int err = TCL_OK;

	var->type = type;
	var->len = -1;
	var->value = NULL;

	switch (type & SCRIPT_TYPE_MASK) {
		case SCRIPT_STRING: {
			char *str;
			int len;

#ifdef USE_TCL_BYTE_ARRAYS
				char *bytes;

				bytes = Tcl_GetByteArrayFromObj(obj, &len);
				str = (char *)malloc(len+1);
				memcpy(str, bytes, len);
				str[len] = 0;
				var->type |= SCRIPT_FREE;
#else
				str = Tcl_GetStringFromObj(obj, &len);
#endif
			var->value = str;
			var->len = len;
			break;
		}
		case SCRIPT_BYTES: {
			byte_array_t *byte_array;

			byte_array = (byte_array_t *)malloc(sizeof(*byte_array));

#ifdef USE_TCL_BYTE_ARRAYS
			byte_array->bytes = Tcl_GetByteArrayFromObj(obj, &byte_array->len);
#else
			byte_array->bytes = Tcl_GetStringFromObj(obj, &byte_array->len);
#endif

			var->value = byte_array;
			var->type |= SCRIPT_FREE;
			break;
		}
		case SCRIPT_UNSIGNED:
		case SCRIPT_INTEGER: {
			int intval = 0;

			err = Tcl_GetIntFromObj(myinterp, obj, &intval);
			var->value = (void *)intval;
			break;
		}
		case SCRIPT_CALLBACK: {
			script_callback_t *cback; /* Callback struct */
			my_callback_cd_t *cdata; /* Our client data */

			cback = (script_callback_t *)calloc(1, sizeof(*cback));
			cdata = (my_callback_cd_t *)calloc(1, sizeof(*cdata));
			cback->callback = (Function) my_tcl_callbacker;
			cback->callback_data = cdata;
			cback->del = (Function) my_tcl_cb_delete;
			cback->name = strdup(Tcl_GetString(obj));
			cdata->myinterp = myinterp;
			cdata->command = obj;
			Tcl_IncrRefCount(obj);

			var->value = cback;
			break;
		}
		case SCRIPT_PARTIER: {
			int pid = -1;

			err = Tcl_GetIntFromObj(myinterp, obj, &pid);
			if (!err) var->value = partymember_lookup_pid(pid);
			else var->value = NULL;
			break;
		}
		case SCRIPT_USER: {
			user_t *u;
			script_var_t handle;

			/* Call ourselves recursively to get the handle as a string. */
			tcl_to_c_var(myinterp, obj, &handle, SCRIPT_STRING);
			u = user_lookup_by_handle((char *)handle.value);
			if (handle.type & SCRIPT_FREE) free(handle.value);
			var->value = u;
			if (!u) {
				Tcl_AppendResult(myinterp, "User not found", NULL);
				err++;
			}
			break;
		}
		default: {
			char vartype[2];

			vartype[0] = type;
			vartype[1] = 0;
			Tcl_AppendResult(myinterp, _("Cannot convert Tcl object to unknown variable type '"), vartype, "'.", NULL);
			err++;
		}
	}

	return(err);
}

/* This handles all the Tcl commands we create. */
static int my_command_handler(ClientData client_data, Tcl_Interp *myinterp, int objc, Tcl_Obj *CONST objv[])
{
	script_raw_command_t *cmd = (script_raw_command_t *)client_data;
	script_var_t retval;
	Tcl_Obj *tcl_retval = NULL;
	script_args_t args;
	my_args_data_t argdata;
	int err;

	/* Initialize args. */
	argdata.irp = myinterp;
	argdata.objv = objv;
	args.module = &my_script_interface;
	args.client_data = &argdata;
	args.len = objc-1;

	/* Initialize retval. */
	retval.type = 0;
	retval.value = NULL;
	retval.len = -1;

	/* Execute callback. */
	cmd->callback(cmd->client_data, &args, &retval);
	err = retval.type & SCRIPT_ERROR;

	/* Process the return value. */
	tcl_retval = c_to_tcl_var(myinterp, &retval);

	if (tcl_retval) Tcl_SetObjResult(myinterp, tcl_retval);
	else Tcl_ResetResult(myinterp);

	if (err) return TCL_ERROR;
	return TCL_OK;
}

static int my_get_arg(void *ignore, script_args_t *args, int num, script_var_t *var, int type)
{
	my_args_data_t *argdata;

	argdata = (my_args_data_t *)args->client_data;
	return tcl_to_c_var(argdata->irp, argdata->objv[num+1], var, type);
}

static int party_tcl(partymember_t *p, char *nick, user_t *u, char *cmd, char *text)
{
	const char *str;

	if (!u || !egg_isowner(u->handle)) {
		partymember_write(p, _("You must be a permanent owner (defined in the config file) to use this command.\n"), -1);
		return(BIND_RET_LOG);
	}

	if (!text) {
		partymember_write(p, _("Syntax: .tcl <tclexpression>"), -1);
		return(0);
	}

	if (Tcl_GlobalEval(ginterp, text) != TCL_OK) {
		str = Tcl_GetVar(ginterp, "errorInfo", TCL_GLOBAL_ONLY);
		if (!str) str = Tcl_GetStringResult(ginterp);
		partymember_printf(p, _("Tcl error: %s\n\n"), str);
	}
	else {
		str = Tcl_GetStringResult(ginterp);
		partymember_printf(p, _("Tcl: %s\n\n"), str);
	}
	return(0);
}

typedef struct tcl_listener {
	struct tcl_listener *next;
	char *name;
	int fd;
} tcl_listener_t;

static tcl_listener_t *listener_list_head = NULL;

/* Two Tcl-only commands to add/remove sockbuf listeners. */
static int add_tcl_chan(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
	Tcl_Channel chan;
	char *chan_name;
	int modes;
	void *fd;
	tcl_listener_t *listener;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 0, NULL, "channel-name");
		return(TCL_ERROR);
	}

	chan_name = Tcl_GetStringFromObj(objv[1], NULL);
	if (!chan_name) return(TCL_ERROR);
	chan = Tcl_GetChannel(interp, chan_name, &modes);
	if (!chan) return(TCL_ERROR);
	if (Tcl_GetChannelHandle(chan, TCL_READABLE, &fd)) return(TCL_ERROR);
	listener = malloc(sizeof(*listener));
	listener->next = listener_list_head;
	listener->name = strdup(chan_name);
	listener->fd = (int) fd;
	listener_list_head = listener;
	sockbuf_attach_listener((int) fd);
	return(0);
}

static int rem_tcl_chan(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
	char *chan_name;
	tcl_listener_t *listener, *prev;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 0, NULL, "channel-name");
		return(TCL_ERROR);
	}

	chan_name = Tcl_GetStringFromObj(objv[1], NULL);
	if (!chan_name) return(TCL_ERROR);

	prev = NULL;
	for (listener = listener_list_head; listener; listener = listener->next) {
		if (!strcasecmp(listener->name, chan_name)) break;
		prev = listener;
	}
	if (!listener) return(TCL_ERROR);

	if (prev) prev->next = listener->next;
	else listener_list_head = listener->next;
	sockbuf_detach_listener(listener->fd);
	free(listener->name);
	free(listener);
	return(0);
}

static int tclscript_secondly()
{
	Tcl_DoOneEvent(TCL_ALL_EVENTS | TCL_DONT_WAIT);
	return(0);
}

#if 0
static void tclscript_report(int idx, int details)
{
	char script[512];
	char *reported;

	if (!details) {
		dprintf(idx, _("Using Tcl version %d.%d (by header).\n"), TCL_MAJOR_VERSION, TCL_MINOR_VERSION);
		return;
	}

	dprintf(idx, _("    Using Tcl version %d.%d (by header).\n"), TCL_MAJOR_VERSION, TCL_MINOR_VERSION);
	sprintf(script, _("return \"    Library: [info library]\\n    Reported version: [info tclversion]\\n    Reported patchlevel: [info patchlevel]\""));
	Tcl_GlobalEval(ginterp, script);
	reported = Tcl_GetStringResult(ginterp);
	dprintf(idx, "%s\n", reported);
}
#endif

static bind_list_t party_commands[] = {
	{"n", "tcl", (Function) party_tcl},
	{0}
};

static bind_list_t secondly_binds[] = {
	{NULL, NULL, tclscript_secondly},
	{0}
};

static int tclscript_close(int why)
{
	Tcl_DeleteInterp(ginterp);

	bind_rem_list("party", party_commands);
	bind_rem_list("secondly", secondly_binds);

	script_unregister_module(&my_script_interface);
	return(0);
}

EXPORT_SCOPE int tclscript_LTX_start(egg_module_t *modinfo);

int tclscript_LTX_start(egg_module_t *modinfo)
{
	modinfo->name = "tclscript";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
	modinfo->description = "provides tcl scripting support";
	modinfo->close_func = tclscript_close;

	/* Create the interpreter and let tcl load its init.tcl */
	ginterp = Tcl_CreateInterp();
	Tcl_Init(ginterp);

	error_logfile = strdup("logs/tcl_errors.log");
	Tcl_LinkVar(ginterp, "error_logfile", (char *)&error_logfile, TCL_LINK_STRING);

	script_register_module(&my_script_interface);
	script_playback(&my_script_interface);

	bind_add_list("party", party_commands);
	bind_add_list("secondly", secondly_binds);

	Tcl_CreateObjCommand(ginterp, "net_add_tcl", add_tcl_chan, NULL, NULL);
	Tcl_CreateObjCommand(ginterp, "net_rem_tcl", rem_tcl_chan, NULL, NULL);
	return(0);
}


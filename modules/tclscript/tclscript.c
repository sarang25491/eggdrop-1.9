#include "lib/eggdrop/module.h"
#include "lib/egglib/mstack.h"
#include "lib/egglib/msprintf.h"
#include <eggdrop/eggdrop.h>

#define MODULE_NAME "tclscript"

static eggdrop_t *egg = NULL;

#if (TCL_MAJOR_VERSION > 8) || (TCL_MAJOR_VERSION == 8 && TCL_MINOR_VERSION > 1)
	#define USE_BYTE_ARRAYS
#endif

/* Data we need for a tcl callback. */
typedef struct {
	Tcl_Interp *myinterp;
	Tcl_Obj *command;
	char *name;
} my_callback_cd_t;

static int my_command_handler(ClientData client_data, Tcl_Interp *myinterp, int objc, Tcl_Obj *CONST objv[]);
static Tcl_Obj *c_to_tcl_var(Tcl_Interp *myinterp, script_var_t *v);
static int tcl_to_c_var(Tcl_Interp *myinterp, Tcl_Obj *obj, script_var_t *var, int type);
static int my_link_var(void *ignore, script_linked_var_t *var);

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
static int my_load_script(registry_entry_t *entry, char *fname)
{
	int result;
	int len;

	/* Check the filename and make sure it ends in .tcl */
	len = strlen(fname);
	if (len < 4 || fname[len-1] != 'l' || fname[len-2] != 'c' || fname[len-3] != 't' || fname[len-4] != '.') {
		/* Nope, let someone else load it. */
		return(0);
	}

	result = Tcl_EvalFile(ginterp, fname);
	return(0);
}

/* This updates the value of a linked variable (c to tcl). */
static void set_linked_var(script_linked_var_t *var)
{
	Tcl_Obj *obj;
	script_var_t script_var;

	script_var.type = var->type & SCRIPT_TYPE_MASK;
	script_var.len = -1;
	script_var.value = *(void **)var->value;

	obj = c_to_tcl_var(interp, &script_var);

	if (var->class && strlen(var->class)) {
		Tcl_SetVar2Ex(interp, var->class, var->name, obj, TCL_GLOBAL_ONLY);
	}
	else {
		Tcl_SetVar2Ex(interp, var->name, NULL, obj, TCL_GLOBAL_ONLY);
	}
}

/* This function handles trace callbacks from Tcl. */
static char *my_trace_callback(ClientData client_data, Tcl_Interp *irp, char *name1, char *name2, int flags)
{
	script_linked_var_t *linked_var = (script_linked_var_t *)client_data;

	if (flags & TCL_INTERP_DESTROYED) return(NULL);

	if (flags & TCL_TRACE_READS) {
		if (linked_var->callbacks && linked_var->callbacks->on_read) {
			int r = (linked_var->callbacks->on_read)(linked_var);
			if (r) return(NULL);
		}
		set_linked_var(linked_var);
	}
	else if (flags & TCL_TRACE_WRITES) {
		Tcl_Obj *obj;
		script_var_t newvalue = {0};

		obj = Tcl_GetVar2Ex(irp, name1, name2, 0);
		if (!obj) return("Error setting variable");

		tcl_to_c_var(irp, obj, &newvalue, linked_var->type);

		/* If they give a callback, then let them handle it. Otherwise, we
			do some default handling for strings and ints. */
		if (linked_var->callbacks && linked_var->callbacks->on_write) {
			int r;

			r = (linked_var->callbacks->on_write)(linked_var, &newvalue);
			if (r) return("Error setting variable");
		}
		else switch (linked_var->type & SCRIPT_TYPE_MASK) {
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
				return("Error setting variable (unsupported type)");
		}
	}
	else if (flags & TCL_TRACE_UNSETS) {
		/* If someone unsets a variable, we'll just reset it. */
		if (flags & TCL_TRACE_DESTROYED) my_link_var(NULL, linked_var);
		else set_linked_var(linked_var);
	}
	return(NULL);
}

/* This function creates the Tcl <-> C variable linkage on reads, writes, and unsets. */
static int my_link_var(void *ignore, script_linked_var_t *var)
{
	char *varname;

	if (var->class && strlen(var->class)) varname = msprintf("%s(%s)", var->class, var->name);
	else varname = strdup(var->name);

	set_linked_var(var);
	Tcl_TraceVar(interp, varname, TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS, my_trace_callback, var);

	free(varname);
	return(0);
}

/* This function deletes Tcl <-> C variable links. */
static int my_unlink_var(void *ignore, script_linked_var_t *var)
{
	char *varname;

	if (var->class && strlen(var->class)) varname = msprintf("%s(%s)", var->class, var->name);
	else varname = strdup(var->name);

	Tcl_UntraceVar(interp, varname, TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS, my_trace_callback, var);

	free(varname);
	return(0);
}

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

/* When you use a script_callback_t's callback() function, this gets executed.
	It converts the C variables to Tcl variables and executes the Tcl script. */
static int my_tcl_callbacker(script_callback_t *me, ...)
{
	Tcl_Obj *arg, *final_command, *result;
	script_var_t var;
	my_callback_cd_t *cd; /* My callback client data */
	int i, n, retval;
	void **al;

	/* This struct contains the interp and the obj command. */
	cd = (my_callback_cd_t *)me->callback_data;

	/* Get a copy of the command, then append args. */
	final_command = Tcl_DuplicateObj(cd->command);

	al = (void **)&me;
	al++;
	if (me->syntax) n = strlen(me->syntax);
	else n = 0;
	for (i = 0; i < n; i++) {
		var.type = me->syntax[i];
		var.value = al[i];
		var.len = -1;
		arg = c_to_tcl_var(cd->myinterp, &var);
		Tcl_ListObjAppendElement(cd->myinterp, final_command, arg);
	}

#ifdef USE_BYTE_ARRAYS
	n = Tcl_EvalObjEx(cd->myinterp, final_command, TCL_EVAL_GLOBAL);
#else
	n = Tcl_GlobalEvalObj(cd->myinterp, final_command);
#endif

	if (n == TCL_OK) {
		result = Tcl_GetObjResult(cd->myinterp);
		Tcl_GetIntFromObj(NULL, result, &retval);
	}
	else {
		log_error_message(cd->myinterp);
		Tcl_BackgroundError(cd->myinterp);
	}

	/* Clear any errors or stray messages. */
	Tcl_ResetResult(cd->myinterp);

	/* If it's a one-time callback, delete it. */
	if (me->flags & SCRIPT_CALLBACK_ONCE) me->del(me);

	return(retval);
}

/* This implements the delete() member of Tcl script callbacks. */
static int my_tcl_cb_delete(script_callback_t *me)
{
	my_callback_cd_t *cd;

	cd = (my_callback_cd_t *)me->callback_data;
	Tcl_DecrRefCount(cd->command);
	free(cd->name);
	if (me->syntax) free(me->syntax);
	free(cd);
	free(me);
	return(0);
}

/* Create Tcl commands with a C callback. */
static int my_create_cmd(void *ignore, script_command_t *info)
{
	char *cmdname;

	if (info->class && strlen(info->class)) {
		cmdname = msprintf("%s_%s", info->class, info->name);
	}
	else {
		cmdname = strdup(info->name);
	}
	Tcl_CreateObjCommand(interp, cmdname, my_command_handler, (ClientData) info, NULL);
	free(cmdname);

	return(0);
}

/* Delete Tcl commands. */
static int my_delete_cmd(void *ignore, script_command_t *info)
{
	char *cmdname;

	if (info->class && strlen(info->class)) {
		cmdname = msprintf("%s_%s", info->class, info->name);
	}
	else {
		cmdname = strdup(info->name);
	}
	Tcl_DeleteCommand(interp, cmdname);
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

			v_sub.type = v->type & (~SCRIPT_ARRAY);
			v_sub.len = -1;
			values = (void **)v->value;
			for (i = 0; i < v->len; i++) {
				v_sub.value = values[i];
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
		case SCRIPT_STRING:
		case SCRIPT_BYTES:
			if (v->len == -1) v->len = strlen((char *)v->value);
			#ifdef USE_BYTE_ARRAYS
			result = Tcl_NewByteArrayObj((char *)v->value, v->len);
			#else
			result = Tcl_NewStringObj((char *)v->value, v->len);
			#endif
			if (v->type & SCRIPT_FREE) free((char *)v->value);
			break;
		case SCRIPT_POINTER: {
			char str[32];

			sprintf(str, "#%u", (unsigned int) v->value);
			result = Tcl_NewStringObj(str, -1);
			break;
		}
		case SCRIPT_USER: {
			/* An eggdrop user record (struct userrec *). */
			char *handle;
			struct userrec *u = (struct userrec *)v->value;

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

	switch (type) {
		case SCRIPT_STRING: {
			char *str;
			int len;

			#ifdef USE_BYTE_ARRAYS
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

			#ifdef USE_BYTE_ARRAYS
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
			cback->callback_data = (void *)cdata;
			cback->del = (Function) my_tcl_cb_delete;
			cback->name = strdup((char *)Tcl_GetStringFromObj(obj, NULL));
			cdata->myinterp = myinterp;
			cdata->command = obj;
			Tcl_IncrRefCount(obj);

			var->value = cback;
			break;
		}
		case SCRIPT_USER: {
			struct userrec *u;
			script_var_t handle;

			/* Call ourselves recursively to get the handle as a string. */
			tcl_to_c_var(myinterp, obj, &handle, SCRIPT_STRING);
			u = get_user_by_handle(userlist, (char *)handle.value);
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
			Tcl_AppendResult(myinterp, "Cannot convert Tcl object to unknown variable type '", vartype, "'", NULL);
			err++;
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

/* This handles all the Tcl commands we create. It converts the Tcl arguments
	to C variables, calls the C callback function, then converts
	the return value to a Tcl variable. */
static int my_command_handler(ClientData client_data, Tcl_Interp *myinterp, int objc, Tcl_Obj *CONST objv[])
{
	script_command_t *cmd = (script_command_t *)client_data;
	script_var_t retval, var;
	Tcl_Obj *tcl_retval = NULL;
	mstack_t *args, *bufs, *cbacks;
	void **al; /* Argument list to pass to the callback*/
	int nopts; /* Number of optional args we're passing. */
	int simple_retval; /* Return value for simple commands. */
	int i, err, skip;
	char *syntax;

	/* Check for proper number of args. */
	if (cmd->flags & SCRIPT_VAR_ARGS) i = (cmd->nargs <= (objc-1));
	else i = (cmd->nargs < 0 || cmd->nargs == (objc-1));
	if (!i) {
		Tcl_WrongNumArgs(myinterp, 0, NULL, cmd->syntax_error);
		return(TCL_ERROR);
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
	for (i = 1; i < objc; i++) {
		err = tcl_to_c_var(myinterp, objv[i], &var, *syntax);

		if (var.type & SCRIPT_FREE) mstack_push(bufs, var.value);
		else if (*syntax == SCRIPT_CALLBACK) mstack_push(cbacks, var.value);

		mstack_push(args, var.value);
		syntax++;
	}

	if (err != TCL_OK) {
		my_argument_cleanup(args, bufs, cbacks, 1);
		Tcl_WrongNumArgs(myinterp, 0, NULL, cmd->syntax_error);
		return(TCL_ERROR);
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
	tcl_retval = c_to_tcl_var(myinterp, &retval);

	if (tcl_retval) Tcl_SetObjResult(myinterp, tcl_retval);
	else Tcl_ResetResult(myinterp);

	my_argument_cleanup(args, bufs, cbacks, 0);

	if (err) return TCL_ERROR;
	return TCL_OK;
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
static int cmd_tcl(struct userrec *u, int idx, char *text)
{
	char *str;

	if (!isowner(dcc[idx].nick)) {
		dprintf(idx, _("You must be a permanent owner (defined in the config file) to use this command.\n"));
		return(BIND_RET_LOG);
	}

	if (Tcl_GlobalEval(ginterp, text) != TCL_OK) {
		str = Tcl_GetVar(ginterp, "errorInfo", TCL_GLOBAL_ONLY);
		if (!str) str = Tcl_GetStringResult(ginterp);
		dprintf(idx, "Tcl error: %s\n", str);
	}
	else {
		str = Tcl_GetStringResult(ginterp);
		dprintf(idx, "Tcl: %s\n", str);
	}
	return(0);
}

static void tclscript_report(int idx, int details)
{
	char script[512];
	char *reported;

	if (!details) {
		dprintf(idx, "Using Tcl version %d.%d (by header)\n", TCL_MAJOR_VERSION, TCL_MINOR_VERSION);
		return;
	}

	dprintf(idx, "    Using Tcl version %d.%d (by header)\n", TCL_MAJOR_VERSION, TCL_MINOR_VERSION);
	sprintf(script, "return \"    Library: [info library]\\n    Reported version: [info tclversion]\\n    Reported patchlevel: [info patchlevel]\"");
	Tcl_GlobalEval(ginterp, script);
	reported = Tcl_GetStringResult(ginterp);
	dprintf(idx, "%s\n", reported);
}

static cmd_t dcc_commands[] = {
	{"tcl", 	"n", 	(Function) cmd_tcl,	NULL},
	{0}
};

EXPORT_SCOPE char *tclscript_LTX_start();
static char *tclscript_close();

static Function tclscript_table[] = {
	(Function) tclscript_LTX_start,
	(Function) tclscript_close,
	(Function) 0,
	(Function) tclscript_report
};

char *tclscript_LTX_start(eggdrop_t *eggdrop)
{
	bind_table_t *dcc_table;

	egg = eggdrop;

	/* When tcl is gone from the core, this will be uncommented. */
	/* ginterp = Tcl_CreateInterp(); */
	ginterp = interp;

	module_register("tclscript", tclscript_table, 1, 2);
	if (!module_depend("tclscript", "eggdrop", 107, 0)) {
		module_undepend("tclscript");
		return "This module requires eggdrop1.7.0 or later";
	}

	error_logfile = strdup("logs/tcl_errors.log");
	Tcl_LinkVar(ginterp, "error_logfile", (char *)&error_logfile, TCL_LINK_STRING);

	registry_add_simple_chains(my_functions);
	registry_lookup("script", "playback", &journal_playback, &journal_playback_h);
	if (journal_playback) journal_playback(journal_playback_h, journal_table);

	dcc_table = find_bind_table2("dcc");
	if (dcc_table) add_builtins2(dcc_table, dcc_commands);

	return(NULL);
}

static char *tclscript_close()
{
	bind_table_t *dcc_table;

	/* When tcl is gone from the core, this will be uncommented. */
	/* Tcl_DeleteInterp(ginterp); */

	dcc_table = find_bind_table2("dcc");
	if (dcc_table) rem_builtins2(dcc_table, dcc_commands);

	module_undepend("tclscript");
	return(NULL);
}

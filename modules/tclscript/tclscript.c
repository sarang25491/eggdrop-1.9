#include "lib/eggdrop/module.h"
#include "lib/egglib/mstack.h"
#include "lib/egglib/msprintf.h"
#include "src/script_api.h"

#define MODULE_NAME "tclscript"

static Function *global = NULL;

#if (TCL_MAJOR_VERSION > 8) || (TCL_MAJOR_VERSION == 8 && TCL_MINOR_VERSION > 1)
	#define USE_BYTE_ARRAYS
#endif

/* Data we need for a tcl callback. */
typedef struct {
	Tcl_Interp *myinterp;
	Tcl_Obj *command;
} my_callback_cd_t;

static int my_command_handler(ClientData client_data, Tcl_Interp *myinterp, int objc, Tcl_Obj *CONST objv[]);
static Tcl_Obj *my_resolve_var(Tcl_Interp *myinterp, script_var_t *v);

static Tcl_Interp *ginterp; /* Our global interpreter. */

static char *error_logfile = NULL;

#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION < 2)
static Tcl_Obj *Tcl_GetVar2Ex(Tcl_Interp *myinterp, const char *name1, const char *name2, int flags) {
	Tcl_Obj *part1, *part2, *obj;

	part1 = Tcl_NewStringObj((char *)name1, -1);
	part2 = Tcl_NewStringObj((char *)name2, -1);
	obj = Tcl_ObjGetVar2(myinterp, part1, part2, flags);
	Tcl_DecrRefCount(part1);
	Tcl_DecrRefCount(part2);
	return(obj);
}

static Tcl_Obj *Tcl_SetVar2Ex(Tcl_Interp *myinterp, const char *name1, const char *name2, Tcl_Obj *newval, int flags) {
	Tcl_Obj *part1, *part2, *obj;

	part1 = Tcl_NewStringObj((char *)name1, -1);
	part2 = Tcl_NewStringObj((char *)name2, -1);
	obj = Tcl_ObjSetVar2(myinterp, part1, part2, newval, flags);
	Tcl_DecrRefCount(part1);
	Tcl_DecrRefCount(part2);
	return(obj);
}
#endif

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

static int my_set_int(void *ignore, script_int_t *i)
{
	Tcl_Obj *obj = Tcl_GetVar2Ex(ginterp, i->class, i->name, TCL_GLOBAL_ONLY);

	if (!obj) {
		obj = Tcl_NewIntObj(*(i->ptr));
		Tcl_SetVar2Ex(interp, i->class, i->name, obj, TCL_GLOBAL_ONLY);
	} else {
		Tcl_SetIntObj(obj, (*i->ptr));
	}
	return(0);
}

static int my_set_str(void *ignore, script_str_t *str)
{
	Tcl_Obj *obj = Tcl_GetVar2Ex(interp, str->class, str->name, TCL_GLOBAL_ONLY);

	if (!obj) {
		obj = Tcl_NewStringObj(*(str->ptr), -1);
		Tcl_SetVar2Ex(interp, str->class, str->name, obj, TCL_GLOBAL_ONLY);
	} else {
		Tcl_SetStringObj(obj, *(str->ptr), -1);
	}
	return(0);
}

static int my_link_int(void *ignore, script_int_t *i, int flags)
{
	int f = TCL_LINK_INT;
	char *newname;

	if (flags & SCRIPT_READ_ONLY) f |= TCL_LINK_READ_ONLY;
	if (i->class && strlen(i->class)) newname = msprintf("%s(%s)", i->class, i->name);
	else newname = msprintf("%s", i->name);
	Tcl_LinkVar(interp, newname, (char *)i->ptr, f);
	free(newname);
	return(0);
}

static int my_link_str(void *ignore, script_str_t *str, int flags)
{
	int f = TCL_LINK_STRING;
	char *newname;

	if (flags & SCRIPT_READ_ONLY) f |= TCL_LINK_READ_ONLY;
	if (str->class && strlen(str->class)) newname = msprintf("%s(%s)", str->class, str->name);
	else newname = msprintf("%s", str->name);
	Tcl_LinkVar(interp, newname, (char *)str->ptr, TCL_LINK_STRING);
	free(newname);
	return(0);
}

/* For unlinking, strings and ints are the same in Tcl. */
static int my_unlink_var(void *ignore, script_str_t *str)
{
	char *newname;

	if (str->class && strlen(str->class)) newname = msprintf("%s(%s)", str->class, str->name);
	else newname = msprintf("%s", str->name);
	Tcl_UnlinkVar(interp, newname);
	free(newname);
	return(0);
}

static int my_tcl_callbacker(script_callback_t *me, ...)
{
	Tcl_Obj *arg, *final_command, *result;
	script_var_t var;
	my_callback_cd_t *cd; /* My callback client data */
	int i, n, retval, *al;

	/* This struct contains the interp and the obj command. */
	cd = (my_callback_cd_t *)me->callback_data;

	/* Get a copy of the command, then append args. */
	final_command = Tcl_DuplicateObj(cd->command);

	al = (int *)&me;
	al++;
	if (me->syntax) n = strlen(me->syntax);
	else n = 0;
	for (i = 0; i < n; i++) {
		var.type = me->syntax[i];
		var.value = (void *)al[i];
		var.len = -1;
		arg = my_resolve_var(cd->myinterp, &var);
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
		FILE *fp;
		char *errmsg;

		errmsg = Tcl_GetStringResult(cd->myinterp);
		putlog(LOG_MISC, "*", "TCL Error: %s", errmsg);

		if (error_logfile && error_logfile[0]) {
			time_t timenow = time(NULL);
			fp = fopen(error_logfile, "a");
			if (fp) {
				errmsg = Tcl_GetVar(cd->myinterp, "errorInfo", TCL_GLOBAL_ONLY);
				fprintf(fp, "%s", asctime(localtime(&timenow)));
				fprintf(fp, "%s\n\n", errmsg);
				fclose(fp);
			}
			else {
				putlog(LOG_MISC, "*", "Error opening TCL error log (%s)!", error_logfile);
			}
		}
	}

	/* Clear any errors or stray messages. */
	Tcl_ResetResult(cd->myinterp);

	/* If it's a one-time callback, delete it. */
	if (me->flags & SCRIPT_CALLBACK_ONCE) me->delete(me);

	return(retval);
}

static int my_tcl_cb_delete(script_callback_t *me)
{
	my_callback_cd_t *cd;

	cd = (my_callback_cd_t *)me->callback_data;
	Tcl_DecrRefCount(cd->command);
	if (me->syntax) free(me->syntax);
	free(cd);
	free(me);
	return(0);
}

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

static int my_delete_cmd(void *ignore, script_command_t *info)
{
	char *cmdname;

	cmdname = msprintf("%s_%s", info->class, info->name);
	Tcl_DeleteCommand(interp, cmdname);
	free(cmdname);
	return(0);
}

static Tcl_Obj *my_resolve_var(Tcl_Interp *myinterp, script_var_t *v)
{
	Tcl_Obj *result;

	result = NULL;
	if (v->type & SCRIPT_ARRAY) {
		Tcl_Obj *element;
		int i;

		result = Tcl_NewListObj(0, NULL);
		if ((v->type & SCRIPT_TYPE_MASK) == SCRIPT_VAR) {
			script_var_t **v_list;

			v_list = (script_var_t **)v->value;
			for (i = 0; i < v->len; i++) {
				element = my_resolve_var(myinterp, v_list[i]);
				Tcl_ListObjAppendElement(myinterp, result, element);
			}
		}
		else {
			script_var_t v_sub;
			void **values;

			v_sub.type = v->type & (~SCRIPT_ARRAY);
			v_sub.len = -1;
			values = (void **)v->value;
			for (i = 0; i < v->len; i++) {
				v_sub.value = values[i];
				element = my_resolve_var(myinterp, &v_sub);
				Tcl_ListObjAppendElement(myinterp, result, element);
			}
		}
		/* Whew */
		if (v->type & SCRIPT_FREE) free(v->value);
		if (v->type & SCRIPT_FREE_VAR) free(v);
		return(result);
	}
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

static int my_argument_cleanup(mstack_t *args, mstack_t *bufs, mstack_t *cbacks)
{
	int i;

	for (i = 0; i < bufs->len; i++) {
		free((void *)bufs->stack[i]);
	}
	if (cbacks) {
		for (i = 0; i < cbacks->len; i++) {
			script_callback_t *cback;

			cback = (script_callback_t *)cbacks->stack[i];
			cback->delete(cback);
		}
		mstack_destroy(cbacks);
	}
	mstack_destroy(bufs);
	mstack_destroy(args);
	return(0);
}

static int my_command_handler(ClientData client_data, Tcl_Interp *myinterp, int objc, Tcl_Obj *CONST objv[])
{
	script_command_t *cmd = (script_command_t *)client_data;
	script_var_t retval;
	Tcl_Obj *tcl_retval = NULL, *objptr;
	mstack_t *args, *bufs, *cbacks;
	int *al; /* Argument list to pass to the callback*/
	int nopts; /* Number of optional args we're passing. */
	int simple_retval; /* Return value for simple commands. */
	int i, len, err = TCL_OK;
	int skip;
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

	for (i = 1; i < objc; i++) {
		objptr = objv[i];

		switch (*syntax++) {
		case SCRIPT_STRING: { /* Null-terminated string. */
			char *nullterm;

			#ifdef USE_BYTE_ARRAYS
				char *orig;
				orig = Tcl_GetByteArrayFromObj(objptr, &len);
				nullterm = (char *)malloc(len+1);
				memcpy(nullterm, orig, len);
				nullterm[len] = 0;
				mstack_push(bufs, nullterm);
			#else
				nullterm = Tcl_GetStringFromObj(objptr, &len);
			#endif
			mstack_push(args, nullterm);
			break;
		}
		case SCRIPT_BYTES: { /* Byte-array (string + length). */
			void *bytes;
			#ifdef USE_BYTE_ARRAYS
			bytes = (void *)Tcl_GetByteArrayFromObj(objptr, &len);
			#else
			bytes = (void *)Tcl_GetStringFromObj(objptr, &len);
			#endif
			mstack_push(args, bytes);
			break;
		}
		case SCRIPT_UNSIGNED:
		case SCRIPT_INTEGER: { /* Integer. */
			int intval;
			err = Tcl_GetIntFromObj(myinterp, objptr, &intval);
			mstack_push(args, (void *)intval);
			break;
		}
		case SCRIPT_CALLBACK: { /* Callback. */
			script_callback_t *cback; /* Callback struct */
			my_callback_cd_t *cdata; /* Our client data */

			cback = (script_callback_t *)calloc(1, sizeof(*cback));
			cdata = (my_callback_cd_t *)calloc(1, sizeof(*cdata));
			cback->callback = (Function) my_tcl_callbacker;
			cback->callback_data = (void *)cdata;
			cback->delete = (Function) my_tcl_cb_delete;
			cback->name = (char *)Tcl_GetStringFromObj(objptr, &len);
			cdata->myinterp = myinterp;
			cdata->command = objptr;
			Tcl_IncrRefCount(objptr);

			mstack_push(args, cback);
			mstack_push(cbacks, cback);
			break;
		}
		case SCRIPT_USER: { /* User. */
			struct userrec *u;
			char *handle;

			handle = Tcl_GetStringFromObj(objptr, NULL);
			if (handle) u = get_user_by_handle(userlist, handle);
			else u = NULL;
			mstack_push(args, u);
			break;
		}
		case 'l': /* Length of previous string/byte-array. */
			mstack_push(args, (void *)len);
			i--; /* Doesn't take up a tcl object. */
			break;
		default:
			err = TCL_ERROR;
		} /* End of switch */

		if (err != TCL_OK) break;
	}

	if (err != TCL_OK) {
		my_argument_cleanup(args, bufs, cbacks);
		Tcl_WrongNumArgs(myinterp, 0, NULL, cmd->syntax_error);
		return(TCL_ERROR);
	}

	memset(&retval, 0, sizeof(retval));

	al = args->stack; /* Argument list shortcut name. */
	nopts = 0;
	if (cmd->flags & SCRIPT_PASS_COUNT) {
		al[2-nopts] = args->len - 3 - skip;
		nopts++;
	}
	if (cmd->flags & SCRIPT_PASS_RETVAL) {
		al[2-nopts] = (int)&retval;
		nopts++;
	}
	if (cmd->flags & SCRIPT_PASS_CDATA) {
		al[2-nopts] = (int)cmd->client_data;
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
	tcl_retval = my_resolve_var(myinterp, &retval);

	if (tcl_retval) Tcl_SetObjResult(myinterp, tcl_retval);
	else Tcl_ResetResult(myinterp);

	my_argument_cleanup(args, bufs, NULL);

	if (err) return TCL_ERROR;
	return TCL_OK;
}

static registry_simple_chain_t my_functions[] = {
	{"script", NULL, 0}, /* First arg gives our class */
	{"load script", my_load_script, 2}, /* name, ptr, nargs */
	{"set int", my_set_int, 2},
	{"set str", my_set_str, 2},
	{"link int", my_link_int, 3},
	{"unlink int", my_unlink_var, 2},
	{"link str", my_link_str, 3},
	{"unlink str", my_unlink_var, 2},
	{"create cmd", my_create_cmd, 2},
	{"delete cmd", my_delete_cmd, 2},
	{0}
};

static Function journal_table[] = {
	(Function)1,
	(Function)SCRIPT_EVENT_MAX,
	my_load_script,
	my_set_int,
	my_set_str,
	my_link_int,
	my_unlink_var,
	my_link_str,
	my_unlink_var,
	my_create_cmd,
	my_delete_cmd
};

static Function journal_playback;
static void *journal_playback_h;

EXPORT_SCOPE char *tclscript_LTX_start();
static char *tclscript_close();

static Function tclscript_table[] = {
	(Function) tclscript_LTX_start,
	(Function) tclscript_close,
	(Function) 0,
	(Function) 0
};

char *tclscript_LTX_start(Function *global_funcs)
{
	global = global_funcs;

	/* When tcl is gone from the core, this will be uncommented. */
	/* interp = Tcl_CreateInterp(); */
	ginterp = interp;

	error_logfile = strdup("logs/tcl_errors.log");
	Tcl_LinkVar(ginterp, "error_logfile", (char *)&error_logfile, TCL_LINK_STRING);

	registry_add_simple_chains(my_functions);
	registry_lookup("script", "playback", &journal_playback, &journal_playback_h);
	if (journal_playback) journal_playback(journal_playback_h, journal_table);

	module_register("tclscript", tclscript_table, 1, 2);
	if (!module_depend("tclscript", "eggdrop", 107, 0)) {
		module_undepend("tclscript");
		return "This module requires eggdrop1.7.0 or later";
	}

	return(NULL);
}

static char *tclscript_close()
{
	/* When tcl is gone from the core, this will be uncommented. */
	/* Tcl_DeleteInterp(interp); */
	module_undepend("tclscript");
	return(NULL);
}

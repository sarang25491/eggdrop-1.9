#include "src/mod/module.h"
#include "src/egglib/mstack.h"
#include "src/egglib/msprintf.h"
#include "src/script_api.h"

static Function *global = NULL;

#if (TCL_MAJOR_VERSION > 8) || (TCL_MAJOR_VERSION == 8 && TCL_MINOR_VERSION > 1)
	#define USE_BYTE_ARRAYS
#endif

/* Stacks to keep track of stuff when we're converting tcl args to c. */
typedef struct {
	mstack_t *args;
	mstack_t *bufs;
} script_argstack_t;

/* Data we need for a tcl callback. */
typedef struct {
	Tcl_Interp *myinterp;
	Tcl_Obj *command;
} my_callback_cd_t;

static int my_command_handler(ClientData client_data, Tcl_Interp *myinterp, int objc, Tcl_Obj *CONST objv[]);
static Tcl_Obj *my_resolve_var(Tcl_Interp *myinterp, script_var_t *v);

static Tcl_Interp *ginterp; /* Our global interpreter. */
static char *my_syntax_error = "syntax error";

static int my_load_script(registry_entry_t *entry, char *fname)
{
	int result;
	int len;

	/* Check the filename and make sure it ends in tcl */
	len = strlen(fname);
	if (len < 3 || fname[len-1] != 'l' || fname[len-2] != 'c' || fname[len-3] != 't') {
		/* Nope, let someone else load it. */
		return(0);
	}

	result = Tcl_EvalFile(ginterp, fname);
	entry->action = REGISTRY_HALT;
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
	newname = msprintf("%s(%s)", i->class, i->name);
	Tcl_LinkVar(interp, newname, (char *)i->ptr, f);
	free(newname);
	return(0);
}

static int my_link_str(void *ignore, script_str_t *str, int flags)
{
	int f = TCL_LINK_STRING;
	char *newname;

	if (flags & SCRIPT_READ_ONLY) f |= TCL_LINK_READ_ONLY;
	newname = msprintf("%s(%s)", str->class, str->name);
	Tcl_LinkVar(interp, newname, (char *)str->ptr, TCL_LINK_STRING);
	free(newname);
	return(0);
}

/* For unlinking, strings and ints are the same in Tcl. */
static int my_unlink_var(void *ignore, script_str_t *str)
{
	char *newname;

	newname = msprintf("%s(%s)", str->class, str->name);
	Tcl_UnlinkVar(interp, newname);
	free(newname);
	return(0);
}

static int my_tcl_callbacker(script_callback_t *me, int n, ...)
{
	Tcl_Obj *arg, *final_command;
	script_var_t *var;
	my_callback_cd_t *cd; /* My callback client data */
	int i, *al;

	/* This struct contains the interp and the obj command. */
	cd = (my_callback_cd_t *)me->callback_data;

	/* Get a copy of the command, then append args. */
	final_command = Tcl_DuplicateObj(cd->command);

	al = &n;
	for (i = 1; i <= n; i++) {
		var = (script_var_t *)al[i];
		arg = my_resolve_var(cd->myinterp, var);
		Tcl_ListObjAppendElement(cd->myinterp, final_command, arg);
	}

	Tcl_EvalObjEx(cd->myinterp, final_command, TCL_EVAL_GLOBAL);

	return(0);
}

static int my_tcl_cb_delete(script_callback_t *me)
{
	my_callback_cd_t *cd;

	cd = (my_callback_cd_t *)me->callback_data;
	Tcl_DecrRefCount(cd->command);
	free(cd);
	free(me);
	return(0);
}

static int my_create_cmd(void *ignore, script_command_t *info)
{
	char *cmdname;

	cmdname = msprintf("%s_%s", info->class, info->name);
	printf("Creating %s\n", cmdname);
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
	if (v->type & SCRIPT_INTEGER) result = Tcl_NewIntObj(v->intval);
	else if (v->type & SCRIPT_STRING) {
		if (v->len == -1) v->len = strlen(v->str);
		#ifdef USE_BYTE_ARRAYS
			result = Tcl_NewByteArrayObj(v->str, v->len);
		#else
			result = Tcl_NewStringObj(v->str, v->len);
		#endif
		if (!(v->type & SCRIPT_STATIC)) free(v->str);
	}
	else if (v->type & SCRIPT_ARRAY) {
		int i;

		result = Tcl_NewListObj(0, NULL);
		for (i = 0; i < v->len; i++) {
			Tcl_Obj *item;
			script_var_t *pv;

			pv = v->ptrarray[i];
			item = my_resolve_var(myinterp, pv);
			if (item) Tcl_ListObjAppendElement(myinterp, result, item);
		}
	}
	else if (v->type & SCRIPT_VARRAY) {
		int i;

		result = Tcl_NewListObj(0, NULL);
		for (i = 0; i < v->len; i++) {
			Tcl_Obj *item;

			item = my_resolve_var(myinterp, &v->varray[i]);
			if (item) Tcl_ListObjAppendElement(myinterp, result, item);
		}
		
	}
	else if (v->type & SCRIPT_POINTER) {
		char str[15];

		sprintf(str, "%#x", v->ptr);
		result = Tcl_NewStringObj(str, -1);
	}
	else if (v->type & SCRIPT_USER) {
		char *handle;
		struct userrec *u = (struct userrec *)v->ptr;

		if (u) handle = u->handle;
		else handle = "*";
		result = Tcl_NewStringObj(handle, -1);
	}
	else {
		result = Tcl_NewStringObj("unsupported return type", -1);
	}
	if (!(v->flags & SCRIPT_STATIC)) free(v);
	return(result);
}

static int my_argument_parser(Tcl_Interp *myinterp, int objc, Tcl_Obj *CONST objv[], char *syntax, script_argstack_t *argstack)
{
	Tcl_Obj *objptr;
	int i, err = 0, len = 0, needs_free = 0;
	void *arg;

	for (i = 1; i < objc; i++) {
		objptr = objv[i];
		switch (*syntax++) {
		case 's': { /* Null-terminated string. */
			char *nullterm;

			#ifdef USE_BYTE_ARRAYS
				char *orig;
				orig = Tcl_GetByteArrayFromObj(objptr, &len);
				nullterm = (char *)malloc(len+1);
				memcpy(nullterm, orig, len);
				nullterm[len] = 0;
				needs_free = 1;
			#else
				nullterm = Tcl_GetStringFromObj(objptr, &len);
			#endif

			arg = (void *)nullterm;
			break;
		}
		case 'b': { /* Byte-array (could be anything). */
			#ifdef USE_BYTE_ARRAYS
				arg = (void *)Tcl_GetByteArrayFromObj(objptr, &len);
			#else
				arg = (void *)Tcl_GetStringFromObj(objptr, &len);
			#endif
			break;
		}
		case 'i': { /* Integer. */
			err = Tcl_GetIntFromObj(myinterp, objptr, (int *)&arg);
			break;
		}
		case 'c': { /* Callback. */
			script_callback_t *cback; /* Callback struct */
			my_callback_cd_t *cdata; /* Our client data */

			cback = (script_callback_t *)malloc(sizeof(*cback));
			cdata = (my_callback_cd_t *)malloc(sizeof(*cdata));
			cback->callback = (Function) my_tcl_callbacker;
			cback->callback_data = (void *)cdata;
			cback->delete = (Function) my_tcl_cb_delete;
			cdata->myinterp = myinterp;
			cdata->command = objptr;
			Tcl_IncrRefCount(objptr);

			arg = (void *)cback;
			break;
		}
		case 'U': { /* User. */
			struct userrec *u;
			char *handle;

			handle = Tcl_GetStringFromObj(objptr, NULL);
			if (handle) u = get_user_by_handle(userlist, handle);
			else u = NULL;

			arg = (void *)u;
			break;
		}
		case 'l': { /* Length of previous string or byte-array. */
			arg = (void *)len;
			i--; /* Doesn't take up a Tcl object. */
			break;
		}
		case '*': { /* Repeat last entry. */
			if (*(syntax-2) == 'l') syntax -= 3;
			else syntax -= 2;
			i--; /* Doesn't take up a Tcl object. */
			continue;
		}
		default: return(1);
		} /* End of switch */

		if (err != TCL_OK) return(2);

		if (needs_free) {
			mstack_push(argstack->bufs, arg);
			needs_free = 0;
		}
		mstack_push(argstack->args, arg);
	}
	return(0);
}

static int my_argument_cleanup(script_argstack_t *argstack)
{
	void *ptr;
	int i;

	while (!mstack_pop(argstack->bufs, &ptr)) free(ptr);
	mstack_destroy(argstack->bufs);
	mstack_destroy(argstack->args);
	return(0);
}

static int my_command_handler(ClientData client_data, Tcl_Interp *myinterp, int objc, Tcl_Obj *CONST objv[])
{
	script_command_t *cmd = (script_command_t *)client_data;
	script_var_t retval;
	Tcl_Obj *tcl_retval = NULL;
	script_argstack_t argstack; /* For my_argument_parser */
	void **al; /* Argument list to pass to the callback*/
	int my_err; /* Flag to indicate we should return a TCL_ERROR */

	/* Check for proper number of args. */
	if (cmd->nargs >= 0 && cmd->nargs != (objc-1)) {
		Tcl_WrongNumArgs(myinterp, 1, objv, cmd->syntax_error);
		return(TCL_ERROR);
	}

	/* Init argstack */
	/* We want space for at least 5 args. */
	argstack.args = mstack_new(2*objc+5);

	/* The command's client data is the first arg. */
	mstack_push(argstack.args, cmd->client_data);

	/* Have some space for buffers too. */
	argstack.bufs = mstack_new(objc);

	/* Parse arguments. */
	if (my_argument_parser(myinterp, objc, objv, cmd->syntax, &argstack)) {
		my_argument_cleanup(&argstack);
		Tcl_WrongNumArgs(myinterp, 0, NULL, cmd->syntax_error);
		return(TCL_ERROR);
	}

	memset(&retval, 0, sizeof(retval));

	al = (void **)argstack.args->stack; /* Argument list shortcut name. */

	/* If they don't want their client data, bump the pointer. */
	if (!(cmd->flags & SCRIPT_WANTS_CD)) {
		al++;
		argstack.args->len--;
	}

	if (cmd->flags & SCRIPT_COMPLEX) {
		if (cmd->pass_array) cmd->callback(&retval, argstack.args->len, al);
		else cmd->callback(&retval, al[0], al[1], al[2], al[3], al[4]);
	}
	else {
		retval.type = cmd->retval_type;
		retval.len = -1;
		if (cmd->pass_array) retval.intval = cmd->callback(&retval, argstack.args->len, al);
		else retval.intval = cmd->callback(al[0], al[1], al[2], al[3], al[4]);
	}

	my_err = retval.type & SCRIPT_ERROR;
	retval.flags |= SCRIPT_STATIC; /* We don't want to segfault. */
	tcl_retval = my_resolve_var(myinterp, &retval);

	if (tcl_retval) Tcl_SetObjResult(myinterp, tcl_retval);

	my_argument_cleanup(&argstack);

	if (my_err) return TCL_ERROR;
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
	0
};

/*
static Function journal_table[] = {
	(Function)1,
	(Function)EV_MAX,
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
*/

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
	registry_add_simple_chains(my_functions);
	registry_lookup("script", "journal playback", &journal_playback, &journal_playback_h);
	/* if (journal_playback) journal_playback(journal_playback_h, journal_table); */

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

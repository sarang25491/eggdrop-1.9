#include <stdio.h>
#include <stdlib.h>
#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>
#include "src/mod/module.h"
#include "src/egglib/mstack.h"
#include "src/egglib/msprintf.h"
#include "src/script_api.h"

#define MODULE_NAME "perlscript"

static Function *global = NULL;

static PerlInterpreter *ginterp; /* Our global interpreter. */

static XS(my_command_handler);
static SV *my_resolve_variable(script_var_t *v);

static int my_load_script(registry_entry_t * entry, char *fname)
{
	FILE *fp;
	char *data;
	int size, len;

	/* Check the filename and make sure it ends in .pl */
	len = strlen(fname);
	if (len < 3 || fname[len-1] != 'l' || fname[len-2] != 'p' || fname[len-3] != '.') {
		/* Nope, not ours. */
		return(0);
	}

	fp = fopen(fname, "r");
	if (!fp) return (0);
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	data = (char *)malloc(size + 1);
	fseek(fp, 0, SEEK_SET);
	fread(data, size, 1, fp);
	data[size] = 0;
	fclose(fp);
	perl_eval_pv(data, TRUE);
	free(data);
	return(0);
}

static int my_perl_callbacker(script_callback_t *me, ...)
{
	int retval, i, n, count;
	script_var_t var;
	SV *arg;
	int *al;
	dSP;

	ENTER;
	SAVETMPS;
	PUSHMARK(SP);

	al = (int *)&me;
	al++;
	if (me->syntax) n = strlen(me->syntax);
	else n = 0;
	for (i = 0; i < n; i++) {
		var.type = me->syntax[i];
		var.value = (void *)al[i];
		var.len = -1;
		arg = my_resolve_variable(&var);
		XPUSHs(sv_2mortal(arg));
	}
	PUTBACK;

	count = call_pv(me->name, G_SCALAR);

	SPAGAIN;

	if (count > 0) {
		retval = POPi;
	}
	else retval = 0;

	PUTBACK;
	FREETMPS;
	LEAVE;

	/* If it's a one-time callback, delete it. */
	if (me->flags & SCRIPT_CALLBACK_ONCE) me->delete(me);

	return(retval);
}

static int my_perl_cb_delete(script_callback_t *me)
{
	if (me->syntax) free(me->syntax);
	if (me->name) free(me->name);
	free(me);
	return(0);
}

static int my_create_cmd(void *ignore, script_command_t *info)
{
	char *cmdname;
	CV *cv;

	if (info->class && strlen(info->class)) {
		cmdname = msprintf("%s_%s", info->class, info->name);
	}
	else {
		malloc_strcpy(cmdname, info->name);
	}
	cv = newXS(cmdname, my_command_handler, "eggdrop");
	XSANY.any_i32 = (int) info;
	free(cmdname);

	return (0);
}

static SV *my_resolve_variable(script_var_t *v)
{
	SV *result;

	switch (v->type & SCRIPT_TYPE_MASK) {
		case SCRIPT_INTEGER:
			result = newSViv((int) v->value);
			break;
		case SCRIPT_STRING:
		case SCRIPT_BYTES:
			if (v->len == -1) v->len = strlen((char *)v->value);
			result = newSVpv((char *)v->value, v->len);
			if (v->type & SCRIPT_FREE) free(v->value);
			break;

/* Save for later when we do arrays again
	else if (v->type & (SCRIPTING_ARRAY | SCRIPTING_VARRAY)) {
		AV *array;
		int i;

		array = newAV();
		for (i = 0; i < v->len; i++) {
			SV *item;

			if (v->type & SCRIPTING_ARRAY) item = my_resolve_variable(v->ptrarray[i]);
			else item = my_resolve_variable(&v->varray[i]);
			av_push(array, item);
		}
		result = newRV_noinc((SV *)array);
	}
end of array code */
		case SCRIPT_POINTER: {
			char str[32];
			int str_len;

			sprintf(str, "#%u", (unsigned int) v->value);
			str_len = strlen(str);
			result = newSVpv(str, str_len);
			break;
		}
		default:
			result = &PL_sv_undef;
	}
	return(result);
}

static XS(my_command_handler)
{
	dXSARGS;
	dXSI32;

	/* Now we have an "items" variable for number of args and also an XSANY.any_i32 variable for client data. This isn't what you would call a "well documented" feature of perl heh. */

	script_command_t *cmd = (script_command_t *) XSANY.any_i32;
	script_var_t retval;
	SV *result = NULL;
	mstack_t *args;
	int i, len, my_err;
	char *syntax;
	void **al;

	/* Check for proper number of args. */
	/* -1 means "any number" and implies pass_array. */
	if (cmd->nargs >= 0 && cmd->nargs != items) {
		Perl_croak(aTHX_ cmd->syntax_error);
		return;
	}

	/* Initialize argstack. We want at least 5 items. */
	args = mstack_new(2*items+5);

	/* Callback's client data is first arg. */
	mstack_push(args, cmd->client_data);

	syntax = cmd->syntax;
	for (i = 0; i < items; i++) {
		switch (*syntax++) {
			case SCRIPT_BYTES: /* Byte-array. */
			case SCRIPT_STRING: { /* String. */
				char *val;
				val = SvPV(ST(i), len);
				mstack_push(args, (void *)val);
				break;
			}
			case SCRIPT_INTEGER: { /* Integer. */
				int val;
				val = SvIV(ST(i));
				mstack_push(args, (void *)val);
				break;
			}
			case SCRIPT_CALLBACK: { /* Callback. */
				script_callback_t *cback;
				char *name;

				cback = (script_callback_t *)calloc(1, sizeof(*cback));
				cback->callback = (Function) my_perl_callbacker;
				cback->delete = (Function) my_perl_cb_delete;
				name = SvPV(ST(i), len);
				malloc_strcpy(cback->name, name);
				cback->callback_data = name;
				mstack_push(args, cback);
				break;
			}
			case SCRIPT_USER: { /* User. */
				struct userrec *u;
				char *handle;

				handle = SvPV(ST(i), len);
				if (handle) u = get_user_by_handle(userlist, handle);
				else u = NULL;
				mstack_push(args, u);
				break;
			}
			case 'l':
				/* Length of previous string or byte-array. */
				mstack_push(args, (void *)len);
				/* Doesn't take up a perl object. */
				i--;
				break;
			case '*':
				/* Repeat last entry. */
				if (*(syntax - 2) == 'l') syntax -= 3;
				else syntax -= 2;
				i--; /* No perl object. */
				break;
			default:
				goto argerror;
		} /* End of switch. */
	} /* End of for loop. */

	/* Ok, now we have our arg stack. */

	memset(&retval, 0, sizeof(retval));

	al = (void **)args->stack; /* Argument list shortcut name. */

	/* If they don't want their client data, bump the pointer. */
	if (!(cmd->flags & SCRIPT_WANTS_CD)) {
		al++;
		args->len--;
	}

	if (cmd->flags & SCRIPT_COMPLEX) {
		if (cmd->pass_array) cmd->callback(&retval, args->len, al);
		else cmd->callback(&retval, al[0], al[1], al[2], al[3], al[4]);
	}
	else {
		retval.type = cmd->retval_type;
		retval.len = -1;
		if (cmd->pass_array) retval.value = (void *)cmd->callback(args->len, al);
		else retval.value = (void *)cmd->callback(al[0], al[1], al[2], al[3], al[4]);
	}

	my_err = retval.type & SCRIPT_ERROR;
	result = my_resolve_variable(&retval);

	mstack_destroy(args);

	if (result) {
		XSprePUSH;
		PUSHs(result);
		XSRETURN(1);
	}
	else {
		XSRETURN_EMPTY;
	}

argerror:
	mstack_destroy(args);
	Perl_croak(aTHX_ cmd->syntax_error);
}

static registry_simple_chain_t my_functions[] = {
	{"script", NULL, 0},
	{"load script", my_load_script, 2},
	{"create cmd", my_create_cmd, 2},
	0
};

static void init_xs_stuff()
{
	extern void boot_DynaLoader();
	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, "eggdrop");
}

static Function journal_table[] = {
        (Function)1, /* Version */
        (Function)SCRIPT_EVENT_MAX, /* Our length */
        my_load_script,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
        my_create_cmd,
	NULL
};

static Function journal_playback;
static void *journal_playback_h;

EXPORT_SCOPE char *perlscript_LTX_start();
static char *perlscript_close();

static Function perlscript_table[] = {
	(Function) perlscript_LTX_start,
	(Function) perlscript_close,
	(Function) 0,
	(Function) 0
};

char *perlscript_LTX_start(Function *global_funcs)
{
	char *embedding[] = {"", "-e", "0"};

	global = global_funcs;
	ginterp = perl_alloc();
	perl_construct(ginterp);
	perl_parse(ginterp, init_xs_stuff, 3, embedding, NULL);
	registry_add_simple_chains(my_functions);
        registry_lookup("script", "playback", &journal_playback, &journal_playback_h);
        if (journal_playback) journal_playback(journal_playback_h, journal_table);

	module_register("perlscript", perlscript_table, 1, 2);
	if (!module_depend("perlscript", "eggdrop", 107, 0)) {
		module_undepend("perlscript");
		return "This module requires eggdrop1.7.0 of later";
	}

	return(NULL);
}

static char *perlscript_close()
{
	perl_destruct(ginterp);
	perl_free(ginterp);
	module_undepend("perlscript");
	return(NULL);
}

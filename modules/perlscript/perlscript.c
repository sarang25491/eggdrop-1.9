#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG
#undef DEBUG
#endif

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

/* For script_api.h */
typedef int (*Function)();

#include "lib/egglib/mstack.h"
#include "lib/egglib/msprintf.h"
#include "src/script_api.h"

static PerlInterpreter *ginterp; /* Our global interpreter. */

static XS(my_command_handler);
static SV *my_resolve_variable(script_var_t *v);

/* Functions from mod_iface.c */
extern void *fake_get_user_by_handle(char *handle);
extern char *fake_get_handle(void *user_record);
extern int log_error(char *msg);

int my_load_script(void *ignore, char *fname)
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
	eval_pv(data, TRUE);
	if (SvTRUE(ERRSV)) {
		char *msg;
		int len;

		msg = SvPV(ERRSV, len);
		log_error(msg);
	}
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

	count = call_sv((SV *)me->callback_data, G_EVAL|G_SCALAR);

	SPAGAIN;

	if (SvTRUE(ERRSV)) {
		char *msg;
		int len;

		msg = SvPV(ERRSV, len);
		retval = POPi;
		log_error(msg);
	}
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
	sv_2mortal((SV *)me->callback_data);
	SvREFCNT_dec((SV *)me->callback_data);
	free(me);
	return(0);
}

int my_create_cmd(void *ignore, script_command_t *info)
{
	char *cmdname;
	CV *cv;

	if (info->class && strlen(info->class)) {
		cmdname = msprintf("%s_%s", info->class, info->name);
	}
	else {
		cmdname = strdup(info->name);
	}
	cv = newXS(cmdname, my_command_handler, "eggdrop");
	XSANY.any_i32 = (int) info;
	free(cmdname);

	return (0);
}

static SV *my_resolve_variable(script_var_t *v)
{
	SV *result;

	if (v->type & SCRIPT_ARRAY) {
		AV *array;
		SV *element;
		int i;

		array = newAV();
		if ((v->type & SCRIPT_TYPE_MASK) == SCRIPT_VAR) {
			script_var_t **v_list;

			v_list = (script_var_t **)v->value;
			for (i = 0; i < v->len; i++) {
				element = my_resolve_variable(v_list[i]);
				av_push(array, element);
			}
		}
		else {
			script_var_t v_sub;
			void **values;

			v_sub.type = v->type & (~SCRIPT_ARRAY);
			values = (void **)v->value;
			for (i = 0; i < v->len; i++) {
				v_sub.value = values[i];
				v_sub.len = -1;
				element = my_resolve_variable(&v_sub);
				av_push(array, element);
			}
		}
		if (v->type & SCRIPT_FREE) free(v->value);
		if (v->type & SCRIPT_FREE_VAR) free(v);
		result = newRV_noinc((SV *)array);
		return(result);
	}

	switch (v->type & SCRIPT_TYPE_MASK) {
		case SCRIPT_INTEGER:
		case SCRIPT_UNSIGNED:
			result = newSViv((int) v->value);
			break;
		case SCRIPT_STRING:
		case SCRIPT_BYTES:
			if (v->len == -1) v->len = strlen((char *)v->value);
			result = newSVpv((char *)v->value, v->len);
			if (v->type & SCRIPT_FREE) free(v->value);
			break;
		case SCRIPT_POINTER: {
			char str[32];
			int str_len;

			sprintf(str, "#%u", (unsigned int) v->value);
			str_len = strlen(str);
			result = newSVpv(str, str_len);
			break;
		}
		case SCRIPT_USER: {
			char *handle;
			int str_len;

			handle = fake_get_handle(v->value);

			str_len = strlen(handle);
			result = newSVpv(handle, str_len);
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
	mstack_t *args, *cbacks;
	int i, len, my_err, simple_retval;
	int skip, nopts;
	char *syntax;
	void **al;

	/* Check for proper number of args. */
	/* -1 means "any number" and implies pass_array. */
	if (cmd->flags & SCRIPT_VAR_ARGS) i = (cmd->nargs <= items);
	else i = (cmd->nargs >= 0 && cmd->nargs == items);
	if (!i) {
		Perl_croak(aTHX_ cmd->syntax_error);
		return;
	}

	/* We want at least 10 items. */
	args = mstack_new(2*items+10);
	cbacks = mstack_new(items);

	/* Reserve space for 3 optional args. */
	mstack_push(args, NULL);
	mstack_push(args, NULL);
	mstack_push(args, NULL);

	/* Parse arguments. */
	syntax = cmd->syntax;
	if (cmd->flags & SCRIPT_VAR_FRONT) {
		skip = items - cmd->nargs;
		if (skip < 0) skip = 0;
		syntax += skip;
	}
	else skip = 0;
	for (i = skip; i < items; i++) {
		switch (*syntax++) {
			case SCRIPT_BYTES: /* Byte-array. */
			case SCRIPT_STRING: { /* String. */
				char *val;
				val = SvPV(ST(i), len);
				mstack_push(args, (void *)val);
				break;
			}
			case SCRIPT_UNSIGNED:
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
				cback->name = strdup(name);
				cback->callback_data = (void *)newSVsv(ST(i));
				mstack_push(args, cback);
				break;
			}
			case SCRIPT_USER: { /* User. */
				void *user_record;
				char *handle;

				handle = SvPV(ST(i), len);
				if (handle) user_record = fake_get_user_by_handle(handle);
				else user_record = NULL;
				mstack_push(args, user_record);
				break;
			}
			case 'l':
				/* Length of previous string or byte-array. */
				mstack_push(args, (void *)len);
				/* Doesn't take up a perl object. */
				i--;
				break;
			default:
				goto argerror;
		} /* End of switch. */
	} /* End of for loop. */

	/* Ok, now we have our args. */

	memset(&retval, 0, sizeof(retval));

	al = (void **)args->stack; /* Argument list shortcut name. */
	nopts = 0;
	if (cmd->flags & SCRIPT_PASS_COUNT) {
		al[2-nopts] = (void *)(args->len - 3);
		nopts++;
	}
	if (cmd->flags & SCRIPT_PASS_RETVAL) {
		al[2-nopts] = (void *)&retval;
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
		simple_retval = cmd->callback(al[0], al[1], al[2], al[3], al[4], al[5], al[6], al[7], al[8], al[9]);
	}

	if (!(cmd->flags & SCRIPT_PASS_RETVAL)) {
		retval.type = cmd->retval_type;
		retval.len = -1;
		retval.value = (void *)simple_retval;
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
	for (i = 0; i < cbacks->len; i++) {
		script_callback_t *cback;
		cback = (script_callback_t *)cbacks->stack[i];
		cback->delete(cback);
	}
	mstack_destroy(cbacks);
	Perl_croak(aTHX_ cmd->syntax_error);
}

char *real_perl_cmd(char *text)
{
	SV *result;
	char *msg, *retval;
	int len;

	result = eval_pv(text, FALSE);
	if (SvTRUE(ERRSV)) {
		msg = SvPV(ERRSV, len);
		retval = msprintf("Perl error: %s", msg);
	}
	else {
		msg = SvPV(result, len);
		retval = msprintf("Perl result: %s\n", msg);
	}

	return(retval);
}

static void init_xs_stuff()
{
	extern void boot_DynaLoader();
	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, "eggdrop");
}

int perlscript_init()
{
	char *embedding[] = {"", "-e", "0"};

	ginterp = perl_alloc();
	perl_construct(ginterp);
	perl_parse(ginterp, init_xs_stuff, 3, embedding, NULL);
	return(0);
}

int perlscript_destroy()
{
	PL_perl_destruct_level = 1;
	perl_destruct(ginterp);
	perl_free(ginterp);
	return(0);
}

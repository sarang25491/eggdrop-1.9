#include <stdio.h>
#include <stdlib.h>
#include "lib/eggdrop/module.h"
#include <eggdrop/eggdrop.h>

#define MODULE_NAME "perlscript"

static eggdrop_t *egg = NULL;

/* Functions from perlscript.c. */
int perlscript_init();
int perlscript_destroy();
int my_load_script(registry_entry_t * entry, char *fname);
int my_link_var(void *ignore, script_linked_var_t *linked_var);
int my_unlink_var(void *ignore, script_linked_var_t *linked_var);
int my_create_cmd(void *ignore, script_command_t *info);
char *real_perl_cmd(char *text);

/* A get_user_by_handle() command for perlscript.c */
void *fake_get_user_by_handle(char *handle)
{
	return get_user_by_handle(userlist, handle);
}

/* Get the handle from a userrec, or "*" if it's NULL. */
char *fake_get_handle(void *user_record)
{
	struct userrec *u = (struct userrec *)user_record;

	if (u && u->handle) return(u->handle);
	return("*");
}

/* Log an error message. */
int log_error(char *msg)
{
	putlog(LOG_MISC, "*", "Perl error: %s", msg);
	return(0);
}

/* A stub for the .perl command. */
int dcc_cmd_perl(struct userrec *u, int idx, char *text)
{
	char *retval;

	/* You must be owner to use this command. */
	if (!isowner(dcc[idx].nick)) return(0);

	retval = real_perl_cmd(text);
	dprintf(idx, "%s", retval);
	free(retval);
	return(0);
}

static cmd_t my_dcc_cmds[] = {
	{"perl", "n", (Function) dcc_cmd_perl, NULL},
	{0}
};

static registry_simple_chain_t my_functions[] = {
	{"script", NULL, 0},
	{"load script", my_load_script, 2},
	{"create cmd", my_create_cmd, 2},
	{0}
};

static Function journal_table[] = {
        (Function)1, /* Version */
        (Function)5, /* Number of functions */
        my_load_script,
	my_link_var,
	my_unlink_var,
        my_create_cmd,
	NULL /* my_delete_cmd */
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

char *perlscript_LTX_start(eggdrop_t *eggdrop)
{
	bind_table_t *BT_dcc;

	egg = eggdrop;

	module_register("perlscript", perlscript_table, 1, 2);
	if (!module_depend("perlscript", "eggdrop", 107, 0)) {
		module_undepend("perlscript");
		return "This module requires eggdrop1.7.0 of later";
	}

	/* Initialize interpreter. */
	perlscript_init();

	registry_add_simple_chains(my_functions);
        registry_lookup("script", "playback", &journal_playback, &journal_playback_h);
        if (journal_playback) journal_playback(journal_playback_h, journal_table);

	BT_dcc = find_bind_table2("dcc");
	if (BT_dcc) add_builtins2(BT_dcc, my_dcc_cmds);
	return(NULL);
}

static char *perlscript_close()
{
	bind_table_t *BT_dcc = find_bind_table2("dcc");
	if (BT_dcc) rem_builtins2(BT_dcc, my_dcc_cmds);

	/* Destroy interpreter. */
	perlscript_destroy();

	module_undepend("perlscript");
	return(NULL);
}


/*
 * mod_iface.c --
 */

#include <stdio.h>
#include <stdlib.h>
#include <eggdrop/eggdrop.h>

#define MODULE_NAME "perlscript"

/* Stuff from perlscript.c. */
extern int perlscript_init();
extern int perlscript_destroy();
extern script_module_t my_script_interface;
extern char *real_perl_cmd(char *text);

/* Log an error message. */
int log_error(char *msg)
{
	putlog(LOG_MISC, "*", "Perl error: %s", msg);
	return(0);
}

/* A stub for the .perl command. */
static int party_perl(partymember_t *p, char *nick, user_t *u, char *cmd, char *text)
{
	char *retval;

	/* You must be owner to use this command. */
	if (!u || !egg_isowner(u->handle)) {
		partymember_write(p, "Sorry, you must be a permanent owner to use this command.", -1);
		return(BIND_RET_LOG);
	}

	if (!text) {
		partymember_write(p, "Syntax: .perl perlexpression", -1);
		return(0);
	}

	retval = real_perl_cmd(text);
	partymember_write(p, retval, -1);
	free(retval);
	return(0);
}

static bind_list_t my_party_cmds[] = {
	{"perl", party_perl},
	{0}
};

static int perlscript_close(int why)
{
	bind_rem_list("party", my_party_cmds);

	/* Destroy interpreter. */
	perlscript_destroy();

	script_unregister_module(&my_script_interface);

	return(0);
}

EXPORT_SCOPE int perlscript_LTX_start(egg_module_t *modinfo);

int perlscript_LTX_start(egg_module_t *modinfo)
{
	modinfo->name = "perlscript";
	modinfo->author = "eggdev";
	modinfo->version = "1.7.0";
	modinfo->description = "provides perl scripting support";
	modinfo->close_func = perlscript_close;

	/* Initialize interpreter. */
	perlscript_init();

	script_register_module(&my_script_interface);
	script_playback(&my_script_interface);

	bind_add_list("party", my_party_cmds);
	return(0);
}

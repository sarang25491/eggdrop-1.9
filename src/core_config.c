#include <eggdrop/eggdrop.h>
#include "core_config.h"

core_config_t core_config = {0};
void *config_root = NULL;

static config_var_t core_config_vars[] = {
	/* General bot stuff. */
	{"botname", &core_config.botname, CONFIG_STRING},
	{"userfile", &core_config.userfile, CONFIG_STRING},

	/* The owner. */
	{"owner", &core_config.owner, CONFIG_STRING},
	{"admin", &core_config.admin, CONFIG_STRING},

	/* Paths. */
	{"help_path", &core_config.help_path, CONFIG_STRING},
	{"temp_path", &core_config.temp_path, CONFIG_STRING},
	{"text_path", &core_config.text_path, CONFIG_STRING},
	{"module_path", &core_config.module_path, CONFIG_STRING},

	/* Telnet stuff. */
	{"telnet_vhost", &core_config.telnet_vhost, CONFIG_STRING},
	{"telnet_port", &core_config.telnet_port, CONFIG_INT},
	{"telnet_stealth", &core_config.telnet_stealth, CONFIG_INT},
	{"telnet_max_retries", &core_config.telnet_max_retries, CONFIG_INT},

	/* Logfile. */
	{"logfile_suffix", &core_config.logfile_suffix, CONFIG_STRING},
	{"max_logsize", &core_config.max_logsize, CONFIG_INT},
	{"switch_logfiles_at", &core_config.switch_logfiles_at, CONFIG_INT},
	{"keep_all_logs", &core_config.keep_all_logs, CONFIG_INT},
	{"quick_logs", &core_config.quick_logs, CONFIG_INT},

	{0}
};

void core_config_init(const char *fname)
{
	/* Set default vals. */
	memset(&core_config, 0, sizeof(core_config));

	/* Hook the owner variable into libeggdrop. */
	egg_setowner(&core_config.owner);

	core_config.botname = strdup("eggdrop");
	core_config.userfile = strdup("users.xml");

	core_config.telnet_port = 3141;
	core_config.telnet_stealth = 0;
	core_config.telnet_max_retries = 3;

	core_config.logfile_suffix = strdup(".%d%b%Y");

	config_root = config_load(fname);
	config_set_root("eggdrop", config_root);
	config_link_table(core_config_vars, config_root, "eggdrop", 0, NULL);
}

void core_config_save()
{
	config_update_table(core_config_vars, config_root, "eggdrop", 0, NULL);
	config_save(config_root, "_config.xml");
}

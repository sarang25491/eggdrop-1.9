#include <eggdrop/eggdrop.h>
#include <eggdrop/eggconfig.h>
#include "core_config.h"

core_config_t core_config = {0};
void *config_root = NULL;

static config_var_t core_config_vars[] = {
	{"botname", &core_config.botname, CONFIG_STRING},
	{"userfile", &core_config.userfile, CONFIG_STRING},

	/* Telnet stuff. */
	{"telnet_vhost", &core_config.telnet_vhost, CONFIG_STRING},
	{"telnet_port", &core_config.telnet_port, CONFIG_INT},
	{"telnet_stealth", &core_config.telnet_stealth, CONFIG_INT},
	{"telnet_max_retries", &core_config.telnet_max_retries, CONFIG_INT},
	{0}
};

void core_config_init()
{
	/* Set default vals. */
	core_config.botname = strdup("eggdrop");
	core_config.userfile = strdup("users.xml");

	core_config.telnet_port = 3141;
	core_config.telnet_stealth = 0;
	core_config.telnet_max_retries = 3;

	config_root = config_load("config.xml");
	config_get_table(core_config_vars, config_root, "eggdrop", 0, NULL);
}

void core_config_save()
{
	config_set_table(core_config_vars, config_root, "eggdrop", 0, NULL);
	config_save(config_root, "config.xml");
}

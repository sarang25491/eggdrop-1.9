#include <eggdrop/eggdrop.h>
#include "proxy.h"

EXPORT_SCOPE int proxy_LTX_start(egg_module_t *modinfo);
static int proxy_close(int why);

proxy_config_t proxy_config = {0};
static config_var_t proxy_config_vars[] = {
	{"username", &proxy_config.username, CONFIG_STRING},
	{"password", &proxy_config.password, CONFIG_STRING},
	{"host", &proxy_config.host, CONFIG_STRING},
	{"port", &proxy_config.port, CONFIG_INT},
	{"type", &proxy_config.type, CONFIG_STRING},
	{0}
};

static egg_proxy_t http_proxy_handler = {
	"http",
	NULL,
	http_reconnect
};

static egg_proxy_t socks5_proxy_handler = {
	"socks5",
	NULL,
	socks5_reconnect
};

static int on_proxy_set(char *setting, char *val)
{
	putlog(LOG_MISC, "*", "Setting default proxy to '%s'", val);
	egg_proxy_set_default(val);
	return(0);
}

static int proxy_init()
{
	void *config_root;

	memset(&proxy_config, 0, sizeof(proxy_config));
	config_root = config_get_root("eggdrop");
	config_link_table(proxy_config_vars, config_root, "proxy", 0, NULL);

	egg_proxy_add(&http_proxy_handler);
	egg_proxy_add(&socks5_proxy_handler);
	if (proxy_config.type) egg_proxy_set_default(proxy_config.type);

	bind_add_simple("config_str", NULL, "proxy.type", on_proxy_set);

	return(0);
}

static int proxy_close(int why)
{
	return(0);
}

int proxy_LTX_start(egg_module_t *modinfo)
{
	modinfo->name = "proxy";
	modinfo->author = "eggdev";
	modinfo->version = "1.0.0";
	modinfo->description = "proxy support (http, socks4, socks5)";
	modinfo->close_func = proxy_close;

	proxy_init();
	return(0);
}

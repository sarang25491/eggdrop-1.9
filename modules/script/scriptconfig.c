#include <eggdrop/eggdrop.h>

static void *resolve_path(void *root, char *path)
{
	char *save, *work, *period;

	if (!root) return(NULL);

	work = strdup(path);
	save = work;
	while ((period = strchr(work, '.'))) {
		*period = 0;
		root = config_lookup_section(root, work, 0, NULL);
		if (!root) goto done;
		work = period+1;
	}
	root = config_lookup_section(root, work, 0, NULL);
done:
	free(save);
	return(root);
}

static char *script_config_get(char *handle, char *path)
{
	void *root;
	char *str = NULL;

	if (!handle) handle = "eggdrop";

	root = config_get_root(handle);
	root = resolve_path(root, path);
	if (!root) return(NULL);

	config_get_str(&str, root, NULL);
	return(str);
}

static int script_config_set(char *handle, char *path, char *val)
{
	void *root;

	if (!handle) handle = "eggdrop";

	root = config_get_root(handle);
	root = resolve_path(root, path);
	if (!root) return(-1);

	config_set_str(val, root, NULL);
	return(0);
}

static int script_config_exists(char *handle, char *path)
{
	void *root;
	char *save, *work, *period;
	int r = 0;

	if (!handle) handle = "eggdrop";

	root = config_get_root(handle);
	if (!root) return(0);

	work = strdup(path);
	save = work;
	while ((period = strchr(work, '.'))) {
		*period = 0;
		if (!config_exists(root, work, 0, NULL)) goto notfound;
		root = config_lookup_section(root, work, 0, NULL);
		if (!root) goto notfound;
		work = period+1;
	}
	r = config_exists(root, work, 0, NULL);

notfound:
	free(save);
	return(r);
}

static int script_config_load(char *handle, char *fname)
{
	void *root;

	root = config_load(fname);
	if (!root) return(-1);
	config_set_root(handle, root);
	return(0);
}

static int script_config_save(char *handle, char *fname)
{
	void *root;

	root = config_get_root(handle);
	if (!root) return(-1);
	config_save(root, fname);
	return(0);
}

script_command_t script_config_cmds[] = {
	{"", "config_exists", script_config_exists, NULL, 1, "ss", "?handle? path", SCRIPT_INTEGER, SCRIPT_VAR_ARGS|SCRIPT_VAR_FRONT},
	{"", "config_get", script_config_get, NULL, 1, "ss", "?handle? path", SCRIPT_STRING, SCRIPT_VAR_ARGS|SCRIPT_VAR_FRONT},
	{"", "config_set", script_config_set, NULL, 2, "sss", "?handle? path value", SCRIPT_INTEGER, SCRIPT_VAR_ARGS|SCRIPT_VAR_FRONT},
	{"", "config_load", script_config_load, NULL, 2, "ss", "handle filename", SCRIPT_INTEGER, 0},
	{"", "config_save", script_config_save, NULL, 2, "ss", "handle filename", SCRIPT_INTEGER, 0},
	{0}
};

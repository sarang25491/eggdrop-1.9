#ifndef _EGGCONFIG_H_
#define _EGGCONFIG_H_

#define CONFIG_INT	1
#define CONFIG_STRING	2

typedef struct {
	const char *name;
	void *ptr;
	int type;
} config_var_t;

int config_init();
void *config_load(const char *fname);
int config_save(void *config_root, const char *fname);
int config_destroy(void *config_root);

void *config_get_root(const char *handle);
int config_set_root(const char *handle, void *config_root);
int config_delete_root(const char *handle);

void *config_lookup_section(void *config_root, ...);
void *config_exists(void *config_root, ...);
int config_get_int(int *intptr, void *config_root, ...);
int config_get_str(char **strptr, void *config_root, ...);
int config_set_int(int intval, void *config_root, ...);
int config_set_str(char *strval, void *config_root, ...);
int config_link_table(config_var_t *table, void *config_root, ...);
int config_update_table(config_var_t *table, void *config_root, ...);
int config_unlink_table(config_var_t *table, void *config_root, ...);

#endif

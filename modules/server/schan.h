#ifndef _EGG_MOD_SERVER_SCHAN_H_
#define _EGG_MOD_SERVER_SCHAN_H_

typedef struct schan {
	struct schan *next;
	char *name;
	char *key;
	xml_node_t *settings;
} schan_t;

void server_schan_init();
void server_schan_destroy();
void schan_on_connect();
schan_t *schan_lookup(const char *chan_name, int create);
int schan_delete(const char *chan_name);
int schan_set(schan_t *chan, const char *value, ...);
int schan_get(schan_t *chan, char **strptr, ...);
int schan_load(const char *fname);
int schan_save(const char *fname);

#endif

#include "server.h"

static schan_t *schan_head = NULL;
static int nschans = 0;

/* Prototypes. */
int schan_load(const char *fname);
int schan_save(const char *fname);

void server_schan_init()
{
	schan_load(server_config.chanfile);
}

void server_schan_destroy()
{
	schan_save(server_config.chanfile);
}

/* We just finished connecting (got motd), join our channels. */
void schan_on_connect()
{
	schan_t *chan;

	for (chan = schan_head; chan; chan = chan->next) {
		if (chan->key && strlen(chan->key)) printserv(SERVER_NORMAL, "JOIN %s %s", chan->name, chan->key);
		else printserv(SERVER_NORMAL, "JOIN %s", chan->name);
	}
}

/* Append a new channel. */
static schan_t *schan_append(const char *chan_name, xml_node_t *settings)
{
	schan_t *chan, *prev = NULL;

	for (chan = schan_head; chan && chan->next; chan = chan->next) {
		prev = chan;
	}
	nschans++;
	chan = calloc(1, sizeof(*chan));
	chan->name = strdup(chan_name);
	if (!settings) {
		settings = xml_node_new();
		settings->name = strdup("settings");
	}
	chan->settings = settings;
	if (prev) prev->next = chan;
	else schan_head = chan;
	return(chan);
}

/* Find or create channel when passed 0 or 1 */
static int schan_probe(const char *chan_name, int create, schan_t **chanptr, schan_t **prevptr)
{
	schan_t *chan, *prev;

	*chanptr = NULL;
	if (prevptr) *prevptr = NULL;

	prev = NULL;
	for (chan = schan_head; chan; chan = chan->next) {
		if (!strcasecmp(chan->name, chan_name)) {
			*chanptr = chan;
			if (prevptr) *prevptr = prev;
			return -create; /* Report error if we were supposed to
					   create new channel */
		}
		prev = chan;
	}
	if (!create) return -1;
	*chanptr = schan_append(chan_name, NULL);
	return(0);
}

schan_t *schan_lookup(const char *chan_name, int create)
{
	schan_t *chan;

	schan_probe(chan_name, create, &chan, NULL);
	return(chan);
}

int schan_delete(const char *chan_name)
{
	schan_t *chan, *prev;

	schan_probe(chan_name, 0, &chan, &prev);
	if (!chan) return(-1);
	if (prev) prev->next = chan->next;
	else schan_head = chan->next;
	xml_node_delete(chan->settings);
	free(chan->name);
	free(chan);
	return(0);
}

int schan_set(schan_t *chan, const char *value, ...)
{
	va_list args;
	xml_node_t *node;

	va_start(args, value);
	node = xml_node_vlookup(chan->settings, args, 1);
	va_end(args);
	return xml_node_set_str(value, node, NULL);
}

int schan_get(schan_t *chan, char **strptr, ...)
{
	va_list args;
	xml_node_t *node;

	va_start(args, strptr);
	node = xml_node_vlookup(chan->settings, args, 0);
	va_end(args);
	return xml_node_get_str(strptr, node, NULL);
}

int schan_load(const char *fname)
{
	xml_node_t *root = xml_parse_file(fname);
	xml_node_t *chan_node, *settings;
	schan_t *chan;
	char *name, *key;

	if (!root) {
		putlog(LOG_MISC, "*", "Could not load channel file '%s': %s", fname, xml_last_error());
		return(-1);
	}

	chan_node = xml_node_lookup(root, 0, "channel", 0, 0);
	for (; chan_node; chan_node = chan_node->next_sibling) {
		xml_node_get_vars(chan_node, "ssn", "name", &name, "key", &key, "settings", &settings);
		if (!name) continue;
		if (settings) xml_node_unlink(settings);
		chan = schan_append(name, settings);
		if (chan) str_redup(&chan->key, key);
	}
	return(0);
}

int schan_save(const char *fname)
{
	xml_node_t *root = xml_node_new();
	xml_node_t *chan_node;
	schan_t *chan;

	if (!fname) fname = "channels.xml";
	root->name = strdup("channels");
	for (chan = schan_head; chan; chan = chan->next) {
		chan_node = xml_node_new();
		chan_node->name = strdup("channel");
		xml_node_set_str(chan->name, chan_node, "name", 0, 0);
		xml_node_set_str(chan->key, chan_node, "key", 0, 0);
		xml_node_append(chan_node, chan->settings);
		xml_node_append(root, chan_node);
	}
	xml_save_file(fname, root, XML_INDENT);
	for (chan = schan_head; chan; chan = chan->next) {
		xml_node_unlink(chan->settings);
	}
	xml_node_delete(root);
	return(0);
}

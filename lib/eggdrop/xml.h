#ifndef _XML_H_
#define _XML_H_

#include <stdarg.h>

/* Comment out to disable &char; conversions. */
#define AMP_CHARS

typedef struct {
	char *name;
	char *value;
	int len;
} xml_attribute_t;

typedef struct xml_node_b {
	char *name;

	char *text;
	int len;
	int whitespace;

	xml_attribute_t *attributes;
	int nattributes;

	struct xml_node_b *parent;
	struct xml_node_b **children;
	int nchildren;

	int type; /* 0 = normal, 1 = decl (<?xml ... ?>), 2 = CDATA,
			3 = comment, 4 = .. nothing yet. */

	char *data;

	void *client_data;
} xml_node_t;

//#ifdef AMP_CHARS
typedef struct {
	char *key;
	char value;
} xml_amp_conversion_t;
//#endif

xml_node_t *xml_node_new();
int xml_node_destroy(xml_node_t *node);
xml_node_t *xml_node_add(xml_node_t *parent, xml_node_t *child);
xml_node_t *xml_node_vlookup(xml_node_t *root, va_list args, int create);
xml_node_t *xml_node_lookup(xml_node_t *root, int create, ...);
char *xml_node_fullname(xml_node_t *thenode);
int xml_node_get_int(int *value, xml_node_t *node, ...);
int xml_node_get_str(char **str, xml_node_t *node, ...);
int xml_node_set_int(int value, xml_node_t *node, ...);
int xml_node_set_str(char *str, xml_node_t *node, ...);
xml_attribute_t *xml_attribute_add(xml_node_t *node, xml_attribute_t *attr);
xml_attribute_t *xml_attribute_get(xml_node_t *node, char *name);
int xml_attribute_set(xml_node_t *node, xml_attribute_t *attr);
int xml_attr_get_int(xml_node_t *node, const char *name);
char *xml_attr_get_str(xml_node_t *node, const char *name);

int xml_write_node(FILE *fp, xml_node_t *node, int indent);
int xml_read_node(xml_node_t *parent, char **data);
int xml_read(xml_node_t *root, const char *fname);

/* _XML_H_ */
#endif

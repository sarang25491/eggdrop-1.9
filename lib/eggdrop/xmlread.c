#include <eggdrop/eggdrop.h>
#include <errno.h>

static const char *name_terminators = "= \t\n\r?/>";

int xml_parse_children(xml_node_t *parent, char **bufptr);

static xml_entity_t entities[] = {
	{"lt", "<"},
	{"gt", ">"},
	{"amp", "&"},
	{"apos", "'"},
	{"quot", "\""}
};
static int nentities = 5;

static int skip_space(char **bufptr)
{
	char *start, *ptr;

	start = ptr = *bufptr;
	while (isspace(*ptr)) ptr++;
	*bufptr = ptr;
	return(ptr - start);
}

static void copy_text(char **ptr, int *ptrlen, int *ptrmax, char *str, int len)
{
	if (*ptrlen + len + 1 > *ptrmax) {
		*ptrmax = *ptrlen + len + 64;
		*ptr = realloc(*ptr, *ptrmax);
	}
	memcpy(*ptr + *ptrlen, str, len);
	*ptrlen += len;
}

char *xml_entity_lookup(const char *name)
{
	int i, num;
	static char numcode[2] = {0, 0};

	/* See if it's a hex code. */
	if (*name == '#') {
		name++;
		if (*name == 'x') num = strtol(name+1, NULL, 16);
		else num = strtol(name, NULL, 10);
		numcode[0] = num;
		return(numcode);
	}

	for (i = 0; i < nentities; i++) {
		if (!strcasecmp(entities[i].name, name)) return(entities[i].value);
	}
	return(NULL);
}

int xml_decode_text(char *text, int len, char **outtext, int *outlen)
{
	char *amp, *colon, *entity;
	int outmax, amplen;

	*outlen = 0;
	outmax = len+1;
	*outtext = malloc(outmax);

	len -= skip_space(&text);

	while ((amp = memchr(text, '&', len))) {
		amplen = amp - text;
		colon = memchr(amp, ';', len - amplen);
		if (!colon) break;
		copy_text(outtext, outlen, &outmax, text, amplen);
		len -= (colon+1 - text);
		text = colon+1;
		*colon = 0;
		entity = xml_entity_lookup(amp+1);
		if (entity) copy_text(outtext, outlen, &outmax, entity, strlen(entity));
	}
	while (len > 0 && isspace(text[len-1])) len--;
	copy_text(outtext, outlen, &outmax, text, len);
	return(0);
}

static void append_text(xml_node_t *node, char *text, int len, int decode)
{
	char *finaltext;
	int finallen;

	if (decode) xml_decode_text(text, len, &finaltext, &finallen);
	else {
		finaltext = text;
		finallen = len;
	}

	node->text = realloc(node->text, node->len + finallen + 1);
	memcpy(node->text + node->len, finaltext, finallen);
	node->len += finallen;
	node->text[node->len] = 0;

	if (finaltext != text) free(finaltext);
}

static char *read_name(char **bufptr)
{
	int n;
	char *name;

	n = strcspn(*bufptr, name_terminators);
	if (!n) return NULL;
	name = malloc(n+1);
	memcpy(name, *bufptr, n);
	name[n] = 0;
	*bufptr += n;
	return(name);
}

static char *read_value(char **bufptr)
{
	const char *term;
	char *value;
	int n;

	if (**bufptr == '\'') term = "'";
	else if (**bufptr == '"') term = "\"";
	else {
		term = name_terminators;
		(*bufptr)--;
	}

	/* Skip past first quote. */
	(*bufptr)++;
	n = strcspn(*bufptr, term);
	value = malloc(n+1);
	memcpy(value, *bufptr, n);
	value[n] = 0;

	if (term != name_terminators) n++;
	*bufptr += n;
	return(value);
}

static void read_attributes(xml_node_t *node, char **bufptr)
{
	xml_attr_t *attr;
	char *name, *value;

	while (1) {
		skip_space(bufptr);
		name = read_name(bufptr);
		if (!name || **bufptr != '=') {
			if (name) free(name);
			return;
		}
		(*bufptr)++;
		value = read_value(bufptr);
		attr = xml_attr_new(name, value);
		xml_node_append_attr(node, attr);
	}
}

/* Parse a node. */
int xml_parse_node(xml_node_t *parent, char **bufptr)
{
	xml_node_t *node;
	char *ptr, *buf = *bufptr;

	if (*buf++ != '<') return(-1);
	skip_space(&buf);

	/* Is it a closing tag? (returns 0) */
	if (*buf == '/') {
		char *name;

		if (!parent->name) return(-1);

		buf++;
		name = read_name(&buf);
		if (strcasecmp(name, parent->name)) {
			free(name);
			xml_set_error("closing tag doesn't match opening tag");
			return(-1);
		}
		free(name);

		ptr = strchr(buf, '>');
		if (!ptr) return(-1);
		*bufptr = ptr+1;
		return(0);
	}

	/* Figure out what type of node this is. */
	if (!strncmp(buf, "!--", 3)) {
		/* A comment. */
		buf += 3;
		ptr = strstr(buf, "-->");
		if (!ptr) {
			xml_set_error("comment has no end");
			return(-1);
		}

		node = xml_node_new();
		node->type = XML_COMMENT;
		append_text(node, buf, ptr-buf, 1);
		xml_node_append(parent, node);
		*bufptr = ptr+3;
		return(1);
	}
	else if (!strncasecmp(buf, "![CDATA[", 8)) {
		/* CDATA section. */
		buf += 7;
		ptr = strstr(buf, "]]>");
		if (!ptr) {
			xml_set_error("CDATA has no end");
			return(-1);
		}

		append_text(parent, buf, ptr-buf, 0);
		*bufptr = ptr+3;
		return(1);
	}
	else if (!strncasecmp(buf, "!DOCTYPE", 8)) {
		/* XXX Incorrect doctype parsing, fix later. */
		buf += 8;
		ptr = strchr(buf, '>');
		if (!ptr) return(-1);
		*bufptr = ptr+1;
		return(1);
	}
	else if (*buf == '?') {
		/* Processing instruction. */
		buf++;
		skip_space(&buf);
		node = xml_node_new();
		node->type = XML_PROCESSING_INSTRUCTION;
	}
	else {
		/* Otherwise, try it as a normal node. */
		node = xml_node_new();
		node->type = XML_ELEMENT;
	}

	node->name = read_name(&buf);
	read_attributes(node, &buf);
	if (node->type == XML_PROCESSING_INSTRUCTION) {
		if (strncmp(buf, "?>", 2)) {
			xml_node_free(node);
			xml_set_error("invalid processing instruction");
			return(-1);
		}
		xml_node_append(parent, node);
		*bufptr = buf+2;
		return(1);
	}

	/* Is it an empty tag? */
	if (!strncmp(buf, "/>", 2)) {
		xml_node_append(parent, node);
		*bufptr = buf+2;
		return(1);
	}
	else if (*buf != '>') {
		xml_set_error("invalid tag");
		return(-1);
	}

	/* Move past the '>'. */
	buf++;
	if (xml_parse_children(node, &buf) != -1) {
		*bufptr = buf;
		xml_node_append(parent, node);
		return(1);
	}

	/* Error reading children. */
	xml_node_free(node);
	return(-1);
}

/* Parse all child nodes and attach them to the parent. */
int xml_parse_children(xml_node_t *parent, char **bufptr)
{
	char *ptr, *buf = *bufptr;
	int ret;
	char temp[8];

	do {
		ptr = strchr(buf, '<');
		if (!ptr) {
			append_text(parent, buf, strlen(buf), 1);
			return(0);
		}

		/* Append the intervening text to the parent. */
		append_text(parent, buf, ptr - buf, 1);
		buf = ptr;

		ret = xml_parse_node(parent, &buf);
		*bufptr = buf;
	} while (ret == 1);
	return(ret);
}

xml_node_t *xml_parse_file(const char *fname)
{
	FILE *fp;
	xml_node_t *root;
	char *buf, *ptr;
	int len;

	fp = fopen(fname, "r");
	if (!fp) {
		xml_set_error(strerror(errno));
		return(NULL);
	}
	fseek(fp, 0l, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0l, SEEK_SET);
	buf = malloc(len+1);
	if (!buf) {
		fclose(fp);
		xml_set_error("out of memory");
		return(NULL);
	}
	fread(buf, 1, len, fp);
	fclose(fp);
	root = xml_node_new();
	root->type = XML_DOCUMENT;
	ptr = buf;
	xml_parse_children(root, &ptr);
	free(buf);
	return xml_root_element(root);
}

/*
 * userent.c --
 *
 *	user-entry handling, new stylem more versatile.
 */

#include "main.h"
#include "users.h"
#include "logfile.h"
#include "modules.h"		/* encrypt_pass				*/
#include "flags.h"		/* USER_*				*/
#include "dccutil.h"		/* dprintf_eggdrop			*/
#include "userrec.h"		/* addhost_by_handle			*/
#include "userent.h"		/* prototypes				*/

extern struct userrec	*userlist;
extern struct dcc_t	*dcc;
extern char		 whois_fields[];

static struct user_entry_type *entry_type_list;


void init_userent()
{
  entry_type_list = 0;
  add_entry_type(&USERENTRY_COMMENT);
  add_entry_type(&USERENTRY_XTRA);
  add_entry_type(&USERENTRY_INFO);
  add_entry_type(&USERENTRY_LASTON);
  add_entry_type(&USERENTRY_BOTADDR);
  add_entry_type(&USERENTRY_PASS);
  add_entry_type(&USERENTRY_HOSTS);
  add_entry_type(&USERENTRY_BOTFL);
}

void list_type_kill(struct list_type *t)
{
  struct list_type *u;

  while (t) {
    u = t->next;
    if (t->extra)
      free(t->extra);
    free(t);
    t = u;
  }
}

int def_unpack(struct userrec *u, struct user_entry *e)
{
  char *tmp;

  tmp = e->u.list->extra;
  e->u.list->extra = NULL;
  list_type_kill(e->u.list);
  e->u.string = tmp;
  return 1;
}

int def_pack(struct userrec *u, struct user_entry *e)
{
  char *tmp;

  tmp = e->u.string;
  e->u.list = malloc(sizeof(struct list_type));
  e->u.list->next = NULL;
  e->u.list->extra = tmp;
  return 1;
}

int def_kill(struct user_entry *e)
{
  free(e->u.string);
  free(e);
  return 1;
}

int def_write_userfile(FILE * f, struct userrec *u, struct user_entry *e)
{
  if (fprintf(f, "--%s %s\n", e->type->name, e->u.string) == EOF)
    return 0;
  return 1;
}

void *def_get(struct userrec *u, struct user_entry *e)
{
  return e->u.string;
}

int def_set(struct userrec *u, struct user_entry *e, void *buf)
{
  char *string = (char *) buf;

  if (string && !string[0])
    string = NULL;
  if (!string && !e->u.string)
    return 1;
  if (string) {
    int l = strlen (string);
    char *i;

    if (l > 160)
      l = 160;

    e->u.string = realloc(e->u.string, l + 1);

    strlcpy (e->u.string, string, l + 1);

    for (i = e->u.string; *i; i++)
      /* Allow bold, inverse, underline, color text here...
       * But never add cr or lf!! --rtc
       */
     if ((unsigned int) *i < 32 && !strchr ("\002\003\026\037", *i))
        *i = '?';
  } else { /* string == NULL && e->u.string != NULL */
    free_null(e->u.string);
  }
  return 1;
}

int def_gotshare(struct userrec *u, struct user_entry *e,
		 char *data, int idx)
{
  return 0;
}

void def_display(int idx, struct user_entry *e)
{
  dprintf(idx, "  %s: %s\n", e->type->name, e->u.string);
}

int def_dupuser(struct userrec *new, struct userrec *old,
		struct user_entry *e)
{
  return set_user(e->type, new, e->u.string);
}

static void comment_display(int idx, struct user_entry *e)
{
  if (dcc[idx].user && (dcc[idx].user->flags & USER_MASTER))
    dprintf(idx, "  COMMENT: %.70s\n", e->u.string);
}

struct user_entry_type USERENTRY_COMMENT =
{
  0,				/* always 0 ;) */
  def_gotshare,
  def_dupuser,
  def_unpack,
  def_pack,
  def_write_userfile,
  def_kill,
  def_get,
  def_set,
  NULL,
  NULL,
  comment_display,
  "COMMENT"
};

struct user_entry_type USERENTRY_INFO =
{
  0,				/* always 0 ;) */
  def_gotshare,
  def_dupuser,
  def_unpack,
  def_pack,
  def_write_userfile,
  def_kill,
  def_get,
  def_set,
  NULL,
  NULL,
  def_display,
  "INFO"
};

int pass_set(struct userrec *u, struct user_entry *e, void *buf)
{
  char new[32];
  register char *pass = buf;

  if (e->u.extra)
    free(e->u.extra);
  if (!pass || !pass[0] || (pass[0] == '-'))
    e->u.extra = NULL;
  else {
    unsigned char *p = (unsigned char *) pass;

    if (strlen(pass) > 15)
      pass[15] = 0;
    while (*p) {
      if ((*p <= 32) || (*p == 127))
	*p = '?';
      p++;
    }
    if ((u->flags & USER_BOT) || (pass[0] == '+'))
      strcpy(new, pass);
    else
      encrypt_pass(pass, new);
    e->u.extra = strdup(new);
  }
  return 1;
}

struct user_entry_type USERENTRY_PASS =
{
  0,
  def_gotshare,
  0,
  def_unpack,
  def_pack,
  def_write_userfile,
  def_kill,
  def_get,
  pass_set,
  NULL,
  NULL,
  0,
  "PASS"
};

static int laston_unpack(struct userrec *u, struct user_entry *e)
{
  char *par, *arg;
  struct laston_info *li;

  par = e->u.list->extra;
  arg = newsplit (&par);
  if (!par[0])
    par = "???";
  li = malloc(sizeof(struct laston_info));
  li->lastonplace = malloc(strlen(par) + 1);
  li->laston = atoi(arg);
  strcpy(li->lastonplace, par);
  list_type_kill(e->u.list);
  e->u.extra = li;
  return 1;
}

static int laston_pack(struct userrec *u, struct user_entry *e)
{
  char work[1024];
  struct laston_info *li;

  li = (struct laston_info *) e->u.extra;
  sprintf(work, "%lu %s", li->laston, li->lastonplace);
  e->u.list = malloc(sizeof(struct list_type));
  e->u.list->next = NULL;
  e->u.list->extra = strdup(work);
  free(li->lastonplace);
  free(li);
  return 1;
}

static int laston_write_userfile(FILE * f,
				 struct userrec *u,
				 struct user_entry *e)
{
  struct laston_info *li = (struct laston_info *) e->u.extra;

  if (fprintf(f, "--LASTON %lu %s\n", li->laston,
	      li->lastonplace ? li->lastonplace : "") == EOF)
    return 0;
  return 1;
}

static int laston_kill(struct user_entry *e)
{
  if (((struct laston_info *) (e->u.extra))->lastonplace)
    free(((struct laston_info *) (e->u.extra))->lastonplace);
  free(e->u.extra);
  free(e);
  return 1;
}

static int laston_set(struct userrec *u, struct user_entry *e, void *buf)
{
  struct laston_info *li = (struct laston_info *) e->u.extra;

  if (li != buf) {
    if (li) {
      free(li->lastonplace);
      free(li);
    }

    li = e->u.extra = buf;
  }
  return 1;
}

static int laston_dupuser(struct userrec *new, struct userrec *old,
			  struct user_entry *e)
{
  struct laston_info *li = e->u.extra, *li2;

  if (li) {
    li2 = malloc(sizeof(struct laston_info));

    li2->laston = li->laston;
    li2->lastonplace = strdup(li->lastonplace);
    return set_user(&USERENTRY_LASTON, new, li2);
  }
  return 0;
}

struct user_entry_type USERENTRY_LASTON =
{
  0,				/* always 0 ;) */
  0,
  laston_dupuser,
  laston_unpack,
  laston_pack,
  laston_write_userfile,
  laston_kill,
  def_get,
  laston_set,
  NULL,
  NULL,
  0,
  "LASTON"
};

static int botaddr_unpack(struct userrec *u, struct user_entry *e)
{
  char *p = NULL, *q;
  struct bot_addr *bi;

  bi = calloc(1, sizeof(struct bot_addr));
  q = (e->u.list->extra);
  p = strdup(q);
  if (!(q = strchr_unescape(p, ':', '\\')))
    bi->address = strdup(p);
  else {
    bi->address = strdup(p);
    bi->telnet_port = atoi(q);
    if ((q = strchr(q, '/')))
      bi->relay_port = atoi(q + 1);
  }
  free(p);
  if (!bi->telnet_port)
    bi->telnet_port = 3333;
  if (!bi->relay_port)
    bi->relay_port = bi->telnet_port;
  list_type_kill(e->u.list);
  e->u.extra = bi;
  return 1;
}

static int botaddr_pack(struct userrec *u, struct user_entry *e)
{
  char work[1024];
  struct bot_addr *bi;
  char *tmp;

  bi = (struct bot_addr *) e->u.extra;
  simple_sprintf(work, "%s:%u/%u",
              (tmp = str_escape(bi->address, ':', '\\')),
              bi->telnet_port, bi->relay_port);
  free(tmp);
  e->u.list = malloc(sizeof(struct list_type));
  e->u.list->next = NULL;
  e->u.list->extra = strdup(work);
  free(bi->address);
  free(bi);
  return 1;
}

static int botaddr_kill(struct user_entry *e)
{
  free(((struct bot_addr *) (e->u.extra))->address);
  free(e->u.extra);
  free(e);
  return 1;
}

static int botaddr_write_userfile(FILE *f, struct userrec *u,
				  struct user_entry *e)
{
  register struct bot_addr *bi = (struct bot_addr *) e->u.extra;
  register char *tmp;
  register int res;

  res = (fprintf(f, "--%s %s:%u/%u\n", e->type->name,
              (tmp = str_escape(bi->address, ':', '\\')),
	      bi->telnet_port, bi->relay_port) != EOF);
  free(tmp);
  return res;
}

static int botaddr_set(struct userrec *u, struct user_entry *e, void *buf)
{
  register struct bot_addr *bi = (struct bot_addr *) e->u.extra;

  if (!bi && !buf)
    return 1;
  if (bi != buf) {
    if (bi) {
      free(bi->address);
      free(bi);
    }
    bi = e->u.extra = buf;
  }
  return 1;
}

static void botaddr_display(int idx, struct user_entry *e)
{
  register struct bot_addr *bi = (struct bot_addr *) e->u.extra;

  dprintf(idx, "  ADDRESS: %.70s\n", bi->address);
  dprintf(idx, "     users: %d, bots: %d\n", bi->relay_port, bi->telnet_port);
}

static int botaddr_gotshare(struct userrec *u, struct user_entry *e,
			    char *buf, int idx)
{
  return 0;
}

static int botaddr_dupuser(struct userrec *new, struct userrec *old,
			   struct user_entry *e)
{
  if (old->flags & USER_BOT) {
    struct bot_addr *bi = e->u.extra, *bi2;

    if (bi) {
      bi2 = malloc(sizeof(struct bot_addr));

      bi2->telnet_port = bi->telnet_port;
      bi2->relay_port = bi->relay_port;
      bi2->address = strdup(bi->address);
      return set_user(&USERENTRY_BOTADDR, new, bi2);
    }
  }
  return 0;
}

struct user_entry_type USERENTRY_BOTADDR =
{
  0,				/* always 0 ;) */
  botaddr_gotshare,
  botaddr_dupuser,
  botaddr_unpack,
  botaddr_pack,
  botaddr_write_userfile,
  botaddr_kill,
  def_get,
  botaddr_set,
  NULL,
  NULL,
  botaddr_display,
  "BOTADDR"
};

int xtra_set(struct userrec *u, struct user_entry *e, void *buf)
{
  struct xtra_key *curr, *old = NULL, *new = buf;

  for (curr = e->u.extra; curr; curr = curr->next) {
    if (curr->key && !strcasecmp(curr->key, new->key)) {
      old = curr;
      break;
    }
  }
  if (!old && (!new->data || !new->data[0])) {
    /* Delete non-existant entry -- doh ++rtc */
    free(new->key);
    if (new->data)
      free(new->data);
    free(new);
    return(0);
  }

  if ((old && old != new) || !new->data || !new->data[0]) {
    list_delete((struct list_type **) (&e->u.extra),
		(struct list_type *) old);
    free(old->key);
    free(old->data);
    free(old);
  }
  if (old != new && new->data) {
    if (new->data[0])
      list_insert((&e->u.extra), new) /* do not add a ';' here */
  } else {
    if (new->data)
      free(new->data);
    free(new->key);
    free(new);
  }
  return(0);
}

int xtra_unpack(struct userrec *u, struct user_entry *e)
{
  struct list_type *curr, *head;
  struct xtra_key *t;
  char *key, *data;

  head = curr = e->u.list;
  e->u.extra = NULL;
  while (curr) {
    t = malloc(sizeof(struct xtra_key));

    data = curr->extra;
    key = newsplit(&data);
    if (data[0]) {
      t->key = strdup(key);
      t->data = strdup(data);
      list_insert((&e->u.extra), t);
    }
    curr = curr->next;
  }
  list_type_kill(head);
  return 1;
}

static int xtra_pack(struct userrec *u, struct user_entry *e)
{
  struct list_type *t;
  struct xtra_key *curr, *next;

  curr = e->u.extra;
  e->u.list = NULL;
  while (curr) {
    t = malloc(sizeof(struct list_type));
    t->extra = malloc(strlen(curr->key) + strlen(curr->data) + 4);
    sprintf(t->extra, "%s %s", curr->key, curr->data);
    list_insert((&e->u.list), t);
    next = curr->next;
    free(curr->key);
    free(curr->data);
    free(curr);
    curr = next;
  }
  return 1;
}

static int xtra_gotshare(struct userrec *u, struct user_entry *e,
			 char *buf, int idx)
{
  return 0;
}

static int xtra_dupuser(struct userrec *new, struct userrec *old,
			struct user_entry *e)
{
  struct xtra_key *x1, *x2;

  for (x1 = e->u.extra; x1; x1 = x1->next) {
    x2 = malloc(sizeof(struct xtra_key));

    x2->key = strdup(x1->key);
    x2->data = strdup(x1->data);
    set_user(&USERENTRY_XTRA, new, x2);
  }
  return 1;
}

static int xtra_write_userfile(FILE *f, struct userrec *u, struct user_entry *e)
{
  struct xtra_key *x;

  for (x = e->u.extra; x; x = x->next)
    if (fprintf(f, "--XTRA %s %s\n", x->key, x->data) == EOF)
      return 0;
  return 1;
}

int xtra_kill(struct user_entry *e)
{
  struct xtra_key *x, *y;

  for (x = e->u.extra; x; x = y) {
    y = x->next;
    free(x->key);
    free(x->data);
    free(x);
  }
  free(e);
  return 1;
}

struct user_entry_type USERENTRY_XTRA =
{
  0,
  xtra_gotshare,
  xtra_dupuser,
  xtra_unpack,
  xtra_pack,
  xtra_write_userfile,
  xtra_kill,
  def_get,
  xtra_set,
  NULL,
  NULL,
  NULL,
  "XTRA"
};

static int hosts_dupuser(struct userrec *new, struct userrec *old,
			 struct user_entry *e)
{
  struct list_type *h;

  for (h = e->u.extra; h; h = h->next)
    set_user(&USERENTRY_HOSTS, new, h->extra);
  return 1;
}

static int hosts_null(struct userrec *u, struct user_entry *e)
{
  return 1;
}

static int hosts_write_userfile(FILE *f, struct userrec *u, struct user_entry *e)
{
  struct list_type *h;

  for (h = e->u.extra; h; h = h->next)
    if (fprintf(f, "--HOSTS %s\n", h->extra) == EOF)
      return 0;
  return 1;
}

static int hosts_kill(struct user_entry *e)
{
  list_type_kill(e->u.list);
  free(e);
  return 1;
}

static void hosts_display(int idx, struct user_entry *e)
{
  char s[1024];
  struct list_type *q;

  s[0] = 0;
  strcpy(s, "  HOSTS: ");
  for (q = e->u.list; q; q = q->next) {
    if (s[0] && !s[9])
      strcat(s, q->extra);
    else if (!s[0])
      sprintf(s, "         %s", q->extra);
    else {
      if (strlen(s) + strlen(q->extra) + 2 > 65) {
	dprintf(idx, "%s\n", s);
	sprintf(s, "         %s", q->extra);
      } else {
	strcat(s, ", ");
	strcat(s, q->extra);
      }
    }
  }
  if (s[0])
    dprintf(idx, "%s\n", s);
}

static int hosts_set(struct userrec *u, struct user_entry *e, void *buf)
{
  if (!buf || !strcasecmp(buf, "none")) {
    /* When the bot crashes, it's in this part, not in the 'else' part */
    list_type_kill(e->u.list);
    e->u.list = NULL;
  } else {
    char *host = buf, *p = strchr(host, ',');
    struct list_type **t;

    /* Can't have ,'s in hostmasks */
    while (p) {
      *p = '?';
      p = strchr(host, ',');
    }
    /* fred1: check for redundant hostmasks with
     * controversial "superpenis" algorithm ;) */
    /* I'm surprised Raistlin hasn't gotten involved in this controversy */
    t = &(e->u.list);
    while (*t) {
      if (wild_match(host, (*t)->extra)) {
	struct list_type *u;

	u = *t;
	*t = (*t)->next;
	if (u->extra)
	  free(u->extra);
	free(u);
      } else
	t = &((*t)->next);
    }
    *t = malloc(sizeof(struct list_type));

    (*t)->next = NULL;
    (*t)->extra = strdup(host);
  }
  return 1;
}

static int hosts_gotshare(struct userrec *u, struct user_entry *e,
			  char *buf, int idx)
{
  return 0;
}

struct user_entry_type USERENTRY_HOSTS =
{
  0,
  hosts_gotshare,
  hosts_dupuser,
  hosts_null,
  hosts_null,
  hosts_write_userfile,
  hosts_kill,
  def_get,
  hosts_set,
  NULL,
  NULL,
  hosts_display,
  "HOSTS"
};

int list_append(struct list_type **h, struct list_type *i)
{
  for (; *h; h = &((*h)->next));
  *h = i;
  return 1;
}

int list_delete(struct list_type **h, struct list_type *i)
{
  for (; *h; h = &((*h)->next))
    if (*h == i) {
      *h = i->next;
      return 1;
    }
  return 0;
}

int list_contains(struct list_type *h, struct list_type *i)
{
  for (; h; h = h->next)
    if (h == i) {
      return 1;
    }
  return 0;
}

int add_entry_type(struct user_entry_type *type)
{
  struct userrec *u;

  list_insert(&entry_type_list, type);
  for (u = userlist; u; u = u->next) {
    struct user_entry *e = find_user_entry(type, u);

    if (e && e->name) {
      e->type = type;
      e->type->unpack(u, e);
      free_null(e->name);
    }
  }
  return 1;
}

int del_entry_type(struct user_entry_type *type)
{
  struct userrec *u;

  for (u = userlist; u; u = u->next) {
    struct user_entry *e = find_user_entry(type, u);

    if (e && !e->name) {
      e->type->pack(u, e);
      e->name = strdup(e->type->name);
      e->type = NULL;
    }
  }
  return list_delete((struct list_type **) &entry_type_list,
		     (struct list_type *) type);
}

struct user_entry_type *find_entry_type(char *name)
{
  struct user_entry_type *p;

  for (p = entry_type_list; p; p = p->next) {
    if (!strcasecmp(name, p->name))
      return p;
  }
  return NULL;
}

struct user_entry *find_user_entry(struct user_entry_type *et,
				   struct userrec *u)
{
  struct user_entry **e, *t;

  for (e = &(u->entries); *e; e = &((*e)->next)) {
    if (((*e)->type == et) ||
	((*e)->name && !strcasecmp((*e)->name, et->name))) {
      t = *e;
      *e = t->next;
      t->next = u->entries;
      u->entries = t;
      return t;
    }
  }
  return NULL;
}

void *get_user(struct user_entry_type *et, struct userrec *u)
{
  struct user_entry *e;

  if (u && (e = find_user_entry(et, u)))
    return et->get(u, e);
  return 0;
}

int set_user(struct user_entry_type *et, struct userrec *u, void *d)
{
  struct user_entry *e;
  int r;

  if (!u || !et)
    return 0;

  if (!(e = find_user_entry(et, u))) {
    e = malloc(sizeof(struct user_entry));

    e->type = et;
    e->name = NULL;
    e->u.list = NULL;
    list_insert((&(u->entries)), e);
  }
  r = et->set(u, e, d);
  if (!e->u.list) {
    list_delete((struct list_type **) &(u->entries), (struct list_type *) e);
    free(e);
  }
  return r;
}

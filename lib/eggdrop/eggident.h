#ifndef _EGGIDENT_H_
#define _EGGIDENT_H_

int egg_ident_lookup(const char *ip, int their_port, int our_port, int timeout, int (*callback)(), void *client_data);
int egg_ident_cancel(int id);

#endif /* _EGGIDENT_H_ */

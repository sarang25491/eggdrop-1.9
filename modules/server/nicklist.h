#ifndef _NICKLIST_H_
#define _NICKLIST_H_

void try_next_nick();
void try_random_nick();
void nick_list_on_connect();
const char *nick_get_next();
int nick_add(const char *nick);
int nick_del(int num);
int nick_clear();

#endif

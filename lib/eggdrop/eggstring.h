#ifndef _EGGSTRING_H_
#define _EGGSTRING_H_

int egg_get_word(const char *text, const char **next, char **word);
int egg_get_words(const char *text, const char **next, char **word, ...);
int egg_get_word_array(const char *text, const char **next, char **word, int nwords);
int egg_free_word_array(char **word, int nwords);
void egg_append_static_str(char **dest, int *remaining, const char *src);
void egg_append_str(char **dest, int *cur, int *max, const char *src);

#endif /* _EGGSTRING_H_ */

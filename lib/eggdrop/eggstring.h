#ifndef _EGGSTRING_H_
#define _EGGSTRING_H_

int egg_get_word(const char *text, const char **next, char **word);
int egg_get_words(const char *text, const char **next, char **word, ...);

#endif /* _EGGSTRING_H_ */

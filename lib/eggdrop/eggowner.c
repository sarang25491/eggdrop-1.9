#include <eggdrop/eggdrop.h>

static char **owner = NULL;

int egg_setowner(char **_owner)
{
	owner = _owner;
	return(0);
}

int egg_isowner(const char *handle)
{
	int len;
	char *powner;

	if (!owner || !*owner) return(0);

	len = strlen(handle);
	if (!len) return(0);

	powner = *owner;
	while (*powner) {
		while (*powner && !isalnum(*powner)) powner++;
		if (!*powner) break;
		if (!strncasecmp(powner, handle, len)) {
			powner += len;
			if (!*powner || !isalnum(*powner)) return(1);
		}
		while (*powner && isalnum(*powner)) powner++;
	}
	return(0);
}

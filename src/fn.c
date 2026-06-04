#include <stdbool.h>
#include <string.h>
#include <strings.h>

#include "fn.h"

bool fn_equal_strcmp(const void* const a, const void* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strcmp(a, b) == 0;
}

bool fn_equal_strcasecmp(const void* const a, const void* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strcasecmp(a, b) == 0;
}

bool fn_equal_strstr(const void* const a, const void* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return strstr(a, b);
}

void *fn_clone_strdup(const void* const val) {
	if (val == NULL)
		return NULL;

	return strdup(val);
}

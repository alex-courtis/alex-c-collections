#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "str.h"

char *vsprintf_alloc(const char *__restrict __format, va_list __args) {

	va_list args;
	va_copy(args, __args);
	size_t len = vsnprintf(NULL, 0, __format, args);
	va_end(args);

	char *str = calloc(len + 1, sizeof(char));

	va_copy(args, __args);
	vsnprintf(str, len + 1, __format, args);
	va_end(args);

	return str;
}

char *vsnprintf_alloc(size_t __maxlen, const char *__restrict __format, va_list __args) {

	va_list args;
	va_copy(args, __args);
	size_t raw = vsnprintf(NULL, 0, __format, args);
	va_end(args);

	size_t len = MIN(raw, __maxlen);

	char *str = calloc(len + 1, sizeof(char));

	va_copy(args, __args);
	vsnprintf(str, len + 1, __format, args);
	va_end(args);

	return str;
}

char *sprintf_alloc(const char *__restrict __format, ...) {

	va_list args;
	va_start(args, __format);
	size_t len = vsnprintf(NULL, 0, __format, args);
	va_end(args);

	char *str = calloc(len + 1, sizeof(char));

	va_start(args, __format);
	vsnprintf(str, len + 1, __format, args);
	va_end(args);

	return str;
}

char *sprintf_append(char *__restrict s, const char *__restrict __format, ...) {
	size_t l_left = s ? strlen(s) : 0;

	va_list args;
	va_start(args, __format);
	size_t l_right = vsnprintf(NULL, 0, __format, args);
	va_end(args);

	char *left = calloc(l_left + l_right + 1, sizeof(char));

	char *right = l_left ? stpncpy(left, s, l_left + 1) : left;

	va_start(args, __format);
	vsnprintf(right, l_right + 1, __format, args);
	va_end(args);

	if (s)
		free(s);

	return left;
}

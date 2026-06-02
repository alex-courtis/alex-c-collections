#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "str.h"

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

char *vsprintf_alloc(const char *__restrict __format, va_list __args) {
	return vsnprintf_alloc(SIZE_MAX, __format, __args);
}

char *sprintf_alloc(const char *__restrict __format, ...) {

	va_list args;
	va_start(args, __format);

	char *str = vsnprintf_alloc(SIZE_MAX, __format, args);

	va_end(args);

	return str;
}

char *snprintf_alloc(size_t __maxlen, const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);

	char *str = vsnprintf_alloc(__maxlen, __format, args);

	va_end(args);

	return str;
}

char *vsnprintf_append(char *__restrict s, size_t __maxlen, const char *__restrict __format, va_list __args) {

	char *full = vsprintf_append(s, __format, __args);

	char *str = strndup(full, __maxlen);

	free(full);

	return str;
}

char *vsprintf_append(char *__restrict s, const char *__restrict __format, va_list __args) {

	size_t l_left = s ? strlen(s) : 0;

	va_list args;
	va_copy(args, __args);
	size_t l_right = vsnprintf(NULL, 0, __format, args);
	va_end(args);

	char *left = calloc(l_left + l_right + 1, sizeof(char));

	char *right = l_left ? stpncpy(left, s, l_left + 1) : left;

	va_copy(args, __args);
	vsnprintf(right, l_right + 1, __format, args);
	va_end(args);

	if (s)
		free(s);

	return left;
}

char *sprintf_append(char *__restrict s, const char *__restrict __format, ...) {

	va_list args;
	va_start(args, __format);

	char *str = vsprintf_append(s, __format, args);

	va_end(args);

	return str;
}

char *snprintf_append(char *__restrict s, size_t __maxlen, const char *__restrict __format, ...) {

	va_list args;
	va_start(args, __format);

	char *str = vsnprintf_append(s, __maxlen, __format, args);

	va_end(args);

	return str;
}

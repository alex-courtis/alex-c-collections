#ifndef STR_H
#define STR_H

#include <stdarg.h>
#include <stddef.h>

// vsprintf to a malloc'd buffer, does not mutate __args
char *vsprintf_alloc(const char *__restrict __format, va_list __args);

// vsnprintf to a malloc'd buffer, does not mutate __args
char *vsnprintf_alloc(size_t __maxlen, const char *__restrict __format, va_list __args);

// sprintf to a malloc'd buffer
char *sprintf_alloc(const char *__restrict __format, ...) __attribute__ ((__format__ (__printf__, 1, 2)));

// append to nullable s, returning a malloc'd buffer, freeing s
char *sprintf_append(char *__restrict s, const char *__restrict __format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));

#endif // STR_H

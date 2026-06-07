#ifndef ASSERT_PTABLE_H
#define ASSERT_PTABLE_H

#include <cmocka.h>

#include "util-file.h"

#include "fn.h"
#include "ptable.h"

void _assert_ptable_equal(const struct PTable *a, const struct PTable *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (!ptable_equal(a, b, equal)) {
		write_file("actual.ptable", ptable_str(a, str));
		write_file("expected.ptable", ptable_str(b, str));
		cmocka_print_error("\n%s != \n%s", ptable_str(a, str), ptable_str(b, str));
		_fail(file, line);
	}
}
#define assert_ptable_equal(a, b, equal, str) _assert_ptable_equal(a, b, equal, str, __FILE__, __LINE__)

void _assert_ptable_not_equal(const struct PTable *a, const struct PTable *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (ptable_equal(a, b, equal)) {
		write_file("actual.ptable", ptable_str(a, str));
		write_file("expected.pet", ptable_str(b, str));
		cmocka_print_error("\n%s == \n%s", ptable_str(a, str), ptable_str(b, str));
		_fail(file, line);
	}
}
#define assert_ptable_not_equal(a, b, equal, str) _assert_ptable_not_equal(a, b, equal, str, __FILE__, __LINE__)

#endif // ASSERT_PTABLE_H

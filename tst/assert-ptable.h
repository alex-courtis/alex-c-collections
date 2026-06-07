#ifndef ASSERT_ptable_H
#define ASSERT_ptable_H

#include <cmocka.h>

#include "util-file.h"

#include "fn.h"
#include "ptable.h"

void _assert_ptable_equal(const struct PTable *a, const struct PTable *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (!ptable_equal(a, b, equal)) {
		char *a_str = ptable_str(a, str);
		char *b_str = ptable_str(b, str);
		write_file("actual.ptable", a_str);
		write_file("expected.ptable", b_str);
		cmocka_print_error("\n%s != \n%s", a_str, b_str);
		_fail(file, line);
	}
}
#define assert_ptable_equal(a, b, equal, str) _assert_ptable_equal(a, b, equal, str, __FILE__, __LINE__)

void _assert_ptable_not_equal(const struct PTable *a, const struct PTable *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (ptable_equal(a, b, equal)) {
		char *a_str = ptable_str(a, str);
		char *b_str = ptable_str(b, str);
		write_file("actual.ptable", a_str);
		write_file("expected.pet", b_str);
		cmocka_print_error("\n%s == \n%s", a_str, b_str);
		_fail(file, line);
	}
}
#define assert_ptable_not_equal(a, b, equal, str) _assert_ptable_not_equal(a, b, equal, str, __FILE__, __LINE__)

#endif // ASSERT_ptable_H

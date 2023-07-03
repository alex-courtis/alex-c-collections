#include "tst.h"

#include <cmocka.h>
#include <stdio.h>
#include <string.h>

#include "itable.h"

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	return 0;
}

int after_each(void **state) {
	return 0;
}

void mock_free_val(void *val) {
	check_expected(val);
}

void itable_init__size(void **state) {
	const struct ITable *tab = itable_init(5, 50);

	assert_non_null(tab);

	assert_int_equal(itable_size(tab), 0);

	itable_free(tab);
}

void itable_init__invalid(void **state) {
	const struct ITable *tab = itable_init(0, 0);

	assert_null(tab);
}

void itable_free_vals__null(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	char *val = strdup("0");

	itable_put(tab, 1, val);

	assert_int_equal(itable_size(tab), 1);

	// not much we can do here but valgrind
	itable_free_vals(tab, NULL);
}

void itable_free_vals__free_val(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	char *vals[] = { "0", "1", };

	expect_value(mock_free_val, val, "0");
	expect_value(mock_free_val, val, "1");

	itable_put(tab, 1, vals[0]);
	itable_put(tab, 2, vals[1]);

	assert_int_equal(itable_size(tab), 2);

	itable_free_vals(tab, mock_free_val);
}

void free_val_itable(void *val) {
	itable_free_vals(val, mock_free_val);
}

void itable_free_vals__free_val_reentrant(void **state) {
	const struct ITable *outer = itable_init(3, 5);
	const struct ITable *inner1 = itable_init(3, 5);
	const struct ITable *inner2 = itable_init(3, 5);

	char *vals[] = { "11", "12", "21", "22", };

	expect_value(mock_free_val, val, "11");
	expect_value(mock_free_val, val, "12");
	expect_value(mock_free_val, val, "21");
	expect_value(mock_free_val, val, "22");

	itable_put(inner1, 1, vals[0]);
	itable_put(inner1, 2, vals[1]);
	itable_put(inner2, 1, vals[2]);
	itable_put(inner2, 2, vals[3]);

	itable_put(outer, 1, (void*)inner1);
	itable_put(outer, 2, (void*)inner2);

	assert_int_equal(itable_size(outer), 2);

	itable_free_vals(outer, free_val_itable);
}

void itable_put__new(void **state) {
	const struct ITable *tab = itable_init(5, 5);

	char *vals[] = { "0", "1", };
	assert_null(itable_put(tab, 0, vals[0]));
	assert_null(itable_put(tab, 1, vals[1]));

	assert_int_equal(itable_size(tab), 2);
	assert_string_equal(itable_get(tab, 0), "0");
	assert_string_equal(itable_get(tab, 1), "1");

	itable_free(tab);
}

void itable_put__overwrite(void **state) {
	const struct ITable *tab = itable_init(5, 5);

	char *initial[] = { "0", "1", "2", };
	assert_null(itable_put(tab, 0, initial[0]));
	assert_null(itable_put(tab, 1, initial[1]));
	assert_null(itable_put(tab, 2, initial[2]));

	char *new1 = "new";
	assert_string_equal(itable_put(tab, 1, new1), "1");

	assert_int_equal(itable_size(tab), 3);
	assert_string_equal(itable_get(tab, 0), "0");
	assert_string_equal(itable_get(tab, 1), "new");
	assert_string_equal(itable_get(tab, 2), "2");

	itable_free(tab);
}

void itable_put__null(void **state) {
	const struct ITable *tab = itable_init(5, 5);

	char *zero = "0";
	assert_null(itable_put(tab, 0, zero));

	assert_string_equal(itable_get(tab, 0), "0");

	assert_string_equal(itable_put(tab, 0, NULL), "0");

	assert_int_equal(itable_size(tab), 0);
	assert_null(itable_get(tab, 0));

	itable_free(tab);
}

void itable_put__grow(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	char *initial[] = { "0", "1", "2", };
	assert_null(itable_put(tab, 0, initial[0]));
	assert_null(itable_put(tab, 1, initial[1]));
	assert_null(itable_put(tab, 2, initial[2]));

	assert_int_equal(itable_size(tab), 3);

	char *grow[] = { "3", "4", "5", };
	assert_null(itable_put(tab, 3, grow[0]));
	assert_null(itable_put(tab, 4, grow[1]));
	assert_null(itable_put(tab, 5, grow[2]));

	assert_int_equal(itable_size(tab), 6);

	assert_string_equal(itable_get(tab, 0), "0");
	assert_string_equal(itable_get(tab, 1), "1");
	assert_string_equal(itable_get(tab, 2), "2");

	assert_string_equal(itable_get(tab, 3), "3");
	assert_string_equal(itable_get(tab, 4), "4");
	assert_string_equal(itable_get(tab, 5), "5");

	itable_free(tab);
}

void itable_iter__empty(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	assert_null(itable_iter(tab));

	itable_free(tab);
}

void itable_iter__cleared(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	char *vals[] = { "0", "1", "2", };
	assert_null(itable_put(tab, 0, vals[0]));
	assert_null(itable_put(tab, 1, vals[1]));
	assert_null(itable_put(tab, 2, vals[2]));

	assert_int_equal(itable_size(tab), 3);

	assert_string_equal(itable_put(tab, 0, NULL), "0");
	assert_string_equal(itable_put(tab, 1, NULL), "1");
	assert_string_equal(itable_put(tab, 2, NULL), "2");

	assert_int_equal(itable_size(tab), 0);

	assert_null(itable_iter(tab));

	itable_free(tab);
}

void itable_iter__vals(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	char *vals[] = { "0", "1", "2", };
	assert_null(itable_put(tab, 0, vals[0]));
	assert_null(itable_put(tab, 1, vals[1]));
	assert_null(itable_put(tab, 2, vals[2]));

	// zero
	const struct ITableIter *iter = itable_iter(tab);
	assert_non_null(iter);
	assert_int_equal(iter->key, 0);
	assert_string_equal(iter->val, "0");

	// one
	iter = itable_next(iter);
	assert_non_null(iter);
	assert_int_equal(iter->key, 1);
	assert_string_equal(iter->val, "1");

	// two
	iter = itable_next(iter);
	assert_non_null(iter);
	assert_int_equal(iter->key, 2);
	assert_string_equal(iter->val, "2");

	// end
	iter = itable_next(iter);
	assert_null(iter);

	itable_free(tab);
}

void itable_iter__holes(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	void *vals[] = { "0", "1", "2", "3", "4", };
	assert_null(itable_put(tab, 0, vals[0]));
	assert_null(itable_put(tab, 1, vals[1]));
	assert_null(itable_put(tab, 2, vals[2]));
	assert_null(itable_put(tab, 3, vals[3]));
	assert_null(itable_put(tab, 4, vals[4]));

	assert_string_equal(itable_put(tab, 0, NULL), "0");
	assert_string_equal(itable_put(tab, 2, NULL), "2");
	assert_string_equal(itable_put(tab, 4, NULL), "4");

	assert_int_equal(itable_size(tab), 2);

	// one
	const struct ITableIter *iter = itable_iter(tab);
	assert_non_null(iter);
	assert_int_equal(iter->key, 1);
	assert_string_equal(iter->val, "1");

	// three
	iter = itable_next(iter);
	assert_non_null(iter);
	assert_int_equal(iter->key, 3);
	assert_string_equal(iter->val, "3");

	// end
	iter = itable_next(iter);
	assert_null(iter);

	itable_free(tab);
}

void itable_put__later(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	void *vals[] = { "0", "1", };
	assert_null(itable_put(tab, 0, vals[0]));
	assert_null(itable_put(tab, 1, vals[1]));

	assert_int_equal(itable_size(tab), 2);
	assert_string_equal(itable_get(tab, 0), "0");
	assert_string_equal(itable_get(tab, 1), "1");

	// remove zero
	assert_string_equal(itable_put(tab, 0, NULL), "0");

	assert_int_equal(itable_size(tab), 1);
	assert_null(itable_get(tab, 0));

	// put zero again afterwards
	assert_null(itable_put(tab, 0, vals[0]));
	assert_int_equal(itable_size(tab), 2);

	// one
	const struct ITableIter *iter = itable_iter(tab);
	assert_non_null(iter);
	assert_int_equal(iter->key, 1);
	assert_string_equal(iter->val, "1");

	// zero moved later
	iter = itable_next(iter);
	assert_non_null(iter);
	assert_int_equal(iter->key, 0);
	assert_string_equal(iter->val, "0");

	// end
	iter = itable_next(iter);
	assert_null(iter);

	itable_free(tab);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(itable_init__size),
		TEST(itable_init__invalid),

		TEST(itable_free_vals__null),
		TEST(itable_free_vals__free_val),
		TEST(itable_free_vals__free_val_reentrant),

		TEST(itable_put__new),
		TEST(itable_put__overwrite),
		TEST(itable_put__null),
		TEST(itable_put__grow),

		TEST(itable_iter__empty),
		TEST(itable_iter__cleared),
		TEST(itable_iter__vals),
		TEST(itable_iter__holes),

		TEST(itable_put__later),
	};

	return RUN(tests);
}


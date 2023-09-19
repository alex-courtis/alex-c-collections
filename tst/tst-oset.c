#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdio.h>
#include <string.h>

#include "fn.h"

#include "oset.h"

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

void mock_free_val(const void* const val) {
	check_expected(val);
}

void oset_init__size(void **state) {
	const struct OSet *set = oset_init(5, 50);

	assert_non_nul(set);

	assert_int_equal(oset_size(set), 0);

	assert_int_equal(oset_capacity(set), 5);

	oset_free(set);
}

void oset_init__invalid(void **state) {
	const struct OSet *set = oset_init(0, 0);

	assert_nul(set);
}

void oset_free_vals__null(void **state) {
	const struct OSet *set = oset_init(3, 5);

	char *vals[] = { strdup("0"), strdup("1"), };
	assert_true(oset_add(set, vals[0]));
	assert_true(oset_add(set, vals[1]));

	assert_int_equal(oset_size(set), 2);
	assert_true(oset_contains(set, vals[0]));
	assert_true(oset_contains(set, vals[1]));

	// not much we can do here but valgrind
	oset_free_vals(set, NULL);
}

void oset_free_vals__free_val(void **state) {
	const struct OSet *set = oset_init(3, 5);

	char *vals[] = { "0", "1", };
	assert_true(oset_add(set, vals[0]));
	assert_true(oset_add(set, vals[1]));

	assert_int_equal(oset_size(set), 2);
	assert_true(oset_contains(set, vals[0]));
	assert_true(oset_contains(set, vals[1]));

	expect_value(mock_free_val, val, vals[0]);
	expect_value(mock_free_val, val, vals[1]);

	oset_free_vals(set, mock_free_val);
}

void oset_add__new(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { "0", "1", };
	assert_true(oset_add(set, vals[0]));
	assert_true(oset_add(set, vals[1]));

	assert_int_equal(oset_size(set), 2);
	assert_true(oset_contains(set, vals[0]));
	assert_true(oset_contains(set, vals[1]));

	oset_free(set);
}

void oset_add__existing(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { "0", "1", };
	assert_true(oset_add(set, vals[0]));
	assert_true(oset_add(set, vals[1]));

	assert_int_equal(oset_size(set), 2);
	assert_true(oset_contains(set, vals[0]));
	assert_true(oset_contains(set, vals[1]));

	assert_false(oset_add(set, vals[0]));
	assert_false(oset_add(set, vals[1]));

	assert_int_equal(oset_size(set), 2);

	oset_free(set);
}

void oset_add__null(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { "0", };
	assert_true(oset_add(set, vals[0]));

	assert_int_equal(oset_size(set), 1);

	assert_false(oset_contains(set, NULL));
	assert_false(oset_add(set, NULL));
	assert_false(oset_contains(set, NULL));

	assert_int_equal(oset_size(set), 1);

	oset_free(set);
}

void oset_add__grow(void **state) {
	const struct OSet *set = oset_init(2, 5);

	void *initial[] = { "0", "1", };
	assert_true(oset_add(set, initial[0]));
	assert_true(oset_add(set, initial[1]));

	assert_int_equal(oset_size(set), 2);
	assert_int_equal(oset_capacity(set), 2);

	assert_true(oset_contains(set, initial[0]));
	assert_true(oset_contains(set, initial[1]));

	void *grow[] = { "2", "3", };
	assert_true(oset_add(set, grow[0]));
	assert_int_equal(oset_size(set), 3);
	assert_int_equal(oset_capacity(set), 7);
	assert_true(oset_contains(set, grow[0]));

	assert_true(oset_add(set, grow[1]));
	assert_int_equal(oset_size(set), 4);
	assert_int_equal(oset_capacity(set), 7);
	assert_true(oset_contains(set, grow[1]));

	void *subsequent[] = { "4", "5", };
	assert_true(oset_add(set, subsequent[0]));
	assert_true(oset_add(set, subsequent[1]));
	assert_int_equal(oset_size(set), 6);
	assert_int_equal(oset_capacity(set), 7);

	assert_true(oset_contains(set, subsequent[0]));
	assert_true(oset_contains(set, subsequent[1]));

	oset_free(set);
}

void oset_remove__existing(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { "0", "2", };
	assert_true(oset_add(set, vals[0]));
	assert_true(oset_add(set, vals[1]));

	assert_int_equal(oset_size(set), 2);
	assert_true(oset_contains(set, vals[0]));
	assert_true(oset_contains(set, vals[1]));

	// 0
	assert_true(oset_remove(set, vals[0]));

	assert_int_equal(oset_size(set), 1);
	assert_false(oset_contains(set, vals[0]));
	assert_true(oset_contains(set, vals[1]));

	// 1
	assert_true(oset_remove(set, vals[1]));

	assert_int_equal(oset_size(set), 0);
	assert_false(oset_contains(set, vals[0]));
	assert_false(oset_contains(set, vals[1]));

	oset_free(set);
}

void oset_remove__inexistent(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { "0", "1", };
	assert_true(oset_add(set, vals[0]));
	assert_true(oset_add(set, vals[1]));

	assert_int_equal(oset_size(set), 2);
	assert_true(oset_contains(set, vals[0]));
	assert_true(oset_contains(set, vals[1]));

	void *inexistent = "inexistent";
	assert_false(oset_remove(set, inexistent));

	assert_int_equal(oset_size(set), 2);
	assert_true(oset_contains(set, vals[0]));
	assert_true(oset_contains(set, vals[1]));

	oset_free(set);
}

void oset_iter__empty(void **state) {
	const struct OSet *set = oset_init(5, 5);

	assert_int_equal(oset_size(set), 0);

	assert_nul(oset_iter(set));

	oset_free(set);
}

void oset_iter__vals(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { "0", "1", };
	assert_true(oset_add(set, vals[0]));
	assert_true(oset_add(set, vals[1]));

	assert_int_equal(oset_size(set), 2);

	const struct OSetIter *iter = oset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "0");

	iter = oset_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "1");

	iter = oset_next(iter);
	assert_nul(iter);

	oset_free(set);
}

void oset_iter__cleared(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { "0", "1", };
	assert_true(oset_add(set, vals[0]));
	assert_true(oset_add(set, vals[1]));

	assert_int_equal(oset_size(set), 2);

	oset_remove(set, vals[0]);
	oset_remove(set, vals[1]);

	assert_int_equal(oset_size(set), 0);

	assert_nul(oset_iter(set));

	oset_free(set);
}

void oset_add__again(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { "0", "1", "2", "3", };
	assert_true(oset_add(set, vals[0]));
	assert_true(oset_add(set, vals[1]));
	assert_true(oset_add(set, vals[2]));
	assert_true(oset_add(set, vals[3]));

	assert_int_equal(oset_size(set), 4);

	// remove 1
	assert_true(oset_remove(set, vals[1]));
	assert_int_equal(oset_size(set), 3);

	// put 1 again afterwards
	assert_true(oset_add(set, vals[1]));
	assert_int_equal(oset_size(set), 4);

	// 0
	const struct OSetIter *iter = oset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "0");

	// 2
	iter = oset_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "2");

	// 3
	iter = oset_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "3");

	// 0 moved later
	iter = oset_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "1");

	// end
	iter = oset_next(iter);
	assert_nul(iter);

	oset_free(set);
}

void oset_equal__length_different(void **state) {
	const struct OSet *a = oset_init(5, 5);
	const struct OSet *b = oset_init(5, 5);

	assert_true(oset_add(a, "0"));

	assert_true(oset_add(b, "0"));
	assert_true(oset_add(b, "1"));

	assert_false(oset_equal(a, b, NULL));

	oset_free(a);
	oset_free(b);
}

void oset_equal__pointers_ok(void **state) {
	const struct OSet *a = oset_init(5, 5);
	const struct OSet *b = oset_init(5, 5);

	void *vals[] = { "0", "1", };
	assert_true(oset_add(a, vals[0]));
	assert_true(oset_add(a, vals[1]));

	assert_true(oset_add(b, vals[0]));
	assert_true(oset_add(b, vals[1]));

	assert_true(oset_equal(a, b, NULL));

	oset_free(a);
	oset_free(b);
}

void oset_equal__pointers_different(void **state) {
	const struct OSet *a = oset_init(5, 5);
	const struct OSet *b = oset_init(5, 5);

	void *vals[] = { "0", "1", "2", };
	assert_true(oset_add(a, vals[0]));
	assert_true(oset_add(a, vals[1]));

	assert_true(oset_add(b, vals[0]));
	assert_true(oset_add(b, vals[2]));

	assert_false(oset_equal(a, b, NULL));

	oset_free(a);
	oset_free(b);
}

void oset_equal__comparison_ok(void **state) {
	const struct OSet *a = oset_init(5, 5);
	const struct OSet *b = oset_init(5, 5);

	assert_true(oset_add(a, "0"));
	assert_true(oset_add(a, "1"));

	assert_true(oset_add(b, "0"));
	assert_true(oset_add(b, "1"));

	assert_true(oset_equal(a, b, fn_comp_equals_strcmp));

	oset_free(a);
	oset_free(b);
}

void oset_equal__comparison_different(void **state) {
	const struct OSet *a = oset_init(5, 5);
	const struct OSet *b = oset_init(5, 5);

	assert_true(oset_add(a, "0"));
	assert_true(oset_add(a, "1"));

	assert_true(oset_add(b, "0"));
	assert_true(oset_add(b, "2"));

	assert_false(oset_equal(a, b, fn_comp_equals_strcmp));

	oset_free(a);
	oset_free(b);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(oset_init__size),
		TEST(oset_init__invalid),

		TEST(oset_free_vals__null),
		TEST(oset_free_vals__free_val),

		TEST(oset_add__new),
		TEST(oset_add__existing),
		TEST(oset_add__null),
		TEST(oset_add__grow),

		TEST(oset_remove__existing),
		TEST(oset_remove__inexistent),

		TEST(oset_iter__empty),
		TEST(oset_iter__vals),
		TEST(oset_iter__cleared),

		TEST(oset_add__again),

		TEST(oset_equal__length_different),
		TEST(oset_equal__pointers_ok),
		TEST(oset_equal__pointers_different),
		TEST(oset_equal__comparison_ok),
		TEST(oset_equal__comparison_different),
	};

	return RUN(tests);
}


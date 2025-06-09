#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"

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

	oset_free_vals(set, NULL);
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

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(oset_add(set, vals[0]));
	assert_true(oset_add(set, vals[1]));

	assert_int_equal(oset_size(set), 2);
	assert_true(oset_contains(set, vals[0]));
	assert_true(oset_contains(set, vals[1]));

	oset_free_vals(set, NULL);
}

void oset_add__existing(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(oset_add(set, vals[0]));
	assert_true(oset_add(set, vals[1]));

	assert_int_equal(oset_size(set), 2);
	assert_true(oset_contains(set, vals[0]));
	assert_true(oset_contains(set, vals[1]));

	assert_false(oset_add(set, vals[0]));
	assert_false(oset_add(set, vals[1]));

	assert_int_equal(oset_size(set), 2);

	oset_free_vals(set, NULL);
}

void oset_add__null(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { strdup("0"), };
	assert_true(oset_add(set, vals[0]));

	assert_int_equal(oset_size(set), 1);

	assert_false(oset_contains(set, NULL));
	assert_false(oset_add(set, NULL));
	assert_false(oset_contains(set, NULL));

	assert_int_equal(oset_size(set), 1);

	oset_free_vals(set, NULL);
}

void oset_add__grow(void **state) {
	const struct OSet *set = oset_init(2, 5);

	void *initial[] = { strdup("0"), strdup("1"), };
	assert_true(oset_add(set, initial[0]));
	assert_true(oset_add(set, initial[1]));

	assert_int_equal(oset_size(set), 2);
	assert_int_equal(oset_capacity(set), 2);

	assert_true(oset_contains(set, initial[0]));
	assert_true(oset_contains(set, initial[1]));

	void *grow[] = { strdup("2"), strdup("3"), };
	assert_true(oset_add(set, grow[0]));
	assert_int_equal(oset_size(set), 3);
	assert_int_equal(oset_capacity(set), 7);
	assert_true(oset_contains(set, grow[0]));

	assert_true(oset_add(set, grow[1]));
	assert_int_equal(oset_size(set), 4);
	assert_int_equal(oset_capacity(set), 7);
	assert_true(oset_contains(set, grow[1]));

	void *subsequent[] = { strdup("4"), strdup("5"), };
	assert_true(oset_add(set, subsequent[0]));
	assert_true(oset_add(set, subsequent[1]));
	assert_int_equal(oset_size(set), 6);
	assert_int_equal(oset_capacity(set), 7);

	assert_true(oset_contains(set, subsequent[0]));
	assert_true(oset_contains(set, subsequent[1]));

	oset_free_vals(set, NULL);
}

void oset_remove__existing(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("2"), };
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

	oset_free_vals(set, NULL);

	free(vals[0]);
	free(vals[1]);
}

void oset_remove__inexistent(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
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

	oset_free_vals(set, NULL);
}

void oset_iter__empty(void **state) {
	const struct OSet *set = oset_init(5, 5);

	assert_int_equal(oset_size(set), 0);

	assert_nul(oset_iter(set));

	oset_free_vals(set, NULL);
}

void oset_iter__free(void **state) {
	const struct OSet *set = oset_init(3, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(oset_add(set, vals[0]));
	assert_true(oset_add(set, vals[1]));

	const struct OSetIter *iter = oset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(oset_iter_val(iter), "0");

	// not much we can do here but valgrind
	oset_iter_free(iter);

	oset_free_vals(set, NULL);
}


void oset_iter__vals(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(oset_add(set, vals[0]));
	assert_true(oset_add(set, vals[1]));

	assert_int_equal(oset_size(set), 2);

	const struct OSetIter *iter = oset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(oset_iter_val(iter), "0");

	iter = oset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(oset_iter_val(iter), "1");

	iter = oset_iter_next(iter);
	assert_nul(iter);

	oset_free_vals(set, NULL);
}

void oset_iter__cleared(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(oset_add(set, vals[0]));
	assert_true(oset_add(set, vals[1]));

	assert_int_equal(oset_size(set), 2);

	oset_remove(set, vals[0]);
	oset_remove(set, vals[1]);

	assert_int_equal(oset_size(set), 0);

	assert_nul(oset_iter(set));

	oset_free_vals(set, NULL);

	free(vals[0]);
	free(vals[1]);
}

void oset_add__again(void **state) {
	const struct OSet *set = oset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), strdup("3"), };
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
	assert_str_equal(oset_iter_val(iter), "0");

	// 2
	iter = oset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(oset_iter_val(iter), "2");

	// 3
	iter = oset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(oset_iter_val(iter), "3");

	// 0 moved later
	iter = oset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(oset_iter_val(iter), "1");

	// end
	iter = oset_iter_next(iter);
	assert_nul(iter);

	oset_free_vals(set, NULL);
}

void oset_equal__length_different(void **state) {
	const struct OSet *a = oset_init(5, 5);
	const struct OSet *b = oset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };

	assert_true(oset_add(a, vals[0]));

	assert_true(oset_add(b, vals[0]));
	assert_true(oset_add(b, vals[1]));

	assert_false(oset_equal(a, b, NULL));

	oset_free(a);
	oset_free_vals(b, NULL);
}

void oset_equal__pointers_ok(void **state) {
	const struct OSet *a = oset_init(5, 5);
	const struct OSet *b = oset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(oset_add(a, vals[0]));
	assert_true(oset_add(a, vals[1]));

	assert_true(oset_add(b, vals[0]));
	assert_true(oset_add(b, vals[1]));

	assert_true(oset_equal(a, b, NULL));

	oset_free_vals(a, NULL);
	oset_free(b);
}

void oset_equal__pointers_different(void **state) {
	const struct OSet *a = oset_init(5, 5);
	const struct OSet *b = oset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), };
	assert_true(oset_add(a, vals[0]));
	assert_true(oset_add(a, vals[1]));

	assert_true(oset_add(b, vals[0]));
	assert_true(oset_add(b, vals[2]));

	assert_false(oset_equal(a, b, NULL));

	oset_free_vals(a, NULL);
	oset_free(b);

	free(vals[2]);
}

void oset_equal__comparison_ok(void **state) {
	const struct OSet *a = oset_init(5, 5);
	const struct OSet *b = oset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };

	assert_true(oset_add(a, vals[0]));
	assert_true(oset_add(a, vals[1]));

	assert_true(oset_add(b, vals[0]));
	assert_true(oset_add(b, vals[1]));

	assert_true(oset_equal(a, b, fn_comp_equals_strcmp));

	oset_free(a);
	oset_free_vals(b, NULL);
}

void oset_equal__comparison_different(void **state) {
	const struct OSet *a = oset_init(5, 5);
	const struct OSet *b = oset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), };

	assert_true(oset_add(a, vals[0]));
	assert_true(oset_add(a, vals[1]));

	assert_true(oset_add(b, vals[0]));
	assert_true(oset_add(b, vals[2]));

	assert_false(oset_equal(a, b, fn_comp_equals_strcmp));

	oset_free_vals(a, NULL);
	oset_free(b);

	free(vals[2]);
}

void oset_vals_slist__empty(void **state) {
	const struct OSet *set = oset_init(3, 5);

	assert_nul(oset_vals_slist(set));

	oset_free_vals(set, NULL);
}

void oset_vals_slist__many(void **state) {
	const struct OSet *tab = oset_init(3, 5);

	void *vals[] = { strdup("0"), strdup("1"), };

	assert_true(oset_add(tab, vals[0]));
	assert_true(oset_add(tab, vals[1]));

	struct SList *list = oset_vals_slist(tab);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "0");
	assert_str_equal(slist_at(list, 1), "1");

	slist_free(&list);
	oset_free_vals(tab, NULL);
}

void oset_str__null(void **state) {
	assert_nul(oset_str(NULL));
}

void oset_str__empty(void **state) {
	const struct OSet *set = oset_init(5, 5);

	char *str = oset_str(set);
	assert_str_equal(str, "");

	free(str);
	oset_free_vals(set, NULL);
}

void oset_str__string_vals(void **state) {
	const struct OSet *set = oset_init(5, 5);

	assert_true(oset_add(set, "ONE"));
	assert_true(oset_add(set, "TWO"));
	assert_true(oset_add(set, "THREE"));

	char *str = oset_str(set);
	assert_str_equal(str,
			"ONE\n"
			"TWO\n"
			"THREE"
			);

	free(str);
	oset_free(set);
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
		TEST(oset_iter__free),
		TEST(oset_iter__vals),
		TEST(oset_iter__cleared),

		TEST(oset_add__again),

		TEST(oset_equal__length_different),
		TEST(oset_equal__pointers_ok),
		TEST(oset_equal__pointers_different),
		TEST(oset_equal__comparison_ok),
		TEST(oset_equal__comparison_different),

		TEST(oset_vals_slist__empty),
		TEST(oset_vals_slist__many),

		TEST(oset_str__null),
		TEST(oset_str__empty),
		TEST(oset_str__string_vals),
	};

	return RUN(tests);
}


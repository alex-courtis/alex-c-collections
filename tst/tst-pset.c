#include "tst.h"
#include "asserts.h"
#include "expects.h"

#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"

#include "pset.h"

static int before_all(void **state) {
	return 0;
}

static int after_all(void **state) {
	return 0;
}

static int before_each(void **state) {
	return 0;
}

static int after_each(void **state) {
	return 0;
}

static void mock_free_val(const void* const val) {
	check_expected_ptr(val);
}

static void pset_init__size(void **state) {
	const struct PSet *set = pset_init(5, 50);

	assert_non_nul(set);

	assert_int_equal(pset_size(set), 0);

	assert_int_equal(pset_capacity(set), 5);

	pset_free_vals(set, NULL);
}

static void pset_init__invalid(void **state) {
	const struct PSet *set = pset_init(0, 0);

	assert_nul(set);
}

static void pset_free_vals__null(void **state) {
	const struct PSet *set = pset_init(3, 5);

	char *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, vals[0]));
	assert_true(pset_add(set, vals[1]));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, vals[0]));
	assert_true(pset_contains(set, vals[1]));

	// not much we can do here but valgrind
	pset_free_vals(set, NULL);
}

static void pset_free_vals__free_val(void **state) {
	const struct PSet *set = pset_init(3, 5);

	char *vals[] = { "0", "1", };
	assert_true(pset_add(set, vals[0]));
	assert_true(pset_add(set, vals[1]));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, vals[0]));
	assert_true(pset_contains(set, vals[1]));

	expect_str(mock_free_val, val, vals[0]);
	expect_str(mock_free_val, val, vals[1]);

	pset_free_vals(set, mock_free_val);
}

static void pset_add__new(void **state) {
	const struct PSet *set = pset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, vals[0]));
	assert_true(pset_add(set, vals[1]));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, vals[0]));
	assert_true(pset_contains(set, vals[1]));

	pset_free_vals(set, NULL);
}

static void pset_add__existing(void **state) {
	const struct PSet *set = pset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, vals[0]));
	assert_true(pset_add(set, vals[1]));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, vals[0]));
	assert_true(pset_contains(set, vals[1]));

	assert_false(pset_add(set, vals[0]));
	assert_false(pset_add(set, vals[1]));

	assert_int_equal(pset_size(set), 2);

	pset_free_vals(set, NULL);
}

static void pset_add__null(void **state) {
	const struct PSet *set = pset_init(5, 5);

	void *vals[] = { strdup("0"), };
	assert_true(pset_add(set, vals[0]));

	assert_int_equal(pset_size(set), 1);

	assert_false(pset_contains(set, NULL));
	assert_false(pset_add(set, NULL));
	assert_false(pset_contains(set, NULL));

	assert_int_equal(pset_size(set), 1);

	pset_free_vals(set, NULL);
}

static void pset_add__grow(void **state) {
	const struct PSet *set = pset_init(2, 5);

	void *initial[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, initial[0]));
	assert_true(pset_add(set, initial[1]));

	assert_int_equal(pset_size(set), 2);
	assert_int_equal(pset_capacity(set), 2);

	assert_true(pset_contains(set, initial[0]));
	assert_true(pset_contains(set, initial[1]));

	void *grow[] = { strdup("2"), strdup("3"), };
	assert_true(pset_add(set, grow[0]));
	assert_int_equal(pset_size(set), 3);
	assert_int_equal(pset_capacity(set), 7);
	assert_true(pset_contains(set, grow[0]));

	assert_true(pset_add(set, grow[1]));
	assert_int_equal(pset_size(set), 4);
	assert_int_equal(pset_capacity(set), 7);
	assert_true(pset_contains(set, grow[1]));

	void *subsequent[] = { strdup("4"), strdup("5"), };
	assert_true(pset_add(set, subsequent[0]));
	assert_true(pset_add(set, subsequent[1]));
	assert_int_equal(pset_size(set), 6);
	assert_int_equal(pset_capacity(set), 7);

	assert_true(pset_contains(set, subsequent[0]));
	assert_true(pset_contains(set, subsequent[1]));

	pset_free_vals(set, NULL);
}

static void pset_remove__existing(void **state) {
	const struct PSet *set = pset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("2"), };
	assert_true(pset_add(set, vals[0]));
	assert_true(pset_add(set, vals[1]));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, vals[0]));
	assert_true(pset_contains(set, vals[1]));

	// 0
	assert_true(pset_remove(set, vals[0]));

	assert_int_equal(pset_size(set), 1);
	assert_false(pset_contains(set, vals[0]));
	assert_true(pset_contains(set, vals[1]));

	// 1
	assert_true(pset_remove(set, vals[1]));

	assert_int_equal(pset_size(set), 0);
	assert_false(pset_contains(set, vals[0]));
	assert_false(pset_contains(set, vals[1]));

	pset_free_vals(set, NULL);

	free(vals[0]);
	free(vals[1]);
}

static void pset_remove__inexistent(void **state) {
	const struct PSet *set = pset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, vals[0]));
	assert_true(pset_add(set, vals[1]));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, vals[0]));
	assert_true(pset_contains(set, vals[1]));

	const void *inexistent = "inexistent";
	assert_false(pset_remove(set, inexistent));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, vals[0]));
	assert_true(pset_contains(set, vals[1]));

	pset_free_vals(set, NULL);
}

static void pset_iter__empty(void **state) {
	const struct PSet *set = pset_init(5, 5);

	assert_int_equal(pset_size(set), 0);

	assert_nul(pset_iter(set));

	pset_free_vals(set, NULL);
}

static void pset_iter__free(void **state) {
	const struct PSet *set = pset_init(3, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, vals[0]));
	assert_true(pset_add(set, vals[1]));

	const struct PSetIter *iter = pset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), "0");

	// not much we can do here but valgrind
	pset_iter_free(iter);

	pset_free_vals(set, NULL);
}


static void pset_iter__vals(void **state) {
	const struct PSet *set = pset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, vals[0]));
	assert_true(pset_add(set, vals[1]));

	assert_int_equal(pset_size(set), 2);

	const struct PSetIter *iter = pset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), "0");

	iter = pset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), "1");

	iter = pset_iter_next(iter);
	assert_nul(iter);

	pset_free_vals(set, NULL);
}

static void pset_iter__cleared(void **state) {
	const struct PSet *set = pset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, vals[0]));
	assert_true(pset_add(set, vals[1]));

	assert_int_equal(pset_size(set), 2);

	pset_remove(set, vals[0]);
	pset_remove(set, vals[1]);

	assert_int_equal(pset_size(set), 0);

	assert_nul(pset_iter(set));

	pset_free_vals(set, NULL);

	free(vals[0]);
	free(vals[1]);
}

static void pset_add__again(void **state) {
	const struct PSet *set = pset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), strdup("3"), };
	assert_true(pset_add(set, vals[0]));
	assert_true(pset_add(set, vals[1]));
	assert_true(pset_add(set, vals[2]));
	assert_true(pset_add(set, vals[3]));

	assert_int_equal(pset_size(set), 4);

	// remove 1
	assert_true(pset_remove(set, vals[1]));
	assert_int_equal(pset_size(set), 3);

	// put 1 again afterwards
	assert_true(pset_add(set, vals[1]));
	assert_int_equal(pset_size(set), 4);

	// 0
	const struct PSetIter *iter = pset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), "0");

	// 2
	iter = pset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), "2");

	// 3
	iter = pset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), "3");

	// 0 moved later
	iter = pset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), "1");

	// end
	iter = pset_iter_next(iter);
	assert_nul(iter);

	pset_free_vals(set, NULL);
}

static void pset_equal__length_different(void **state) {
	const struct PSet *a = pset_init(5, 5);
	const struct PSet *b = pset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };

	assert_true(pset_add(a, vals[0]));

	assert_true(pset_add(b, vals[0]));
	assert_true(pset_add(b, vals[1]));

	assert_false(pset_equal(a, b, NULL));

	pset_free(a);
	pset_free_vals(b, NULL);
}

static void pset_equal__pointers_ok(void **state) {
	const struct PSet *a = pset_init(5, 5);
	const struct PSet *b = pset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(a, vals[0]));
	assert_true(pset_add(a, vals[1]));

	assert_true(pset_add(b, vals[0]));
	assert_true(pset_add(b, vals[1]));

	assert_true(pset_equal(a, b, NULL));

	pset_free_vals(a, NULL);
	pset_free(b);
}

static void pset_equal__pointers_different(void **state) {
	const struct PSet *a = pset_init(5, 5);
	const struct PSet *b = pset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), };
	assert_true(pset_add(a, vals[0]));
	assert_true(pset_add(a, vals[1]));

	assert_true(pset_add(b, vals[0]));
	assert_true(pset_add(b, vals[2]));

	assert_false(pset_equal(a, b, NULL));

	pset_free_vals(a, NULL);
	pset_free(b);

	free(vals[2]);
}

static void pset_equal__comparison_ok(void **state) {
	const struct PSet *a = pset_init(5, 5);
	const struct PSet *b = pset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };

	assert_true(pset_add(a, vals[0]));
	assert_true(pset_add(a, vals[1]));

	assert_true(pset_add(b, vals[0]));
	assert_true(pset_add(b, vals[1]));

	assert_true(pset_equal(a, b, fn_comp_equals_strcmp));

	pset_free(a);
	pset_free_vals(b, NULL);
}

static void pset_equal__comparison_different(void **state) {
	const struct PSet *a = pset_init(5, 5);
	const struct PSet *b = pset_init(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), };

	assert_true(pset_add(a, vals[0]));
	assert_true(pset_add(a, vals[1]));

	assert_true(pset_add(b, vals[0]));
	assert_true(pset_add(b, vals[2]));

	assert_false(pset_equal(a, b, fn_comp_equals_strcmp));

	pset_free_vals(a, NULL);
	pset_free(b);

	free(vals[2]);
}

static void pset_vals_slist__empty(void **state) {
	const struct PSet *set = pset_init(3, 5);

	assert_nul(pset_vals_slist(set));

	pset_free_vals(set, NULL);
}

static void pset_vals_slist__many(void **state) {
	const struct PSet *tab = pset_init(3, 5);

	void *vals[] = { strdup("0"), strdup("1"), };

	assert_true(pset_add(tab, vals[0]));
	assert_true(pset_add(tab, vals[1]));

	struct SList *list = pset_vals_slist(tab);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "0");
	assert_str_equal(slist_at(list, 1), "1");

	slist_free(&list);
	pset_free_vals(tab, NULL);
}

static void pset_str__null(void **state) {
	assert_nul(pset_str(NULL));
}

static void pset_str__empty(void **state) {
	const struct PSet *set = pset_init(5, 5);

	char *str = pset_str(set);
	assert_str_equal(str, "");

	free(str);
	pset_free_vals(set, NULL);
}

static void pset_str__string_vals(void **state) {
	const struct PSet *set = pset_init(5, 5);

	assert_true(pset_add(set, "ONE"));
	assert_true(pset_add(set, "TWO"));
	assert_true(pset_add(set, "THREE"));

	char *str = pset_str(set);
	assert_str_equal(str,
			"ONE\n"
			"TWO\n"
			"THREE\n"
			);

	free(str);
	pset_free(set);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(pset_init__size),
		TEST(pset_init__invalid),

		TEST(pset_free_vals__null),
		TEST(pset_free_vals__free_val),

		TEST(pset_add__new),
		TEST(pset_add__existing),
		TEST(pset_add__null),
		TEST(pset_add__grow),

		TEST(pset_remove__existing),
		TEST(pset_remove__inexistent),

		TEST(pset_iter__empty),
		TEST(pset_iter__free),
		TEST(pset_iter__vals),
		TEST(pset_iter__cleared),

		TEST(pset_add__again),

		TEST(pset_equal__length_different),
		TEST(pset_equal__pointers_ok),
		TEST(pset_equal__pointers_different),
		TEST(pset_equal__comparison_ok),
		TEST(pset_equal__comparison_different),

		TEST(pset_vals_slist__empty),
		TEST(pset_vals_slist__many),

		TEST(pset_str__null),
		TEST(pset_str__empty),
		TEST(pset_str__string_vals),
	};

	return RUN(tests);
}


#include "tst.h"
#include "asserts.h"
#include "assert-pset.h"
#include "expects.h"

#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"

#include "pset.h"

#include "data/words-sorted.c"
#include "data/words-unsorted.c"

/*
   diff -u \
   <(sed -e 's/pset/xset/g ; s/PSet/XSet/g' tst/tst-pset.c) \
   <(sed -e 's/sset/xset/g ; s/SSet/XSet/g' tst/tst-sset.c)
   */

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

static char* fn_str_first(const void *val) {
	return strndup(val, 1);
}

static void pset_init__size(void **state) {
	const struct PSet *set = pset_init_with(5, 50);

	assert_non_nul(set);

	assert_int_equal(pset_size(set), 0);

	assert_int_equal(pset_capacity(set), 5);

	pset_free_vals(set, NULL);
}

static void pset_init__invalid(void **state) {
	const struct PSet *set = pset_init_with(0, 0);

	assert_nul(set);
}

static void pset_clone__null(void **state) {
	assert_nul(pset_clone(NULL, NULL));
}

static void pset_clone__empty(void **state) {
	const struct PSet *set = pset_init();

	const struct PSet *clone = pset_clone(set, NULL);

	assert_non_nul(clone);

	assert_int_equal(pset_size(clone), 0);

	pset_free_vals(set, NULL);
	pset_free_vals(clone, NULL);
}

static void pset_clone__params(void **state) {
	const struct PSet *set = pset_init_with(3, 4);

	const struct PSet *clone = pset_clone(set, NULL);

	assert_non_nul(clone);

	assert_int_equal(pset_size(clone), 0);

	assert_int_equal(pset_capacity(clone), 3);

	pset_free_vals(set, NULL);
	pset_free_vals(clone, NULL);
}

static void pset_clone__shallow_many(void **state) {
	const struct PSet *set = pset_init();

	char *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, vals[0], NULL));
	assert_true(pset_add(set, vals[1], NULL));

	const struct PSet *clone = pset_clone(set, NULL);

	assert_int_equal(pset_size(clone), 2);

	assert_true(pset_contains(clone, vals[0], NULL));
	assert_true(pset_contains(clone, vals[1], NULL));

	assert_pset_equal(set, clone, NULL, NULL);
	assert_pset_equal(clone, set, NULL, NULL);

	pset_free_vals(clone, NULL);
	pset_free(set);
}

static void pset_clone__deep_many(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, strdup("0"), NULL));
	assert_true(pset_add(set, strdup("1"), NULL));

	const struct PSet *clone = pset_clone(set, fn_clone_strdup);

	assert_int_equal(pset_size(clone), 2);

	assert_true(pset_contains(clone, "0", fn_equal_strcmp));
	assert_true(pset_contains(clone, "1", fn_equal_strcmp));

	assert_pset_equal(set, clone, fn_equal_strcmp, NULL);
	assert_pset_equal(clone, set, fn_equal_strcmp, NULL);

	pset_free_vals(clone, NULL);
	pset_free_vals(set, NULL);
}

static void pset_free_vals__null(void **state) {
	const struct PSet *set = pset_init();

	char *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, vals[0], NULL));
	assert_true(pset_add(set, vals[1], NULL));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, vals[0], NULL));
	assert_true(pset_contains(set, vals[1], NULL));

	// not much we can do here but valgrind
	pset_free_vals(set, NULL);
}

static void pset_free_vals__free_val(void **state) {
	const struct PSet *set = pset_init_with(3, 5);

	char *vals[] = { "0", "1", };
	assert_true(pset_add(set, vals[0], NULL));
	assert_true(pset_add(set, vals[1], NULL));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, vals[0], NULL));
	assert_true(pset_contains(set, vals[1], NULL));

	expect_str(mock_free_val, val, vals[0]);
	expect_str(mock_free_val, val, vals[1]);

	pset_free_vals(set, mock_free_val);
}

static void pset_add__new(void **state) {
	const struct PSet *set = pset_init_with(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, vals[0], NULL));
	assert_true(pset_add(set, vals[1], NULL));

	assert_int_equal(pset_size(set), 2);

	assert_false(pset_add(set, vals[1], NULL));

	assert_true(pset_contains(set, vals[0], NULL));
	assert_true(pset_contains(set, vals[1], NULL));

	pset_free_vals(set, NULL);
}

static void pset_add__new_fn_equal(void **state) {
	const struct PSet *set = pset_init_with(5, 5);

	assert_true(pset_add(set, strdup("0"), fn_equal_strcmp));
	assert_true(pset_add(set, strdup("1"), fn_equal_strcmp));

	assert_int_equal(pset_size(set), 2);

	assert_false(pset_add(set, "1", fn_equal_strcmp));

	assert_true(pset_contains(set, "0", fn_equal_strcmp));
	assert_true(pset_contains(set, "1", fn_equal_strcmp));

	pset_free_vals(set, NULL);
}

static void pset_add__null(void **state) {
	const struct PSet *set = pset_init_with(5, 5);

	void *vals[] = { strdup("0"), };
	assert_true(pset_add(set, vals[0], NULL));

	assert_int_equal(pset_size(set), 1);

	assert_false(pset_contains(set, NULL, NULL));
	assert_false(pset_add(set, NULL, NULL));
	assert_false(pset_contains(set, NULL, NULL));

	assert_int_equal(pset_size(set), 1);

	pset_free_vals(set, NULL);
}

static void pset_add__grow(void **state) {
	const struct PSet *set = pset_init_with(2, 5);

	void *initial[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, initial[0], NULL));
	assert_true(pset_add(set, initial[1], NULL));

	assert_int_equal(pset_size(set), 2);
	assert_int_equal(pset_capacity(set), 2);

	assert_true(pset_contains(set, initial[0], NULL));
	assert_true(pset_contains(set, initial[1], NULL));

	void *grow[] = { strdup("2"), strdup("3"), };
	assert_true(pset_add(set, grow[0], NULL));
	assert_int_equal(pset_size(set), 3);
	assert_int_equal(pset_capacity(set), 7);
	assert_true(pset_contains(set, grow[0], NULL));

	assert_true(pset_add(set, grow[1], NULL));
	assert_int_equal(pset_size(set), 4);
	assert_int_equal(pset_capacity(set), 7);
	assert_true(pset_contains(set, grow[1], NULL));

	void *subsequent[] = { strdup("4"), strdup("5"), };
	assert_true(pset_add(set, subsequent[0], NULL));
	assert_true(pset_add(set, subsequent[1], NULL));
	assert_int_equal(pset_size(set), 6);
	assert_int_equal(pset_capacity(set), 7);

	assert_true(pset_contains(set, subsequent[0], NULL));
	assert_true(pset_contains(set, subsequent[1], NULL));

	pset_free_vals(set, NULL);
}

static void pset_remove__existing(void **state) {
	const struct PSet *set = pset_init_with(5, 5);

	void *vals[] = { strdup("0"), strdup("2"), };
	assert_true(pset_add(set, vals[0], NULL));
	assert_true(pset_add(set, vals[1], NULL));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, vals[0], NULL));
	assert_true(pset_contains(set, vals[1], NULL));

	// 0
	assert_non_nul(pset_remove(set, vals[0], NULL));

	assert_int_equal(pset_size(set), 1);
	assert_false(pset_contains(set, vals[0], NULL));
	assert_true(pset_contains(set, vals[1], NULL));

	// 1
	assert_non_nul(pset_remove(set, vals[1], NULL));

	assert_int_equal(pset_size(set), 0);
	assert_false(pset_contains(set, vals[0], NULL));
	assert_false(pset_contains(set, vals[1], NULL));

	pset_free_vals(set, NULL);

	free(vals[0]);
	free(vals[1]);
}

static void pset_remove__inexistent(void **state) {
	const struct PSet *set = pset_init_with(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, vals[0], NULL));
	assert_true(pset_add(set, vals[1], NULL));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, vals[0], NULL));
	assert_true(pset_contains(set, vals[1], NULL));

	const void *inexistent = "inexistent";
	assert_null(pset_remove(set, inexistent, NULL));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, vals[0], NULL));
	assert_true(pset_contains(set, vals[1], NULL));

	pset_free_vals(set, NULL);
}

static void pset_remove__fn_equal(void **state) {
	const struct PSet *set = pset_init_with(5, 5);

	assert_true(pset_add(set, strdup("0"), fn_equal_strcmp));
	assert_true(pset_add(set, strdup("1"), fn_equal_strcmp));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, "0", fn_equal_strcmp));
	assert_true(pset_contains(set, "1", fn_equal_strcmp));

	// 0
	const char *removed = pset_remove(set, "0", fn_equal_strcmp);
	assert_str_equal(removed, "0");
	free((void*)removed);

	assert_int_equal(pset_size(set), 1);
	assert_false(pset_contains(set, "0", fn_equal_strcmp));
	assert_true(pset_contains(set, "1", fn_equal_strcmp));

	// 1
	removed = pset_remove(set, "1", fn_equal_strcmp);
	assert_str_equal(removed, "1");
	free((void*)removed);

	assert_int_equal(pset_size(set), 0);
	assert_false(pset_contains(set, "0", fn_equal_strcmp));
	assert_false(pset_contains(set, "1", fn_equal_strcmp));

	pset_free_vals(set, NULL);
}

static void pset_iter__empty(void **state) {
	const struct PSet *set = pset_init_with(5, 5);

	assert_int_equal(pset_size(set), 0);

	assert_nul(pset_iter(set));

	pset_free_vals(set, NULL);
}

static void pset_iter__free(void **state) {
	const struct PSet *set = pset_init_with(3, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, vals[0], NULL));
	assert_true(pset_add(set, vals[1], NULL));

	const struct PSetIter *iter = pset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), "0");

	// not much we can do here but valgrind
	pset_iter_free(iter);

	pset_free_vals(set, NULL);
}


static void pset_iter__many(void **state) {
	const struct PSet *set = pset_init_with(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, vals[0], NULL));
	assert_true(pset_add(set, vals[1], NULL));

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
	const struct PSet *set = pset_init_with(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(set, vals[0], NULL));
	assert_true(pset_add(set, vals[1], NULL));

	assert_int_equal(pset_size(set), 2);

	assert_non_nul(pset_remove(set, vals[0], NULL));
	assert_non_nul(pset_remove(set, vals[1], NULL));

	assert_int_equal(pset_size(set), 0);

	assert_nul(pset_iter(set));

	pset_free_vals(set, NULL);

	free(vals[0]);
	free(vals[1]);
}

static void pset_add__again(void **state) {
	const struct PSet *set = pset_init_with(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), strdup("3"), };
	assert_true(pset_add(set, vals[0], NULL));
	assert_true(pset_add(set, vals[1], NULL));
	assert_true(pset_add(set, vals[2], NULL));
	assert_true(pset_add(set, vals[3], NULL));

	assert_int_equal(pset_size(set), 4);

	// remove 1
	assert_non_nul(pset_remove(set, vals[1], NULL));
	assert_int_equal(pset_size(set), 3);

	// put 1 again afterwards
	assert_true(pset_add(set, vals[1], NULL));
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

static void pset_sort__empty(void **state) {
	const struct PSet *actual = pset_init();

	const struct PSet *expected = pset_init();

	pset_sort(actual, fn_less_than_strcmp);

	assert_int_equal(pset_size(actual), 0);

	assert_pset_equal(actual, expected, fn_equal_strcmp, NULL);

	pset_free_vals(actual, NULL);
	pset_free(expected);
}

static void pset_sort__one(void **state) {
	const struct PSet *actual = pset_init();

	assert_true(pset_add(actual, strdup("A"), NULL));

	const struct PSet *expected = pset_init();
	assert_true(pset_add(expected, strdup("A"), NULL));

	pset_sort(actual, fn_less_than_strcmp);

	assert_pset_equal(actual, expected, fn_equal_strcmp, NULL);

	pset_free_vals(actual, NULL);
	pset_free_vals(expected, NULL);
}

static void pset_sort__many(void **state) {
	const struct PSet *actual = pset_init();

	assert_true(pset_add(actual, "3", NULL));
	assert_true(pset_add(actual, "1", NULL));
	assert_true(pset_add(actual, "0", NULL));
	assert_true(pset_add(actual, "2", NULL));

	const struct PSet *expected = pset_init();
	assert_true(pset_add(expected, "0", NULL));
	assert_true(pset_add(expected, "1", NULL));
	assert_true(pset_add(expected, "2", NULL));
	assert_true(pset_add(expected, "3", NULL));

	pset_sort(actual, fn_less_than_strcmp);

	assert_pset_equal(actual, expected, fn_equal_strcmp, fn_str_first);

	pset_free(actual);
	pset_free(expected);
}

static void pset_sort__words(void **state) {
	const struct PSet *actual = pset_init_with(1000, 1000);

	for (size_t i = sizeof(words_unsorted) / sizeof(words_unsorted[0]); i > 0; i--) {
		assert_true(pset_add(actual, words_unsorted[i - 1], NULL));
	}

	const struct PSet *expected = pset_init_with(1000, 1000);

	for (size_t i = 0; i < sizeof(words_sorted) / sizeof(words_sorted[0]); i++ ) {
		assert_true(pset_add(expected, words_sorted[i], NULL));
	}

	pset_sort(actual, fn_less_than_strcmp);

	assert_pset_equal(actual, expected, fn_equal_strcmp, NULL);

	pset_free(actual);
	pset_free(expected);
}

static void pset_equal__length_different(void **state) {
	const struct PSet *a = pset_init_with(5, 5);
	const struct PSet *b = pset_init_with(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };

	assert_true(pset_add(a, vals[0], NULL));

	assert_true(pset_add(b, vals[0], NULL));
	assert_true(pset_add(b, vals[1], NULL));

	assert_pset_not_equal(a, b, NULL, NULL);

	pset_free(a);
	pset_free_vals(b, NULL);
}

static void pset_equal__pointers_ok(void **state) {
	const struct PSet *a = pset_init_with(5, 5);
	const struct PSet *b = pset_init_with(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };
	assert_true(pset_add(a, vals[0], NULL));
	assert_true(pset_add(a, vals[1], NULL));

	assert_true(pset_add(b, vals[0], NULL));
	assert_true(pset_add(b, vals[1], NULL));

	assert_pset_equal(a, b, NULL, NULL);

	pset_free_vals(a, NULL);
	pset_free(b);
}

static void pset_equal__pointers_different(void **state) {
	const struct PSet *a = pset_init_with(5, 5);
	const struct PSet *b = pset_init_with(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), };
	assert_true(pset_add(a, vals[0], NULL));
	assert_true(pset_add(a, vals[1], NULL));

	assert_true(pset_add(b, vals[0], NULL));
	assert_true(pset_add(b, vals[2], NULL));

	assert_pset_not_equal(a, b, NULL, NULL);

	pset_free_vals(a, NULL);
	pset_free(b);

	free(vals[2]);
}

static void pset_equal__comparison_ok(void **state) {
	const struct PSet *a = pset_init_with(5, 5);
	const struct PSet *b = pset_init_with(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), };

	assert_true(pset_add(a, vals[0], NULL));
	assert_true(pset_add(a, vals[1], NULL));

	assert_true(pset_add(b, vals[0], NULL));
	assert_true(pset_add(b, vals[1], NULL));

	assert_pset_equal(a, b, fn_equal_strcmp, NULL);

	pset_free(a);
	pset_free_vals(b, NULL);
}

static void pset_equal__comparison_different(void **state) {
	const struct PSet *a = pset_init_with(5, 5);
	const struct PSet *b = pset_init_with(5, 5);

	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), };

	assert_true(pset_add(a, vals[0], NULL));
	assert_true(pset_add(a, vals[1], NULL));

	assert_true(pset_add(b, vals[0], NULL));
	assert_true(pset_add(b, vals[2], NULL));

	assert_pset_not_equal(a, b, fn_equal_strcmp, NULL);

	pset_free_vals(a, NULL);
	pset_free(b);

	free(vals[2]);
}

static void pset_vals_slist__empty(void **state) {
	const struct PSet *set = pset_init_with(3, 5);

	assert_nul(pset_slist(set));

	pset_free_vals(set, NULL);
}

static void pset_vals_slist__many(void **state) {
	const struct PSet *tab = pset_init_with(3, 5);

	void *vals[] = { strdup("0"), strdup("1"), };

	assert_true(pset_add(tab, vals[0], NULL));
	assert_true(pset_add(tab, vals[1], NULL));

	struct SList *list = pset_slist(tab);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "0");
	assert_str_equal(slist_at(list, 1), "1");

	slist_free(&list);
	pset_free_vals(tab, NULL);
}

static void pset_str__null(void **state) {
	assert_nul(pset_str(NULL, NULL));
}

static void pset_str__empty(void **state) {
	const struct PSet *set = pset_init_with(5, 5);

	char *str = pset_str(set, NULL);
	assert_str_equal(str, "");

	free(str);
	pset_free_vals(set, NULL);
}

static void pset_str__many(void **state) {
	const struct PSet *set = pset_init_with(5, 5);

	assert_true(pset_add(set, "ONE", NULL));
	assert_true(pset_add(set, "TWO", NULL));
	assert_true(pset_add(set, "THREE", NULL));

	char *str = pset_str(set, NULL);
	assert_str_equal(str,
			"ONE\n"
			"TWO\n"
			"THREE\n"
			);

	free(str);
	pset_free(set);
}

static void pset_str__fn_str(void **state) {
	const struct PSet *set = pset_init_with(5, 5);

	assert_true(pset_add(set, "ONE", NULL));
	assert_true(pset_add(set, "TWO", NULL));
	assert_true(pset_add(set, "THREE", NULL));

	char *str = pset_str(set, fn_str_first);
	assert_str_equal(str,
			"O\n"
			"T\n"
			"T\n"
			);

	free(str);
	pset_free(set);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(pset_init__size),
		TEST(pset_init__invalid),

		TEST(pset_clone__null),
		TEST(pset_clone__empty),
		TEST(pset_clone__params),
		TEST(pset_clone__shallow_many),
		TEST(pset_clone__deep_many),

		TEST(pset_free_vals__null),
		TEST(pset_free_vals__free_val),

		TEST(pset_add__new),
		TEST(pset_add__new_fn_equal),
		TEST(pset_add__null),
		TEST(pset_add__grow),

		TEST(pset_remove__existing),
		TEST(pset_remove__inexistent),
		TEST(pset_remove__fn_equal),

		TEST(pset_iter__empty),
		TEST(pset_iter__free),
		TEST(pset_iter__many),
		TEST(pset_iter__cleared),

		TEST(pset_add__again),

		TEST(pset_sort__empty),
		TEST(pset_sort__one),
		TEST(pset_sort__many),
		TEST(pset_sort__words),

		TEST(pset_equal__length_different),
		TEST(pset_equal__pointers_ok),
		TEST(pset_equal__pointers_different),
		TEST(pset_equal__comparison_ok),
		TEST(pset_equal__comparison_different),

		TEST(pset_vals_slist__empty),
		TEST(pset_vals_slist__many),

		TEST(pset_str__null),
		TEST(pset_str__empty),
		TEST(pset_str__many),
		TEST(pset_str__fn_str),
	};

	return RUN(tests);
}


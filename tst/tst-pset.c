#include "tst.h"
#include "asserts.h"
#include "assert-pset.h"
#include "expects.h"
#include "mock-fn.h"

#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"
#include "str.h"

#include "pset.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "data/words-sorted.c"
#include "data/words-unsorted.c"
#pragma GCC diagnostic pop // "-Wunused-variable"

static int vals[6] = { 20, 21, 22, 23, 24, 25, };
static void *V0 = &V0;
static void *V1 = &V1;
static void *V2 = &V2;
static void *V3 = &vals[3];
static void *V4 = &vals[4];
static void *V5 = &vals[5];

struct PSet {
	const void **vals;
	size_t capacity;
	size_t grow;
	size_t size;
};

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

static char* fn_str_first(const void *val) {
	return strndup(val, 1);
}

static void pset_init__size(void **state) {
	const struct PSetParams params = { .initial = 2, .grow = 4 };
	const struct PSet *set = pset_init_with(params);

	assert_non_nul(set);

	assert_int_equal(set->size, 0);
	assert_int_equal(set->capacity, 2);
	assert_int_equal(set->grow, 4);

	pset_free(set);
}

static void pset_init__defaults(void **state) {
	const struct PSetParams params = { .initial = 0, .grow = 0 };
	const struct PSet *set = pset_init_with(params);

	assert_non_nul(set);

	assert_int_equal(set->size, 0);
	assert_int_equal(set->capacity, 10);
	assert_int_equal(set->grow, 10);

	pset_free(set);
}

static void pset_clone__null(void **state) {
	assert_nul(pset_clone(NULL, NULL));
}

static void pset_clone__empty(void **state) {
	const struct PSet *set = pset_init();

	const struct PSet *clone = pset_clone(set, NULL);

	assert_non_nul(clone);

	assert_int_equal(pset_size(clone), 0);

	pset_free(set);
	pset_free(clone);
}

static void pset_clone__params(void **state) {
	const struct PSetParams params = { .initial = 3, .grow = 4, };
	const struct PSet *set = pset_init_with(params);

	const struct PSet *clone = pset_clone(set, NULL);

	assert_non_nul(clone);

	assert_int_equal(set->size, 0);
	assert_int_equal(set->capacity, 3);
	assert_int_equal(set->grow, 4);

	pset_free(set);
	pset_free(clone);
}

static void pset_clone__shallow_many(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));
	assert_true(pset_add(set, V1, NULL));

	const struct PSet *clone = pset_clone(set, NULL);

	assert_int_equal(pset_size(clone), 2);

	assert_true(pset_contains(clone, V0, NULL));
	assert_true(pset_contains(clone, V1, NULL));

	assert_pset_equal(set, clone, NULL, NULL);
	assert_pset_equal(clone, set, NULL, NULL);

	pset_free(clone);
	pset_free(set);
}

static void pset_clone__deep_many(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));
	assert_true(pset_add(set, V1, NULL));

	expect_ptr(mock_clone, val, V0);
	will_return_ptr_type(mock_clone, V2, void*);
	expect_ptr(mock_clone, val, V1);
	will_return_ptr_type(mock_clone, V3, void*);

	const struct PSet *clone = pset_clone(set, mock_clone);

	assert_int_equal(pset_size(clone), 2);

	assert_true(pset_contains(clone, V2, NULL));
	assert_true(pset_contains(clone, V3, NULL));

	pset_free(clone);
	pset_free(set);
}

static void pset_free_vals__null(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));
	assert_true(pset_add(set, V1, NULL));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, V0, NULL));
	assert_true(pset_contains(set, V1, NULL));

	// not much we can do here but valgrind
	pset_free(set);
}

static void pset_free_vals__free_val(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));
	assert_true(pset_add(set, V1, NULL));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, V0, NULL));
	assert_true(pset_contains(set, V1, NULL));

	expect_str(mock_free, val, V0);
	expect_str(mock_free, val, V1);

	pset_free_vals(set, mock_free);
}

static void pset_add__new(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));
	assert_true(pset_add(set, V1, NULL));

	assert_int_equal(pset_size(set), 2);

	assert_false(pset_add(set, V1, NULL));

	assert_true(pset_contains(set, V0, NULL));
	assert_true(pset_contains(set, V1, NULL));

	pset_free(set);
}

static void pset_add__new_fn_equal(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, fn_equal_strcmp));
	assert_true(pset_add(set, V1, fn_equal_strcmp));

	assert_int_equal(pset_size(set), 2);

	assert_false(pset_add(set, V1, fn_equal_strcmp));

	assert_true(pset_contains(set, V0, fn_equal_strcmp));
	assert_true(pset_contains(set, V1, fn_equal_strcmp));

	pset_free(set);
}

static void pset_add__null(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));

	assert_int_equal(pset_size(set), 1);

	assert_false(pset_contains(set, NULL, NULL));
	assert_false(pset_add(set, NULL, NULL));
	assert_false(pset_contains(set, NULL, NULL));

	assert_int_equal(pset_size(set), 1);

	pset_free(set);
}

static void pset_add__grow(void **state) {
	const struct PSetParams params = { .initial = 2, .grow = 5 };
	const struct PSet *set = pset_init_with(params);

	void *initial[] = { V0, V1, };
	assert_true(pset_add(set, initial[0], NULL));
	assert_true(pset_add(set, initial[1], NULL));

	assert_int_equal(set->size, 2);
	assert_int_equal(set->capacity, 2);
	assert_int_equal(set->grow, 5);

	assert_true(pset_contains(set, initial[0], NULL));
	assert_true(pset_contains(set, initial[1], NULL));

	void *grow[] = { V2, V3, };
	assert_true(pset_add(set, grow[0], NULL));
	assert_int_equal(set->size, 3);
	assert_int_equal(set->capacity, 7);
	assert_true(pset_contains(set, grow[0], NULL));

	assert_true(pset_add(set, grow[1], NULL));
	assert_int_equal(set->size, 4);
	assert_int_equal(set->capacity, 7);
	assert_true(pset_contains(set, grow[1], NULL));

	void *subsequent[] = { V4, V5, };
	assert_true(pset_add(set, subsequent[0], NULL));
	assert_true(pset_add(set, subsequent[1], NULL));
	assert_int_equal(set->size, 6);
	assert_int_equal(set->capacity, 7);

	assert_true(pset_contains(set, subsequent[0], NULL));
	assert_true(pset_contains(set, subsequent[1], NULL));

	pset_free(set);
}

static void pset_remove__existing(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));
	assert_true(pset_add(set, V1, NULL));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, V0, NULL));
	assert_true(pset_contains(set, V1, NULL));

	// 0
	assert_non_nul(pset_remove(set, V0, NULL));

	assert_int_equal(pset_size(set), 1);
	assert_false(pset_contains(set, V0, NULL));
	assert_true(pset_contains(set, V1, NULL));

	// 1
	assert_non_nul(pset_remove(set, V1, NULL));

	assert_int_equal(pset_size(set), 0);
	assert_false(pset_contains(set, V0, NULL));
	assert_false(pset_contains(set, V1, NULL));

	pset_free(set);
}

static void pset_remove__inexistent(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));
	assert_true(pset_add(set, V1, NULL));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, V0, NULL));
	assert_true(pset_contains(set, V1, NULL));

	assert_null(pset_remove(set, V2, NULL));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, V0, NULL));
	assert_true(pset_contains(set, V1, NULL));

	pset_free(set);
}

static void pset_remove__fn_equal(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));
	assert_true(pset_add(set, V1, NULL));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, V0, NULL));
	assert_true(pset_contains(set, V1, NULL));

	// 1
	expect_ptr(mock_equal, a, V0);
	expect_ptr(mock_equal, b, V1);
	will_return(mock_equal, false);
	expect_ptr(mock_equal, a, V1);
	expect_ptr(mock_equal, b, V1);
	will_return(mock_equal, true);

	assert_ptr_equal(pset_remove(set, V1, mock_equal), V1);

	// 0
	expect_ptr(mock_equal, a, V0);
	expect_ptr(mock_equal, b, V0);
	will_return(mock_equal, true);

	assert_ptr_equal(pset_remove(set, V0, mock_equal), V0);

	assert_int_equal(pset_size(set), 0);

	pset_free(set);
}

static void pset_iter__empty(void **state) {
	const struct PSet *set = pset_init();

	assert_int_equal(pset_size(set), 0);

	assert_nul(pset_iter(set));

	pset_free(set);
}

static void pset_iter__free(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));
	assert_true(pset_add(set, V1, NULL));

	const struct PSetIter *iter = pset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), V0);

	// not much we can do here but valgrind
	pset_iter_free(iter);

	pset_free(set);
}


static void pset_iter__many(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));
	assert_true(pset_add(set, V1, NULL));

	assert_int_equal(pset_size(set), 2);

	const struct PSetIter *iter = pset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), V0);

	iter = pset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), V1);

	iter = pset_iter_next(iter);
	assert_nul(iter);

	pset_free(set);
}

static void pset_iter__cleared(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));
	assert_true(pset_add(set, V1, NULL));

	assert_int_equal(pset_size(set), 2);

	assert_non_nul(pset_remove(set, V0, NULL));
	assert_non_nul(pset_remove(set, V1, NULL));

	assert_int_equal(pset_size(set), 0);

	assert_nul(pset_iter(set));

	pset_free(set);
}

static void pset_add__again(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));
	assert_true(pset_add(set, V1, NULL));
	assert_true(pset_add(set, V2, NULL));
	assert_true(pset_add(set, V3, NULL));

	assert_int_equal(pset_size(set), 4);

	// remove 1
	assert_non_nul(pset_remove(set, V1, NULL));
	assert_int_equal(pset_size(set), 3);

	// put 1 again afterwards
	assert_true(pset_add(set, V1, NULL));
	assert_int_equal(pset_size(set), 4);

	// 0
	const struct PSetIter *iter = pset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), V0);

	// 2
	iter = pset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), V2);

	// 3
	iter = pset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), V3);

	// 0 moved later
	iter = pset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(pset_iter_val(iter), V1);

	// end
	iter = pset_iter_next(iter);
	assert_nul(iter);

	pset_free(set);
}

static void pset_sort__empty(void **state) {
	const struct PSet *actual = pset_init();

	const struct PSet *expected = pset_init();

	pset_sort(actual, mock_test);

	assert_int_equal(pset_size(actual), 0);

	assert_pset_equal(actual, expected, mock_test, NULL);

	pset_free(actual);
	pset_free(expected);
}

static void pset_sort__one(void **state) {
	const struct PSet *actual = pset_init();

	assert_true(pset_add(actual, V0, NULL));

	const struct PSet *expected = pset_init();
	assert_true(pset_add(expected, V0, NULL));

	pset_sort(actual, mock_less_than);

	assert_pset_equal(actual, expected, NULL, NULL);

	pset_free(actual);
	pset_free(expected);
}

static void pset_sort__words(void **state) {
	const struct PSetParams params = { .initial = 400, .grow = 400, };
	const struct PSet *actual = pset_init_with(params);

	for (size_t i = sizeof(words_unsorted) / sizeof(words_unsorted[0]); i > 0; i--) {
		assert_true(pset_add(actual, words_unsorted[i - 1], NULL));
	}

	const struct PSet *expected = pset_init_with(params);

	for (size_t i = 0; i < sizeof(words_sorted) / sizeof(words_sorted[0]); i++ ) {
		assert_true(pset_add(expected, words_sorted[i], NULL));
	}

	pset_sort(actual, fn_less_than_strcmp);

	assert_pset_equal(actual, expected, fn_equal_strcmp, NULL);

	pset_free(actual);
	pset_free(expected);
}

static void pset_equal__length_different(void **state) {
	const struct PSet *a = pset_init();
	const struct PSet *b = pset_init();

	assert_true(pset_add(a, V0, NULL));

	assert_true(pset_add(b, V0, NULL));
	assert_true(pset_add(b, V1, NULL));

	assert_pset_not_equal(a, b, NULL, NULL);

	pset_free(a);
	pset_free(b);
}

static void pset_equal__pointers_ok(void **state) {
	const struct PSet *a = pset_init();
	const struct PSet *b = pset_init();

	assert_true(pset_add(a, V0, NULL));
	assert_true(pset_add(a, V1, NULL));

	assert_true(pset_add(b, V0, NULL));
	assert_true(pset_add(b, V1, NULL));

	assert_pset_equal(a, b, NULL, NULL);

	pset_free(a);
	pset_free(b);
}

static void pset_equal__pointers_different(void **state) {
	const struct PSet *a = pset_init();
	const struct PSet *b = pset_init();

	assert_true(pset_add(a, V0, NULL));
	assert_true(pset_add(a, V1, NULL));

	assert_true(pset_add(b, V0, NULL));
	assert_true(pset_add(b, V2, NULL));

	assert_pset_not_equal(a, b, NULL, NULL);

	pset_free(a);
	pset_free(b);
}

static void pset_equal__comparison_ok(void **state) {
	const struct PSet *a = pset_init();
	const struct PSet *b = pset_init();

	assert_true(pset_add(a, V0, NULL));
	assert_true(pset_add(a, V1, NULL));

	assert_true(pset_add(b, V0, NULL));
	assert_true(pset_add(b, V1, NULL));

	assert_pset_equal(a, b, fn_equal_strcmp, NULL);

	pset_free(a);
	pset_free(b);
}

static void pset_equal__comparison_different(void **state) {
	const struct PSet *a = pset_init();
	const struct PSet *b = pset_init();

	assert_true(pset_add(a, "0", NULL));
	assert_true(pset_add(a, "1", NULL));

	assert_true(pset_add(b, "0", NULL));
	assert_true(pset_add(b, "2", NULL));

	assert_pset_not_equal(a, b, fn_equal_strcmp, NULL);

	pset_free(a);
	pset_free(b);
}

static void pset_vals_slist__empty(void **state) {
	const struct PSet *set = pset_init();

	assert_nul(pset_slist(set));

	pset_free(set);
}

static void pset_vals_slist__many(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));
	assert_true(pset_add(set, V1, NULL));

	struct SList *list = pset_slist(set);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), V0);
	assert_str_equal(slist_at(list, 1), V1);

	slist_free(&list);
	pset_free(set);
}

static void pset_str__null(void **state) {
	assert_nul(pset_str(NULL, NULL));
}

static void pset_str__empty(void **state) {
	const struct PSet *set = pset_init();

	char *str = pset_str(set, NULL);
	assert_str_equal(str, "");

	free(str);
	pset_free(set);
}

static void pset_str__many(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0, NULL));
	assert_true(pset_add(set, V1, NULL));
	assert_true(pset_add(set, V2, NULL));

	char *expected = sprintf_alloc(
			"%p\n"
			"%p\n"
			"%p\n",
			V0,
			V1,
			V2
			);

	char *actual = pset_str(set, NULL);
	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	pset_free(set);
}

static void pset_str__fn_str(void **state) {
	const struct PSet *set = pset_init();

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
		TEST(pset_init__defaults),

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


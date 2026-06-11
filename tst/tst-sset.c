#include "tst.h"
#include "asserts.h"
#include "assert-sset.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "slist.h"
#include "fn.h"

#include "sset.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "data/words-sorted.c"
#include "data/words-unsorted.c"
#pragma GCC diagnostic pop // "-Wunused-variable"

struct PSet {
	const void **vals;
	size_t capacity;
	size_t grow;
	size_t size;
	fn_equal equal_val;
	fn_less_than less_than_val;
	fn_free free_val;
	fn_str str_val;
	fn_clone clone_val;
};

struct SSet {
	const struct PSet *pset;
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

static void sset_init__size(void **state) {
	const struct SSetParams params = { .initial = 2, .grow = 4 };
	const struct SSet *set = sset_init_with(params);

	assert_non_nul(set);

	assert_int_equal(set->pset->size, 0);
	assert_int_equal(set->pset->capacity, 2);
	assert_int_equal(set->pset->grow, 4);

	sset_free(set);
}

static void sset_init__defaults(void **state) {
	const struct SSetParams params = { .initial = 0, .grow = 0 };
	const struct SSet *set = sset_init_with(params);

	assert_non_nul(set);

	assert_int_equal(set->pset->size, 0);
	assert_int_equal(set->pset->capacity, 10);
	assert_int_equal(set->pset->grow, 10);

	sset_free(set);
}

static void sset_clone__null(void **state) {
	assert_nul(sset_clone(NULL));
}

static void sset_clone__empty(void **state) {
	const struct SSet *set = sset_init();

	const struct SSet *clone = sset_clone(set);

	assert_non_nul(clone);

	assert_int_equal(sset_size(clone), 0);

	sset_free(set);
	sset_free(clone);
}

static void sset_clone__params(void **state) {
	const struct SSetParams params = { .initial = 3, .grow = 4, .case_insensitive = true, };
	const struct SSet *set = sset_init_with(params);

	const struct SSet *clone = sset_clone(set);

	assert_non_nul(clone);

	assert_int_equal(set->pset->size, 0);
	assert_int_equal(set->pset->capacity, 3);
	assert_int_equal(set->pset->grow, 4);

	assert_true(sset_add(set, "A"));
	assert_false(sset_add(set, "a"));

	sset_free(set);
	sset_free(clone);
}

static void sset_clone__many(void **state) {
	const struct SSet *set = sset_init();

	assert_true(sset_add(set, "ONE"));
	assert_true(sset_add(set, "TWO"));

	const struct SSet *clone = sset_clone(set);

	assert_int_equal(sset_size(clone), 2);

	assert_true(sset_contains(clone, "ONE"));
	assert_true(sset_contains(clone, "TWO"));

	assert_sset_equal(set, clone);
	assert_sset_equal(clone, set);

	sset_free(clone);
	sset_free(set);
}

static void sset_free__ok(void **state) {
	const struct SSet *set = sset_init();

	char *vals[] = { "0", "1", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	sset_free(set);
}

static void sset_add__new(void **state) {
	const struct SSet *set = sset_init();

	void *vals[] = { "0", "1", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	sset_free(set);
}

static void sset_add__existing(void **state) {
	const struct SSet *set = sset_init();

	void *vals[] = { "0", "1", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	assert_false(sset_add(set, vals[0]));
	assert_false(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);

	sset_free(set);
}

static void sset_add__null(void **state) {
	const struct SSet *set = sset_init();

	void *vals[] = { "0", };
	assert_true(sset_add(set, vals[0]));

	assert_int_equal(sset_size(set), 1);

	assert_false(sset_contains(set, NULL));
	assert_false(sset_add(set, NULL));
	assert_false(sset_contains(set, NULL));

	assert_int_equal(sset_size(set), 1);

	sset_free(set);
}

static void sset_add__case_insensitive(void **state) {
	const struct SSetParams params = { .case_insensitive = true, };
	const struct SSet *set = sset_init_with(params);

	void *vals[] = { "a", "A", };
	assert_true(sset_add(set, vals[0]));
	assert_false(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 1);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	sset_free(set);
}

static void sset_add__grow(void **state) {
	const struct SSetParams params = { .initial = 2, .grow = 5 };
	const struct SSet *set = sset_init_with(params);

	void *initial[] = { "0", "1", };
	assert_true(sset_add(set, initial[0]));
	assert_true(sset_add(set, initial[1]));

	assert_int_equal(set->pset->size, 2);
	assert_int_equal(set->pset->capacity, 2);
	assert_int_equal(set->pset->grow, 5);

	assert_true(sset_contains(set, initial[0]));
	assert_true(sset_contains(set, initial[1]));

	void *grow[] = { "2", "3", };
	assert_true(sset_add(set, grow[0]));
	assert_int_equal(set->pset->size, 3);
	assert_int_equal(set->pset->capacity, 7);
	assert_true(sset_contains(set, grow[0]));

	assert_true(sset_add(set, grow[1]));
	assert_int_equal(set->pset->size, 4);
	assert_int_equal(set->pset->capacity, 7);
	assert_true(sset_contains(set, grow[1]));

	void *subsequent[] = { "4", "5", };
	assert_true(sset_add(set, subsequent[0]));
	assert_true(sset_add(set, subsequent[1]));
	assert_int_equal(set->pset->size, 6);
	assert_int_equal(set->pset->capacity, 7);

	assert_true(sset_contains(set, subsequent[0]));
	assert_true(sset_contains(set, subsequent[1]));

	sset_free(set);
}

static void sset_remove__existing(void **state) {
	const struct SSet *set = sset_init();


	void *vals[] = { "0", "2", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	// 0
	assert_true(sset_remove(set, vals[0]));

	assert_int_equal(sset_size(set), 1);
	assert_false(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	// 1
	assert_true(sset_remove(set, vals[1]));

	assert_int_equal(sset_size(set), 0);
	assert_false(sset_contains(set, vals[0]));
	assert_false(sset_contains(set, vals[1]));

	sset_free(set);
}

static void sset_remove__inexistent(void **state) {
	const struct SSet *set = sset_init();

	void *vals[] = { "0", "1", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	const void *inexistent = "inexistent";
	assert_false(sset_remove(set, inexistent));

	assert_int_equal(sset_size(set), 2);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	sset_free(set);
}

static void sset_remove__case_insensitive(void **state) {
	const struct SSetParams params = { .case_insensitive = true, };
	const struct SSet *set = sset_init_with(params);

	void *vals[] = { "a", "A", "b", };
	assert_true(sset_add(set, vals[0]));
	assert_false(sset_add(set, vals[1]));
	assert_true(sset_add(set, vals[2]));

	assert_int_equal(sset_size(set), 2);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));
	assert_true(sset_contains(set, vals[2]));

	assert_true(sset_remove(set, vals[0]));
	assert_false(sset_remove(set, vals[1]));

	assert_int_equal(sset_size(set), 1);
	assert_true(sset_contains(set, vals[2]));

	sset_free(set);
}

static void sset_iter__empty(void **state) {
	const struct SSet *set = sset_init();

	assert_int_equal(sset_size(set), 0);

	assert_nul(sset_filter_iter(set, NULL, NULL));

	sset_free(set);
}

static void sset_iter__free(void **state) {
	const struct SSet *set = sset_init();

	void *vals[] = { "0", "1", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	const struct SSetIter *iter = sset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "0");

	sset_iter_free(iter);

	sset_free(set);
}


static void sset_iter__many(void **state) {
	const struct SSet *set = sset_init();

	void *vals[] = { "0", "1", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);

	const struct SSetIter *iter = sset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "0");

	iter = sset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "1");

	iter = sset_iter_next(iter);
	assert_nul(iter);

	sset_free(set);
}

static void sset_iter__cleared(void **state) {
	const struct SSet *set = sset_init();

	void *vals[] = { "0", "1", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);

	sset_remove(set, vals[0]);
	sset_remove(set, vals[1]);

	assert_int_equal(sset_size(set), 0);

	assert_nul(sset_iter(set));

	sset_free(set);
}

static void sset_add__again(void **state) {
	const struct SSet *set = sset_init();

	void *vals[] = { "0", "1", "2", "3", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));
	assert_true(sset_add(set, vals[2]));
	assert_true(sset_add(set, vals[3]));

	assert_int_equal(sset_size(set), 4);

	// remove 1
	assert_true(sset_remove(set, vals[1]));
	assert_int_equal(sset_size(set), 3);

	// put 1 again afterwards
	assert_true(sset_add(set, vals[1]));
	assert_int_equal(sset_size(set), 4);

	// 0
	const struct SSetIter *iter = sset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "0");

	// 2
	iter = sset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "2");

	// 3
	iter = sset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "3");

	// 0 moved later
	iter = sset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "1");

	// end
	iter = sset_iter_next(iter);
	assert_nul(iter);

	sset_free(set);
}

static void sset_sort__empty(void **state) {
	const struct SSet *actual = sset_init();

	const struct SSet *expected = sset_init();

	sset_sort(actual);

	assert_int_equal(sset_size(actual), 0);

	assert_sset_equal(actual, expected);

	sset_free(actual);
	sset_free(expected);
}

static void sset_sort__one(void **state) {
	const struct SSet *actual = sset_init();

	assert_true(sset_add(actual, "A"));

	const struct SSet *expected = sset_init();
	assert_true(sset_add(expected, "A"));

	sset_sort(actual);

	assert_sset_equal(actual, expected);

	sset_free(actual);
	sset_free(expected);
}

static void sset_sort__many(void **state) {
	const struct SSet *actual = sset_init();

	assert_true(sset_add(actual, "3"));
	assert_true(sset_add(actual, "1"));
	assert_true(sset_add(actual, "0"));
	assert_true(sset_add(actual, "2"));

	const struct SSet *expected = sset_init();
	assert_true(sset_add(expected, "0"));
	assert_true(sset_add(expected, "1"));
	assert_true(sset_add(expected, "2"));
	assert_true(sset_add(expected, "3"));

	sset_sort(actual);

	assert_sset_equal(actual, expected);

	sset_free(actual);
	sset_free(expected);
}

static void sset_sort__words(void **state) {
	const struct SSetParams params = { .initial = 400, .grow = 400, };

	const struct SSet *actual = sset_init_with(params);

	for (size_t i = sizeof(words_unsorted) / sizeof(words_unsorted[0]); i > 0; i--) {
		assert_true(sset_add(actual, words_unsorted[i - 1]));
	}

	const struct SSet *expected = sset_init_with(params);

	for (size_t i = 0; i < sizeof(words_sorted) / sizeof(words_sorted[0]); i++ ) {
		assert_true(sset_add(expected, words_sorted[i]));
	}

	sset_sort(actual);

	assert_sset_equal(actual, expected);

	sset_free(actual);
	sset_free(expected);
}

static void sset_sort__many_case_insensitive(void **state) {
	const struct SSetParams params = { .case_insensitive = true, };
	const struct SSet *actual = sset_init_with(params);

	assert_true(sset_add(actual, "Bb3"));
	assert_true(sset_add(actual, "aa1"));
	assert_true(sset_add(actual, "Aa0"));
	assert_true(sset_add(actual, "bb2"));

	const struct SSet *expected = sset_init();
	assert_true(sset_add(expected, "Aa0"));
	assert_true(sset_add(expected, "aa1"));
	assert_true(sset_add(expected, "Bb2"));
	assert_true(sset_add(expected, "bb3"));

	sset_sort(actual);

	assert_sset_equal(actual, expected);

	sset_free(actual);
	sset_free(expected);
}

static void sset_equal__length_different(void **state) {
	const struct SSet *a = sset_init();
	const struct SSet *b = sset_init();

	void *vals[] = { "0", "1", };

	assert_true(sset_add(a, vals[0]));

	assert_true(sset_add(b, vals[0]));
	assert_true(sset_add(b, vals[1]));

	assert_sset_not_equal(a, b);

	sset_free(a);
	sset_free(b);
}

static void sset_equal__comparison_ok(void **state) {
	const struct SSet *a = sset_init();
	const struct SSet *b = sset_init();

	void *vals[] = { "0", "1", };
	assert_true(sset_add(a, vals[0]));
	assert_true(sset_add(a, vals[1]));

	assert_true(sset_add(b, vals[0]));
	assert_true(sset_add(b, vals[1]));

	assert_sset_equal(a, b);

	sset_free(a);
	sset_free(b);
}

static void sset_equal__comparison_different(void **state) {
	const struct SSet *a = sset_init();
	const struct SSet *b = sset_init();

	void *vals[] = { "0", "1", "2", };

	assert_true(sset_add(a, vals[0]));
	assert_true(sset_add(a, vals[1]));

	assert_true(sset_add(b, vals[0]));
	assert_true(sset_add(b, vals[2]));

	assert_sset_not_equal(a, b);

	sset_free(a);
	sset_free(b);
}

static void sset_equal__case_insensitive(void **state) {
	const struct SSetParams params = { .case_insensitive = true, };

	const struct SSet *a = sset_init_with(params);
	const struct SSet *b = sset_init();

	assert_true(sset_add(a, "a"));
	assert_true(sset_add(a, "B"));

	assert_true(sset_add(b, "A"));
	assert_true(sset_add(b, "b"));

	assert_sset_equal(a, b);
	assert_sset_not_equal(b, a);

	sset_free(a);
	sset_free(b);
}

static void sset_slist__empty(void **state) {
	const struct SSet *set = sset_init();

	assert_nul(sset_slist(set));

	sset_free(set);
}

static void sset_slist__many(void **state) {
	const struct SSet *set = sset_init();

	void *vals[] = { "0", "1", };

	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	struct SList *list = sset_slist(set);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "0");
	assert_str_equal(slist_at(list, 1), "1");

	slist_free(&list);
	sset_free(set);
}

static void sset_str__null(void **state) {
	assert_nul(sset_str(NULL));
}

static void sset_str__empty(void **state) {
	const struct SSet *set = sset_init();

	char *str = sset_str(set);
	assert_str_equal(str, "");

	free(str);
	sset_free(set);
}

static void sset_str__many(void **state) {
	const struct SSet *set = sset_init();

	assert_true(sset_add(set, "ONE"));
	assert_true(sset_add(set, "TWO"));
	assert_true(sset_add(set, "THREE"));

	char *str = sset_str(set);
	assert_str_equal(str,
			"ONE\n"
			"TWO\n"
			"THREE\n"
			);

	free(str);
	sset_free(set);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(sset_init__size),
		TEST(sset_init__defaults),

		TEST(sset_clone__null),
		TEST(sset_clone__empty),
		TEST(sset_clone__params),
		TEST(sset_clone__many),

		TEST(sset_free__ok),

		TEST(sset_add__new),
		TEST(sset_add__existing),
		TEST(sset_add__null),
		TEST(sset_add__grow),
		TEST(sset_add__case_insensitive),

		TEST(sset_remove__existing),
		TEST(sset_remove__inexistent),
		TEST(sset_remove__case_insensitive),

		TEST(sset_iter__empty),
		TEST(sset_iter__free),
		TEST(sset_iter__many),
		TEST(sset_iter__cleared),

		TEST(sset_add__again),

		TEST(sset_sort__empty),
		TEST(sset_sort__one),
		TEST(sset_sort__many),
		TEST(sset_sort__words),
		TEST(sset_sort__many_case_insensitive),

		TEST(sset_equal__length_different),
		TEST(sset_equal__comparison_ok),
		TEST(sset_equal__comparison_different),
		TEST(sset_equal__case_insensitive),

		TEST(sset_slist__empty),
		TEST(sset_slist__many),

		TEST(sset_str__null),
		TEST(sset_str__empty),
		TEST(sset_str__many),
	};

	return RUN(tests);
}


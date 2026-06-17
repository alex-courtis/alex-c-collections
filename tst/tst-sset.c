#include "tst.h"
#include "asserts.h"
#include "assert-sset.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"
#include "pset.h"

#include "sset.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "data/words-sorted.c"
#include "data/words-unsorted.c"
#pragma GCC diagnostic pop // "-Wunused-variable"

struct PSet {
	const struct PSetParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct SSet {
	const struct SSetParams params;
	const struct PSet *pset;
};

struct SSetIterState {
	const struct PSetIter *pit;
};

static void sset_add__alloc_val_free_val(void **state) {
	const struct SSet *set = sset_init();

	char *added = strdup("a");
	char *rejected = strdup("a");

	assert_true(sset_add(set, added));
	assert_false(sset_add(set, rejected));

	assert_true(sset_contains(set, "a"));

	free(added);
	free(rejected);

	assert_true(sset_contains(set, "a"));

	sset_free_vals(set);
}

static void sset_add_contains_remove__case_sensitive(void **state) {
	const struct SSetParams params = { .case_insensitive = false, };
	const struct SSet *set = sset_init_with(params);

	assert_true(sset_add(set, "A"));
	assert_true(sset_add(set, "B"));

	assert_false(sset_add(set, "B"));

	assert_false(sset_contains(set, "b"));
	assert_true(sset_contains(set, "B"));

	assert_false(sset_remove(set, "b"));
	assert_true(sset_remove(set, "B"));

	sset_free_vals(set);
}

static void sset_add_contains_remove__case_insensitive(void **state) {
	const struct SSetParams params = { .case_insensitive = true, };
	const struct SSet *set = sset_init_with(params);

	assert_true(sset_add(set, "A"));
	assert_true(sset_add(set, "B"));

	assert_false(sset_add(set, "B"));

	assert_true(sset_contains(set, "b"));
	assert_true(sset_contains(set, "B"));

	assert_true(sset_remove(set, "b"));

	sset_free_vals(set);
}

static void sset_iter__(void **state) {

	const struct SSet *set = sset_init();
	assert_true(sset_add(set, "a"));
	assert_true(sset_add(set, "b"));

	const struct SSetIter *iter = sset_iter(set);

	assert_non_nul(iter);
	assert_str_equal(iter->val, "a");

	iter = sset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "b");

	sset_iter_free(iter);

	sset_free_vals(set);
}

static void sset_iter__state_deleted(void **state) {
	const struct SSet *set = sset_init();

	assert_true(sset_add(set, "a"));

	const struct SSetIter *iter = sset_iter(set);
	assert_non_nul(iter);

	const struct SSetIterState *st = iter->st;
	((struct SSetIter*)iter)->st = NULL;

	iter = sset_iter_next(iter);
	assert_nul(iter);

	pset_iter_free(st->pit);
	free((void*)st);
	sset_free_vals(set);
}

static void sset_iter__state_set_deleted(void **state) {
	const struct SSet *set = sset_init();

	assert_true(sset_add(set, "a"));

	const struct SSetIter *iter = sset_iter(set);
	assert_non_nul(iter);

	const struct PSetIter *piter = iter->st->pit;
	iter->st->pit = NULL;

	iter = sset_iter_next(iter);
	assert_nul(iter);

	pset_iter_free(piter);
	sset_free_vals(set);
}

static void sset_iter__empty(void **state) {

	const struct SSet *set = sset_init();

	const struct SSetIter *iter = sset_iter(set);

	assert_nul(iter);

	sset_free_vals(set);
}

static bool fn_equal_starts_with_a(const void* const a, const void* const b) {
	return *(char*)a == 'a';
}

static void sset_filter_iter__(void **state) {
	const struct SSet *set = sset_init();

	assert_true(sset_add(set, "a1"));
	assert_true(sset_add(set, "b1"));
	assert_true(sset_add(set, "a2"));
	assert_true(sset_add(set, "b2"));

	const struct SSetIter *iter = sset_filter_iter(set, fn_equal_starts_with_a, NULL);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "a1");

	iter = sset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "a2");

	assert_nul(sset_iter_next(iter));

	sset_free_vals(set);
}

static void sset_equal__case_sensitive(void **state) {

	const struct SSet *actual = sset_init();
	assert_true(sset_add(actual, "a"));
	assert_true(sset_add(actual, "b"));

	assert_sset_not_equal(actual, NULL);

	const struct SSet *expected = sset_init();
	assert_true(sset_add(expected, "a"));
	assert_true(sset_add(expected, "b"));

	assert_sset_equal(actual, expected);

	assert_true(sset_add(actual, "c"));
	assert_true(sset_add(expected, "C"));

	assert_sset_not_equal(actual, expected);

	sset_free_vals(actual);
	sset_free_vals(expected);
}

static void sset_equal__case_insensitive(void **state) {
	const struct SSetParams params = { .case_insensitive = true, };
	const struct SSet *actual = sset_init_with(params);

	assert_true(sset_add(actual, "a"));
	assert_true(sset_add(actual, "b"));

	const struct SSet *expected = sset_init();
	assert_true(sset_add(expected, "A"));
	assert_true(sset_add(expected, "B"));

	assert_sset_equal(actual, expected);

	sset_free_vals(actual);
	sset_free_vals(expected);
}

static void sset_str__(void **state) {

	const struct SSet *set = sset_init();
	assert_true(sset_add(set, "a"));
	assert_true(sset_add(set, "b"));
	assert_true(sset_add(set, "c"));

	char *actual = sset_str(set);

	assert_str_equal(actual, "a\nb\nc\n");

	free(actual);
	sset_free_vals(set);
}

static void sset_sort__empty(void **state) {
	const struct SSet *actual = sset_init();

	const struct SSet *expected = sset_init();

	sset_sort(actual);

	assert_int_equal(sset_size(actual), 0);

	assert_sset_equal(actual, expected);

	sset_free_vals(actual);
	sset_free_vals(expected);
}

static void sset_sort__one(void **state) {
	const struct SSet *actual = sset_init();

	assert_true(sset_add(actual, "A"));

	const struct SSet *expected = sset_init();
	assert_true(sset_add(expected, "A"));

	sset_sort(actual);

	assert_sset_equal(actual, expected);

	sset_free_vals(actual);
	sset_free_vals(expected);
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

	sset_free_vals(actual);
	sset_free_vals(expected);
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

	sset_free_vals(actual);
	sset_free_vals(expected);
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

	sset_free_vals(actual);
	sset_free_vals(expected);
}

static void sset_slist__(void **state) {
	const struct SSet *set = sset_init();

	sset_add(set, "a");
	sset_add(set, "b");

	struct SList *list = sset_slist(set);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "a");
	assert_str_equal(slist_at(list, 1), "b");

	slist_free_vals(&list, NULL);
	sset_free_vals(set);
}

// also tests constructor
static void sset_clone__(void **state) {
	const struct SSetParams params = {
		.case_insensitive = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SSet *from = sset_init_with(params);

	assert_true(sset_add(from, "a"));
	assert_true(sset_add(from, "b"));

	const struct SSet *to = sset_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->pset->size, 2);
	assert_int_equal(to->pset->capacity, 99);
	assert_int_equal(to->pset->params.grow, 1);
	assert_ptr_equal(to->pset->params.equal_val, fn_equal_strcasecmp);

	assert_ptr_equal(to->params.case_insensitive, true);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	assert_true(sset_contains(to, "a"));
	assert_true(sset_contains(to, "b"));
	assert_int_equal(sset_size(to), 2);

	sset_free_vals(from);
	sset_free_vals(to);
}

static void sset__null_inputs(void **state) {
	assert_nul(sset_clone(NULL));
	sset_free_vals(NULL);
	sset_iter_free(NULL);
	assert_false(sset_contains(NULL, NULL));
	assert_nul(sset_iter(NULL));
	assert_nul(sset_filter_iter(NULL, NULL, NULL));
	assert_nul(sset_iter_next(NULL));
	assert_false(sset_add(NULL, NULL));
	assert_false(sset_remove(NULL, NULL));
	assert_false(sset_equal(NULL, NULL));
	assert_nul(sset_slist(NULL));
	assert_nul(sset_str(NULL));
	sset_sort(NULL);
	assert_int_equal(sset_size(NULL), 0);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(sset_add__alloc_val_free_val),

		TEST(sset_add_contains_remove__case_insensitive),
		TEST(sset_add_contains_remove__case_sensitive),

		TEST(sset_iter__),
		TEST(sset_iter__empty),
		TEST(sset_iter__state_deleted),
		TEST(sset_iter__state_set_deleted),

		TEST(sset_filter_iter__),

		TEST(sset_equal__case_sensitive),
		TEST(sset_equal__case_insensitive),

		TEST(sset_str__),

		TEST(sset_sort__empty),
		TEST(sset_sort__one),
		TEST(sset_sort__many),
		TEST(sset_sort__words),
		TEST(sset_sort__many_case_insensitive),

		TEST(sset_slist__),

		TEST(sset_clone__),

		TEST(sset__null_inputs),
	};

	return RUN(tests);
}


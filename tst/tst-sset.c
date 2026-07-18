#include "assert-sset.h"
#include "asserts.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pslist.h"
#include "pset.h"

#include "sset.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "data/words-sorted.c"
#include "data/words-unsorted.c"
#pragma GCC diagnostic pop // "-Wunused-variable"

struct Pset {
	const struct PsetParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct Sset {
	const struct SsetParams params;
	const struct Pset *pset;
};

static bool match_starts_with_a(const char* const a, const void* const b) {
	return *a == 'a';
}

static void sset_add__clone_val_free_val(void **state) {
	const struct Sset *set = sset_init();

	char *added = strdup("a");
	char *rejected = strdup("a");

	assert_true(sset_add(set, added));
	assert_false(sset_add(set, rejected));

	assert_true(sset_contains(set, "a"));

	free(added);
	free(rejected);

	assert_true(sset_contains(set, "a"));

	sset_free(set);
}

static void sset_add_contains_remove_free__case_sensitive(void **state) {
	const struct SsetParams params = { .case_insensitive = false, };
	const struct Sset *set = sset_init_with(params);

	assert_true(sset_add(set, "A"));
	assert_true(sset_add(set, "B"));

	assert_false(sset_add(set, "B"));

	assert_false(sset_contains(set, "b"));
	assert_true(sset_contains(set, "B"));

	assert_false(sset_remove(set, "b"));
	assert_true(sset_remove(set, "B"));

	sset_free(set);
}

static void sset_add_all__many(void **state) {
	const struct Sset *to = sset_init();
	assert_true(sset_add(to, "A"));
	assert_true(sset_add(to, "B"));

	const struct Sset *from = sset_init();
	assert_true(sset_add(from, "A"));
	assert_true(sset_add(from, "C"));

	const struct Sset *expected = sset_init();
	assert_true(sset_add(expected, "A"));
	assert_true(sset_add(expected, "B"));
	assert_true(sset_add(expected, "C"));

	assert_int_equal(sset_add_all(to, from), 1);

	assert_sset_equal(to, expected);

	sset_free(to);
	sset_free(from);
	sset_free(expected);
}

static void sset_add_many__many(void **state) {
	const struct Sset *to = sset_init();
	assert_true(sset_add(to, "a"));
	assert_true(sset_add(to, "b"));

	const struct Sset *expected = sset_init();
	assert_true(sset_add(expected, "a"));
	assert_true(sset_add(expected, "b"));
	assert_true(sset_add(expected, "c"));

	assert_int_equal(sset_add_many(to, "a", "b", "c", NULL), 1);

	assert_sset_equal(to, expected);

	sset_free(to);
	sset_free(expected);
}

static void sset_at__(void **state) {
	const struct Sset *set = sset_init();
	assert_true(sset_add(set, "a"));
	assert_true(sset_add(set, "b"));
	assert_true(sset_add(set, "c"));

	assert_str_equal(sset_at(set, 1), "b");

	sset_free(set);
}

static void sset_remove_all__many(void **state) {
	const struct Sset *set = sset_init();
	assert_true(sset_add(set, "a"));
	assert_true(sset_add(set, "b"));
	assert_true(sset_add(set, "c"));

	const struct Sset *from = sset_init();
	assert_true(sset_add(from, "a"));
	assert_true(sset_add(from, "c"));
	assert_true(sset_add(from, "d"));

	const struct Sset *expected = sset_init();
	assert_true(sset_add(expected, "b"));

	assert_int_equal(sset_remove_all(set, from), 2);

	assert_sset_equal(set, expected);

	sset_free(set);
	sset_free(from);
	sset_free(expected);
}

static void sset_it_remove__many(void **state) {
	const struct Sset *set = sset_init();

	assert_true(sset_add(set, "a"));
	assert_true(sset_add(set, "b"));
	assert_true(sset_add(set, "c"));
	assert_true(sset_add(set, "d"));
	assert_true(sset_add(set, "e"));

	const struct Sset *expected = sset_init();
	assert_true(sset_add(expected, "b"));
	assert_true(sset_add(expected, "d"));

	size_t iterations = 0;
	for (const struct SsetIt *it = sset_it(set); it; it = sset_it_next(it)) {
		iterations++;
		if (strcmp(it->val, "a") == 0 || strcmp(it->val, "c") == 0 || strcmp(it->val, "e") == 0) {
			sset_it_remove(it);
		}
	}

	assert_int_equal(iterations, 5);

	assert_sset_equal(set, expected);

	sset_free(set);
	sset_free(expected);
}

static void sset_it_remove__partial(void **state) {
	const struct SsetIt *it = calloc(1, sizeof(struct SsetIt));

	sset_it_remove(it);
}

static void sset_add_contains_remove_free__case_insensitive(void **state) {
	const struct SsetParams params = { .case_insensitive = true, };
	const struct Sset *set = sset_init_with(params);

	assert_true(sset_add(set, "A"));
	assert_true(sset_add(set, "B"));

	assert_false(sset_add(set, "B"));

	assert_true(sset_contains(set, "b"));
	assert_true(sset_contains(set, "B"));

	assert_true(sset_remove(set, "b"));

	sset_free(set);
}

static void sset_find__matches(void **state) {
	const struct Sset *set = sset_init();

	assert_true(sset_add(set, "x0"));
	assert_true(sset_add(set, "x1"));
	assert_true(sset_add(set, "a2"));
	assert_true(sset_add(set, "x3"));

	assert_str_equal(sset_find(set, match_starts_with_a, NULL), "a2");

	sset_free(set);
}

static void sset_it__many(void **state) {

	const struct Sset *set = sset_init();
	assert_true(sset_add(set, "a"));
	assert_true(sset_add(set, "b"));

	const struct SsetIt *it = sset_it(set);

	assert_non_nul(it);
	assert_str_equal(it->val, "a");

	it = sset_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->val, "b");

	sset_it_free(it);

	sset_free(set);
}

static void sset_it__empty(void **state) {

	const struct Sset *set = sset_init();

	const struct SsetIt *it = sset_it(set);

	assert_nul(it);

	sset_free(set);
}

static void sset_it_free__partial(void **state) {
	const struct SsetIt *it = calloc(1, sizeof(struct SsetIt));

	sset_it_free(it);
}

static void sset_it_next__partial(void **state) {
	const struct SsetIt *it = calloc(1, sizeof(struct SsetIt));

	assert_nul(sset_it_next(it));
}

static void sset_filter_it__(void **state) {
	const struct Sset *set = sset_init();

	assert_true(sset_add(set, "a1"));
	assert_true(sset_add(set, "b1"));
	assert_true(sset_add(set, "a2"));
	assert_true(sset_add(set, "b2"));

	const struct SsetIt *it = sset_filter_it(set, match_starts_with_a, NULL);
	assert_non_nul(it);
	assert_str_equal(it->val, "a1");

	it = sset_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->val, "a2");

	assert_nul(sset_it_next(it));

	sset_free(set);
}

static void sset_equal__case_sensitive(void **state) {

	const struct Sset *actual = sset_init();
	assert_true(sset_add(actual, "a"));
	assert_true(sset_add(actual, "b"));

	assert_sset_not_equal(actual, NULL);

	const struct Sset *expected = sset_init();
	assert_true(sset_add(expected, "a"));
	assert_true(sset_add(expected, "b"));

	assert_sset_equal(actual, expected);

	assert_true(sset_add(actual, "c"));
	assert_true(sset_add(expected, "C"));

	assert_sset_not_equal(actual, expected);

	sset_free(actual);
	sset_free(expected);
}

static void sset_equal__case_insensitive(void **state) {
	const struct SsetParams params = { .case_insensitive = true, };
	const struct Sset *actual = sset_init_with(params);

	assert_true(sset_add(actual, "a"));
	assert_true(sset_add(actual, "b"));

	const struct Sset *expected = sset_init();
	assert_true(sset_add(expected, "A"));
	assert_true(sset_add(expected, "B"));

	assert_sset_equal(actual, expected);

	sset_free(actual);
	sset_free(expected);
}

static void sset_str__(void **state) {

	const struct Sset *set = sset_init();
	assert_true(sset_add(set, "a"));
	assert_true(sset_add(set, "b"));
	assert_true(sset_add(set, "c"));

	char *actual = sset_str(set);

	assert_str_equal(actual, "a\nb\nc\n");

	free(actual);
	sset_free(set);
}

static void sset_sort__empty(void **state) {
	const struct Sset *actual = sset_init();

	const struct Sset *expected = sset_init();

	sset_sort(actual);

	assert_int_equal(sset_size(actual), 0);

	assert_sset_equal(actual, expected);

	sset_free(actual);
	sset_free(expected);
}

static void sset_sort__one(void **state) {
	const struct Sset *actual = sset_init();

	assert_true(sset_add(actual, "A"));

	const struct Sset *expected = sset_init();
	assert_true(sset_add(expected, "A"));

	sset_sort(actual);

	assert_sset_equal(actual, expected);

	sset_free(actual);
	sset_free(expected);
}

static void sset_sort__many(void **state) {
	const struct Sset *actual = sset_init();

	assert_true(sset_add(actual, "3"));
	assert_true(sset_add(actual, "1"));
	assert_true(sset_add(actual, "0"));
	assert_true(sset_add(actual, "2"));

	const struct Sset *expected = sset_init();
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
	const struct SsetParams params = { .initial = 400, .grow = 400, };

	const struct Sset *actual = sset_init_with(params);

	for (size_t i = sizeof(words_unsorted) / sizeof(words_unsorted[0]); i > 0; i--) {
		assert_true(sset_add(actual, words_unsorted[i - 1]));
	}

	const struct Sset *expected = sset_init_with(params);

	for (size_t i = 0; i < sizeof(words_sorted) / sizeof(words_sorted[0]); i++ ) {
		assert_true(sset_add(expected, words_sorted[i]));
	}

	sset_sort(actual);

	assert_sset_equal(actual, expected);

	sset_free(actual);
	sset_free(expected);
}

static void sset_sort__many_case_insensitive(void **state) {
	const struct SsetParams params = { .case_insensitive = true, };
	const struct Sset *actual = sset_init_with(params);

	assert_true(sset_add(actual, "Bb3"));
	assert_true(sset_add(actual, "aa1"));
	assert_true(sset_add(actual, "Aa0"));
	assert_true(sset_add(actual, "bb2"));

	const struct Sset *expected = sset_init();
	assert_true(sset_add(expected, "Aa0"));
	assert_true(sset_add(expected, "aa1"));
	assert_true(sset_add(expected, "Bb2"));
	assert_true(sset_add(expected, "bb3"));

	sset_sort(actual);

	assert_sset_equal(actual, expected);

	sset_free(actual);
	sset_free(expected);
}

static void sset_pslist__(void **state) {
	const struct Sset *set = sset_init();

	sset_add(set, "a");
	sset_add(set, "b");

	struct Pslist *list = sset_pslist(set);

	assert_int_equal(pslist_length(list), 2);
	assert_str_equal(pslist_at(list, 0), "a");
	assert_str_equal(pslist_at(list, 1), "b");

	pslist_free_vals(&list, NULL);
	sset_free(set);
}

// also tests constructor
static void sset_clone__(void **state) {
	const struct SsetParams params = {
		.case_insensitive = true,
		.initial = 99,
		.grow = 1,
	};
	const struct Sset *from = sset_init_with(params);

	assert_true(sset_add(from, "a"));
	assert_true(sset_add(from, "b"));

	const struct Sset *to = sset_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->pset->size, 2);
	assert_int_equal(to->pset->capacity, 99);
	assert_int_equal(to->pset->params.grow, 1);
	assert_ptr_equal(to->pset->params.equal_val, equal_strcasecmp);

	assert_ptr_equal(to->params.case_insensitive, true);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	assert_true(sset_contains(to, "a"));
	assert_true(sset_contains(to, "b"));
	assert_int_equal(sset_size(to), 2);

	sset_free(from);
	sset_free(to);
}

static void sset__null_inputs(void **state) {
	const struct Sset *set = sset_init();

	assert_int_equal(sset_add_all(NULL, NULL), 0);
	assert_int_equal(sset_add_all(set, NULL), 0);
	assert_nul(sset_clone(NULL));
	sset_free(NULL);
	sset_it_free(NULL);
	assert_false(sset_contains(NULL, NULL));
	assert_false(sset_contains(set, NULL));
	assert_nul(sset_at(NULL, 0));
	sset_find(NULL, NULL, NULL);
	sset_find(set, NULL, NULL);
	assert_nul(sset_it(NULL));
	assert_nul(sset_filter_it(NULL, NULL, NULL));
	assert_nul(sset_filter_it(set, NULL, NULL));
	assert_nul(sset_it_next(NULL));
	sset_it_remove(NULL);
	assert_false(sset_add(NULL, NULL));
	assert_false(sset_add(set, NULL));
	assert_int_equal(sset_add_many(NULL, NULL), 0);
	assert_false(sset_remove(NULL, NULL));
	assert_false(sset_remove(set, NULL));
	assert_int_equal(sset_remove_all(NULL, NULL), 0);
	assert_int_equal(sset_remove_all(set, NULL), 0);
	assert_int_equal(sset_remove_all(NULL, set), 0);
	assert_false(sset_equal(NULL, NULL));
	assert_false(sset_equal(set, NULL));
	assert_nul(sset_pslist(NULL));
	assert_nul(sset_str(NULL));
	sset_sort(NULL);
	assert_int_equal(sset_size(NULL), 0);

	sset_free(set);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(sset_add__clone_val_free_val),

		TEST(sset_add_contains_remove_free__case_insensitive),
		TEST(sset_add_contains_remove_free__case_sensitive),

		TEST(sset_add_all__many),

		TEST(sset_add_many__many),

		TEST(sset_at__),

		TEST(sset_remove_all__many),

		TEST(sset_it_remove__many),
		TEST(sset_it_remove__partial),

		TEST(sset_find__matches),

		TEST(sset_it__many),
		TEST(sset_it__empty),

		TEST(sset_it_free__partial),

		TEST(sset_it_next__partial),

		TEST(sset_filter_it__),

		TEST(sset_equal__case_sensitive),
		TEST(sset_equal__case_insensitive),

		TEST(sset_str__),

		TEST(sset_sort__empty),
		TEST(sset_sort__one),
		TEST(sset_sort__many),
		TEST(sset_sort__words),
		TEST(sset_sort__many_case_insensitive),

		TEST(sset_pslist__),

		TEST(sset_clone__),

		TEST(sset__null_inputs),
	};

	return RUN(tests);
}


#include "assert-sset.h"
#include "asserts.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "slist.h"
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

struct Slist {
	const struct SlistParams params;
	const struct Plist *plist;
};

struct Sset {
	const struct SsetParams params;
	const struct Pset *pset;
};

static bool match_starts_with_a(const char* const a, const void* const b) {
	return *a == 'a';
}

static void sset_clone__params__constructor(void **state) {
	assert_nul(sset_clone(NULL));

	const struct Sset *set = sset_init_with((struct SsetParams){ .case_insensitive = true, .initial = 99, .grow = 1, });
	sset_add_many(set, "a", "b", NULL);

	const struct Sset *clone = sset_clone(set);

	assert_non_nul(clone);

	assert_int_equal(clone->pset->size, 2);
	assert_int_equal(clone->pset->capacity, 99);
	assert_int_equal(clone->params.grow, 1);
	assert_ptr_equal(clone->pset->params.equal_val, equal_strcasecmp);

	assert_ptr_equal(clone->params.case_insensitive, true);
	assert_ptr_equal(clone->params.initial, 99);
	assert_ptr_equal(clone->params.grow, 1);

	assert_sset_equal(set, clone);

	sset_free(set);
	sset_free(clone);
}

static void sset_free__(void **state) {
	sset_free(NULL);
}

static void sset_it_free__(void **state) {
	sset_it_free(NULL);

	const struct SsetIt *it = calloc(1, sizeof(struct SsetIt));

	sset_it_free(it);
}

static void sset_contains__(void **state) {
	assert_false(sset_contains(NULL, "x"));

	const struct Sset *set = sset_init();

	assert_false(sset_contains(set, "x"));

	sset_add_many(set, "a", "b", "c", NULL);

	assert_true(sset_contains(set, "b"));

	assert_false(sset_contains(set, "x"));

	sset_free(set);
}

static void sset_contains__case_insensitive(void **state) {
	const struct Sset *set = sset_init_with((struct SsetParams){ .case_insensitive = true, });
	sset_add_many(set, "a", NULL);

	assert_true(sset_contains(set, "A"));

	sset_free(set);
}

static void sset_at__(void **state) {
	assert_nul(sset_at(NULL, 0));

	const struct Sset *set = sset_init();

	assert_nul(sset_at(set, 0));

	sset_add_many(set, "a", "b", "c", NULL);

	assert_str_equal(sset_at(set, 1), "b");

	assert_nul(sset_at(set, 3));

	sset_free(set);
}

static void sset_find__(void **state) {
	assert_nul(sset_find(NULL, (struct SsetFilter){ 0 }));

	const struct Sset *set = sset_init();

	assert_nul(sset_find(set, (struct SsetFilter){ 0 }));

	sset_add_many(set, "x0", "x1", "a2", "x3", NULL);

	assert_str_equal(sset_find(set, (struct SsetFilter){ 0 }), "x0");

	assert_str_equal(sset_find(set, (struct SsetFilter){ .val_data = match_starts_with_a, .data = "x", }), "a2");

	sset_free(set);
}

static void sset_it__(void **state) {
	assert_nul(sset_it(NULL));

	const struct Sset *set = sset_init();

	assert_nul(sset_it(set));

	sset_add_many(set, "a", "b", NULL);

	const struct SsetIt *it = sset_it(set);

	assert_non_nul(it);
	assert_str_equal(it->val, "a");

	it = sset_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->val, "b");

	assert_nul(sset_it_next(it));

	sset_free(set);
}

static void sset_filter_it__(void **state) {
	assert_nul(sset_filter_it(NULL, (struct SsetFilter){ 0 }));

	const struct Sset *set = sset_init();

	assert_nul(sset_filter_it(set, (struct SsetFilter){ 0 }));

	sset_add_many(set, "a1", "b1", "a2", "b2", NULL);

	const struct SsetIt *it = sset_filter_it(set, (struct SsetFilter){ .val_data = match_starts_with_a, .data = "x", });

	assert_non_nul(it);
	assert_str_equal(it->val, "a1");

	it = sset_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->val, "a2");

	assert_nul(sset_it_next(it));

	sset_free(set);
}

static void sset_it_next__(void **state) {
	assert_nul(sset_it_next(NULL));

	const struct SsetIt *it = calloc(1, sizeof(struct SsetIt));

	assert_nul(sset_it_next(it));
}

static void sset_add__(void **state) {
	const struct Sset *expected = sset_init();
	sset_add_many(expected, "a", "b", NULL);

	assert_false(sset_add(NULL, "x"));

	const struct Sset *set = sset_init();

	assert_true(sset_add(set, "a"));

	assert_false(sset_add(set, "a"));

	assert_false(sset_add(set, NULL));

	assert_true(sset_add(set, "b"));

	assert_sset_equal(set, expected);

	sset_free(set);
	sset_free(expected);
}

static void sset_add__case_insensitive(void **state) {
	const struct Sset *set = sset_init_with((struct SsetParams){ .case_insensitive = true, });

	assert_true(sset_add(set, "a"));

	assert_false(sset_add(set, "A"));

	sset_free(set);
}

static void sset_add_all__(void **state) {
	const struct Sset *expected = sset_init();
	sset_add_many(expected, "A", "B", "C", NULL);

	assert_int_equal(sset_add_all(NULL, NULL), 0);

	const struct Sset *to = sset_init();
	sset_add_many(to, "A", "B", NULL);

	assert_int_equal(sset_add_all(to, NULL), 0);

	const struct Sset *from = sset_init();
	sset_add_many(from, "A", "C", NULL);

	assert_int_equal(sset_add_all(to, from), 1);

	assert_sset_equal(to, expected);

	assert_int_equal(sset_add_all(NULL, from), 0);

	sset_free(to);
	sset_free(from);
	sset_free(expected);
}

static void sset_remove__(void **state) {
	const struct Sset *expected = sset_init();
	sset_add_many(expected, "B", NULL);

	assert_false(sset_remove(NULL, "x"));

	const struct Sset *set = sset_init();
	sset_add_many(set, "A", "B", NULL);

	assert_true(sset_remove(set, "A"));

	assert_false(sset_remove(set, NULL));

	assert_false(sset_remove(set, "x"));

	assert_sset_equal(set, expected);

	sset_free(expected);
	sset_free(set);
}

static void sset_remove__case_insensitive(void **state) {
	const struct Sset *expected = sset_init();
	sset_add_many(expected, "B", NULL);

	const struct Sset *set = sset_init_with((struct SsetParams){ .case_insensitive = true, });
	sset_add_many(set, "A", "B", NULL);

	assert_true(sset_remove(set, "a"));

	assert_sset_equal(set, expected);

	sset_free(expected);
	sset_free(set);
}

static void sset_remove_all__(void **state) {
	assert_int_equal(sset_remove_all(NULL), 0);

	const struct Sset *set = sset_init();

	assert_int_equal(sset_remove_all(set), 0);

	sset_add_many(set, "a", "b", NULL);

	assert_int_equal(sset_remove_all(set), 2);

	assert_int_equal(sset_size(set), 0);

	sset_free(set);
}

static void sset_remove_in__(void **state) {
	const struct Sset *expected = sset_init();
	sset_add_many(expected, "b", NULL);

	assert_int_equal(sset_remove_in(NULL, NULL), 0);

	const struct Sset *set = sset_init();
	sset_add_many(set, "a", "b", "c", NULL);

	assert_int_equal(sset_remove_in(set, NULL), 0);

	assert_int_equal(sset_remove_in(NULL, set), 0);

	const struct Sset *in = sset_init();
	sset_add_many(in, "a", "c", "d", NULL);

	assert_int_equal(sset_remove_in(set, in), 2);

	assert_sset_equal(set, expected);

	sset_free(set);
	sset_free(in);
	sset_free(expected);
}

static void sset_remove_in__case_insensitive(void **state) {
	const struct Sset *set = sset_init_with((struct SsetParams){ .case_insensitive = true, });
	sset_add_many(set, "A", "B", NULL);

	const struct Sset *in = sset_init();
	sset_add_many(in, "b", "c", NULL);

	assert_int_equal(sset_remove_in(set, in), 1);

	sset_free(set);
	sset_free(in);
}

static void sset_it_remove__(void **state) {
	const struct Sset *expected = sset_init();
	sset_add_many(expected, "a", "c", "d", "e", NULL);

	assert_false(sset_it_remove(NULL));

	const struct SsetIt *it = calloc(1, sizeof(struct SsetIt));

	assert_false(sset_it_remove(it));

	const struct Sset *set = sset_init();
	sset_add_many(set, "a", "b", "c", "d", "e", NULL);

	it = sset_it(set);
	it = sset_it_next(it);
	assert_str_equal(it->val, "b");

	assert_true(sset_it_remove(it));

	assert_false(sset_contains(set, "b"));

	it = sset_it_next(it);
	assert_str_equal(it->val, "c");

	assert_sset_equal(set, expected);

	sset_it_free(it);
	sset_free(expected);
	sset_free(set);
}

static void sset_sort__(void **state) {
	sset_sort(NULL);

	const struct Sset *actual = sset_init_with((struct SsetParams){ .initial = 400, .grow = 400, });

	sset_sort(actual);

	for (size_t i = sizeof(words_unsorted) / sizeof(words_unsorted[0]); i > 0; i--) {
		sset_add(actual, words_unsorted[i - 1]);
	}

	const struct Sset *expected = sset_init_with((struct SsetParams){ .initial = 400, .grow = 400, });

	for (size_t i = 0; i < sizeof(words_sorted) / sizeof(words_sorted[0]); i++ ) {
		sset_add(expected, words_sorted[i]);
	}

	sset_sort(actual);

	assert_sset_equal(actual, expected);

	sset_free(actual);
	sset_free(expected);
}

static void sset_sort__case_insensitive(void **state) {
	const struct Sset *expected = sset_init();
	sset_add_many(expected, "Aa0", "aa1", "Bb2", "bb3", NULL);

	const struct Sset *actual = sset_init_with((struct SsetParams){ .case_insensitive = true, });
	sset_add_many(actual, "Bb3", "aa1", "Aa0", "bb2", NULL);

	sset_sort(actual);

	assert_sset_equal(actual, expected);

	sset_free(actual);
	sset_free(expected);
}

static void sset_equal__(void **state) {
	assert_false(sset_equal(NULL, NULL));

	const struct Sset *a = sset_init();

	assert_false(sset_equal(a, NULL));
	assert_false(sset_equal(NULL, a));

	const struct Sset *b = sset_init();

	assert_true(sset_equal(a, a));

	sset_add(a, "x");

	assert_false(sset_equal(a, b));

	sset_add(b, "x");

	assert_true(sset_equal(a, b));

	sset_free(a);
	sset_free(b);
}

static void sset_equal__case_insensitive(void **state) {
	const struct Sset *a = sset_init_with((struct SsetParams){ .case_insensitive = true, });
	sset_add_many(a, "a", "b", "c", NULL);

	const struct Sset *b = sset_init_with((struct SsetParams){ .case_insensitive = true, });
	sset_add_many(b, "a", "B", "c", NULL);

	assert_true(sset_equal(a, b));

	sset_free(a);
	sset_free(b);
}

static void sset_slist__(void **state) {
	assert_nul(sset_slist(NULL));

	const struct Sset *set = sset_init_with((struct SsetParams){ .case_insensitive = true, .initial = 2, .grow = 1, });

	const struct Slist *list = sset_slist(set);
	assert_int_equal(slist_size(list), 0);

	assert_true(list->params.case_insensitive);
	assert_int_equal(list->params.initial, 2);
	assert_int_equal(list->params.grow, 1);

	slist_free(list);

	sset_add_many(set, "a", "b", "c", NULL);

	list = sset_slist(set);

	assert_int_equal(slist_size(list), 3);

	slist_free(list);
	sset_free(set);
}

static void sset_str__(void **state) {
	assert_nul(sset_str(NULL));

	const struct Sset *set = sset_init();
	sset_add_many(set, "a", "b", "c", NULL);

	char *actual = sset_str(set);

	assert_str_equal(actual, "a\nb\nc\n");

	free(actual);
	sset_free(set);
}

static void sset_size__(void **state) {
	assert_int_equal(sset_size(NULL), 0);

	const struct Sset *set = sset_init();

	assert_int_equal(sset_size(set), 0);

	sset_add_many(set, "a", "b", "c", NULL);

	assert_int_equal(sset_size(set), 3);

	sset_free(set);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(sset_clone__params__constructor),

		TEST(sset_free__),

		TEST(sset_it_free__),

		TEST(sset_contains__),
		TEST(sset_contains__case_insensitive),

		TEST(sset_at__),

		TEST(sset_find__),

		TEST(sset_it__),

		TEST(sset_filter_it__),

		TEST(sset_it_next__),

		TEST(sset_add__),
		TEST(sset_add__case_insensitive),

		TEST(sset_add_all__),

		TEST(sset_remove__),
		TEST(sset_remove__case_insensitive),

		TEST(sset_remove_all__),

		TEST(sset_remove_in__),
		TEST(sset_remove_in__case_insensitive),

		TEST(sset_it_remove__),

		TEST(sset_sort__),
		TEST(sset_sort__case_insensitive),

		TEST(sset_equal__),
		TEST(sset_equal__case_insensitive),

		TEST(sset_slist__),

		TEST(sset_str__),

		TEST(sset_size__),
	};

	return RUN(tests);
}


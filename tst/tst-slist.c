#include "assert-slist.h"
#include "asserts.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "plist.h"

#include "slist.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "data/words-sorted.c"
#include "data/words-unsorted.c"
#pragma GCC diagnostic pop // "-Wunused-variable"

struct Slist {
	const struct SlistParams params;
	const struct Plist *plist;
};

struct Plist {
	const struct PlistParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

static bool match_starts_with_a(const char* const a, const void* const b) {
	return *a == 'a';
}

static void slist_clone__params__constructor(void **state) {
	assert_nul(slist_clone(NULL));

	const struct Slist *set = slist_init_with((struct SlistParams){ .case_insensitive = true, .initial = 99, .grow = 1, });
	slist_append_many(set, "a", "b", NULL);

	const struct Slist *clone = slist_clone(set);

	assert_non_nul(clone);

	assert_int_equal(clone->plist->size, 2);
	assert_int_equal(clone->plist->capacity, 99);
	assert_int_equal(clone->params.grow, 1);
	assert_ptr_equal(clone->plist->params.equal_val, equal_strcasecmp);

	assert_ptr_equal(clone->params.case_insensitive, true);
	assert_ptr_equal(clone->params.initial, 99);
	assert_ptr_equal(clone->params.grow, 1);

	assert_slist_equal(set, clone);

	slist_free(set);
	slist_free(clone);
}

static void slist_free__(void **state) {
	slist_free(NULL);
}

static void slist_it_free__(void **state) {
	slist_it_free(NULL);

	const struct SlistIt *it = calloc(1, sizeof(struct SlistIt));

	slist_it_free(it);
}

static void slist_contains__(void **state) {
	assert_false(slist_contains(NULL, "x"));

	const struct Slist *set = slist_init();

	assert_false(slist_contains(set, "x"));

	slist_append_many(set, "a", "b", "c", NULL);

	assert_true(slist_contains(set, "b"));

	assert_false(slist_contains(set, "x"));

	slist_free(set);
}

static void slist_contains__case_insensitive(void **state) {
	const struct Slist *set = slist_init_with((struct SlistParams){ .case_insensitive = true, });
	slist_append_many(set, "a", NULL);

	assert_true(slist_contains(set, "A"));

	slist_free(set);
}

static void slist_index_of__(void **state) {
	assert_false(slist_index_of(NULL, NULL, "x"));

	size_t i = 99;

	assert_false(slist_index_of(&i, NULL, "x"));
	assert_int_equal(i, 0);

	const struct Slist *set = slist_init();
	slist_append_many(set, "a", "b", "c", NULL);

	i = 99;
	assert_true(slist_index_of(&i, set, "c"));
	assert_int_equal(i, 2);

	assert_false(slist_index_of(&i, set, "x"));
	assert_int_equal(i, 0);

	slist_free(set);
}

static void slist_index_of__case_insensitive(void **state) {
	size_t i = 99;

	const struct Slist *set = slist_init();
	slist_append_many(set, "a", "b", "c", NULL);

	assert_true(slist_index_of(&i, set, "b"));
	assert_int_equal(i, 1);

	slist_free(set);
}

static void slist_at__(void **state) {
	assert_nul(slist_at(NULL, 0));

	const struct Slist *set = slist_init();

	assert_nul(slist_at(set, 0));

	slist_append_many(set, "a", "b", "c", NULL);

	assert_str_equal(slist_at(set, 1), "b");

	assert_nul(slist_at(set, 3));

	slist_free(set);
}

static void slist_find__(void **state) {
	assert_nul(slist_find(NULL, (struct SlistFilter){ 0 }));

	const struct Slist *set = slist_init();

	assert_nul(slist_find(set, (struct SlistFilter){ 0 }));

	slist_append_many(set, "x0", "x1", "a2", "x3", NULL);

	assert_str_equal(slist_find(set, (struct SlistFilter){ 0 }), "x0");

	assert_str_equal(slist_find(set, (struct SlistFilter){ .val_data = match_starts_with_a, .data = "x", }), "a2");

	slist_free(set);
}

static void slist_it_start__(void **state) {
	assert_nul(slist_it_start(NULL));

	const struct Slist *set = slist_init();

	assert_nul(slist_it_start(set));

	slist_append_many(set, "a", "b", NULL);

	const struct SlistIt *it = slist_it_start(set);

	assert_non_nul(it);
	assert_str_equal(it->val, "a");

	it = slist_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->val, "b");

	assert_nul(slist_it_next(it));

	slist_free(set);
}

static void slist_it_end__(void **state) {
	assert_nul(slist_it_end(NULL));

	const struct Slist *set = slist_init();

	assert_nul(slist_it_start(set));

	slist_append_many(set, "a", "b", NULL);

	const struct SlistIt *it = slist_it_end(set);

	assert_non_nul(it);
	assert_str_equal(it->val, "b");

	it = slist_it_prev(it);
	assert_non_nul(it);
	assert_str_equal(it->val, "a");

	assert_nul(slist_it_prev(it));

	slist_free(set);
}

static void slist_filter_it_start__(void **state) {
	assert_nul(slist_filter_it_start(NULL, (struct SlistFilter){ 0 }));

	const struct Slist *set = slist_init();

	assert_nul(slist_filter_it_start(set, (struct SlistFilter){ 0 }));

	slist_append_many(set, "a1", "b1", "a2", "b2", NULL);

	const struct SlistIt *it = slist_filter_it_start(set, (struct SlistFilter){ .val_data = match_starts_with_a, .data = "x", });

	assert_non_nul(it);
	assert_str_equal(it->val, "a1");

	it = slist_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->val, "a2");

	assert_nul(slist_it_next(it));

	slist_free(set);
}

static void slist_filter_it_end__(void **state) {
	assert_nul(slist_filter_it_end(NULL, (struct SlistFilter){ 0 }));

	const struct Slist *set = slist_init();

	assert_nul(slist_filter_it_end(set, (struct SlistFilter){ 0 }));

	slist_append_many(set, "a1", "b1", "a2", "b2", NULL);

	const struct SlistIt *it = slist_filter_it_end(set, (struct SlistFilter){ .val_data = match_starts_with_a, .data = "x", });

	assert_non_nul(it);
	assert_str_equal(it->val, "a2");

	it = slist_it_prev(it);
	assert_non_nul(it);
	assert_str_equal(it->val, "a1");

	assert_nul(slist_it_prev(it));

	slist_free(set);
}

static void slist_it_next__(void **state) {
	assert_nul(slist_it_next(NULL));

	const struct SlistIt *it = calloc(1, sizeof(struct SlistIt));

	assert_nul(slist_it_next(it));
}

static void slist_it_prev__(void **state) {
	assert_nul(slist_it_prev(NULL));

	const struct SlistIt *it = calloc(1, sizeof(struct SlistIt));

	assert_nul(slist_it_prev(it));
}

static void slist_insert__(void **state) {
	assert_false(slist_insert(NULL, 0, "x"));

	const struct Slist *set = slist_init();
	slist_append_many(set, "a", "b", NULL);

	assert_true(slist_insert(set, 0, "0"));

	assert_true(slist_insert(set, 2, "1"));

	assert_true(slist_insert(set, 999, "2"));

	assert_false(slist_insert(set, 0, NULL));

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "0", "a", "1", "b", "2", NULL);

	assert_slist_equal(set, expected);

	slist_free(set);
	slist_free(expected);
}

static void slist_append__(void **state) {
	assert_false(slist_append(NULL, "x"));

	const struct Slist *set = slist_init();

	assert_true(slist_append(set, "a"));

	assert_false(slist_append(set, NULL));

	assert_true(slist_append(set, "b"));

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "a", "b", NULL);

	assert_slist_equal(set, expected);

	slist_free(set);
	slist_free(expected);
}

static void slist_prepend__(void **state) {
	assert_false(slist_prepend(NULL, "x"));

	const struct Slist *set = slist_init();

	assert_true(slist_prepend(set, "a"));

	assert_false(slist_prepend(set, NULL));

	assert_true(slist_prepend(set, "b"));

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "b", "a", NULL);

	assert_slist_equal(set, expected);

	slist_free(set);
	slist_free(expected);
}

static void slist_replace__(void **state) {
	assert_false(slist_replace(NULL, 0, "x"));

	const struct Slist *set = slist_init();
	slist_append_many(set, "a", "b", NULL);

	assert_true(slist_replace(set, 0, "0"));

	assert_false(slist_replace(set, 0, NULL));

	assert_false(slist_replace(set, 2, "2"));

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "0", "b", NULL);

	assert_slist_equal(set, expected);

	slist_free(set);
	slist_free(expected);
}

static void slist_append_all__(void **state) {
	assert_int_equal(slist_append_all(NULL, NULL), 0);

	const struct Slist *to = slist_init();
	slist_append_many(to, "A", "B", NULL);

	assert_int_equal(slist_append_all(to, NULL), 0);

	const struct Slist *from = slist_init();
	slist_append_many(from, "A", "C", NULL);

	assert_int_equal(slist_append_all(to, from), 2);

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "A", "B", "A", "C", NULL);

	assert_slist_equal(to, expected);

	assert_int_equal(slist_append_all(NULL, from), 0);

	slist_free(to);
	slist_free(from);
	slist_free(expected);
}

static void slist_remove__(void **state) {
	assert_false(slist_remove(NULL, "x"));

	const struct Slist *set = slist_init();
	slist_append_many(set, "A", "B", NULL);

	assert_true(slist_remove(set, "A"));

	assert_false(slist_remove(set, NULL));

	assert_false(slist_remove(set, "x"));

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "B", NULL);

	assert_slist_equal(set, expected);

	slist_free(expected);
	slist_free(set);
}

static void slist_remove__case_insensitive(void **state) {
	const struct Slist *set = slist_init_with((struct SlistParams){ .case_insensitive = true, });
	slist_append_many(set, "A", "B", NULL);

	assert_true(slist_remove(set, "a"));

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "B", NULL);

	assert_slist_equal(set, expected);

	slist_free(expected);
	slist_free(set);
}

static void slist_remove_at__(void **state) {
	assert_false(slist_remove_at(NULL, 0));

	const struct Slist *set = slist_init();
	slist_append_many(set, "A", "B", "C", NULL);

	assert_true(slist_remove_at(set, 0));

	assert_true(slist_remove_at(set, 1));

	assert_false(slist_remove_at(set, 999));

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "B", NULL);

	assert_slist_equal(set, expected);

	slist_free(expected);
	slist_free(set);
}

static void slist_remove_all__(void **state) {
	assert_int_equal(slist_remove_all(NULL), 0);

	const struct Slist *set = slist_init();

	assert_int_equal(slist_remove_all(set), 0);

	slist_append_many(set, "a", "b", NULL);

	assert_int_equal(slist_remove_all(set), 2);

	assert_int_equal(slist_size(set), 0);

	slist_free(set);
}

static void slist_it_remove__(void **state) {
	assert_false(slist_it_remove(NULL));

	const struct SlistIt *it = calloc(1, sizeof(struct SlistIt));

	assert_false(slist_it_remove(it));

	const struct Slist *set = slist_init();
	slist_append_many(set, "a", "b", "c", "d", "e", NULL);

	it = slist_it_start(set);
	it = slist_it_next(it);
	assert_str_equal(it->val, "b");

	assert_true(slist_it_remove(it));

	assert_false(slist_contains(set, "b"));

	it = slist_it_next(it);
	assert_str_equal(it->val, "c");

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "a", "c", "d", "e", NULL);

	assert_slist_equal(set, expected);

	slist_it_free(it);
	slist_free(expected);
	slist_free(set);
}

static void slist_sort__(void **state) {
	slist_sort(NULL);

	const struct Slist *actual = slist_init_with((struct SlistParams){ .initial = 400, .grow = 400, });

	slist_sort(actual);

	for (size_t i = sizeof(words_unsorted) / sizeof(words_unsorted[0]); i > 0; i--) {
		slist_append(actual, words_unsorted[i - 1]);
	}

	const struct Slist *expected = slist_init_with((struct SlistParams){ .initial = 400, .grow = 400, });

	for (size_t i = 0; i < sizeof(words_sorted) / sizeof(words_sorted[0]); i++ ) {
		slist_append(expected, words_sorted[i]);
	}

	slist_sort(actual);

	assert_slist_equal(actual, expected);

	slist_free(actual);
	slist_free(expected);
}

static void slist_sort__case_insensitive(void **state) {
	const struct Slist *actual = slist_init_with((struct SlistParams){ .case_insensitive = true, });
	slist_append_many(actual, "Bb3", "aa1", "Aa0", "bb2", NULL);

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "Aa0", "aa1", "Bb2", "bb3", NULL);

	slist_sort(actual);

	assert_slist_equal(actual, expected);

	slist_free(actual);
	slist_free(expected);
}

static void slist_equal__(void **state) {
	assert_false(slist_equal(NULL, NULL));

	const struct Slist *a = slist_init();

	assert_false(slist_equal(a, NULL));
	assert_false(slist_equal(NULL, a));

	const struct Slist *b = slist_init();

	assert_true(slist_equal(a, a));

	slist_append(a, "x");

	assert_false(slist_equal(a, b));

	slist_append(b, "x");

	assert_true(slist_equal(a, b));

	slist_free(a);
	slist_free(b);
}

static void slist_equal__case_insensitive(void **state) {
	const struct Slist *a = slist_init_with((struct SlistParams){ .case_insensitive = true, });
	slist_append_many(a, "a", "b", "c", NULL);

	const struct Slist *b = slist_init_with((struct SlistParams){ .case_insensitive = true, });
	slist_append_many(b, "a", "B", "c", NULL);

	assert_true(slist_equal(a, b));

	slist_free(a);
	slist_free(b);
}

static void slist_str__(void **state) {
	assert_nul(slist_str(NULL));

	const struct Slist *set = slist_init();
	slist_append_many(set, "a", "b", "c", NULL);

	char *actual = slist_str(set);

	assert_str_equal(actual, "a\nb\nc\n");

	free(actual);
	slist_free(set);
}

static void slist_size__(void **state) {
	assert_int_equal(slist_size(NULL), 0);

	const struct Slist *set = slist_init();

	assert_int_equal(slist_size(set), 0);

	slist_append_many(set, "a", "b", "c", NULL);

	assert_int_equal(slist_size(set), 3);

	slist_free(set);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(slist_clone__params__constructor),

		TEST(slist_free__),

		TEST(slist_it_free__),

		TEST(slist_contains__),
		TEST(slist_contains__case_insensitive),

		TEST(slist_index_of__),
		TEST(slist_index_of__case_insensitive),

		TEST(slist_at__),

		TEST(slist_find__),

		TEST(slist_it_start__),

		TEST(slist_it_end__),

		TEST(slist_filter_it_start__),

		TEST(slist_filter_it_end__),

		TEST(slist_it_next__),

		TEST(slist_it_prev__),

		TEST(slist_insert__),

		TEST(slist_append__),

		TEST(slist_prepend__),

		TEST(slist_replace__),

		TEST(slist_append_all__),

		TEST(slist_remove__),
		TEST(slist_remove__case_insensitive),

		TEST(slist_remove_at__),

		TEST(slist_remove_all__),

		TEST(slist_it_remove__),

		TEST(slist_sort__),
		TEST(slist_sort__case_insensitive),

		TEST(slist_equal__),
		TEST(slist_equal__case_insensitive),

		TEST(slist_str__),

		TEST(slist_size__),
	};

	return RUN(tests);
}


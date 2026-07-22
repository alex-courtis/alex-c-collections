#include "assert-slist.h"
#include "asserts.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "plist.h"

#include "slist.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "data/words-sorted.c"
#include "data/words-unsorted.c"
#pragma GCC diagnostic pop // "-Wunused-variable"

struct Plist {
	const struct PlistParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct Slist {
	const struct SlistParams params;
	const struct Plist *plist;
};

static bool match_starts_with_a(const char* const a, const void* const b) {
	return *a == 'a';
}

static void slist_append__clone_val_free_val(void **state) {
	const struct Slist *list = slist_init();

	char *added = strdup("a");

	assert_true(slist_append(list, added));

	assert_true(slist_contains(list, "a"));

	free(added);

	assert_true(slist_contains(list, "a"));

	slist_free(list);
}

static void slist_append_contains_remove_free__case_sensitive(void **state) {
	const struct SlistParams params = { .case_insensitive = false, };
	const struct Slist *list = slist_init_with(params);

	assert_true(slist_append(list, "A"));
	assert_true(slist_append(list, "B"));

	assert_false(slist_contains(list, "b"));
	assert_true(slist_contains(list, "B"));

	slist_remove(list, "b");

	assert_int_equal(slist_size(list), 2);
	assert_false(slist_contains(list, "b"));
	assert_true(slist_contains(list, "B"));

	slist_remove(list, "B");

	assert_int_equal(slist_size(list), 1);
	assert_false(slist_contains(list, "b"));
	assert_false(slist_contains(list, "b"));

	slist_free(list);
}

static void slist_append__(void **state) {
	const struct Slist *list = slist_init();

	assert_true(slist_append(list, "0"));
	assert_true(slist_append(list, "1"));

	assert_int_equal(slist_size(list), 2);

	assert_str_equal(slist_at(list, 0), "0");
	assert_str_equal(slist_at(list, 1), "1");

	slist_free(list);
}

static void slist_prepend__(void **state) {
	const struct Slist *list = slist_init();

	assert_true(slist_prepend(list, "0"));
	assert_true(slist_prepend(list, "1"));

	assert_int_equal(slist_size(list), 2);

	assert_str_equal(slist_at(list, 0), "1");
	assert_str_equal(slist_at(list, 1), "0");

	slist_free(list);
}

static void slist_insert__(void **state) {
	const struct Slist *list = slist_init();

	assert_true(slist_insert(list, 999, "2"));

	assert_int_equal(slist_size(list), 1);

	assert_str_equal(slist_at(list, 0), "2");

	assert_true(slist_insert(list, 0, "0"));
	assert_true(slist_insert(list, 1, "1"));

	assert_true(slist_insert(list, 999, "3"));

	assert_int_equal(slist_size(list), 4);

	assert_str_equal(slist_at(list, 0), "0");
	assert_str_equal(slist_at(list, 1), "1");
	assert_str_equal(slist_at(list, 2), "2");
	assert_str_equal(slist_at(list, 3), "3");

	slist_free(list);
}

static void slist_replace__(void **state) {
	const struct Slist *list = slist_init();
	slist_append_many(list, "0", "1", "2", NULL);

	slist_replace(list, 0, "00");
	slist_replace(list, 1, "11");
	slist_replace(list, 2, "22");
	slist_replace(list, 3, "33");

	assert_int_equal(slist_size(list), 3);

	assert_str_equal(slist_at(list, 0), "00");
	assert_str_equal(slist_at(list, 1), "11");
	assert_str_equal(slist_at(list, 2), "22");

	slist_free(list);
}

static void slist_append_contains_remove_free__case_insensitive(void **state) {
	const struct SlistParams params = { .case_insensitive = true, };
	const struct Slist *list = slist_init_with(params);
	slist_append_many(list, "A", "B", NULL);

	assert_true(slist_contains(list, "b"));
	assert_true(slist_contains(list, "B"));

	slist_remove(list, "b");

	assert_int_equal(slist_size(list), 1);

	assert_true(slist_contains(list, "a"));
	assert_true(slist_contains(list, "A"));
	assert_false(slist_contains(list, "b"));
	assert_false(slist_contains(list, "B"));

	slist_free(list);
}

static void slist_append_all__many(void **state) {
	const struct Slist *to = slist_init();
	slist_append_many(to, "A", "B", NULL);

	const struct Slist *from = slist_init();
	slist_append_many(from, "A", "C", NULL);

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "A", "B", "A", "C", NULL);

	assert_int_equal(slist_append_all(to, from), 2);

	assert_slist_equal(to, expected);

	slist_free(to);
	slist_free(from);
	slist_free(expected);
}

static void slist_index_of__case_sensitive(void **state) {
	const struct Slist *list = slist_init();
	slist_append_many(list, "a", "b", NULL);

	size_t i = 10;
	assert_true(slist_index_of(&i, list, "b"));
	assert_int_equal(i, 1);

	assert_false(slist_index_of(&i, list, "X"));
	assert_int_equal(i, 0);

	slist_free(list);
}

static void slist_index_of__case_insensitive(void **state) {
	const struct SlistParams params = { .case_insensitive = true, };
	const struct Slist *list = slist_init_with(params);
	slist_append_many(list, "a", "b", NULL);

	size_t i = 10;
	assert_true(slist_index_of(&i, list, "B"));
	assert_int_equal(i, 1);

	assert_false(slist_index_of(&i, list, "X"));
	assert_int_equal(i, 0);

	slist_free(list);
}

static void slist_at__(void **state) {
	const struct Slist *list = slist_init();
	slist_append_many(list, "a", "b", "c", NULL);

	assert_str_equal(slist_at(list, 1), "b");

	slist_free(list);
}

static void slist_remove_at__(void **state) {
	const struct Slist *list = slist_init();
	slist_append_many(list, "0", "1", "2", "3", NULL);

	slist_remove_at(list, 1);

	assert_int_equal(slist_size(list), 3);
	assert_str_equal(slist_at(list, 0), "0");
	assert_str_equal(slist_at(list, 1), "2");
	assert_str_equal(slist_at(list, 2), "3");

	slist_remove_at(list, 0);

	assert_int_equal(slist_size(list), 2);
	assert_str_equal(slist_at(list, 0), "2");
	assert_str_equal(slist_at(list, 1), "3");

	slist_remove_at(list, 1);

	assert_int_equal(slist_size(list), 1);
	assert_str_equal(slist_at(list, 0), "2");

	slist_remove_at(list, 0);
	assert_int_equal(slist_size(list), 0);

	slist_free(list);
}

static void slist_remove_all__(void **state) {
	const struct Slist *list = slist_init();

	assert_int_equal(slist_remove_all(list), 0);

	slist_append_many(list, "a", "b", NULL);

	assert_int_equal(slist_remove_all(list), 2);

	assert_int_equal(slist_size(list), 0);

	assert_nul(slist_at(list, 0));
	assert_nul(slist_at(list, 1));

	slist_free(list);
}

static void slist_it_remove__forwards(void **state) {
	const struct Slist *list = slist_init();
	slist_append_many(list, "a", "b", "c", "d", "e", NULL);

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "a", "b", "d", "e", NULL);

	size_t iterations = 0;
	for (const struct SlistIt *it = slist_it_start(list); it; it = slist_it_next(it)) {
		iterations++;
		if (strcmp(it->val, "c") == 0) {
			slist_it_remove(it);
		}
	}

	assert_int_equal(iterations, 5);

	assert_slist_equal(list, expected);

	slist_free(list);
	slist_free(expected);
}

static void slist_it_remove__backwards(void **state) {
	const struct Slist *list = slist_init();
	slist_append_many(list, "a", "b", "c", "d", "e", NULL);

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "a", "b", "d", "e", NULL);

	size_t iterations = 0;
	for (const struct SlistIt *it = slist_it_end(list); it; it = slist_it_prev(it)) {
		iterations++;
		if (strcmp(it->val, "c") == 0) {
			slist_it_remove(it);
		}
	}

	assert_slist_equal(list, expected);

	assert_int_equal(iterations, 5);

	slist_free(list);
	slist_free(expected);
}

static void slist_it_remove__partial(void **state) {
	const struct SlistIt *it = calloc(1, sizeof(struct SlistIt));

	slist_it_remove(it);
}

static void slist_find__(void **state) {
	const struct Slist *list = slist_init();
	slist_append_many(list, "x0", "x1", "a2", "x3", NULL);

	const struct SlistFilter filter = { .val_data = match_starts_with_a, .data = "x", };

	assert_str_equal(slist_find(list, filter), "a2");

	slist_free(list);
}

static void slist_it_start__many(void **state) {
	const struct Slist *list = slist_init();
	slist_append_many(list, "a", "b", NULL);

	const struct SlistIt *it = slist_it_start(list);

	assert_non_nul(it);
	assert_str_equal(it->val, "a");

	it = slist_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->val, "b");

	slist_it_free(it);

	slist_free(list);
}

static void slist_it_start__empty(void **state) {
	const struct Slist *list = slist_init();

	const struct SlistIt *it = slist_it_start(list);

	assert_nul(it);

	slist_free(list);
}

static void slist_it_end__many(void **state) {
	const struct Slist *list = slist_init();
	slist_append_many(list, "a", "b", NULL);

	const struct SlistIt *it = slist_it_end(list);

	assert_non_nul(it);
	assert_str_equal(it->val, "b");

	it = slist_it_prev(it);
	assert_non_nul(it);
	assert_str_equal(it->val, "a");

	slist_it_free(it);

	slist_free(list);
}

static void slist_it_end__empty(void **state) {
	const struct Slist *list = slist_init();

	const struct SlistIt *it = slist_it_end(list);

	assert_nul(it);

	slist_free(list);
}

static void slist_it_free__partial(void **state) {
	const struct SlistIt *it = calloc(1, sizeof(struct SlistIt));

	slist_it_free(it);
}

static void slist_it_next__partial(void **state) {
	const struct SlistIt *it = calloc(1, sizeof(struct SlistIt));

	assert_nul(slist_it_next(it));
}

static void slist_filter_it_start__(void **state) {
	const struct Slist *list = slist_init();
	slist_append_many(list, "a1", "b1", "a2", "b2", NULL);

	const struct SlistFilter filter = { .val_data = match_starts_with_a, .data = "x", };
	const struct SlistIt *it = slist_filter_it_start(list, filter);

	assert_non_nul(it);
	assert_str_equal(it->val, "a1");

	it = slist_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->val, "a2");

	assert_nul(slist_it_next(it));

	slist_free(list);
}

static void slist_filter_it_end__(void **state) {
	const struct Slist *list = slist_init();
	slist_append_many(list, "a1", "b1", "a2", "b2", NULL);

	const struct SlistFilter filter = { .val_data = match_starts_with_a, .data = "x", };
	const struct SlistIt *it = slist_filter_it_end(list, filter);

	assert_non_nul(it);
	assert_str_equal(it->val, "a2");

	it = slist_it_prev(it);
	assert_non_nul(it);
	assert_str_equal(it->val, "a1");

	assert_nul(slist_it_prev(it));

	slist_free(list);
}

static void slist_equal__case_sensitive(void **state) {
	const struct Slist *actual = slist_init();
	slist_append_many(actual, "a", "b", NULL);

	assert_slist_not_equal(actual, NULL);

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "a", "b", NULL);

	assert_slist_equal(actual, expected);

	assert_true(slist_append(actual, "c"));
	assert_true(slist_append(expected, "C"));

	assert_slist_not_equal(actual, expected);

	slist_free(actual);
	slist_free(expected);
}

static void slist_equal__case_insensitive(void **state) {
	const struct SlistParams params = { .case_insensitive = true, };
	const struct Slist *actual = slist_init_with(params);
	slist_append_many(actual, "a", "b", NULL);

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "A", "b", NULL);

	assert_slist_equal(actual, expected);

	slist_free(actual);
	slist_free(expected);
}

static void slist_str__(void **state) {
	const struct Slist *list = slist_init();
	slist_append_many(list, "a", "b", "c", NULL);

	char *actual = slist_str(list);

	assert_str_equal(actual, "a\nb\nc\n");

	free(actual);
	slist_free(list);
}

static void slist_sort__empty(void **state) {
	const struct Slist *actual = slist_init();

	const struct Slist *expected = slist_init();

	slist_sort(actual);

	assert_int_equal(slist_size(actual), 0);

	assert_slist_equal(actual, expected);

	slist_free(actual);
	slist_free(expected);
}

static void slist_sort__one(void **state) {
	const struct Slist *actual = slist_init();
	slist_append_many(actual, "A", NULL);

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "A", NULL);

	slist_sort(actual);

	assert_slist_equal(actual, expected);

	slist_free(actual);
	slist_free(expected);
}

static void slist_sort__many(void **state) {
	const struct Slist *actual = slist_init();
	slist_append_many(actual, "3", "1", "0", "2", NULL);

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "0", "1", "2", "3", NULL);

	slist_sort(actual);

	assert_slist_equal(actual, expected);

	slist_free(actual);
	slist_free(expected);
}

static void slist_sort__words(void **state) {
	const struct SlistParams params = { .initial = 400, .grow = 400, };

	const struct Slist *actual = slist_init_with(params);

	for (size_t i = sizeof(words_unsorted) / sizeof(words_unsorted[0]); i > 0; i--) {
		assert_true(slist_append(actual, words_unsorted[i - 1]));
	}

	const struct Slist *expected = slist_init_with(params);

	for (size_t i = 0; i < sizeof(words_sorted) / sizeof(words_sorted[0]); i++ ) {
		assert_true(slist_append(expected, words_sorted[i]));
	}

	slist_sort(actual);

	assert_slist_equal(actual, expected);

	slist_free(actual);
	slist_free(expected);
}

static void slist_sort__many_case_insensitive(void **state) {
	const struct SlistParams params = { .case_insensitive = true, };
	const struct Slist *actual = slist_init_with(params);
	slist_append_many(actual, "Bb3", "aa1", "Aa0", "bb2", NULL);

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "Aa0", "aa1", "Bb2", "bb3", NULL);

	slist_sort(actual);

	assert_slist_equal(actual, expected);

	slist_free(actual);
	slist_free(expected);
}

// also tests constructor
static void slist_clone__(void **state) {
	const struct SlistParams params = {
		.case_insensitive = true,
		.initial = 99,
		.grow = 1,
	};
	const struct Slist *from = slist_init_with(params);
	slist_append_many(from, "a", "b", NULL);

	const struct Slist *to = slist_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->plist->size, 2);
	assert_int_equal(to->plist->capacity, 99);
	assert_int_equal(to->plist->params.grow, 1);
	assert_ptr_equal(to->plist->params.equal_val, equal_strcasecmp);

	assert_ptr_equal(to->params.case_insensitive, true);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	assert_true(slist_contains(to, "a"));
	assert_true(slist_contains(to, "b"));
	assert_int_equal(slist_size(to), 2);

	slist_free(from);
	slist_free(to);
}

static void slist__null_inputs(void **state) {
	const struct Slist *list = slist_init();
	const struct SlistFilter filter = { 0 };

	assert_int_equal(slist_append_all(NULL, NULL), 0);
	assert_int_equal(slist_append_all(list, NULL), 0);
	assert_nul(slist_clone(NULL));
	slist_free(NULL);
	slist_it_free(NULL);
	assert_false(slist_contains(NULL, NULL));
	assert_false(slist_contains(list, NULL));
	assert_nul(slist_at(NULL, 0));
	slist_index_of(NULL, NULL, NULL);
	slist_index_of(NULL, list, NULL);
	slist_find(NULL, filter);
	assert_nul(slist_it_start(NULL));
	assert_nul(slist_it_end(NULL));
	assert_nul(slist_filter_it_start(NULL, filter));
	assert_nul(slist_filter_it_end(NULL, filter));
	assert_nul(slist_it_next(NULL));
	assert_nul(slist_it_prev(NULL));
	slist_it_remove(NULL);
	assert_false(slist_insert(NULL, 0, NULL));
	assert_false(slist_insert(list, 0, NULL));
	assert_false(slist_append(NULL, NULL));
	assert_false(slist_append(list, NULL));
	assert_false(slist_prepend(NULL, NULL));
	assert_false(slist_prepend(list, NULL));
	slist_replace(NULL, 0, NULL);
	assert_int_equal(slist_append_many(NULL, NULL), 0);
	slist_remove_at(NULL, 0);
	slist_remove(NULL, NULL);
	slist_remove(list, NULL);
	assert_int_equal(slist_remove_all(NULL), 0);
	assert_false(slist_equal(NULL, NULL));
	assert_false(slist_equal(list, NULL));
	assert_nul(slist_str(NULL));
	slist_sort(NULL);
	assert_int_equal(slist_size(NULL), 0);

	slist_free(list);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(slist_append__clone_val_free_val),

		TEST(slist_append_contains_remove_free__case_insensitive),
		TEST(slist_append_contains_remove_free__case_sensitive),

		TEST(slist_append__),

		TEST(slist_prepend__),

		TEST(slist_insert__),

		TEST(slist_replace__),

		TEST(slist_append_all__many),

		TEST(slist_index_of__case_sensitive),
		TEST(slist_index_of__case_insensitive),

		TEST(slist_at__),

		TEST(slist_remove_at__),

		TEST(slist_remove_all__),

		TEST(slist_it_remove__forwards),
		TEST(slist_it_remove__backwards),
		TEST(slist_it_remove__partial),

		TEST(slist_find__),

		TEST(slist_it_start__many),
		TEST(slist_it_start__empty),
		TEST(slist_it_end__many),
		TEST(slist_it_end__empty),

		TEST(slist_it_free__partial),

		TEST(slist_it_next__partial),

		TEST(slist_filter_it_start__),
		TEST(slist_filter_it_end__),

		TEST(slist_equal__case_sensitive),
		TEST(slist_equal__case_insensitive),

		TEST(slist_str__),

		TEST(slist_sort__empty),
		TEST(slist_sort__one),
		TEST(slist_sort__many),
		TEST(slist_sort__words),
		TEST(slist_sort__many_case_insensitive),

		TEST(slist_clone__),

		TEST(slist__null_inputs),
	};

	return RUN(tests);
}


#include "tst.h"
#include "asserts.h"
#include "assert-sstable.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "ptable.h"
#include "slist.h"
#include "str.h"

#include "sstable.h"

struct PTable {
	const struct PTableParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct SSTable {
	const struct SSTableParams params;
	const struct PTable *ptab;
};

struct SSTableIterState {
	const struct PTableIter *pit;
};

static void sstable_put_get_remove__case_sensitive(void **state) {

	const struct SSTable *tab = sstable_init();
	assert_false(sstable_put(tab, "a", "A"));
	assert_false(sstable_put(tab, "b", "B"));
	assert_false(sstable_put(tab, "c", "C"));

	assert_true(sstable_put(tab, "c", "duplicate"));

	assert_int_equal(sstable_size(tab), 3);

	assert_str_equal(sstable_get(tab, "b"), "B");

	assert_nul(sstable_get(tab, "x"));

	assert_true(sstable_remove(tab, "b"));
	assert_false(sstable_remove(tab, "b"));

	assert_nul(sstable_get(tab, "b"));

	sstable_free(tab);
}

static void sstable_put_get_remove__case_insensitive(void **state) {
	const struct SSTableParams params = { .case_insensitive_key = true, };
	const struct SSTable *tab = sstable_init_with(params);

	assert_false(sstable_put(tab, "A", "aaa"));
	assert_false(sstable_put(tab, "B", "bbb"));

	assert_str_equal(sstable_get(tab, "b"), "bbb");

	assert_nul(sstable_get(tab, "x"));

	assert_true(sstable_remove(tab, "b"));

	assert_nul(sstable_get(tab, "b"));

	sstable_free(tab);
}

static void sstable_iter__(void **state) {

	const struct SSTable *tab = sstable_init();
	assert_false(sstable_put(tab, "a", "aa"));
	assert_false(sstable_put(tab, "b", NULL));
	assert_false(sstable_put(tab, "c", "cc"));

	const struct SSTableIter *iter = sstable_iter(tab);

	assert_non_nul(iter);
	assert_str_equal(iter->key, "a");
	assert_str_equal(iter->val, "aa");

	iter = sstable_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "b");
	assert_nul(iter->val);

	sstable_iter_free(iter);

	sstable_free(tab);
}

static void sstable_iter__state_deleted(void **state) {
	const struct SSTable *tab = sstable_init();

	assert_false(sstable_put(tab, "a", "aa"));

	const struct SSTableIter *iter = sstable_iter(tab);
	assert_non_nul(iter);

	const struct SSTableIterState *st = iter->st;
	((struct SSTableIter*)iter)->st = NULL;

	iter = sstable_iter_next(iter);
	assert_nul(iter);

	ptable_iter_free(st->pit);
	free((void*)st);
	sstable_free(tab);
}

static void sstable_iter__state_ptab_deleted(void **state) {
	const struct SSTable *tab = sstable_init();

	assert_false(sstable_put(tab, "a", "aa"));

	const struct SSTableIter *iter = sstable_iter(tab);
	assert_non_nul(iter);

	const struct PTableIter *piter = iter->st->pit;
	iter->st->pit = NULL;

	iter = sstable_iter_next(iter);
	assert_nul(iter);

	ptable_iter_free(piter);
	sstable_free(tab);
}

static void sstable_iter__empty(void **state) {

	const struct SSTable *tab = sstable_init();

	const struct SSTableIter *iter = sstable_iter(tab);

	assert_nul(iter);

	sstable_free(tab);
}

static bool fn_equal_starts_with_a(const void* const a, const void* const b) {
	return *(char*)a == 'a';
}

static bool fn_equal_starts_with_b(const void* const a, const void* const b) {
	return *(char*)a == 'b';
}

static void sstable_filter_iter__(void **state) {
	const struct SSTable *tab = sstable_init();

	assert_false(sstable_put(tab, "a0", "b0"));
	assert_false(sstable_put(tab, "a1", "x1"));
	assert_false(sstable_put(tab, "a2", "b2"));
	assert_false(sstable_put(tab, "x3", "b3"));
	assert_false(sstable_put(tab, "a4", "b4"));
	assert_false(sstable_put(tab, "a5", "x5"));

	const struct SSTableIter *iter = sstable_filter_iter(tab, fn_equal_starts_with_a, fn_equal_starts_with_b, NULL);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "a0");
	assert_str_equal(iter->val, "b0");

	iter = sstable_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "a2");
	assert_str_equal(iter->val, "b2");

	iter = sstable_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "a4");
	assert_str_equal(iter->val, "b4");

	assert_nul(sstable_iter_next(iter));

	sstable_free(tab);
}

static void sstable_equal__case_sensitive(void **state) {

	const struct SSTable *actual = sstable_init();
	assert_false(sstable_put(actual, "a", "aa"));
	assert_false(sstable_put(actual, "b", "aa"));

	assert_sstable_not_equal(actual, NULL);

	const struct SSTable *expected = sstable_init();
	assert_false(sstable_put(expected, "a", "aa"));
	assert_false(sstable_put(expected, "b", "aa"));

	assert_sstable_equal(actual, expected);

	assert_false(sstable_put(actual, "c", "cc"));

	assert_sstable_not_equal(actual, expected);

	sstable_free(actual);
	sstable_free(expected);
}

static void sstable_equal__case_insensitive_key(void **state) {

	const struct SSTableParams params = { .case_insensitive_key = true, };
	const struct SSTable *actual = sstable_init_with(params);

	assert_false(sstable_put(actual, "a", "aa"));
	assert_false(sstable_put(actual, "b", "bb"));

	const struct SSTable *expected = sstable_init();
	assert_false(sstable_put(expected, "A", "aa"));
	assert_false(sstable_put(expected, "B", "bb"));

	assert_sstable_equal(actual, expected);

	sstable_free(actual);
	sstable_free(expected);
}

static void sstable_equal__case_insensitive_val(void **state) {

	const struct SSTableParams params = { .case_insensitive_val = true, };
	const struct SSTable *actual = sstable_init_with(params);

	assert_false(sstable_put(actual, "a", "aa"));
	assert_false(sstable_put(actual, "b", "bb"));

	const struct SSTable *expected = sstable_init();
	assert_false(sstable_put(expected, "a", "AA"));
	assert_false(sstable_put(expected, "b", "BB"));

	assert_sstable_equal(actual, expected);

	sstable_free(actual);
	sstable_free(expected);
}

static void sstable_str__(void **state) {

	const struct SSTable *tab = sstable_init();
	assert_false(sstable_put(tab, "a", "aa"));
	assert_false(sstable_put(tab, "b", NULL));
	assert_false(sstable_put(tab, "c", "cc"));

	char *expected = sprintf_alloc(
			"a = aa\n"
			"b = (null)\n"
			"c = cc\n"
			);

	char *actual = sstable_str(tab);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	sstable_free(tab);
}

static void sstable_keys_slist__many(void **state) {
	const struct SSTable *tab = sstable_init();

	sstable_put(tab, "a", "aa");
	sstable_put(tab, "b", "bb");

	struct SList *list = sstable_keys_slist(tab);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "a");
	assert_str_equal(slist_at(list, 1), "b");

	sstable_free(tab);
	slist_free_vals(&list, NULL);
}

static void sstable_vals_slist__many(void **state) {
	const struct SSTable *tab = sstable_init();

	sstable_put(tab, "a", "aa");
	sstable_put(tab, "b", NULL);
	sstable_put(tab, "c", "cc");

	struct SList *list = sstable_vals_slist(tab);

	assert_int_equal(slist_length(list), 3);
	assert_str_equal(slist_at(list, 0), "aa");
	assert_nul(slist_at(list, 1));
	assert_str_equal(slist_at(list, 2), "cc");

	slist_free(&list);
	sstable_free(tab);
}

// also tests constructor
static void sstable_clone__(void **state) {
	const struct SSTableParams params = {
		.case_insensitive_key = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SSTable *from = sstable_init_with(params);

	const struct SSTable *to = sstable_clone(from);

	assert_non_nul(to);

	assert_non_nul(to);

	assert_int_equal(to->ptab->size, 0);
	assert_int_equal(to->ptab->capacity, 99);
	assert_int_equal(to->ptab->params.grow, 1);
	assert_ptr_equal(to->ptab->params.equal_key, fn_equal_strcasecmp);
	assert_ptr_equal(to->ptab->params.equal_val, fn_equal_strcmp);
	assert_ptr_equal(to->ptab->params.alloc_key, (fn_alloc)strdup);
	assert_ptr_equal(to->ptab->params.free_key, (fn_free)free);

	assert_true(to->params.case_insensitive_key);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	sstable_free(from);
	sstable_free(to);
}

static void sstable__null_inputs(void **state) {
	assert_nul(sstable_clone(NULL));
	sstable_free(NULL);
	sstable_iter_free(NULL);
	assert_false(sstable_get(NULL, NULL));
	assert_nul(sstable_iter(NULL));
	assert_nul(sstable_filter_iter(NULL, NULL, NULL, NULL));
	assert_nul(sstable_iter_next(NULL));
	assert_false(sstable_put(NULL, NULL, NULL));
	assert_false(sstable_remove(NULL, NULL));
	assert_false(sstable_equal(NULL, NULL));
	assert_nul(sstable_keys_slist(NULL));
	assert_nul(sstable_vals_slist(NULL));
	assert_nul(sstable_str(NULL));
	assert_int_equal(sstable_size(NULL), 0);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(sstable_put_get_remove__case_sensitive),
		TEST(sstable_put_get_remove__case_insensitive),

		TEST(sstable_iter__),
		TEST(sstable_iter__empty),
		TEST(sstable_iter__state_deleted),
		TEST(sstable_iter__state_ptab_deleted),

		TEST(sstable_filter_iter__),

		TEST(sstable_equal__case_sensitive),
		TEST(sstable_equal__case_insensitive_key),
		TEST(sstable_equal__case_insensitive_val),

		TEST(sstable_str__),

		TEST(sstable_keys_slist__many),

		TEST(sstable_vals_slist__many),

		TEST(sstable_clone__),

		TEST(sstable__null_inputs),
	};

	return RUN(tests);
}


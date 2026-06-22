#include "tst.h"
#include "asserts.h"
#include "assert-smaps.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "pmap.h"
#include "slist.h"
#include "str.h"

#include "smaps.h"

struct PMap {
	const struct PMapParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct SMapS {
	const struct SMapSParams params;
	const struct PMap *ptab;
};

static void smaps_put_get_remove_free__case_sensitive(void **state) {

	const struct SMapS *tab = smaps_init();
	assert_false(smaps_put(tab, "a", "A"));
	assert_false(smaps_put(tab, "b", "B"));
	assert_false(smaps_put(tab, "c", "C"));

	assert_true(smaps_put(tab, "c", "duplicate"));

	assert_int_equal(smaps_size(tab), 3);

	assert_str_equal(smaps_get(tab, "b"), "B");

	assert_nul(smaps_get(tab, "x"));

	assert_true(smaps_remove(tab, "b"));
	assert_false(smaps_remove(tab, "b"));

	assert_nul(smaps_get(tab, "b"));

	assert_true(smaps_put(tab, "a", NULL));

	smaps_free(tab);
}

static void smaps_put_get_remove_free__case_insensitive(void **state) {
	const struct SMapSParams params = { .case_insensitive_key = true, };
	const struct SMapS *tab = smaps_init_with(params);

	assert_false(smaps_put(tab, "A", "aaa"));
	assert_false(smaps_put(tab, "B", "bbb"));

	assert_str_equal(smaps_get(tab, "b"), "bbb");

	assert_nul(smaps_get(tab, "x"));

	assert_true(smaps_remove(tab, "b"));

	assert_nul(smaps_get(tab, "b"));

	smaps_free(tab);
}

static void smaps_iter__many(void **state) {

	const struct SMapS *tab = smaps_init();
	assert_false(smaps_put(tab, "a", "aa"));
	assert_false(smaps_put(tab, "b", NULL));
	assert_false(smaps_put(tab, "c", "cc"));

	const struct SMapSIter *iter = smaps_iter(tab);

	assert_non_nul(iter);
	assert_str_equal(iter->key, "a");
	assert_str_equal(iter->val, "aa");

	iter = smaps_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "b");
	assert_nul(iter->val);

	smaps_iter_free(iter);

	smaps_free(tab);
}

static void smaps_iter__empty(void **state) {

	const struct SMapS *tab = smaps_init();

	const struct SMapSIter *iter = smaps_iter(tab);

	assert_nul(iter);

	smaps_free(tab);
}

static void smaps_iter_free__partial(void **state) {
	const struct SMapSIter *iter = calloc(1, sizeof(struct SMapSIter));

	smaps_iter_free(iter);
}

static void smaps_iter_next__partial(void **state) {
	const struct SMapSIter *iter = calloc(1, sizeof(struct SMapSIter));

	assert_nul(smaps_iter_next(iter));
}

static bool fn_equal_starts_with_a(const void* const a, const void* const b) {
	return *(char*)a == 'a';
}

static bool fn_equal_starts_with_b(const void* const a, const void* const b) {
	return *(char*)a == 'b';
}

static void smaps_filter_iter__(void **state) {
	const struct SMapS *tab = smaps_init();

	assert_false(smaps_put(tab, "a0", "b0"));
	assert_false(smaps_put(tab, "a1", "x1"));
	assert_false(smaps_put(tab, "a2", "b2"));
	assert_false(smaps_put(tab, "x3", "b3"));
	assert_false(smaps_put(tab, "a4", "b4"));
	assert_false(smaps_put(tab, "a5", "x5"));

	const struct SMapSIter *iter = smaps_filter_iter(tab, fn_equal_starts_with_a, fn_equal_starts_with_b, NULL);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "a0");
	assert_str_equal(iter->val, "b0");

	iter = smaps_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "a2");
	assert_str_equal(iter->val, "b2");

	iter = smaps_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "a4");
	assert_str_equal(iter->val, "b4");

	assert_nul(smaps_iter_next(iter));

	smaps_free(tab);
}

static void smaps_equal__case_sensitive(void **state) {

	const struct SMapS *actual = smaps_init();
	assert_false(smaps_put(actual, "a", "aa"));
	assert_false(smaps_put(actual, "b", "aa"));

	assert_smaps_not_equal(actual, NULL);

	const struct SMapS *expected = smaps_init();
	assert_false(smaps_put(expected, "a", "aa"));
	assert_false(smaps_put(expected, "b", "aa"));

	assert_smaps_equal(actual, expected);

	assert_false(smaps_put(actual, "c", "cc"));

	assert_smaps_not_equal(actual, expected);

	smaps_free(actual);
	smaps_free(expected);
}

static void smaps_equal__case_insensitive_key(void **state) {

	const struct SMapSParams params = { .case_insensitive_key = true, };
	const struct SMapS *actual = smaps_init_with(params);

	assert_false(smaps_put(actual, "a", "aa"));
	assert_false(smaps_put(actual, "b", "bb"));

	const struct SMapS *expected = smaps_init();
	assert_false(smaps_put(expected, "A", "aa"));
	assert_false(smaps_put(expected, "B", "bb"));

	assert_smaps_equal(actual, expected);

	smaps_free(actual);
	smaps_free(expected);
}

static void smaps_equal__case_insensitive_val(void **state) {

	const struct SMapSParams params = { .case_insensitive_val = true, };
	const struct SMapS *actual = smaps_init_with(params);

	assert_false(smaps_put(actual, "a", "aa"));
	assert_false(smaps_put(actual, "b", "bb"));

	const struct SMapS *expected = smaps_init();
	assert_false(smaps_put(expected, "a", "AA"));
	assert_false(smaps_put(expected, "b", "BB"));

	assert_smaps_equal(actual, expected);

	smaps_free(actual);
	smaps_free(expected);
}

static void smaps_contains_key__(void **state) {
	const struct SMapS *tab = smaps_init();

	assert_false(smaps_contains_key(tab, "a"));

	assert_false(smaps_put(tab, "a", "aa"));
	assert_false(smaps_put(tab, "b", "bb"));

	assert_true(smaps_contains_key(tab, "a"));
	assert_true(smaps_contains_key(tab, "b"));

	assert_false(smaps_contains_key(tab, "c"));

	assert_false(smaps_contains_key(tab, NULL));

	smaps_free(tab);
}

static void smaps_put_if_absent__(void **state) {
	const struct SMapS *tab = smaps_init();

	assert_false(smaps_put_if_absent(tab, "a", "aa"));
	assert_str_equal(smaps_get(tab, "a"), "aa");

	assert_true(smaps_put_if_absent(tab, "a", "xx"));

	assert_false(smaps_put_if_absent(tab, "b", NULL));
	assert_nul(smaps_get(tab, "b"));

	smaps_free(tab);
}

static void smaps_str__(void **state) {

	const struct SMapS *tab = smaps_init();
	assert_false(smaps_put(tab, "a", "aa"));
	assert_false(smaps_put(tab, "b", NULL));
	assert_false(smaps_put(tab, "c", "cc"));

	char *expected = sprintf_alloc(
			"a = aa\n"
			"b = (null)\n"
			"c = cc\n"
			);

	char *actual = smaps_str(tab);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	smaps_free(tab);
}

static void smaps_keys_slist_deep__many(void **state) {
	const struct SMapS *tab = smaps_init();

	smaps_put(tab, "a", "aa");
	smaps_put(tab, "b", "bb");

	struct SList *list = smaps_keys_slist_deep(tab);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "a");
	assert_str_equal(slist_at(list, 1), "b");

	smaps_free(tab);
	slist_free_vals(&list, NULL);
}

static void smaps_vals_slist_deep__many(void **state) {
	const struct SMapS *tab = smaps_init();

	smaps_put(tab, "a", "aa");
	smaps_put(tab, "b", NULL);
	smaps_put(tab, "c", "cc");

	struct SList *list = smaps_vals_slist_deep(tab);

	assert_int_equal(slist_length(list), 3);
	assert_str_equal(slist_at(list, 0), "aa");
	assert_nul(slist_at(list, 1));
	assert_str_equal(slist_at(list, 2), "cc");

	slist_free_vals(&list, NULL);
	smaps_free(tab);
}

// also tests constructor
static void smaps_clone__(void **state) {
	const struct SMapSParams params = {
		.case_insensitive_key = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SMapS *from = smaps_init_with(params);

	const struct SMapS *to = smaps_clone(from);

	assert_non_nul(to);

	assert_non_nul(to);

	assert_int_equal(to->ptab->size, 0);
	assert_int_equal(to->ptab->capacity, 99);
	assert_int_equal(to->ptab->params.grow, 1);
	assert_ptr_equal(to->ptab->params.equal_key, fn_equal_strcasecmp);
	assert_ptr_equal(to->ptab->params.equal_val, fn_equal_strcmp);
	assert_ptr_equal(to->ptab->params.alloc_key, fn_clone_strdup);
	assert_ptr_equal(to->ptab->params.alloc_val, fn_clone_strdup);
	assert_ptr_equal(to->ptab->params.free_key, (fn_free)free);
	assert_ptr_equal(to->ptab->params.free_val, (fn_free)free);
	assert_ptr_equal(to->ptab->params.clone_val, fn_clone_strdup);

	assert_true(to->params.case_insensitive_key);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	smaps_free(from);
	smaps_free(to);
}

static void smaps__null_inputs(void **state) {
	const struct SMapS *tab = smaps_init();

	assert_nul(smaps_clone(NULL));
	smaps_free(NULL);
	smaps_iter_free(NULL);
	assert_false(smaps_get(NULL, NULL));
	assert_false(smaps_get(tab, NULL));
	assert_false(smaps_contains_key(NULL, NULL));
	assert_false(smaps_contains_key(tab, NULL));
	assert_nul(smaps_iter(NULL));
	assert_nul(smaps_filter_iter(NULL, NULL, NULL, NULL));
	assert_nul(smaps_iter_next(NULL));
	assert_false(smaps_put(NULL, NULL, NULL));
	assert_false(smaps_put(tab, NULL, NULL));
	assert_false(smaps_put_if_absent(NULL, NULL, NULL));
	assert_false(smaps_put_if_absent(tab, NULL, NULL));
	assert_false(smaps_remove(NULL, NULL));
	assert_false(smaps_remove(tab, NULL));
	assert_false(smaps_equal(NULL, NULL));
	assert_false(smaps_equal(tab, NULL));
	assert_nul(smaps_keys_slist_deep(NULL));
	assert_nul(smaps_vals_slist_deep(NULL));
	assert_nul(smaps_str(NULL));
	assert_int_equal(smaps_size(NULL), 0);

	smaps_free(tab);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(smaps_put_get_remove_free__case_sensitive),
		TEST(smaps_put_get_remove_free__case_insensitive),

		TEST(smaps_iter__many),
		TEST(smaps_iter__empty),

		TEST(smaps_iter_free__partial),

		TEST(smaps_iter_next__partial),

		TEST(smaps_filter_iter__),

		TEST(smaps_equal__case_sensitive),
		TEST(smaps_equal__case_insensitive_key),
		TEST(smaps_equal__case_insensitive_val),

		TEST(smaps_contains_key__),

		TEST(smaps_put_if_absent__),

		TEST(smaps_str__),

		TEST(smaps_keys_slist_deep__many),

		TEST(smaps_vals_slist_deep__many),

		TEST(smaps_clone__),

		TEST(smaps__null_inputs),
	};

	return RUN(tests);
}


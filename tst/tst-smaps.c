#include "tst.h"
#include "asserts.h"
#include "assert-smaps.h"
#include "mock-fn.h"

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
	const struct PMap *pmap;
};

static void smaps_put_get_remove_free__case_sensitive(void **state) {

	const struct SMapS *map = smaps_init();
	assert_false(smaps_put(map, "a", "A"));
	assert_false(smaps_put(map, "b", "B"));
	assert_false(smaps_put(map, "c", "C"));

	assert_true(smaps_put(map, "c", "duplicate"));

	assert_int_equal(smaps_size(map), 3);

	assert_str_equal(smaps_get(map, "b"), "B");

	assert_nul(smaps_get(map, "x"));

	assert_true(smaps_remove(map, "b"));
	assert_false(smaps_remove(map, "b"));

	assert_nul(smaps_get(map, "b"));

	assert_true(smaps_put(map, "a", NULL));

	smaps_free(map);
}

static void smaps_put_get_remove_free__case_insensitive(void **state) {
	const struct SMapSParams params = { .case_insensitive_key = true, };
	const struct SMapS *map = smaps_init_with(params);

	assert_false(smaps_put(map, "A", "aaa"));
	assert_false(smaps_put(map, "B", "bbb"));

	assert_str_equal(smaps_get(map, "b"), "bbb");

	assert_nul(smaps_get(map, "x"));

	assert_true(smaps_remove(map, "b"));

	assert_nul(smaps_get(map, "b"));

	smaps_free(map);
}

static void smaps_match__matches(void **state) {
	const struct SMapS *map = smaps_init();

	assert_false(smaps_put(map, "0", "aaa"));
	assert_false(smaps_put(map, "1", "bbb"));
	assert_false(smaps_put(map, "2", "ccc"));

	// skip 0
	expect_string(mock_match_key_val, key, "0");
	expect_string(mock_match_key_val, val, "aaa");
	expect_string(mock_match_key_val, data, "x");
	will_return(mock_match_key_val, false);

	// get 1
	expect_string(mock_match_key_val, key, "1");
	expect_string(mock_match_key_val, val, "bbb");
	expect_string(mock_match_key_val, data, "x");
	will_return(mock_match_key_val, true);

	const struct SMapSPair pair = smaps_match(map, mock_match_key_val, "x");
	assert_str_equal(pair.key, "1");
	assert_str_equal(pair.val, "bbb");

	smaps_free(map);
}

static void smaps_iter__many(void **state) {

	const struct SMapS *map = smaps_init();
	assert_false(smaps_put(map, "a", "aa"));
	assert_false(smaps_put(map, "b", NULL));
	assert_false(smaps_put(map, "c", "cc"));

	const struct SMapSIter *iter = smaps_iter(map);

	assert_non_nul(iter);
	assert_str_equal(iter->key, "a");
	assert_str_equal(iter->val, "aa");

	iter = smaps_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "b");
	assert_nul(iter->val);

	smaps_iter_free(iter);

	smaps_free(map);
}

static void smaps_iter__empty(void **state) {

	const struct SMapS *map = smaps_init();

	const struct SMapSIter *iter = smaps_iter(map);

	assert_nul(iter);

	smaps_free(map);
}

static void smaps_iter_free__partial(void **state) {
	const struct SMapSIter *iter = calloc(1, sizeof(struct SMapSIter));

	smaps_iter_free(iter);
}

static void smaps_iter_next__partial(void **state) {
	const struct SMapSIter *iter = calloc(1, sizeof(struct SMapSIter));

	assert_nul(smaps_iter_next(iter));
}

static bool fn_match_both_start_with_a(const void* const key, const void* const val, const void* const data) {
	return *(char*)key == 'a' && *(char*)val == 'a';
}

static void smaps_match_iter__many(void **state) {
	const struct SMapS *map = smaps_init();

	assert_false(smaps_put(map, "ak0", "bv0"));
	assert_false(smaps_put(map, "ak1", "av1"));
	assert_false(smaps_put(map, "bk2", "av2"));
	assert_false(smaps_put(map, "ak3", "av3"));
	assert_false(smaps_put(map, "ak4", "bv4"));

	const struct SMapSIter *iter = smaps_match_iter(map, fn_match_both_start_with_a, NULL);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "ak1");
	assert_str_equal(iter->val, "av1");

	iter = smaps_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "ak3");
	assert_str_equal(iter->val, "av3");

	assert_nul(smaps_iter_next(iter));

	smaps_free(map);
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
	const struct SMapS *map = smaps_init();

	assert_false(smaps_contains_key(map, "a"));

	assert_false(smaps_put(map, "a", "aa"));
	assert_false(smaps_put(map, "b", "bb"));

	assert_true(smaps_contains_key(map, "a"));
	assert_true(smaps_contains_key(map, "b"));

	assert_false(smaps_contains_key(map, "c"));

	assert_false(smaps_contains_key(map, NULL));

	smaps_free(map);
}

static void smaps_put_if_absent__(void **state) {
	const struct SMapS *map = smaps_init();

	assert_false(smaps_put_if_absent(map, "a", "aa"));
	assert_str_equal(smaps_get(map, "a"), "aa");

	assert_true(smaps_put_if_absent(map, "a", "xx"));

	assert_false(smaps_put_if_absent(map, "b", NULL));
	assert_nul(smaps_get(map, "b"));

	smaps_free(map);
}

static void smaps_str__(void **state) {

	const struct SMapS *map = smaps_init();
	assert_false(smaps_put(map, "a", "aa"));
	assert_false(smaps_put(map, "b", NULL));
	assert_false(smaps_put(map, "c", "cc"));

	char *expected = sprintf_alloc(
			"a = aa\n"
			"b = (null)\n"
			"c = cc\n"
			);

	char *actual = smaps_str(map);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	smaps_free(map);
}

static void smaps_keys_slist_deep__many(void **state) {
	const struct SMapS *map = smaps_init();

	smaps_put(map, "a", "aa");
	smaps_put(map, "b", "bb");

	struct SList *list = smaps_keys_slist_deep(map);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "a");
	assert_str_equal(slist_at(list, 1), "b");

	smaps_free(map);
	slist_free_vals(&list, NULL);
}

static void smaps_vals_slist_deep__many(void **state) {
	const struct SMapS *map = smaps_init();

	smaps_put(map, "a", "aa");
	smaps_put(map, "b", NULL);
	smaps_put(map, "c", "cc");

	struct SList *list = smaps_vals_slist_deep(map);

	assert_int_equal(slist_length(list), 3);
	assert_str_equal(slist_at(list, 0), "aa");
	assert_nul(slist_at(list, 1));
	assert_str_equal(slist_at(list, 2), "cc");

	slist_free_vals(&list, NULL);
	smaps_free(map);
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

	assert_int_equal(to->pmap->size, 0);
	assert_int_equal(to->pmap->capacity, 99);
	assert_int_equal(to->pmap->params.grow, 1);
	assert_ptr_equal(to->pmap->params.equal_key, fn_equal_strcasecmp);
	assert_ptr_equal(to->pmap->params.equal_val, fn_equal_strcmp);
	assert_ptr_equal(to->pmap->params.alloc_key, fn_clone_strdup);
	assert_ptr_equal(to->pmap->params.alloc_val, fn_clone_strdup);
	assert_ptr_equal(to->pmap->params.free_key, (fn_free)free);
	assert_ptr_equal(to->pmap->params.free_val, (fn_free)free);
	assert_ptr_equal(to->pmap->params.clone_val, fn_clone_strdup);

	assert_true(to->params.case_insensitive_key);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	smaps_free(from);
	smaps_free(to);
}

static void smaps__null_inputs(void **state) {
	const struct SMapS *map = smaps_init();

	assert_nul(smaps_clone(NULL));
	smaps_free(NULL);
	smaps_iter_free(NULL);
	assert_false(smaps_get(NULL, NULL));
	assert_false(smaps_get(map, NULL));
	assert_false(smaps_contains_key(NULL, NULL));
	assert_false(smaps_contains_key(map, NULL));
	smaps_match(NULL, NULL, NULL);
	smaps_match(NULL, mock_match_key_val, NULL);
	assert_nul(smaps_iter(NULL));
	assert_nul(smaps_match_iter(NULL, NULL, NULL));
	assert_nul(smaps_match_iter(map, NULL, NULL));
	assert_nul(smaps_match_iter(map, mock_match_key_val, NULL));
	assert_nul(smaps_iter_next(NULL));
	assert_false(smaps_put(NULL, NULL, NULL));
	assert_false(smaps_put(map, NULL, NULL));
	assert_false(smaps_put_if_absent(NULL, NULL, NULL));
	assert_false(smaps_put_if_absent(map, NULL, NULL));
	assert_false(smaps_remove(NULL, NULL));
	assert_false(smaps_remove(map, NULL));
	assert_false(smaps_equal(NULL, NULL));
	assert_false(smaps_equal(map, NULL));
	assert_nul(smaps_keys_slist_deep(NULL));
	assert_nul(smaps_vals_slist_deep(NULL));
	assert_nul(smaps_str(NULL));
	assert_int_equal(smaps_size(NULL), 0);

	smaps_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(smaps_put_get_remove_free__case_sensitive),
		TEST(smaps_put_get_remove_free__case_insensitive),

		TEST(smaps_match__matches),

		TEST(smaps_iter__many),
		TEST(smaps_iter__empty),

		TEST(smaps_iter_free__partial),

		TEST(smaps_iter_next__partial),

		TEST(smaps_match_iter__many),

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


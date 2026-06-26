#include "assert-smapi.h"
#include "asserts.h"
#include "mock-fn.h"
#include "tst.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "pmap.h"
#include "slist.h"
#include "str.h"

#include "smapi.h"

struct PMap {
	const struct PMapParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct SMapI {
	const struct SMapIParams params;
	const struct PMap *pmap;
};

static void smapi_put_get_remove_free__case_sensitive(void **state) {
	const struct SMapI *map = smapi_init();
	assert_false(smapi_put(map, "a", 10));
	assert_false(smapi_put(map, "b", 11));
	assert_false(smapi_put(map, "c", 12));

	assert_true(smapi_put(map, "c", 13));

	assert_int_equal(smapi_size(map), 3);

	assert_int_equal(smapi_get(map, "b"), 11);

	assert_int_equal(smapi_get(map, "x"), 0);

	assert_true(smapi_remove(map, "b"));
	assert_false(smapi_remove(map, "b"));

	assert_int_equal(smapi_get(map, "b"), 0);

	smapi_free(map);
}

static void smapi_put_get_remove_free__case_insensitive(void **state) {
	const struct SMapIParams params = { .case_insensitive_key = true, };
	const struct SMapI *map = smapi_init_with(params);

	assert_false(smapi_put(map, "A", 10));
	assert_false(smapi_put(map, "B", 11));

	assert_int_equal(smapi_get(map, "b"), 11);

	assert_int_equal(smapi_get(map, "x"), 0);

	assert_true(smapi_remove(map, "b"));

	assert_int_equal(smapi_get(map, "b"), 0);

	smapi_free(map);
}

static void smapi_getp__zero(void **state) {

	const struct SMapI *map = smapi_init();
	assert_false(smapi_put(map, "a", 11));

	assert_int_equal(smapi_size(map), 1);

	assert_int_equal(smapi_get(map, "a"), 11);

	size_t a = 99;
	assert_true(smapi_getp(&a, map, "a"));
	assert_int_equal(a, 11);

	assert_int_equal(smapi_get(map, "x"), 0);

	size_t x = 99;
	assert_false(smapi_getp(&x, map, "x"));
	assert_int_equal(x, 0);

	assert_false(smapi_put(map, "b", 0));

	size_t b = 99;
	assert_int_equal(smapi_get(map, "b"), 0);
	assert_true(smapi_getp(&b, map, "b"));
	assert_int_equal(b, 0);

	smapi_free(map);
}

static void smapi_match__none(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "0", 10));

	// skip
	expect_string(mock_match_smapi, key, "0");
	expect_int_value(mock_match_smapi, val, 10);
	expect_string(mock_match_smapi, data, "x");
	will_return(mock_match_smapi, false);

	const struct SMapIPair pair = smapi_match(map, mock_match_smapi, "x");
	assert_nul(pair.key);
	assert_int_equal(pair.val, 0);

	smapi_free(map);
}

static void smapi_match__matches(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "0", 10));
	assert_false(smapi_put(map, "1", 11));
	assert_false(smapi_put(map, "2", 12));

	// skip 0
	expect_string(mock_match_smapi, key, "0");
	expect_int_value(mock_match_smapi, val, 10);
	expect_string(mock_match_smapi, data, "x");
	will_return(mock_match_smapi, false);

	// get 1
	expect_string(mock_match_smapi, key, "1");
	expect_int_value(mock_match_smapi, val, 11);
	expect_string(mock_match_smapi, data, "x");
	will_return(mock_match_smapi, true);

	const struct SMapIPair pair = smapi_match(map, mock_match_smapi, "x");
	assert_str_equal(pair.key, "1");
	assert_int_equal(pair.val, 11);

	smapi_free(map);
}

static void smapi_it__many(void **state) {

	const struct SMapI *map = smapi_init();
	assert_false(smapi_put(map, "a", 10));
	assert_false(smapi_put(map, "b", 11));
	assert_false(smapi_put(map, "c", 12));

	const struct SMapIIt *it = smapi_it(map);

	assert_non_nul(it);
	assert_str_equal(it->key, "a");
	assert_int_equal(it->val, 10);

	it = smapi_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "b");
	assert_int_equal(it->val, 11);

	smapi_it_free(it);

	smapi_free(map);
}

static void smapi_it__empty(void **state) {

	const struct SMapI *map = smapi_init();

	const struct SMapIIt *it = smapi_it(map);

	assert_nul(it);

	smapi_free(map);
}

static void smapi_it_free__partial(void **state) {
	const struct SMapIIt *it = calloc(1, sizeof(struct SMapIIt));

	smapi_it_free(it);
}

static void smapi_it_next__partial(void **state) {
	const struct SMapIIt *it = calloc(1, sizeof(struct SMapIIt));

	assert_nul(smapi_it_next(it));
}

static bool fn_match_key_a_val_lt_100(const char* const key, const size_t val, const void* const data) {
	return *key == 'a' && val < 100;
}

static void smapi_match_it__many(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "ak0", 100));
	assert_false(smapi_put(map, "ak1", 11));
	assert_false(smapi_put(map, "bk2", 12));
	assert_false(smapi_put(map, "ak3", 13));
	assert_false(smapi_put(map, "ak4", 101));

	const struct SMapIIt *it = smapi_match_it(map, fn_match_key_a_val_lt_100, NULL);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak1");
	assert_int_equal(it->val, 11);

	it = smapi_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak3");
	assert_int_equal(it->val, 13);

	assert_nul(smapi_it_next(it));

	smapi_free(map);
}

static void smapi_equal__case_sensitive(void **state) {

	const struct SMapI *actual = smapi_init();
	assert_false(smapi_put(actual, "a", 10));
	assert_false(smapi_put(actual, "b", 11));

	assert_smapi_not_equal(actual, NULL);

	const struct SMapI *expected = smapi_init();
	assert_false(smapi_put(expected, "a", 10));
	assert_false(smapi_put(expected, "b", 11));

	assert_smapi_equal(actual, expected);

	assert_false(smapi_put(actual, "c", 12));

	assert_smapi_not_equal(actual, expected);

	smapi_free(actual);
	smapi_free(expected);
}

static void smapi_equal__case_insensitive_key(void **state) {

	const struct SMapIParams params = { .case_insensitive_key = true, };
	const struct SMapI *actual = smapi_init_with(params);

	assert_false(smapi_put(actual, "a", 10));
	assert_false(smapi_put(actual, "b", 11));

	const struct SMapI *expected = smapi_init();
	assert_false(smapi_put(expected, "A", 10));
	assert_false(smapi_put(expected, "B", 11));

	assert_smapi_equal(actual, expected);

	smapi_free(actual);
	smapi_free(expected);
}

static void smapi_contains_key__(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_contains_key(map, "a"));

	assert_false(smapi_put(map, "a", 10));
	assert_false(smapi_put(map, "b", 11));

	assert_true(smapi_contains_key(map, "a"));
	assert_true(smapi_contains_key(map, "b"));

	assert_false(smapi_contains_key(map, "c"));

	assert_false(smapi_contains_key(map, NULL));

	smapi_free(map);
}

static void smapi_put_if_absent__(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put_if_absent(map, "a", 10));
	assert_int_equal(smapi_get(map, "a"), 10);

	assert_true(smapi_put_if_absent(map, "a", 10000));
	assert_int_equal(smapi_get(map, "a"), 10);

	assert_false(smapi_put_if_absent(map, "b", 11));
	assert_int_equal(smapi_get(map, "b"), 11);

	smapi_free(map);
}

static void smapi_str__(void **state) {

	const struct SMapI *map = smapi_init();
	assert_false(smapi_put(map, "a", 10));
	assert_false(smapi_put(map, "b", 11));
	assert_false(smapi_put(map, "c", 12));
	assert_false(smapi_put(map, "x", 0));

	char *expected = sprintf_alloc(
			"a = 10\n"
			"b = 11\n"
			"c = 12\n"
			"x = 0\n"
			);

	char *actual = smapi_str(map);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	smapi_free(map);
}

static void smapi_keys_slist_deep__many(void **state) {
	const struct SMapI *map = smapi_init();

	smapi_put(map, "a", 10);
	smapi_put(map, "b", 11);

	struct SList *list = smapi_keys_slist_deep(map);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "a");
	assert_str_equal(slist_at(list, 1), "b");

	smapi_free(map);
	slist_free_vals(&list, NULL);
}

// also tests constructor
static void smapi_clone__(void **state) {
	const struct SMapIParams params = {
		.case_insensitive_key = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SMapI *from = smapi_init_with(params);

	const struct SMapI *to = smapi_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->pmap->size, 0);
	assert_int_equal(to->pmap->capacity, 99);
	assert_int_equal(to->pmap->params.grow, 1);
	assert_ptr_equal(to->pmap->params.equal_key, fn_equal_strcasecmp);
	assert_ptr_equal(to->pmap->params.alloc_key, fn_clone_strdup);
	assert_ptr_equal(to->pmap->params.free_key, (fn_free)free);
	assert_ptr_equal(to->pmap->params.free_val, (fn_free)free);
	assert_ptr_equal(to->pmap->params.str_key, (fn_str)fn_str_or_null);

	assert_true(to->params.case_insensitive_key);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	assert_smapi_equal(from, to);

	smapi_free(from);
	smapi_free(to);
}

static void smapi__null_inputs(void **state) {
	const struct SMapI *map = smapi_init();

	assert_nul(smapi_clone(NULL));
	smapi_free(NULL);
	smapi_it_free(NULL);
	assert_false(smapi_get(NULL, NULL));
	assert_false(smapi_get(map, NULL));
	assert_false(smapi_getp(NULL, NULL, NULL));
	assert_false(smapi_getp(NULL, map, NULL));
	assert_false(smapi_contains_key(NULL, NULL));
	assert_false(smapi_contains_key(map, NULL));
	smapi_match(NULL, NULL, NULL);
	smapi_match(map, NULL, NULL);
	smapi_match(NULL, mock_match_smapi, NULL);
	assert_nul(smapi_it(NULL));
	assert_nul(smapi_match_it(NULL, NULL, NULL));
	assert_nul(smapi_match_it(map, NULL, NULL));
	assert_nul(smapi_match_it(map, mock_match_smapi, NULL));
	assert_nul(smapi_it_next(NULL));
	assert_false(smapi_put(NULL, NULL, 0));
	assert_false(smapi_put(map, NULL, 0));
	assert_false(smapi_put_if_absent(NULL, NULL, 0));
	assert_false(smapi_put_if_absent(map, NULL, 0));
	assert_false(smapi_remove(NULL, NULL));
	assert_false(smapi_remove(map, NULL));
	assert_false(smapi_equal(NULL, NULL));
	assert_false(smapi_equal(map, NULL));
	assert_nul(smapi_keys_slist_deep(NULL));
	assert_nul(smapi_str(NULL));
	assert_int_equal(smapi_size(NULL), 0);

	smapi_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(smapi_put_get_remove_free__case_sensitive),
		TEST(smapi_put_get_remove_free__case_insensitive),

		TEST(smapi_getp__zero),

		TEST(smapi_match__none),
		TEST(smapi_match__matches),

		TEST(smapi_it__many),
		TEST(smapi_it__empty),

		TEST(smapi_it_free__partial),

		TEST(smapi_it_next__partial),

		TEST(smapi_match_it__many),

		TEST(smapi_equal__case_sensitive),
		TEST(smapi_equal__case_insensitive_key),

		TEST(smapi_contains_key__),

		TEST(smapi_put_if_absent__),

		TEST(smapi_str__),

		TEST(smapi_keys_slist_deep__many),

		TEST(smapi_clone__),

		TEST(smapi__null_inputs),
	};

	return RUN(tests);
}


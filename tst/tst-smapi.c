#include "assert-smapi.h"
#include "assert-sset.h"
#include "asserts.h"
#include "mock-fn.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "pmap.h"
#include "slist.h"
#include "sset.h"
#include "str.h"

#include "smapi.h"

struct PMap {
	const struct PMapParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct SSet {
	const struct SSetParams params;
	const struct PSet *pset;
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
	assert_true(smapi_get_ptr(&a, map, "a"));
	assert_int_equal(a, 11);

	assert_int_equal(smapi_get(map, "x"), 0);

	size_t x = 99;
	assert_false(smapi_get_ptr(&x, map, "x"));
	assert_int_equal(x, 0);

	assert_false(smapi_put(map, "b", 0));

	size_t b = 99;
	assert_int_equal(smapi_get(map, "b"), 0);
	assert_true(smapi_get_ptr(&b, map, "b"));
	assert_int_equal(b, 0);

	smapi_free(map);
}

static void smapi_match__none(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "0", 10));

	// skip
	expect_string(mock_3pred_str_szt, str, "0");
	expect_int_value(mock_3pred_str_szt, n, 10);
	expect_string(mock_3pred_str_szt, data, "x");
	will_return(mock_3pred_str_szt, false);

	const struct SMapIPair kv_pair = smapi_match(map, mock_3pred_str_szt, "x");
	assert_nul(kv_pair.key);
	assert_int_equal(kv_pair.val, 0);

	smapi_free(map);
}

static void smapi_match_key__none(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "0", 10));

	// skip
	expect_string(mock_2pred_str, str, "0");
	expect_string(mock_2pred_str, data, "x");
	will_return(mock_2pred_str, false);

	const struct SMapIPair k_pair = smapi_match_key(map, mock_2pred_str, "x");
	assert_nul(k_pair.key);
	assert_int_equal(k_pair.val, 0);

	smapi_free(map);
}

static void smapi_match_val__none(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "0", 10));

	// skip
	expect_int_value(mock_2pred_szt, n, 10);
	expect_string(mock_2pred_szt, data, "x");
	will_return(mock_2pred_szt, false);

	const struct SMapIPair v_pair = smapi_match_val(map, mock_2pred_szt, "x");
	assert_nul(v_pair.key);
	assert_int_equal(v_pair.val, 0);

	smapi_free(map);
}

static void smapi_match__matches(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "0", 10));
	assert_false(smapi_put(map, "1", 11));
	assert_false(smapi_put(map, "2", 12));

	// skip 0
	expect_string(mock_3pred_str_szt, str, "0");
	expect_int_value(mock_3pred_str_szt, n, 10);
	expect_string(mock_3pred_str_szt, data, "x");
	will_return(mock_3pred_str_szt, false);

	// get 1
	expect_string(mock_3pred_str_szt, str, "1");
	expect_int_value(mock_3pred_str_szt, n, 11);
	expect_string(mock_3pred_str_szt, data, "x");
	will_return(mock_3pred_str_szt, true);

	const struct SMapIPair kv_pair = smapi_match(map, mock_3pred_str_szt, "x");
	assert_str_equal(kv_pair.key, "1");
	assert_int_equal(kv_pair.val, 11);

	smapi_free(map);
}

static void smapi_match_key__matches(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "0", 10));
	assert_false(smapi_put(map, "1", 11));
	assert_false(smapi_put(map, "2", 12));

	// skip 0
	expect_string(mock_2pred_str, str, "0");
	expect_string(mock_2pred_str, data, "x");
	will_return(mock_2pred_str, false);

	// get 1
	expect_string(mock_2pred_str, str, "1");
	expect_string(mock_2pred_str, data, "x");
	will_return(mock_2pred_str, true);

	const struct SMapIPair k_pair = smapi_match_key(map, mock_2pred_str, "x");
	assert_str_equal(k_pair.key, "1");
	assert_int_equal(k_pair.val, 11);

	smapi_free(map);
}

static void smapi_match_val__matches(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "0", 10));
	assert_false(smapi_put(map, "1", 11));
	assert_false(smapi_put(map, "2", 12));

	// skip 0
	expect_int_value(mock_2pred_szt, n, 10);
	expect_string(mock_2pred_szt, data, "x");
	will_return(mock_2pred_szt, false);

	// get 1
	expect_int_value(mock_2pred_szt, n, 11);
	expect_string(mock_2pred_szt, data, "x");
	will_return(mock_2pred_szt, true);

	const struct SMapIPair v_pair = smapi_match_val(map, mock_2pred_szt, "x");
	assert_str_equal(v_pair.key, "1");
	assert_int_equal(v_pair.val, 11);

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

static bool match_key_a(const char* const key, const void* const data) {
	return *key == 'a';
}

static bool match_key_a_val_lt_100(const char* const key, const size_t val, const void* const data) {
	return *key == 'a' && val < 100;
}

static bool match_val_lt_13(const size_t val, const void* const data) {
	return val < 13;
}

static void smapi_match_it__many(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "ak0", 100));
	assert_false(smapi_put(map, "ak1", 11));
	assert_false(smapi_put(map, "bk2", 12));
	assert_false(smapi_put(map, "ak3", 13));
	assert_false(smapi_put(map, "ak4", 101));

	const struct SMapIIt *it = smapi_match_it(map, match_key_a_val_lt_100, NULL);
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

static void smapi_match_key_it__many(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "bk0", 100));
	assert_false(smapi_put(map, "ak1", 11));
	assert_false(smapi_put(map, "bk2", 12));
	assert_false(smapi_put(map, "ak3", 13));
	assert_false(smapi_put(map, "bk4", 101));

	const struct SMapIIt *it = smapi_match_key_it(map, match_key_a, NULL);
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

static void smapi_match_val_it__many(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "ak0", 100));
	assert_false(smapi_put(map, "ak1", 11));
	assert_false(smapi_put(map, "bk2", 12));
	assert_false(smapi_put(map, "ak3", 13));
	assert_false(smapi_put(map, "ak4", 101));

	const struct SMapIIt *it = smapi_match_val_it(map, match_val_lt_13, NULL);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak1");
	assert_int_equal(it->val, 11);

	it = smapi_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "bk2");
	assert_int_equal(it->val, 12);

	assert_nul(smapi_it_next(it));

	smapi_free(map);
}

static void smapi_match_it__none(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "ak0", 100));
	assert_false(smapi_put(map, "ak1", 101));
	assert_false(smapi_put(map, "bk2", 102));
	assert_false(smapi_put(map, "ak3", 103));
	assert_false(smapi_put(map, "ak4", 104));

	assert_nul(smapi_match_it(map, match_key_a_val_lt_100, NULL));

	smapi_free(map);
}

static void smapi_match_key_it__none(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "bk3", 103));
	assert_false(smapi_put(map, "bk4", 104));

	assert_nul(smapi_match_key_it(map, match_key_a, NULL));

	smapi_free(map);
}

static void smapi_match_val_it__none(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_put(map, "ak0", 100));
	assert_false(smapi_put(map, "ak1", 101));

	assert_nul(smapi_match_val_it(map, match_val_lt_13, NULL));

	smapi_free(map);
}

static void smapi_match_it__empty(void **state) {
	const struct SMapI *map = smapi_init();

	assert_nul(smapi_match_it(map, mock_3pred_str_szt, NULL));

	smapi_free(map);
}

static void smapi_match_key_it__empty(void **state) {
	const struct SMapI *map = smapi_init();

	assert_nul(smapi_match_key_it(map, mock_2pred_str, NULL));

	smapi_free(map);
}

static void smapi_match_val_it__empty(void **state) {
	const struct SMapI *map = smapi_init();

	assert_nul(smapi_match_val_it(map, mock_2pred_szt, NULL));

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

static void smapi_contains_val__(void **state) {
	const struct SMapI *map = smapi_init();

	assert_false(smapi_contains_val(map, 10));

	assert_false(smapi_put(map, "a", 10));
	assert_false(smapi_put(map, "b", 11));

	assert_true(smapi_contains_val(map, 10));
	assert_true(smapi_contains_val(map, 11));

	assert_false(smapi_contains_val(map, 12));

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

static void smapi_put_all__many(void **state) {
	const struct SMapI *to = smapi_init();
	assert_false(smapi_put(to, "a", 0));
	assert_false(smapi_put(to, "b", 1));

	const struct SMapI *from = smapi_init();
	assert_false(smapi_put(from, "b", 2));
	assert_false(smapi_put(from, "c", 3));

	const struct SMapI *expected = smapi_init();
	assert_false(smapi_put(expected, "a", 0));
	assert_false(smapi_put(expected, "b", 2));
	assert_false(smapi_put(expected, "c", 3));

	assert_int_equal(smapi_put_all(to, from), 1);

	assert_smapi_equal(to, expected);

	smapi_free(to);
	smapi_free(from);
	smapi_free(expected);
}

static void smapi_put_many__many(void **state) {
	const struct SMapI *to = smapi_init();
	assert_false(smapi_put(to, "a", 0));
	assert_false(smapi_put(to, "b", 1));

	const struct SMapI *expected = smapi_init();
	assert_false(smapi_put(expected, "a", 0));
	assert_false(smapi_put(expected, "b", 1));
	assert_false(smapi_put(expected, "c", 2));

	assert_int_equal(smapi_put_many(to,
				"b", 1,
				"c", 2,
				NULL),
			1);

	assert_smapi_equal(to, expected);

	smapi_free(to);
	smapi_free(expected);
}

static void smapi_put_many__no_keyvals(void **state) {
	const struct SMapI *to = smapi_init();

	assert_int_equal(smapi_put_many(to, NULL), 0);

	smapi_free(to);
}

static void smapi_remove_all__(void **state) {
	const struct SMapI *map = smapi_init();

	smapi_put(map, "a", 0);
	smapi_put(map, "b", 1);
	smapi_put(map, "c", 2);

	const struct SMapI *from = smapi_init();

	smapi_put(from, "b", 1);
	smapi_put(from, "d", 3);

	const struct SMapI *expected = smapi_init();

	smapi_put(expected, "a", 0);
	smapi_put(expected, "c", 2);

	assert_int_equal(smapi_remove_all(map, from), 1);

	assert_smapi_equal(map, expected);

	smapi_free(map);
	smapi_free(from);
	smapi_free(expected);
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

static void smapi_keys_slist__many(void **state) {
	const struct SMapI *map = smapi_init();

	smapi_put(map, "a", 10);
	smapi_put(map, "b", 11);

	struct SList *list = smapi_keys_slist(map);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "a");
	assert_str_equal(slist_at(list, 1), "b");

	smapi_free(map);
	slist_free_vals(&list, NULL);
}

static void smapi_keys_sset__many(void **state) {
	const struct SMapI *map = smapi_init();

	smapi_put(map, "a", 0);
	smapi_put(map, "b", 1);

	const struct SSet *expected = sset_init();
	sset_add(expected, "a");
	sset_add(expected, "b");

	const struct SSet *actual = smapi_keys_sset(map);

	assert_sset_equal(actual, expected);

	smapi_free(map);
	sset_free(expected);
	sset_free(actual);
}

static void smapi_keys_sset__params(void **state) {
	const struct SMapIParams params = {
		.case_insensitive_key = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SMapI *map = smapi_init_with(params);

	const struct SSet *set = smapi_keys_sset(map);

	assert_true(set->params.case_insensitive);
	assert_int_equal(set->params.initial, 99);
	assert_int_equal(set->params.grow, 1);

	smapi_free(map);

	sset_free(set);
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
	assert_false(to->pmap->params.allow_null_val);
	assert_int_equal(to->pmap->params.grow, 1);
	assert_ptr_equal(to->pmap->params.equal_key, equal_strcasecmp);
	assert_ptr_equal(to->pmap->params.alloc_key, clone_strdup);
	assert_ptr_equal(to->pmap->params.free_key, (fn_free)free);
	assert_ptr_equal(to->pmap->params.free_val, (fn_free)free);
	assert_ptr_equal(to->pmap->params.str_key, (fn_str)str_or_null);

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
	assert_false(smapi_get_ptr(NULL, NULL, NULL));
	assert_false(smapi_get_ptr(NULL, map, NULL));
	assert_false(smapi_contains_key(NULL, NULL));
	assert_false(smapi_contains_key(map, NULL));
	assert_false(smapi_contains_val(NULL, 0));
	assert_false(smapi_contains_val(map, 0));
	smapi_match(NULL, NULL, NULL);
	smapi_match(map, NULL, NULL);
	smapi_match_key(NULL, NULL, NULL);
	smapi_match_key(map, NULL, NULL);
	smapi_match_val(NULL, NULL, NULL);
	smapi_match_val(map, NULL, NULL);
	assert_nul(smapi_it(NULL));
	assert_nul(smapi_match_it(NULL, NULL, NULL));
	assert_nul(smapi_match_it(map, NULL, NULL));
	assert_nul(smapi_match_key_it(NULL, NULL, NULL));
	assert_nul(smapi_match_key_it(map, NULL, NULL));
	assert_nul(smapi_match_val_it(NULL, NULL, NULL));
	assert_nul(smapi_match_val_it(map, NULL, NULL));
	assert_nul(smapi_it_next(NULL));
	assert_false(smapi_put(NULL, NULL, 0));
	assert_false(smapi_put(map, NULL, 0));
	assert_false(smapi_put_if_absent(NULL, NULL, 0));
	assert_false(smapi_put_if_absent(map, NULL, 0));
	assert_int_equal(smapi_put_all(NULL, NULL), 0);
	assert_int_equal(smapi_put_all(map, NULL), 0);
	assert_int_equal(smapi_put_many(NULL, NULL), 0);
	assert_false(smapi_remove(NULL, NULL));
	assert_false(smapi_remove(map, NULL));
	assert_int_equal(smapi_remove_all(NULL, NULL), 0);
	assert_int_equal(smapi_remove_all(map, NULL), 0);
	assert_int_equal(smapi_remove_all(NULL, map), 0);
	assert_false(smapi_equal(NULL, NULL));
	assert_false(smapi_equal(map, NULL));
	assert_nul(smapi_keys_slist(NULL));
	assert_nul(smapi_keys_sset(NULL));
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
		TEST(smapi_match_key__none),
		TEST(smapi_match_val__none),

		TEST(smapi_match__matches),
		TEST(smapi_match_key__matches),
		TEST(smapi_match_val__matches),

		TEST(smapi_it__many),
		TEST(smapi_it__empty),

		TEST(smapi_it_free__partial),

		TEST(smapi_it_next__partial),

		TEST(smapi_match_it__many),
		TEST(smapi_match_key_it__many),
		TEST(smapi_match_val_it__many),

		TEST(smapi_match_it__none),
		TEST(smapi_match_key_it__none),
		TEST(smapi_match_val_it__none),

		TEST(smapi_match_it__empty),
		TEST(smapi_match_key_it__empty),
		TEST(smapi_match_val_it__empty),

		TEST(smapi_equal__case_sensitive),
		TEST(smapi_equal__case_insensitive_key),

		TEST(smapi_contains_key__),

		TEST(smapi_contains_val__),

		TEST(smapi_put_if_absent__),

		TEST(smapi_put_all__many),

		TEST(smapi_put_many__many),
		TEST(smapi_put_many__no_keyvals),

		TEST(smapi_remove_all__),

		TEST(smapi_str__),

		TEST(smapi_keys_slist__many),

		TEST(smapi_keys_sset__many),
		TEST(smapi_keys_sset__params),

		TEST(smapi_clone__),

		TEST(smapi__null_inputs),
	};

	return RUN(tests);
}


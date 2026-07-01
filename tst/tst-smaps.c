#include "assert-smaps.h"
#include "assert-sset.h"
#include "asserts.h"
#include "mock-fn.h"
#include "tst.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "pmap.h"
#include "slist.h"
#include "sset.h"
#include "str.h"

#include "smaps.h"

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

struct SMapS {
	const struct SMapSParams params;
	const struct PMap *pmap;
};

static void smaps_put_get_remove_free__case_sensitive(void **state) {
	const struct SMapSParams params = { .allow_null_val = true, };
	const struct SMapS *map = smaps_init_with(params);

	assert_false(smaps_put(map, "a", "A"));
	assert_false(smaps_put(map, "b", "B"));
	assert_false(smaps_put(map, "c", "C"));
	assert_false(smaps_put(map, "d", NULL));

	assert_true(smaps_put(map, "c", "duplicate"));

	assert_int_equal(smaps_size(map), 4);

	assert_str_equal(smaps_get(map, "b"), "B");

	assert_nul(smaps_get(map, "d"));
	assert_true(smaps_contains_key(map, "d"));

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

	//
	// key/val
	//

	// skip 0
	expect_string(mock_match_str_str, key, "0");
	expect_string(mock_match_str_str, val, "aaa");
	expect_string(mock_match_str_str, data, "x");
	will_return(mock_match_str_str, false);

	// get 1
	expect_string(mock_match_str_str, key, "1");
	expect_string(mock_match_str_str, val, "bbb");
	expect_string(mock_match_str_str, data, "x");
	will_return(mock_match_str_str, true);

	const struct SMapSPair kv_pair = smaps_match(map, mock_match_str_str, "x");
	assert_str_equal(kv_pair.key, "1");
	assert_str_equal(kv_pair.val, "bbb");

	//
	// val
	//

	// skip 0
	expect_string(mock_match_str, val, "aaa");
	expect_string(mock_match_str, data, "x");
	will_return(mock_match_str, false);

	// get 1
	expect_string(mock_match_str, val, "bbb");
	expect_string(mock_match_str, data, "x");
	will_return(mock_match_str, true);

	const struct SMapSPair v_pair = smaps_match_val(map, mock_match_str, "x");
	assert_str_equal(v_pair.key, "1");
	assert_str_equal(v_pair.val, "bbb");

	smaps_free(map);
}

static void smaps_it__many(void **state) {
	const struct SMapSParams params = { .allow_null_val = true, };
	const struct SMapS *map = smaps_init_with(params);

	assert_false(smaps_put(map, "a", "aa"));
	assert_false(smaps_put(map, "b", NULL));
	assert_false(smaps_put(map, "c", "cc"));

	const struct SMapSIt *it = smaps_it(map);

	assert_non_nul(it);
	assert_str_equal(it->key, "a");
	assert_str_equal(it->val, "aa");

	it = smaps_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "b");
	assert_nul(it->val);

	smaps_it_free(it);

	smaps_free(map);
}

static void smaps_it__empty(void **state) {

	const struct SMapS *map = smaps_init();

	const struct SMapSIt *it = smaps_it(map);

	assert_nul(it);

	smaps_free(map);
}

static void smaps_it_free__partial(void **state) {
	const struct SMapSIt *it = calloc(1, sizeof(struct SMapSIt));

	smaps_it_free(it);
}

static void smaps_it_next__partial(void **state) {
	const struct SMapSIt *it = calloc(1, sizeof(struct SMapSIt));

	assert_nul(smaps_it_next(it));
}

static bool match_both_start_with_a(const char* const key, const char* const val, const void* const data) {
	return *key == 'a' && *val == 'a';
}

static bool match_val_start_with_b(const char* const val, const void* const data) {
	return *val == 'b';
}

static void smaps_match_it__many(void **state) {
	const struct SMapS *map = smaps_init();

	assert_false(smaps_put(map, "ak0", "bv0"));
	assert_false(smaps_put(map, "ak1", "av1"));
	assert_false(smaps_put(map, "bk2", "av2"));
	assert_false(smaps_put(map, "ak3", "av3"));
	assert_false(smaps_put(map, "ak4", "bv4"));

	//
	// key/val
	//

	const struct SMapSIt *it = smaps_match_it(map, match_both_start_with_a, NULL);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak1");
	assert_str_equal(it->val, "av1");

	it = smaps_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak3");
	assert_str_equal(it->val, "av3");

	assert_nul(smaps_it_next(it));

	//
	// val
	//

	it = smaps_match_val_it(map, match_val_start_with_b, NULL);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak0");
	assert_str_equal(it->val, "bv0");

	it = smaps_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak4");
	assert_str_equal(it->val, "bv4");

	assert_nul(smaps_it_next(it));

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

static void smaps_contains_val__(void **state) {
	const struct SMapS *map = smaps_init();

	assert_false(smaps_contains_key(map, "aa"));

	assert_false(smaps_put(map, "a", "aa"));
	assert_false(smaps_put(map, "b", "bb"));

	assert_true(smaps_contains_val(map, "aa"));
	assert_true(smaps_contains_val(map, "bb"));

	assert_false(smaps_contains_val(map, "c"));

	assert_false(smaps_contains_val(map, NULL));

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
	const struct SMapSParams params = { .allow_null_val = true, };
	const struct SMapS *map = smaps_init_with(params);

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

static void smaps_keys_sset__many(void **state) {
	const struct SMapS *map = smaps_init();

	smaps_put(map, "a", "aa");
	smaps_put(map, "b", "bb");

	const struct SSet *expected = sset_init();
	sset_add(expected, "a");
	sset_add(expected, "b");

	const struct SSet *actual = smaps_keys_sset(map);

	assert_sset_equal(actual, expected);

	smaps_free(map);
	sset_free(expected);
	sset_free(actual);
}

static void smaps_keys_sset__params(void **state) {
	const struct SMapSParams params = {
		.case_insensitive_key = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SMapS *map = smaps_init_with(params);

	const struct SSet *set = smaps_keys_sset(map);

	assert_true(set->params.case_insensitive);
	assert_int_equal(set->params.initial, 99);
	assert_int_equal(set->params.grow, 1);

	smaps_free(map);

	sset_free(set);
}

static void smaps_vals_slist_deep__many(void **state) {
	const struct SMapSParams params = { .allow_null_val = true, };
	const struct SMapS *map = smaps_init_with(params);

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

static void smaps_vals_sset__many(void **state) {
	const struct SMapS *map = smaps_init();

	smaps_put(map, "a", "aa");
	smaps_put(map, "b", "bb");

	const struct SSet *expected = sset_init();
	sset_add(expected, "aa");
	sset_add(expected, "bb");

	const struct SSet *actual = smaps_vals_sset(map);

	assert_sset_equal(actual, expected);

	smaps_free(map);
	sset_free(expected);
	sset_free(actual);
}

static void smaps_vals_sset__params(void **state) {
	const struct SMapSParams params = {
		.case_insensitive_val = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SMapS *map = smaps_init_with(params);

	const struct SSet *set = smaps_vals_sset(map);

	assert_true(set->params.case_insensitive);
	assert_int_equal(set->params.initial, 99);
	assert_int_equal(set->params.grow, 1);

	smaps_free(map);

	sset_free(set);
}

// also tests constructor
static void smaps_clone__(void **state) {
	const struct SMapSParams params = {
		.case_insensitive_key = true,
		.case_insensitive_val = true,
		.allow_null_val = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SMapS *from = smaps_init_with(params);

	const struct SMapS *to = smaps_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->pmap->size, 0);
	assert_int_equal(to->pmap->capacity, 99);
	assert_int_equal(to->pmap->params.grow, 1);
	assert_true(to->pmap->params.allow_null_val);
	assert_ptr_equal(to->pmap->params.equal_key, equal_strcasecmp);
	assert_ptr_equal(to->pmap->params.equal_val, equal_strcasecmp);
	assert_ptr_equal(to->pmap->params.alloc_key, clone_strdup);
	assert_ptr_equal(to->pmap->params.alloc_val, clone_strdup);
	assert_ptr_equal(to->pmap->params.free_key, (fn_free)free);
	assert_ptr_equal(to->pmap->params.free_val, (fn_free)free);
	assert_ptr_equal(to->pmap->params.clone_val, clone_strdup);

	assert_true(to->params.case_insensitive_key);
	assert_true(to->params.case_insensitive_val);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	assert_smaps_equal(from, to);

	smaps_free(from);
	smaps_free(to);
}

static void smaps__null_inputs(void **state) {
	const struct SMapS *map = smaps_init();

	assert_nul(smaps_clone(NULL));
	smaps_free(NULL);
	smaps_it_free(NULL);
	assert_false(smaps_get(NULL, NULL));
	assert_false(smaps_get(map, NULL));
	assert_false(smaps_contains_key(NULL, NULL));
	assert_false(smaps_contains_key(map, NULL));
	assert_false(smaps_contains_val(NULL, NULL));
	assert_false(smaps_contains_val(map, NULL));
	smaps_match(NULL, NULL, NULL);
	smaps_match(map, NULL, NULL);
	smaps_match_val(NULL, NULL, NULL);
	smaps_match_val(map, NULL, NULL);
	assert_nul(smaps_it(NULL));
	assert_nul(smaps_match_it(NULL, NULL, NULL));
	assert_nul(smaps_match_it(map, NULL, NULL));
	assert_nul(smaps_match_val_it(NULL, NULL, NULL));
	assert_nul(smaps_match_val_it(map, NULL, NULL));
	assert_nul(smaps_it_next(NULL));
	assert_false(smaps_put(NULL, NULL, NULL));
	assert_false(smaps_put(map, NULL, NULL));
	assert_false(smaps_put_if_absent(NULL, NULL, NULL));
	assert_false(smaps_put_if_absent(map, NULL, NULL));
	assert_false(smaps_remove(NULL, NULL));
	assert_false(smaps_remove(map, NULL));
	assert_false(smaps_equal(NULL, NULL));
	assert_false(smaps_equal(map, NULL));
	assert_nul(smaps_keys_slist_deep(NULL));
	assert_nul(smaps_keys_sset(NULL));
	assert_nul(smaps_vals_slist_deep(NULL));
	assert_nul(smaps_vals_sset(NULL));
	assert_nul(smaps_str(NULL));
	assert_int_equal(smaps_size(NULL), 0);

	smaps_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(smaps_put_get_remove_free__case_sensitive),
		TEST(smaps_put_get_remove_free__case_insensitive),

		TEST(smaps_match__matches),

		TEST(smaps_it__many),
		TEST(smaps_it__empty),

		TEST(smaps_it_free__partial),

		TEST(smaps_it_next__partial),

		TEST(smaps_match_it__many),

		TEST(smaps_equal__case_sensitive),
		TEST(smaps_equal__case_insensitive_key),
		TEST(smaps_equal__case_insensitive_val),

		TEST(smaps_contains_key__),

		TEST(smaps_contains_val__),

		TEST(smaps_put_if_absent__),

		TEST(smaps_str__),

		TEST(smaps_keys_slist_deep__many),

		TEST(smaps_keys_sset__many),
		TEST(smaps_keys_sset__params),

		TEST(smaps_vals_slist_deep__many),

		TEST(smaps_vals_sset__many),
		TEST(smaps_vals_sset__params),

		TEST(smaps_clone__),

		TEST(smaps__null_inputs),
	};

	return RUN(tests);
}


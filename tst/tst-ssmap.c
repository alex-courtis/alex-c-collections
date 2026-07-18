#include "assert-sset.h"
#include "assert-ssmap.h"
#include "asserts.h"
#include "mock-fn.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "ppmap.h"
#include "pslist.h"
#include "sset.h"
#include "str.h"

#include "ssmap.h"

struct PPmap {
	const struct PPmapParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct Sset {
	const struct SsetParams params;
	const struct Pset *pset;
};

struct SSmap {
	const struct SSmapParams params;
	const struct PPmap *ppmap;
};

static void ssmap_put_get_remove_free__case_sensitive(void **state) {
	const struct SSmapParams params = { .allow_null_val = true, };
	const struct SSmap *map = ssmap_init_with(params);

	assert_false(ssmap_put(map, "a", "A"));
	assert_false(ssmap_put(map, "b", "B"));
	assert_false(ssmap_put(map, "c", "C"));
	assert_false(ssmap_put(map, "d", NULL));

	assert_true(ssmap_put(map, "c", "duplicate"));

	assert_int_equal(ssmap_size(map), 4);

	assert_str_equal(ssmap_get(map, "b"), "B");

	assert_nul(ssmap_get(map, "d"));
	assert_true(ssmap_contains_key(map, "d"));

	assert_nul(ssmap_get(map, "x"));

	assert_true(ssmap_remove(map, "b"));
	assert_false(ssmap_remove(map, "b"));

	assert_nul(ssmap_get(map, "b"));

	assert_true(ssmap_put(map, "a", NULL));

	ssmap_free(map);
}

static void ssmap_put_get_remove_free__case_insensitive(void **state) {
	const struct SSmapParams params = { .case_insensitive_key = true, };
	const struct SSmap *map = ssmap_init_with(params);

	assert_false(ssmap_put(map, "A", "aaa"));
	assert_false(ssmap_put(map, "B", "bbb"));

	assert_str_equal(ssmap_get(map, "b"), "bbb");

	assert_nul(ssmap_get(map, "x"));

	assert_true(ssmap_remove(map, "b"));

	assert_nul(ssmap_get(map, "b"));

	ssmap_free(map);
}

static void ssmap_find__matches(void **state) {
	const struct SSmap *map = ssmap_init();

	assert_false(ssmap_put(map, "0", "aaa"));
	assert_false(ssmap_put(map, "1", "bbb"));
	assert_false(ssmap_put(map, "2", "ccc"));

	// skip 0
	expect_string(mock_3pred_str_str, str1, "0");
	expect_string(mock_3pred_str_str, str2, "aaa");
	expect_string(mock_3pred_str_str, data, "x");
	will_return(mock_3pred_str_str, false);

	// get 1
	expect_string(mock_3pred_str_str, str1, "1");
	expect_string(mock_3pred_str_str, str2, "bbb");
	expect_string(mock_3pred_str_str, data, "x");
	will_return(mock_3pred_str_str, true);

	const struct SSmapPair kv_pair = ssmap_find(map, mock_3pred_str_str, "x");
	assert_str_equal(kv_pair.key, "1");
	assert_str_equal(kv_pair.val, "bbb");


	ssmap_free(map);
}

static void ssmap_find_key__matches(void **state) {
	const struct SSmap *map = ssmap_init();

	assert_false(ssmap_put(map, "0", "aaa"));
	assert_false(ssmap_put(map, "1", "bbb"));
	assert_false(ssmap_put(map, "2", "ccc"));

	// skip 0
	expect_string(mock_2pred_str, str, "0");
	expect_string(mock_2pred_str, data, "x");
	will_return(mock_2pred_str, false);

	// get 1
	expect_string(mock_2pred_str, str, "1");
	expect_string(mock_2pred_str, data, "x");
	will_return(mock_2pred_str, true);

	const struct SSmapPair k_pair = ssmap_find_key(map, mock_2pred_str, "x");
	assert_str_equal(k_pair.key, "1");
	assert_str_equal(k_pair.val, "bbb");

	ssmap_free(map);
}

static void ssmap_find_val__matches(void **state) {
	const struct SSmap *map = ssmap_init();

	assert_false(ssmap_put(map, "0", "aaa"));
	assert_false(ssmap_put(map, "1", "bbb"));
	assert_false(ssmap_put(map, "2", "ccc"));

	// skip 0
	expect_string(mock_2pred_str, str, "aaa");
	expect_string(mock_2pred_str, data, "x");
	will_return(mock_2pred_str, false);

	// get 1
	expect_string(mock_2pred_str, str, "bbb");
	expect_string(mock_2pred_str, data, "x");
	will_return(mock_2pred_str, true);

	const struct SSmapPair v_pair = ssmap_find_val(map, mock_2pred_str, "x");
	assert_str_equal(v_pair.key, "1");
	assert_str_equal(v_pair.val, "bbb");

	ssmap_free(map);
}

static void ssmap_it__many(void **state) {
	const struct SSmapParams params = { .allow_null_val = true, };
	const struct SSmap *map = ssmap_init_with(params);

	assert_false(ssmap_put(map, "a", "aa"));
	assert_false(ssmap_put(map, "b", NULL));
	assert_false(ssmap_put(map, "c", "cc"));

	const struct SSmapIt *it = ssmap_it(map);

	assert_non_nul(it);
	assert_str_equal(it->key, "a");
	assert_str_equal(it->val, "aa");

	it = ssmap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "b");
	assert_nul(it->val);

	ssmap_it_free(it);

	ssmap_free(map);
}

static void ssmap_it__empty(void **state) {

	const struct SSmap *map = ssmap_init();

	const struct SSmapIt *it = ssmap_it(map);

	assert_nul(it);

	ssmap_free(map);
}

static void ssmap_it_free__partial(void **state) {
	const struct SSmapIt *it = calloc(1, sizeof(struct SSmapIt));

	ssmap_it_free(it);
}

static void ssmap_it_next__partial(void **state) {
	const struct SSmapIt *it = calloc(1, sizeof(struct SSmapIt));

	assert_nul(ssmap_it_next(it));
}

static bool match_both_start_with_a(const char* const key, const char* const val, const void* const data) {
	return *key == 'a' && *val == 'a';
}

static bool match_val_start_with_b(const char* const val, const void* const data) {
	return *val == 'b';
}

static void ssmap_filter_it__many(void **state) {
	const struct SSmap *map = ssmap_init();

	assert_false(ssmap_put(map, "ak0", "bv0"));
	assert_false(ssmap_put(map, "ak1", "av1"));
	assert_false(ssmap_put(map, "bk2", "av2"));
	assert_false(ssmap_put(map, "ak3", "av3"));
	assert_false(ssmap_put(map, "ak4", "bv4"));

	const struct SSmapIt *it = ssmap_filter_it(map, match_both_start_with_a, NULL);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak1");
	assert_str_equal(it->val, "av1");

	it = ssmap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak3");
	assert_str_equal(it->val, "av3");

	assert_nul(ssmap_it_next(it));

	ssmap_free(map);
}

static void ssmap_key_filter_it__many(void **state) {
	const struct SSmap *map = ssmap_init();

	assert_false(ssmap_put(map, "ak0", "bv0"));
	assert_false(ssmap_put(map, "ak1", "av1"));
	assert_false(ssmap_put(map, "bk2", "av2"));
	assert_false(ssmap_put(map, "ak3", "av3"));
	assert_false(ssmap_put(map, "ak4", "bv4"));

	const struct SSmapIt *it = ssmap_key_filter_it(map, match_val_start_with_b, NULL);
	assert_non_nul(it);
	assert_str_equal(it->key, "bk2");
	assert_str_equal(it->val, "av2");

	assert_nul(ssmap_it_next(it));

	ssmap_free(map);
}

static void ssmap_val_filter_it__many(void **state) {
	const struct SSmap *map = ssmap_init();

	assert_false(ssmap_put(map, "ak0", "bv0"));
	assert_false(ssmap_put(map, "ak1", "av1"));
	assert_false(ssmap_put(map, "bk2", "av2"));
	assert_false(ssmap_put(map, "ak3", "av3"));
	assert_false(ssmap_put(map, "ak4", "bv4"));

	const struct SSmapIt *it = ssmap_val_filter_it(map, match_val_start_with_b, NULL);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak0");
	assert_str_equal(it->val, "bv0");

	it = ssmap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak4");
	assert_str_equal(it->val, "bv4");

	assert_nul(ssmap_it_next(it));

	ssmap_free(map);
}

static void ssmap_equal__case_sensitive(void **state) {

	const struct SSmap *actual = ssmap_init();
	assert_false(ssmap_put(actual, "a", "aa"));
	assert_false(ssmap_put(actual, "b", "aa"));

	assert_ssmap_not_equal(actual, NULL);

	const struct SSmap *expected = ssmap_init();
	assert_false(ssmap_put(expected, "a", "aa"));
	assert_false(ssmap_put(expected, "b", "aa"));

	assert_ssmap_equal(actual, expected);

	assert_false(ssmap_put(actual, "c", "cc"));

	assert_ssmap_not_equal(actual, expected);

	ssmap_free(actual);
	ssmap_free(expected);
}

static void ssmap_equal__case_insensitive_key(void **state) {

	const struct SSmapParams params = { .case_insensitive_key = true, };
	const struct SSmap *actual = ssmap_init_with(params);

	assert_false(ssmap_put(actual, "a", "aa"));
	assert_false(ssmap_put(actual, "b", "bb"));

	const struct SSmap *expected = ssmap_init();
	assert_false(ssmap_put(expected, "A", "aa"));
	assert_false(ssmap_put(expected, "B", "bb"));

	assert_ssmap_equal(actual, expected);

	ssmap_free(actual);
	ssmap_free(expected);
}

static void ssmap_equal__case_insensitive_val(void **state) {

	const struct SSmapParams params = { .case_insensitive_val = true, };
	const struct SSmap *actual = ssmap_init_with(params);

	assert_false(ssmap_put(actual, "a", "aa"));
	assert_false(ssmap_put(actual, "b", "bb"));

	const struct SSmap *expected = ssmap_init();
	assert_false(ssmap_put(expected, "a", "AA"));
	assert_false(ssmap_put(expected, "b", "BB"));

	assert_ssmap_equal(actual, expected);

	ssmap_free(actual);
	ssmap_free(expected);
}

static void ssmap_contains_key__(void **state) {
	const struct SSmap *map = ssmap_init();

	assert_false(ssmap_contains_key(map, "a"));

	assert_false(ssmap_put(map, "a", "aa"));
	assert_false(ssmap_put(map, "b", "bb"));

	assert_true(ssmap_contains_key(map, "a"));
	assert_true(ssmap_contains_key(map, "b"));

	assert_false(ssmap_contains_key(map, "c"));

	assert_false(ssmap_contains_key(map, NULL));

	ssmap_free(map);
}

static void ssmap_contains_val__(void **state) {
	const struct SSmap *map = ssmap_init();

	assert_false(ssmap_contains_key(map, "aa"));

	assert_false(ssmap_put(map, "a", "aa"));
	assert_false(ssmap_put(map, "b", "bb"));

	assert_true(ssmap_contains_val(map, "aa"));
	assert_true(ssmap_contains_val(map, "bb"));

	assert_false(ssmap_contains_val(map, "c"));

	assert_false(ssmap_contains_val(map, NULL));

	ssmap_free(map);
}

static void ssmap_at__(void **state) {
	const struct SSmap *map = ssmap_init();
	assert_false(ssmap_put(map, "a", "aa"));
	assert_false(ssmap_put(map, "b", "bb"));
	assert_false(ssmap_put(map, "c", "cc"));

	assert_str_equal(ssmap_at(map, 1).key, "b");
	assert_str_equal(ssmap_at(map, 1).val, "bb");

	ssmap_free(map);
}

static void ssmap_put_if_absent__(void **state) {
	const struct SSmap *map = ssmap_init();

	assert_false(ssmap_put_if_absent(map, "a", "aa"));
	assert_str_equal(ssmap_get(map, "a"), "aa");

	assert_true(ssmap_put_if_absent(map, "a", "xx"));

	assert_false(ssmap_put_if_absent(map, "b", NULL));
	assert_nul(ssmap_get(map, "b"));

	ssmap_free(map);
}

static void ssmap_put_all__many(void **state) {
	const struct SSmap *to = ssmap_init();
	assert_false(ssmap_put(to, "a", "0"));
	assert_false(ssmap_put(to, "b", "1"));

	const struct SSmap *from = ssmap_init();
	assert_false(ssmap_put(from, "b", "11"));
	assert_false(ssmap_put(from, "c", "2"));

	const struct SSmap *expected = ssmap_init();
	assert_false(ssmap_put(expected, "a", "0"));
	assert_false(ssmap_put(expected, "b", "11"));
	assert_false(ssmap_put(expected, "c", "2"));

	assert_int_equal(ssmap_put_all(to, from), 1);

	assert_ssmap_equal(to, expected);

	ssmap_free(to);
	ssmap_free(from);
	ssmap_free(expected);
}

static void ssmap_put_many__many(void **state) {
	const struct SSmap *to = ssmap_init();
	assert_false(ssmap_put(to, "a", "0"));
	assert_false(ssmap_put(to, "b", "1"));

	const struct SSmap *expected = ssmap_init();
	assert_false(ssmap_put(expected, "a", "0"));
	assert_false(ssmap_put(expected, "b", "1"));
	assert_false(ssmap_put(expected, "c", "2"));

	assert_int_equal(ssmap_put_many(to,
				"b", "1",
				"c", "2",
				NULL),
			1);

	assert_ssmap_equal(to, expected);

	ssmap_free(to);
	ssmap_free(expected);
}

static void ssmap_remove_all__(void **state) {
	const struct SSmap *map = ssmap_init();

	ssmap_put(map, "a", "0");
	ssmap_put(map, "b", "1");
	ssmap_put(map, "c", "2");

	const struct SSmap *from = ssmap_init();

	ssmap_put(from, "b", "1");
	ssmap_put(from, "d", "3");

	const struct SSmap *expected = ssmap_init();

	ssmap_put(expected, "a", "0");
	ssmap_put(expected, "c", "2");

	assert_int_equal(ssmap_remove_all(map, from), 1);

	assert_ssmap_equal(map, expected);

	ssmap_free(map);
	ssmap_free(from);
	ssmap_free(expected);
}

static void ssmap_it_remove__many(void **state) {
	const struct SSmap *map = ssmap_init();

	assert_false(ssmap_put(map, "a", "0"));
	assert_false(ssmap_put(map, "b", "1"));
	assert_false(ssmap_put(map, "c", "2"));
	assert_false(ssmap_put(map, "d", "3"));
	assert_false(ssmap_put(map, "e", "4"));

	const struct SSmap *expected = ssmap_init();

	assert_false(ssmap_put(expected, "b", "1"));
	assert_false(ssmap_put(expected, "d", "3"));

	size_t iterations = 0;
	for (const struct SSmapIt *it = ssmap_it(map); it; it = ssmap_it_next(it)) {
		iterations++;
		if (strcmp(it->key, "a") == 0 || strcmp(it->key, "c") == 0 || strcmp(it->key, "e") == 0) {
			ssmap_it_remove(it);
		}
	}

	assert_int_equal(ssmap_size(map), 2);
	assert_int_equal(iterations, 5);

	assert_ssmap_equal(map, expected);

	ssmap_free(map);
	ssmap_free(expected);
}

static void ssmap_it_remove__partial(void **state) {
	const struct SSmapIt *it = calloc(1, sizeof(struct SSmapIt));

	ssmap_it_remove(it);
}

static void ssmap_str__(void **state) {
	const struct SSmapParams params = { .allow_null_val = true, };
	const struct SSmap *map = ssmap_init_with(params);

	assert_false(ssmap_put(map, "a", "aa"));
	assert_false(ssmap_put(map, "b", NULL));
	assert_false(ssmap_put(map, "c", "cc"));

	char *expected = sprintf_alloc(
			"a = aa\n"
			"b = (null)\n"
			"c = cc\n"
			);

	char *actual = ssmap_str(map);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	ssmap_free(map);
}

static void ssmap_keys_pslist__many(void **state) {
	const struct SSmap *map = ssmap_init();

	ssmap_put(map, "a", "aa");
	ssmap_put(map, "b", "bb");

	struct Pslist *list = ssmap_keys_pslist(map);

	assert_int_equal(pslist_length(list), 2);
	assert_str_equal(pslist_at(list, 0), "a");
	assert_str_equal(pslist_at(list, 1), "b");

	ssmap_free(map);
	pslist_free_vals(&list, NULL);
}

static void ssmap_keys_sset__many(void **state) {
	const struct SSmap *map = ssmap_init();

	ssmap_put(map, "a", "aa");
	ssmap_put(map, "b", "bb");

	const struct Sset *expected = sset_init();
	sset_add(expected, "a");
	sset_add(expected, "b");

	const struct Sset *actual = ssmap_keys_sset(map);

	assert_sset_equal(actual, expected);

	ssmap_free(map);
	sset_free(expected);
	sset_free(actual);
}

static void ssmap_keys_sset__params(void **state) {
	const struct SSmapParams params = {
		.case_insensitive_key = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SSmap *map = ssmap_init_with(params);

	const struct Sset *set = ssmap_keys_sset(map);

	assert_true(set->params.case_insensitive);
	assert_int_equal(set->params.initial, 99);
	assert_int_equal(set->params.grow, 1);

	ssmap_free(map);

	sset_free(set);
}

static void ssmap_vals_pslist__many(void **state) {
	const struct SSmapParams params = { .allow_null_val = true, };
	const struct SSmap *map = ssmap_init_with(params);

	ssmap_put(map, "a", "aa");
	ssmap_put(map, "b", NULL);
	ssmap_put(map, "c", "cc");

	struct Pslist *list = ssmap_vals_pslist(map);

	assert_int_equal(pslist_length(list), 3);
	assert_str_equal(pslist_at(list, 0), "aa");
	assert_nul(pslist_at(list, 1));
	assert_str_equal(pslist_at(list, 2), "cc");

	pslist_free_vals(&list, NULL);
	ssmap_free(map);
}

static void ssmap_vals_sset__many(void **state) {
	const struct SSmap *map = ssmap_init();

	ssmap_put(map, "a", "aa");
	ssmap_put(map, "b", "bb");

	const struct Sset *expected = sset_init();
	sset_add(expected, "aa");
	sset_add(expected, "bb");

	const struct Sset *actual = ssmap_vals_sset(map);

	assert_sset_equal(actual, expected);

	ssmap_free(map);
	sset_free(expected);
	sset_free(actual);
}

static void ssmap_vals_sset__params(void **state) {
	const struct SSmapParams params = {
		.case_insensitive_val = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SSmap *map = ssmap_init_with(params);

	const struct Sset *set = ssmap_vals_sset(map);

	assert_true(set->params.case_insensitive);
	assert_int_equal(set->params.initial, 99);
	assert_int_equal(set->params.grow, 1);

	ssmap_free(map);

	sset_free(set);
}

// also tests constructor
static void ssmap_clone__(void **state) {
	const struct SSmapParams params = {
		.case_insensitive_key = true,
		.case_insensitive_val = true,
		.allow_null_val = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SSmap *from = ssmap_init_with(params);

	const struct SSmap *to = ssmap_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->ppmap->size, 0);
	assert_int_equal(to->ppmap->capacity, 99);
	assert_int_equal(to->ppmap->params.grow, 1);
	assert_true(to->ppmap->params.allow_null_val);
	assert_ptr_equal(to->ppmap->params.equal_key, equal_strcasecmp);
	assert_ptr_equal(to->ppmap->params.equal_val, equal_strcasecmp);
	assert_ptr_equal(to->ppmap->params.alloc_key, clone_strdup);
	assert_ptr_equal(to->ppmap->params.alloc_val, clone_strdup);
	assert_ptr_equal(to->ppmap->params.free_key, free);
	assert_ptr_equal(to->ppmap->params.free_val, free);

	assert_true(to->params.case_insensitive_key);
	assert_true(to->params.case_insensitive_val);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	assert_ssmap_equal(from, to);

	ssmap_free(from);
	ssmap_free(to);
}

static void ssmap__null_inputs(void **state) {
	const struct SSmap *map = ssmap_init();

	assert_nul(ssmap_clone(NULL));
	ssmap_free(NULL);
	ssmap_it_free(NULL);
	assert_false(ssmap_get(NULL, NULL));
	assert_false(ssmap_get(map, NULL));
	assert_false(ssmap_contains_key(NULL, NULL));
	assert_false(ssmap_contains_key(map, NULL));
	assert_false(ssmap_contains_val(NULL, NULL));
	assert_false(ssmap_contains_val(map, NULL));
	assert_nul(ssmap_at(NULL, 0).val);
	ssmap_find(NULL, NULL, NULL);
	ssmap_find(map, NULL, NULL);
	ssmap_find_key(NULL, NULL, NULL);
	ssmap_find_key(map, NULL, NULL);
	ssmap_find_val(NULL, NULL, NULL);
	ssmap_find_val(map, NULL, NULL);
	assert_nul(ssmap_it(NULL));
	assert_nul(ssmap_filter_it(NULL, NULL, NULL));
	assert_nul(ssmap_filter_it(map, NULL, NULL));
	assert_nul(ssmap_key_filter_it(NULL, NULL, NULL));
	assert_nul(ssmap_key_filter_it(map, NULL, NULL));
	assert_nul(ssmap_val_filter_it(NULL, NULL, NULL));
	assert_nul(ssmap_val_filter_it(map, NULL, NULL));
	assert_nul(ssmap_it_next(NULL));
	assert_false(ssmap_put(NULL, NULL, NULL));
	assert_false(ssmap_put(map, NULL, NULL));
	assert_false(ssmap_put_if_absent(NULL, NULL, NULL));
	assert_false(ssmap_put_if_absent(map, NULL, NULL));
	assert_int_equal(ssmap_put_all(NULL, NULL), 0);
	assert_int_equal(ssmap_put_all(map, NULL), 0);
	assert_int_equal(ssmap_put_many(NULL, NULL), 0);
	assert_false(ssmap_remove(NULL, NULL));
	assert_false(ssmap_remove(map, NULL));
	assert_int_equal(ssmap_remove_all(NULL, NULL), 0);
	assert_int_equal(ssmap_remove_all(map, NULL), 0);
	assert_int_equal(ssmap_remove_all(NULL, map), 0);
	ssmap_it_remove(NULL);
	assert_false(ssmap_equal(NULL, NULL));
	assert_false(ssmap_equal(map, NULL));
	assert_nul(ssmap_keys_pslist(NULL));
	assert_nul(ssmap_keys_sset(NULL));
	assert_nul(ssmap_vals_pslist(NULL));
	assert_nul(ssmap_vals_sset(NULL));
	assert_nul(ssmap_str(NULL));
	assert_int_equal(ssmap_size(NULL), 0);

	ssmap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(ssmap_put_get_remove_free__case_sensitive),
		TEST(ssmap_put_get_remove_free__case_insensitive),

		TEST(ssmap_find__matches),
		TEST(ssmap_find_key__matches),
		TEST(ssmap_find_val__matches),

		TEST(ssmap_it__many),
		TEST(ssmap_it__empty),

		TEST(ssmap_it_free__partial),

		TEST(ssmap_it_next__partial),

		TEST(ssmap_filter_it__many),
		TEST(ssmap_key_filter_it__many),
		TEST(ssmap_val_filter_it__many),

		TEST(ssmap_equal__case_sensitive),
		TEST(ssmap_equal__case_insensitive_key),
		TEST(ssmap_equal__case_insensitive_val),

		TEST(ssmap_contains_key__),

		TEST(ssmap_contains_val__),

		TEST(ssmap_at__),

		TEST(ssmap_put_if_absent__),

		TEST(ssmap_put_all__many),

		TEST(ssmap_put_many__many),

		TEST(ssmap_remove_all__),

		TEST(ssmap_it_remove__many),
		TEST(ssmap_it_remove__partial),

		TEST(ssmap_str__),

		TEST(ssmap_keys_pslist__many),

		TEST(ssmap_keys_sset__many),
		TEST(ssmap_keys_sset__params),

		TEST(ssmap_vals_pslist__many),

		TEST(ssmap_vals_sset__many),
		TEST(ssmap_vals_sset__params),

		TEST(ssmap_clone__),

		TEST(ssmap__null_inputs),
	};

	return RUN(tests);
}


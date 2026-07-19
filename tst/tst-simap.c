#include "assert-simap.h"
#include "assert-sset.h"
#include "asserts.h"
#include "mock-fn.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "ppmap.h"
#include "pslist.h"
#include "sset.h"
#include "str.h"

#include "simap.h"

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

struct SImap {
	const struct SImapParams params;
	const struct PPmap *ppmap;
};

static bool match_key_a_val_lt_100(const char* const key, const size_t val, const void* const data) {
	return *key == 'a' && val < 100;
}

static bool starts_with_a(const char* const key) {
	return *key == 'a';
}

static bool ne_1(const size_t i) {
	return i != 1;
}

static void simap_put_get_remove_free__case_sensitive(void **state) {
	const struct SImap *map = simap_init();
	assert_false(simap_put(map, "a", 10));
	assert_false(simap_put(map, "b", 11));
	assert_false(simap_put(map, "c", 12));

	assert_true(simap_put(map, "c", 13));

	assert_int_equal(simap_size(map), 3);

	assert_int_equal(simap_get(map, "b"), 11);

	assert_int_equal(simap_get(map, "x"), 0);

	assert_true(simap_remove(map, "b"));
	assert_false(simap_remove(map, "b"));

	assert_int_equal(simap_get(map, "b"), 0);

	simap_free(map);
}

static void simap_put_get_remove_free__case_insensitive(void **state) {
	const struct SImapParams params = { .case_insensitive_key = true, };
	const struct SImap *map = simap_init_with(params);

	assert_false(simap_put(map, "A", 10));
	assert_false(simap_put(map, "B", 11));

	assert_int_equal(simap_get(map, "b"), 11);

	assert_int_equal(simap_get(map, "x"), 0);

	assert_true(simap_remove(map, "b"));

	assert_int_equal(simap_get(map, "b"), 0);

	simap_free(map);
}

static void simap_getp__zero(void **state) {

	const struct SImap *map = simap_init();
	assert_false(simap_put(map, "a", 11));

	assert_int_equal(simap_size(map), 1);

	assert_int_equal(simap_get(map, "a"), 11);

	size_t a = 99;
	assert_true(simap_get_ptr(&a, map, "a"));
	assert_int_equal(a, 11);

	assert_int_equal(simap_get(map, "x"), 0);

	size_t x = 99;
	assert_false(simap_get_ptr(&x, map, "x"));
	assert_int_equal(x, 0);

	assert_false(simap_put(map, "b", 0));

	size_t b = 99;
	assert_int_equal(simap_get(map, "b"), 0);
	assert_true(simap_get_ptr(&b, map, "b"));
	assert_int_equal(b, 0);

	simap_free(map);
}

static void simap_it__many(void **state) {

	const struct SImap *map = simap_init();
	assert_false(simap_put(map, "a", 10));
	assert_false(simap_put(map, "b", 11));
	assert_false(simap_put(map, "c", 12));

	const struct SImapIt *it = simap_it(map);

	assert_non_nul(it);
	assert_str_equal(it->key, "a");
	assert_int_equal(it->val, 10);

	it = simap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "b");
	assert_int_equal(it->val, 11);

	simap_it_free(it);

	simap_free(map);
}

static void simap_it__empty(void **state) {

	const struct SImap *map = simap_init();

	const struct SImapIt *it = simap_it(map);

	assert_nul(it);

	simap_free(map);
}

static void simap_it_free__partial(void **state) {
	const struct SImapIt *it = calloc(1, sizeof(struct SImapIt));

	simap_it_free(it);
}

static void simap_it_next__partial(void **state) {
	const struct SImapIt *it = calloc(1, sizeof(struct SImapIt));

	assert_nul(simap_it_next(it));
}

static void simap_filter_it__empty(void **state) {
	const struct SImap *map = simap_init();

	const struct SImapFilter filter = { .key = mock_pred_s, };
	assert_nul(simap_filter_it(map, filter));

	simap_free(map);
}

static void simap_filter_it__many(void **state) {
	const struct SImap *map = simap_init();

	assert_false(simap_put(map, "ak0", 100));
	assert_false(simap_put(map, "ak1", 11));
	assert_false(simap_put(map, "bk2", 12));
	assert_false(simap_put(map, "ak3", 13));
	assert_false(simap_put(map, "ak4", 101));

	const struct SImapFilter filter = { .key_val_data = match_key_a_val_lt_100, };
	const struct SImapIt *it = simap_filter_it(map, filter);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak1");
	assert_int_equal(it->val, 11);

	it = simap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak3");
	assert_int_equal(it->val, 13);

	assert_nul(simap_it_next(it));

	simap_free(map);
}

static void simap_find__variants(void **state) {
	const struct SImap *map = simap_init();

	assert_false(simap_put(map, "0", 10));
	assert_false(simap_put(map, "1", 11));
	assert_false(simap_put(map, "2", 12));

	// key
	expect_string(mock_pred_s, s, "0");
	will_return(mock_pred_s, false);
	expect_string(mock_pred_s, s, "1");
	will_return(mock_pred_s, true);

	const struct SImapFilter filter_k = { .key = mock_pred_s, .data = "x", };
	const struct SImapPair pair_k = simap_find(map, filter_k);
	assert_str_equal(pair_k.key, "1");
	assert_int_equal(pair_k.val, 11);

	// key_data
	expect_string(mock_pred_s_p, s, "0");
	expect_string(mock_pred_s_p, p, "x");
	will_return(mock_pred_s_p, false);
	expect_string(mock_pred_s_p, s, "1");
	expect_string(mock_pred_s_p, p, "x");
	will_return(mock_pred_s_p, true);

	const struct SImapFilter filter_kd = { .key_data = mock_pred_s_p, .data = "x", };
	const struct SImapPair pair_kd = simap_find(map, filter_kd);
	assert_str_equal(pair_kd.key, "1");
	assert_int_equal(pair_kd.val, 11);

	// val
	expect_int_value(mock_pred_i, i, 10);
	will_return(mock_pred_i, false);
	expect_int_value(mock_pred_i, i, 11);
	will_return(mock_pred_i, true);

	const struct SImapFilter filter_v = { .val = mock_pred_i, .data = "x", };
	const struct SImapPair pair_v = simap_find(map, filter_v);
	assert_str_equal(pair_v.key, "1");
	assert_int_equal(pair_v.val, 11);

	// val_data
	expect_int_value(mock_pred_i_p, i, 10);
	expect_string(mock_pred_i_p, p, "x");
	will_return(mock_pred_i_p, false);
	expect_int_value(mock_pred_i_p, i, 11);
	expect_string(mock_pred_i_p, p, "x");
	will_return(mock_pred_i_p, true);

	const struct SImapFilter filter_vd = { .val_data = mock_pred_i_p, .data = "x", };
	const struct SImapPair pair_vd = simap_find(map, filter_vd);
	assert_str_equal(pair_vd.key, "1");
	assert_int_equal(pair_vd.val, 11);

	// key_val
	expect_string(mock_pred_s_i, s, "0");
	expect_int_value(mock_pred_s_i, i, 10);
	will_return(mock_pred_s_i, false);
	expect_string(mock_pred_s_i, s, "1");
	expect_int_value(mock_pred_s_i, i, 11);
	will_return(mock_pred_s_i, true);

	const struct SImapFilter filter_kv = { .key_val = mock_pred_s_i, .data = "x", };
	const struct SImapPair pair_kv = simap_find(map, filter_kv);
	assert_str_equal(pair_kv.key, "1");
	assert_int_equal(pair_kv.val, 11);

	// key_val_data
	expect_string(mock_pred_s_i_p, s, "0");
	expect_int_value(mock_pred_s_i_p, i, 10);
	expect_string(mock_pred_s_i_p, p, "x");
	will_return(mock_pred_s_i_p, false);
	expect_string(mock_pred_s_i_p, s, "1");
	expect_int_value(mock_pred_s_i_p, i, 11);
	expect_string(mock_pred_s_i_p, p, "x");
	will_return(mock_pred_s_i_p, true);

	const struct SImapFilter filter_kvd = { .key_val_data = mock_pred_s_i_p, .data = "x", };
	const struct SImapPair pair_kvd = simap_find(map, filter_kvd);
	assert_str_equal(pair_kvd.key, "1");
	assert_int_equal(pair_kvd.val, 11);

	simap_free(map);
}

static void simap_find__some_block(void **state) {
	const struct SImap *map = simap_init();

	// key blocks
	assert_false(simap_put(map, "b0", 0));

	// key passes, val blocks
	assert_false(simap_put(map, "a1", 1));

	// both pass
	assert_false(simap_put(map, "a2", 2));

	const struct SImapFilter filter = {
		.key = starts_with_a,
		.val = ne_1,
	};
	const struct SImapPair pair = simap_find(map, filter);
	assert_str_equal(pair.key, "a2");
	assert_int_equal(pair.val, 2);

	simap_free(map);
}

static void simap_find__all_block(void **state) {
	const struct SImap *map = simap_init();

	assert_false(simap_put(map, "a0", 1));
	assert_false(simap_put(map, "a1", 1));
	assert_false(simap_put(map, "a2", 1));

	const struct SImapFilter filter = {
		.key = starts_with_a,
		.val = ne_1,
	};
	const struct SImapPair pair = simap_find(map, filter);
	assert_nul(pair.key);
	assert_int_equal(pair.val, 0);

	simap_free(map);
}

static void simap_find__none_block(void **state) {
	const struct SImap *map = simap_init();

	assert_false(simap_put(map, "a0", 0));
	assert_false(simap_put(map, "a1", 0));
	assert_false(simap_put(map, "a2", 0));

	const struct SImapFilter filter = {
		.key = starts_with_a,
		.val = ne_1,
	};
	const struct SImapPair pair = simap_find(map, filter);
	assert_str_equal(pair.key, "a0");
	assert_int_equal(pair.val, 0);

	simap_free(map);
}

static void simap_equal__case_sensitive(void **state) {

	const struct SImap *actual = simap_init();
	assert_false(simap_put(actual, "a", 10));
	assert_false(simap_put(actual, "b", 11));

	assert_simap_not_equal(actual, NULL);

	const struct SImap *expected = simap_init();
	assert_false(simap_put(expected, "a", 10));
	assert_false(simap_put(expected, "b", 11));

	assert_simap_equal(actual, expected);

	assert_false(simap_put(actual, "c", 12));

	assert_simap_not_equal(actual, expected);

	simap_free(actual);
	simap_free(expected);
}

static void simap_equal__case_insensitive_key(void **state) {

	const struct SImapParams params = { .case_insensitive_key = true, };
	const struct SImap *actual = simap_init_with(params);

	assert_false(simap_put(actual, "a", 10));
	assert_false(simap_put(actual, "b", 11));

	const struct SImap *expected = simap_init();
	assert_false(simap_put(expected, "A", 10));
	assert_false(simap_put(expected, "B", 11));

	assert_simap_equal(actual, expected);

	simap_free(actual);
	simap_free(expected);
}

static void simap_contains_key__(void **state) {
	const struct SImap *map = simap_init();

	assert_false(simap_contains_key(map, "a"));

	assert_false(simap_put(map, "a", 10));
	assert_false(simap_put(map, "b", 11));

	assert_true(simap_contains_key(map, "a"));
	assert_true(simap_contains_key(map, "b"));

	assert_false(simap_contains_key(map, "c"));

	assert_false(simap_contains_key(map, NULL));

	simap_free(map);
}

static void simap_contains_val__(void **state) {
	const struct SImap *map = simap_init();

	assert_false(simap_contains_val(map, 10));

	assert_false(simap_put(map, "a", 10));
	assert_false(simap_put(map, "b", 11));

	assert_true(simap_contains_val(map, 10));
	assert_true(simap_contains_val(map, 11));

	assert_false(simap_contains_val(map, 12));

	simap_free(map);
}

static void simap_first_key__(void **state) {
	const struct SImap *map = simap_init();

	assert_false(simap_put(map, "a", 0));
	assert_false(simap_put(map, "b", 1));

	assert_str_equal(simap_first_key(map, 0), "a");
	assert_str_equal(simap_first_key(map, 1), "b");
	assert_nul(simap_first_key(map, 2));

	simap_free(map);
}

static void simap_at__(void **state) {
	const struct SImap *map = simap_init();
	assert_false(simap_put(map, "a", 0));
	assert_false(simap_put(map, "b", 1));
	assert_false(simap_put(map, "c", 2));

	assert_str_equal(simap_at(map, 1).key, "b");
	assert_int_equal(simap_at(map, 1).val, 1);

	int *removed_val = (int*)map->ppmap->vals[1];
	map->ppmap->vals[1] = NULL;
	free(removed_val);

	assert_str_equal(simap_at(map, 1).key, "b");
	assert_int_equal(simap_at(map, 1).val, 0);

	simap_free(map);
}

static void simap_put_if_absent__(void **state) {
	const struct SImap *map = simap_init();

	assert_false(simap_put_if_absent(map, "a", 10));
	assert_int_equal(simap_get(map, "a"), 10);

	assert_true(simap_put_if_absent(map, "a", 10000));
	assert_int_equal(simap_get(map, "a"), 10);

	assert_false(simap_put_if_absent(map, "b", 11));
	assert_int_equal(simap_get(map, "b"), 11);

	simap_free(map);
}

static void simap_put_all__many(void **state) {
	const struct SImap *to = simap_init();
	assert_false(simap_put(to, "a", 0));
	assert_false(simap_put(to, "b", 1));

	const struct SImap *from = simap_init();
	assert_false(simap_put(from, "b", 2));
	assert_false(simap_put(from, "c", 3));

	const struct SImap *expected = simap_init();
	assert_false(simap_put(expected, "a", 0));
	assert_false(simap_put(expected, "b", 2));
	assert_false(simap_put(expected, "c", 3));

	assert_int_equal(simap_put_all(to, from), 1);

	assert_simap_equal(to, expected);

	simap_free(to);
	simap_free(from);
	simap_free(expected);
}

static void simap_put_many__many(void **state) {
	const struct SImap *to = simap_init();
	assert_false(simap_put(to, "a", 0));
	assert_false(simap_put(to, "b", 1));

	const struct SImap *expected = simap_init();
	assert_false(simap_put(expected, "a", 0));
	assert_false(simap_put(expected, "b", 1));
	assert_false(simap_put(expected, "c", 2));

	assert_int_equal(simap_put_many(to,
				"b", 1,
				"c", 2,
				NULL),
			1);

	assert_simap_equal(to, expected);

	simap_free(to);
	simap_free(expected);
}

static void simap_put_many__no_keyvals(void **state) {
	const struct SImap *to = simap_init();

	assert_int_equal(simap_put_many(to, NULL), 0);

	simap_free(to);
}

static void simap_remove_all__(void **state) {
	const struct SImap *map = simap_init();

	assert_int_equal(simap_remove_all(map), 0);

	assert_false(simap_put(map, "a", 0));
	assert_false(simap_put(map, "b", 1));

	assert_int_equal(simap_remove_all(map), 2);

	assert_int_equal(simap_size(map), 0);

	assert_false(simap_contains_key(map, "a"));
	assert_false(simap_contains_val(map, 0));
	assert_false(simap_contains_key(map, "b"));
	assert_false(simap_contains_val(map, 1));

	simap_free(map);
}

static void simap_remove_in__(void **state) {
	const struct SImap *map = simap_init();

	simap_put(map, "a", 0);
	simap_put(map, "b", 1);
	simap_put(map, "c", 2);

	const struct SImap *from = simap_init();

	simap_put(from, "b", 1);
	simap_put(from, "d", 3);

	const struct SImap *expected = simap_init();

	simap_put(expected, "a", 0);
	simap_put(expected, "c", 2);

	assert_int_equal(simap_remove_in(map, from), 1);

	assert_simap_equal(map, expected);

	simap_free(map);
	simap_free(from);
	simap_free(expected);
}

static void simap_it_remove__many(void **state) {
	const struct SImap *map = simap_init();

	assert_false(simap_put(map, "a", 0));
	assert_false(simap_put(map, "b", 1));
	assert_false(simap_put(map, "c", 2));
	assert_false(simap_put(map, "d", 3));
	assert_false(simap_put(map, "e", 4));

	const struct SImap *expected = simap_init();

	assert_false(simap_put(expected, "b", 1));
	assert_false(simap_put(expected, "d", 3));

	size_t iterations = 0;
	for (const struct SImapIt *it = simap_it(map); it; it = simap_it_next(it)) {
		iterations++;
		if (it->val == 0 || it->val == 2 || it->val == 4) {
			simap_it_remove(it);
		}
	}

	assert_int_equal(simap_size(map), 2);
	assert_int_equal(iterations, 5);

	assert_simap_equal(map, expected);

	simap_free(map);
	simap_free(expected);
}

static void simap_it_remove__partial(void **state) {
	const struct SImapIt *it = calloc(1, sizeof(struct SImapIt));

	simap_it_remove(it);
}

static void simap_str__(void **state) {

	const struct SImap *map = simap_init();
	assert_false(simap_put(map, "a", 10));
	assert_false(simap_put(map, "b", 11));
	assert_false(simap_put(map, "c", 12));
	assert_false(simap_put(map, "x", 0));

	char *expected = sprintf_alloc(
			"a = 10\n"
			"b = 11\n"
			"c = 12\n"
			"x = 0\n"
			);

	char *actual = simap_str(map);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	simap_free(map);
}

static void simap_keys_pslist__many(void **state) {
	const struct SImap *map = simap_init();

	simap_put(map, "a", 10);
	simap_put(map, "b", 11);

	struct Pslist *list = simap_keys_pslist(map);

	assert_int_equal(pslist_length(list), 2);
	assert_str_equal(pslist_at(list, 0), "a");
	assert_str_equal(pslist_at(list, 1), "b");

	simap_free(map);
	pslist_free_vals(&list, NULL);
}

static void simap_keys_sset__many(void **state) {
	const struct SImap *map = simap_init();

	simap_put(map, "a", 0);
	simap_put(map, "b", 1);

	const struct Sset *expected = sset_init();
	sset_add(expected, "a");
	sset_add(expected, "b");

	const struct Sset *actual = simap_keys_sset(map);

	assert_sset_equal(actual, expected);

	simap_free(map);
	sset_free(expected);
	sset_free(actual);
}

static void simap_keys_sset__params(void **state) {
	const struct SImapParams params = {
		.case_insensitive_key = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SImap *map = simap_init_with(params);

	const struct Sset *set = simap_keys_sset(map);

	assert_true(set->params.case_insensitive);
	assert_int_equal(set->params.initial, 99);
	assert_int_equal(set->params.grow, 1);

	simap_free(map);

	sset_free(set);
}

// also tests constructor
static void simap_clone__(void **state) {
	const struct SImapParams params = {
		.case_insensitive_key = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SImap *from = simap_init_with(params);

	const struct SImap *to = simap_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->ppmap->size, 0);
	assert_int_equal(to->ppmap->capacity, 99);
	assert_false(to->ppmap->params.allow_null_val);
	assert_int_equal(to->ppmap->params.grow, 1);
	assert_ptr_equal(to->ppmap->params.equal_key, equal_strcasecmp);
	assert_ptr_equal(to->ppmap->params.alloc_key, clone_strdup);
	assert_ptr_equal(to->ppmap->params.free_key, free);
	assert_ptr_equal(to->ppmap->params.free_val, free);
	assert_ptr_equal(to->ppmap->params.str_key, (fn_str)str_or_null);

	assert_true(to->params.case_insensitive_key);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	assert_simap_equal(from, to);

	simap_free(from);
	simap_free(to);
}

static void simap__null_inputs(void **state) {
	const struct SImap *map = simap_init();
	const struct SImapFilter filter = { 0 };

	assert_nul(simap_clone(NULL));
	simap_free(NULL);
	simap_it_free(NULL);
	assert_false(simap_get(NULL, NULL));
	assert_false(simap_get(map, NULL));
	assert_false(simap_get_ptr(NULL, NULL, NULL));
	assert_false(simap_get_ptr(NULL, map, NULL));
	assert_false(simap_contains_key(NULL, NULL));
	assert_false(simap_contains_key(map, NULL));
	assert_false(simap_contains_val(NULL, 0));
	assert_false(simap_contains_val(map, 0));
	assert_nul(simap_first_key(NULL, 0));
	assert_nul(simap_at(NULL, 0).key);
	simap_find(NULL, filter);
	assert_nul(simap_it(NULL));
	assert_nul(simap_filter_it(NULL, filter));
	assert_nul(simap_it_next(NULL));
	assert_false(simap_put(NULL, NULL, 0));
	assert_false(simap_put(map, NULL, 0));
	assert_false(simap_put_if_absent(NULL, NULL, 0));
	assert_false(simap_put_if_absent(map, NULL, 0));
	assert_int_equal(simap_put_all(NULL, NULL), 0);
	assert_int_equal(simap_put_all(map, NULL), 0);
	assert_int_equal(simap_put_many(NULL, NULL), 0);
	assert_false(simap_remove(NULL, NULL));
	assert_false(simap_remove(map, NULL));
	assert_int_equal(simap_remove_all(NULL), 0);
	simap_it_remove(NULL);
	assert_int_equal(simap_remove_in(NULL, NULL), 0);
	assert_int_equal(simap_remove_in(map, NULL), 0);
	assert_int_equal(simap_remove_in(NULL, map), 0);
	assert_false(simap_equal(NULL, NULL));
	assert_false(simap_equal(map, NULL));
	assert_nul(simap_keys_pslist(NULL));
	assert_nul(simap_keys_sset(NULL));
	assert_nul(simap_str(NULL));
	assert_int_equal(simap_size(NULL), 0);

	simap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(simap_put_get_remove_free__case_sensitive),
		TEST(simap_put_get_remove_free__case_insensitive),

		TEST(simap_getp__zero),

		TEST(simap_find__variants),
		TEST(simap_find__some_block),
		TEST(simap_find__all_block),
		TEST(simap_find__none_block),

		TEST(simap_it__many),
		TEST(simap_it__empty),

		TEST(simap_it_free__partial),

		TEST(simap_it_next__partial),

		TEST(simap_filter_it__empty),
		TEST(simap_filter_it__many),

		TEST(simap_equal__case_sensitive),
		TEST(simap_equal__case_insensitive_key),

		TEST(simap_contains_key__),

		TEST(simap_contains_val__),

		TEST(simap_at__),

		TEST(simap_first_key__),

		TEST(simap_put_if_absent__),

		TEST(simap_put_all__many),

		TEST(simap_put_many__many),
		TEST(simap_put_many__no_keyvals),

		TEST(simap_remove_all__),

		TEST(simap_remove_in__),

		TEST(simap_it_remove__many),
		TEST(simap_it_remove__partial),

		TEST(simap_str__),

		TEST(simap_keys_pslist__many),

		TEST(simap_keys_sset__many),
		TEST(simap_keys_sset__params),

		TEST(simap_clone__),

		TEST(simap__null_inputs),
	};

	return RUN(tests);
}


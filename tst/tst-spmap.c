#include "assert-pset.h"
#include "assert-spmap.h"
#include "assert-sset.h"
#include "asserts.h"
#include "data.h"
#include "expects.h"
#include "mock-fn.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "ppmap.h"
#include "pset.h"
#include "pslist.h"
#include "sset.h"
#include "str.h"

#include "spmap.h"

// TODO try and generify all xxmap tests with a single macro driven template

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

struct SPmap {
	const struct SPmapParams params;
	const struct PPmap *ppmap;
};

static void spmap_put_get_remove__case_sensitive(void **state) {

	const struct SPmap *map = spmap_init();
	assert_nul(spmap_put(map, "a", V0));
	assert_nul(spmap_put(map, "b", V1));
	assert_nul(spmap_put(map, "c", V2));

	assert_int_equal(spmap_size(map), 3);

	assert_ptr_equal(spmap_get(map, "b"), V1);

	assert_nul(spmap_get(map, "x"));

	assert_ptr_equal(spmap_remove(map, "b"), V1);

	assert_nul(spmap_get(map, "b"));

	spmap_free(map);
}

static void spmap_put_get_remove__case_insensitive(void **state) {
	const struct SPmapParams params = { .case_insensitive = true, };
	const struct SPmap *map = spmap_init_with(params);

	assert_nul(spmap_put(map, "A", V0));
	assert_nul(spmap_put(map, "B", V1));

	assert_ptr_equal(spmap_get(map, "b"), V1);

	assert_nul(spmap_get(map, "x"));

	assert_ptr_equal(spmap_remove(map, "b"), V1);

	assert_nul(spmap_get(map, "b"));

	spmap_free(map);
}

static void spmap_free_vals__(void **state) {
	const struct SPmap *map = spmap_init();
	assert_nul(spmap_put(map, "a", strdup("zero")));

	spmap_free_vals(map);
}

static void spmap_find__matches(void **state) {
	const struct SPmap *map = spmap_init();

	assert_nul(spmap_put(map, "0", V0));
	assert_nul(spmap_put(map, "1", V1));
	assert_nul(spmap_put(map, "2", V2));

	// skip 0
	expect_string(mock_3pred_str_ptr, str, "0");
	expect_ptr(mock_3pred_str_ptr, ptr, V0);
	expect_ptr(mock_3pred_str_ptr, data, V2);
	will_return(mock_3pred_str_ptr, false);

	// get 1
	expect_string(mock_3pred_str_ptr, str, "1");
	expect_ptr(mock_3pred_str_ptr, ptr, V1);
	expect_ptr(mock_3pred_str_ptr, data, V2);
	will_return(mock_3pred_str_ptr, true);

	const struct SPmapPair kv_pair = spmap_find(map, mock_3pred_str_ptr, V2);
	assert_str_equal(kv_pair.key, "1");
	assert_ptr_equal(kv_pair.val, V1);

	spmap_free(map);
}

static void spmap_find_key__matches(void **state) {
	const struct SPmap *map = spmap_init();

	assert_nul(spmap_put(map, "0", V0));
	assert_nul(spmap_put(map, "1", V1));
	assert_nul(spmap_put(map, "2", V2));

	// skip 0
	expect_string(mock_2pred_str, str, "0");
	expect_ptr(mock_2pred_str, data, V2);
	will_return(mock_2pred_str, false);

	// get 1
	expect_string(mock_2pred_str, str, "1");
	expect_ptr(mock_2pred_str, data, V2);
	will_return(mock_2pred_str, true);

	const struct SPmapPair k_pair = spmap_find_key(map, mock_2pred_str, V2);
	assert_str_equal(k_pair.key, "1");
	assert_ptr_equal(k_pair.val, V1);

	spmap_free(map);
}

static void spmap_find_val__matches(void **state) {
	const struct SPmap *map = spmap_init();

	assert_nul(spmap_put(map, "0", V0));
	assert_nul(spmap_put(map, "1", V1));
	assert_nul(spmap_put(map, "2", V2));

	// skip 0
	expect_ptr(mock_2pred, ptr, V0);
	expect_ptr(mock_2pred, data, V2);
	will_return(mock_2pred, false);

	// get 1
	expect_ptr(mock_2pred, ptr, V1);
	expect_ptr(mock_2pred, data, V2);
	will_return(mock_2pred, true);

	const struct SPmapPair v_pair = spmap_find_val(map, mock_2pred, V2);
	assert_str_equal(v_pair.key, "1");
	assert_ptr_equal(v_pair.val, V1);

	spmap_free(map);
}

static void spmap_it__many(void **state) {
	const struct SPmapParams params = { .allow_null_val = true, };
	const struct SPmap *map = spmap_init_with(params);

	assert_nul(spmap_put(map, "a", V0));
	assert_nul(spmap_put(map, "b", NULL));
	assert_nul(spmap_put(map, "c", V2));

	const struct SPmapIt *it = spmap_it(map);

	assert_non_nul(it);
	assert_str_equal(it->key, "a");
	assert_ptr_equal(it->val, V0);

	it = spmap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "b");
	assert_nul(it->val);

	spmap_it_free(it);

	spmap_free(map);
}

static void spmap_it_free__partial(void **state) {
	const struct SPmapIt *it = calloc(1, sizeof(struct SPmapIt));

	spmap_it_free(it);
}

static void spmap_it_next__partial(void **state) {
	const struct SPmapIt *it = calloc(1, sizeof(struct SPmapIt));

	assert_nul(spmap_it_next(it));
}

static void spmap_it__empty(void **state) {

	const struct SPmap *map = spmap_init();

	const struct SPmapIt *it = spmap_it(map);

	assert_nul(it);

	spmap_free(map);
}

static void spmap_filter_it__many(void **state) {
	const struct SPmap *map = spmap_init();

	assert_nul(spmap_put(map, "0", V0));
	assert_nul(spmap_put(map, "1", V1));
	assert_nul(spmap_put(map, "2", V2));

	// skip "0"
	expect_string(mock_3pred_str_ptr, str, "0");
	expect_ptr(mock_3pred_str_ptr, ptr, V0);
	expect_ptr(mock_3pred_str_ptr, data, D0);
	will_return(mock_3pred_str_ptr, false);

	// get "1"
	expect_string(mock_3pred_str_ptr, str, "1");
	expect_ptr(mock_3pred_str_ptr, ptr, V1);
	expect_ptr(mock_3pred_str_ptr, data, D0);
	will_return(mock_3pred_str_ptr, true);

	const struct SPmapIt *it = spmap_filter_it(map, mock_3pred_str_ptr, D0);
	assert_non_nul(it);
	assert_str_equal(it->key, "1");
	assert_ptr_equal(it->val, V1);

	// skip "2"
	expect_string(mock_3pred_str_ptr, str, "2");
	expect_ptr(mock_3pred_str_ptr, ptr, V2);
	expect_ptr(mock_3pred_str_ptr, data, D0);
	will_return(mock_3pred_str_ptr, false);

	// done
	it = spmap_it_next(it);
	assert_nul(it);

	spmap_free(map);
}

static void spmap_key_filter_it__many(void **state) {
	const struct SPmap *map = spmap_init();

	assert_nul(spmap_put(map, "0", V0));
	assert_nul(spmap_put(map, "1", V1));
	assert_nul(spmap_put(map, "2", V2));

	// skip V0
	expect_string(mock_2pred_str, str, "0");
	expect_ptr(mock_2pred_str, data, D0);
	will_return(mock_2pred_str, false);

	// get V1
	expect_string(mock_2pred_str, str, "1");
	expect_ptr(mock_2pred_str, data, D0);
	will_return(mock_2pred_str, true);

	const struct SPmapIt *it = spmap_key_filter_it(map, mock_2pred_str, D0);
	assert_non_nul(it);
	assert_str_equal(it->key, "1");
	assert_ptr_equal(it->val, V1);

	// skip V2
	expect_string(mock_2pred_str, str, "2");
	expect_ptr(mock_2pred_str, data, D0);
	will_return(mock_2pred_str, false);

	// done
	it = spmap_it_next(it);
	assert_nul(it);

	spmap_free(map);
}

static void spmap_val_filter_it__many(void **state) {
	const struct SPmap *map = spmap_init();

	assert_nul(spmap_put(map, "0", V0));
	assert_nul(spmap_put(map, "1", V1));
	assert_nul(spmap_put(map, "2", V2));

	// skip V0
	expect_ptr(mock_2pred, ptr, V0);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, false);

	// get V1
	expect_ptr(mock_2pred, ptr, V1);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, true);

	const struct SPmapIt *it = spmap_val_filter_it(map, mock_2pred, D0);
	assert_non_nul(it);
	assert_str_equal(it->key, "1");
	assert_ptr_equal(it->val, V1);

	// skip V2
	expect_ptr(mock_2pred, ptr, V2);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, false);

	// done
	it = spmap_it_next(it);
	assert_nul(it);

	spmap_free(map);
}

static void spmap_equal__case_sensitive(void **state) {

	const struct SPmap *actual = spmap_init();
	assert_nul(spmap_put(actual, "a", V0));
	assert_nul(spmap_put(actual, "b", V1));

	assert_spmap_not_equal(actual, NULL);

	const struct SPmap *expected = spmap_init();
	assert_nul(spmap_put(expected, "a", V0));
	assert_nul(spmap_put(expected, "b", V1));

	assert_spmap_equal(actual, expected);

	spmap_free(actual);
	spmap_free(expected);
}

static void spmap_equal__case_insensitive(void **state) {

	const struct SPmapParams params = { .case_insensitive = true, };
	const struct SPmap *actual = spmap_init_with(params);

	assert_nul(spmap_put(actual, "a", V0));
	assert_nul(spmap_put(actual, "b", V1));

	const struct SPmap *expected = spmap_init();
	assert_nul(spmap_put(expected, "A", V0));
	assert_nul(spmap_put(expected, "B", V1));

	assert_spmap_equal(actual, expected);

	assert_nul(spmap_put(actual, "c", V2));

	assert_spmap_not_equal(actual, expected);

	spmap_free(actual);
	spmap_free(expected);
}

static void spmap_contains_key__(void **state) {
	const struct SPmap *map = spmap_init();

	assert_false(spmap_contains_key(map, "a"));

	assert_nul(spmap_put(map, "a", V0));
	assert_nul(spmap_put(map, "b", V1));

	assert_true(spmap_contains_key(map, "a"));
	assert_true(spmap_contains_key(map, "b"));

	assert_false(spmap_contains_key(map, "c"));

	assert_false(spmap_contains_key(map, NULL));

	spmap_free(map);
}

static void spmap_contains_val__(void **state) {
	const struct SPmap *map = spmap_init();

	assert_false(spmap_contains_key(map, V0));

	assert_nul(spmap_put(map, "a", V0));
	assert_nul(spmap_put(map, "b", V1));

	assert_true(spmap_contains_val(map, V0));
	assert_true(spmap_contains_val(map, V1));

	assert_false(spmap_contains_val(map, V2));

	assert_false(spmap_contains_val(map, NULL));

	spmap_free(map);
}

static void spmap_at__(void **state) {
	const struct SPmap *map = spmap_init();
	assert_false(spmap_put(map, K0, V0));
	assert_false(spmap_put(map, K1, V1));
	assert_false(spmap_put(map, K2, V2));

	assert_str_equal(spmap_at(map, 1).key, K1);
	assert_str_equal(spmap_at(map, 1).val, V1);

	spmap_free(map);
}

static void spmap_put_free__(void **state) {
	const struct SPmapParams params = { .free_val = mock_free, };
	const struct SPmap *map = spmap_init_with(params);

	assert_nul(spmap_put(map, "a", V0));

	assert_false(spmap_put_free(map, "b", V1));

	expect_ptr(mock_free, ptr, V0);
	assert_true(spmap_put_free(map, "a", V0));

	spmap_free(map);
}

static void spmap_put_all__variants(void **state) {
	const struct SPmap *from = spmap_init();
	assert_nul(spmap_put(from, "a", V0));
	assert_nul(spmap_put(from, "b", V1));

	const struct SPmap *expected = spmap_init();
	assert_nul(spmap_put(expected, "a", V0));
	assert_nul(spmap_put(expected, "b", V1));

	const struct SPmapParams params = {
		.free_val = mock_free,
		.clone_val = mock_clone,
	};

	const struct SPmap *to = spmap_init_with(params);
	assert_nul(spmap_put(to, "a", V3));

	// put_all
	const struct SPmap *actual = spmap_clone(to);

	assert_int_equal(spmap_put_all(actual, from), 1);

	assert_spmap_equal(actual, expected);
	spmap_free(actual);

	// put_all_free
	actual = spmap_clone(to);
	expect_ptr(mock_free, ptr, V3);

	assert_int_equal(spmap_put_all_free(actual, from), 1);

	assert_spmap_equal(actual, expected);
	spmap_free(actual);

	// put_all_clone
	actual = spmap_clone(to);
	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);
	expect_ptr(mock_clone, ptr, V1);
	will_return_ptr_type(mock_clone, V1, void*);

	assert_int_equal(spmap_put_all_clone(actual, from), 1);

	assert_spmap_equal(actual, expected);
	spmap_free(actual);

	// put_all_clone_free
	actual = spmap_clone(to);
	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);
	expect_ptr(mock_clone, ptr, V1);
	will_return_ptr_type(mock_clone, V1, void*);
	expect_ptr(mock_free, ptr, V3);

	assert_int_equal(spmap_put_all_clone_free(actual, from), 1);

	assert_spmap_equal(actual, expected);
	spmap_free(actual);

	spmap_free(to);
	spmap_free(from);
	spmap_free(expected);
}

static void spmap_put_many__many(void **state) {
	const struct SPmap *to = spmap_init();
	assert_nul(spmap_put(to, "a", V0));
	assert_nul(spmap_put(to, "b", strdup("replaced")));

	const struct SPmap *expected = spmap_init();
	assert_nul(spmap_put(expected, "a", V0));
	assert_nul(spmap_put(expected, "b", V1));
	assert_nul(spmap_put(expected, "c", V2));

	assert_int_equal(spmap_put_many(to,
				"b", V1,
				"c", V2,
				NULL),
			1);

	assert_spmap_equal(to, expected);

	spmap_free(to);
	spmap_free(expected);
}

static void spmap_put_if_absent__(void **state) {
	const struct SPmap *map = spmap_init();

	assert_nul(spmap_put_if_absent(map, "a", V0));
	assert_ptr_equal(spmap_get(map, "a"), V0);

	const void *existing = spmap_put_if_absent(map, "a", V1);
	assert_ptr_equal(existing, V0);

	spmap_free(map);
}

static void spmap_remove_free__(void **state) {
	const struct SPmapParams params = { .free_val = mock_free, };
	const struct SPmap *map = spmap_init_with(params);

	assert_nul(spmap_put(map, "a", V0));

	assert_false(spmap_remove_free(map, "b"));

	expect_ptr(mock_free, ptr, V0);
	assert_true(spmap_remove_free(map, "a"));

	spmap_free(map);
}

static void spmap_remove_from__(void **state) {
	const struct SPmap *map = spmap_init();

	assert_nul(spmap_put(map, "a", V0));
	assert_nul(spmap_put(map, "b", V1));
	assert_nul(spmap_put(map, "c", V2));

	const struct SPmap *from = spmap_init();

	assert_nul(spmap_put(from, "b", V1));
	assert_nul(spmap_put(from, "d", V3));

	const struct SPmap *expected = spmap_init();

	assert_nul(spmap_put(expected, "a", V0));
	assert_nul(spmap_put(expected, "c", V2));

	assert_int_equal(spmap_remove_from(map, from), 1);

	assert_spmap_equal(map, expected);

	spmap_free(map);
	spmap_free(from);
	spmap_free(expected);
}

static void spmap_remove_from_free__(void **state) {
	const struct SPmapParams params = { .free_val = mock_free, };
	const struct SPmap *map = spmap_init_with(params);

	assert_nul(spmap_put(map, "a", V0));

	const struct SPmap *from = spmap_init();

	assert_nul(spmap_put(from, "a", V0));

	expect_ptr(mock_free, ptr, V0);

	assert_int_equal(spmap_remove_from_free(map, from), 1);

	spmap_free(map);
	spmap_free(from);
}

static void spmap_it_remove__many(void **state) {
	const struct SPmapParams params = { .free_val = mock_free, };
	const struct SPmap *map = spmap_init_with(params);

	assert_false(spmap_put(map, "a", V0));
	assert_false(spmap_put(map, "b", V1));
	assert_false(spmap_put(map, "c", V2));
	assert_false(spmap_put(map, "d", V3));
	assert_false(spmap_put(map, "e", V4));

	const struct SPmap *expected = spmap_init();

	assert_false(spmap_put(expected, "a", V0));
	assert_false(spmap_put(expected, "c", V2));
	assert_false(spmap_put(expected, "e", V4));

	size_t iterations = 0;
	for (const struct SPmapIt *it = spmap_it(map); it; it = spmap_it_next(it)) {
		iterations++;
		if (it->val == V1 || it->val == V3) {
			spmap_it_remove(it);
		}
	}

	assert_int_equal(spmap_size(map), 3);
	assert_int_equal(iterations, 5);

	assert_spmap_equal(map, expected);

	spmap_free(map);
	spmap_free(expected);
}

static void spmap_it_remove__partial(void **state) {
	const struct SPmapIt *it = calloc(1, sizeof(struct SPmapIt));

	spmap_it_remove(it);
}

static void spmap_it_remove_free__many(void **state) {
	const struct SPmapParams params = { .free_val = mock_free, };
	const struct SPmap *map = spmap_init_with(params);

	assert_false(spmap_put(map, "a", V0));
	assert_false(spmap_put(map, "b", V1));
	assert_false(spmap_put(map, "c", V2));
	assert_false(spmap_put(map, "d", V3));
	assert_false(spmap_put(map, "e", V4));

	const struct SPmap *expected = spmap_init();

	assert_false(spmap_put(expected, "a", V0));
	assert_false(spmap_put(expected, "c", V2));
	assert_false(spmap_put(expected, "e", V4));

	expect_ptr(mock_free, ptr, V1);
	expect_ptr(mock_free, ptr, V3);

	size_t iterations = 0;
	for (const struct SPmapIt *it = spmap_it(map); it; it = spmap_it_next(it)) {
		iterations++;
		if (it->val == V1 || it->val == V3) {
			spmap_it_remove_free(it);
		}
	}

	assert_int_equal(spmap_size(map), 3);
	assert_int_equal(iterations, 5);

	assert_spmap_equal(map, expected);

	spmap_free(map);
	spmap_free(expected);
}

static void spmap_it_remove_free__partial(void **state) {
	const struct SPmapIt *it = calloc(1, sizeof(struct SPmapIt));

	spmap_it_remove_free(it);
}

static void spmap_str__(void **state) {
	const struct SPmapParams params = { .allow_null_val = true, };
	const struct SPmap *map = spmap_init_with(params);

	assert_nul(spmap_put(map, "a", V0));
	assert_nul(spmap_put(map, "b", NULL));
	assert_nul(spmap_put(map, "c", V2));

	char *expected = sprintf_alloc(
			"a = %p\n"
			"b = (null)\n"
			"c = %p\n",
			V0,
			V2
			);

	char *actual = spmap_str(map);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	spmap_free(map);
}

static void spmap_keys_pslist__many(void **state) {
	const struct SPmap *map = spmap_init();

	spmap_put(map, "a", V0);
	spmap_put(map, "b", V1);

	struct Pslist *list = spmap_keys_pslist(map);

	assert_int_equal(pslist_length(list), 2);
	assert_str_equal(pslist_at(list, 0), "a");
	assert_str_equal(pslist_at(list, 1), "b");

	spmap_free(map);
	pslist_free_vals(&list, NULL);
}

static void spmap_keys_sset__many(void **state) {
	const struct SPmap *map = spmap_init();

	spmap_put(map, "a", V0);
	spmap_put(map, "b", V1);

	const struct Sset *expected = sset_init();
	sset_add(expected, "a");
	sset_add(expected, "b");

	const struct Sset *actual = spmap_keys_sset(map);

	assert_sset_equal(actual, expected);

	spmap_free(map);
	sset_free(expected);
	sset_free(actual);
}

static void spmap_keys_sset__params(void **state) {
	const struct SPmapParams params = {
		.case_insensitive = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SPmap *map = spmap_init_with(params);

	const struct Sset *set = spmap_keys_sset(map);

	assert_true(set->params.case_insensitive);
	assert_int_equal(set->params.initial, 99);
	assert_int_equal(set->params.grow, 1);

	spmap_free(map);

	sset_free(set);
}

static void spmap_vals_pslist__many(void **state) {
	const struct SPmapParams params = { .allow_null_val = true, };
	const struct SPmap *map = spmap_init_with(params);

	spmap_put(map, "a", V0);
	spmap_put(map, "b", NULL);
	spmap_put(map, "c", V2);

	struct Pslist *list = spmap_vals_pslist(map);

	assert_int_equal(pslist_length(list), 3);
	assert_ptr_equal(pslist_at(list, 0), V0);
	assert_nul(pslist_at(list, 1));
	assert_ptr_equal(pslist_at(list, 2), V2);

	pslist_free(&list);
	spmap_free(map);
}

static void spmap_vals_pset__many(void **state) {
	const struct SPmapParams params = { .allow_null_val = true, };
	const struct SPmap *map = spmap_init_with(params);

	spmap_put(map, "a", V0);
	spmap_put(map, "b", NULL);
	spmap_put(map, "c", V2);

	const struct Pset *expected = pset_init();
	pset_add(expected, V0);
	pset_add(expected, V2);

	const struct Pset *actual = spmap_vals_pset(map);

	assert_pset_equal(actual, expected);

	spmap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void spmap_vals_pset_clone__many(void **state) {
	const struct SPmapParams params = { .clone_val = mock_clone, };
	const struct SPmap *map = spmap_init_with(params);

	spmap_put(map, "a", V0);

	const struct Pset *expected = pset_init();
	pset_add(expected, V0);

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	const struct Pset *actual = spmap_vals_pset_clone(map);

	assert_pset_equal(actual, expected);

	spmap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void spmap_vals_pslist_clone__many(void **state) {
	const struct SPmapParams params = {
		.allow_null_val = true,
		.clone_val = (fn_clone)clone_strdup,
	};
	const struct SPmap *map = spmap_init_with(params);

	spmap_put(map, "a", "aa");
	spmap_put(map, "b", NULL);
	spmap_put(map, "c", "bb");

	struct Pslist *list = spmap_vals_pslist_clone(map);

	assert_int_equal(pslist_length(list), 3);
	assert_str_equal(pslist_at(list, 0), "aa");
	assert_nul(pslist_at(list, 1));
	assert_str_equal(pslist_at(list, 2), "bb");

	pslist_free_vals(&list, NULL);
	spmap_free(map);
}

static void spmap_clone__many(void **state) {
	const struct SPmapParams params = { .allow_null_val = true, };
	const struct SPmap *from = spmap_init_with(params);

	assert_nul(spmap_put(from, "a", V0));
	assert_nul(spmap_put(from, "b", NULL));
	assert_nul(spmap_put(from, "c", V2));

	const struct SPmap *to = spmap_clone(from);

	assert_non_nul(to);

	assert_int_equal(spmap_size(to), 3);

	assert_spmap_equal(from, to);

	spmap_free(from);
	spmap_free(to);
}

// also tests constructor
static void spmap_clone__params(void **state) {
	const struct SPmapParams params = {
		.case_insensitive = true,
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.free_val = mock_free,
		.clone_val = mock_clone,
		.allow_null_val = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SPmap *from = spmap_init_with(params);

	const struct SPmap *to = spmap_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->ppmap->size, 0);
	assert_int_equal(to->ppmap->capacity, 99);
	assert_true(to->ppmap->params.allow_null_val);
	assert_int_equal(to->ppmap->params.grow, 1);
	assert_ptr_equal(to->ppmap->params.equal_key, equal_strcasecmp);
	assert_ptr_equal(to->ppmap->params.equal_val, mock_equal);
	assert_ptr_equal(to->ppmap->params.alloc_key, clone_strdup);
	assert_ptr_equal(to->ppmap->params.alloc_val, mock_alloc);
	assert_ptr_equal(to->ppmap->params.free_key, free);
	assert_ptr_equal(to->ppmap->params.free_val, mock_free);
	assert_ptr_equal(to->ppmap->params.clone_val, mock_clone);

	assert_true(to->params.case_insensitive);
	assert_ptr_equal(to->params.equal_val, mock_equal);
	assert_ptr_equal(to->params.alloc_val, mock_alloc);
	assert_ptr_equal(to->params.free_val, mock_free);
	assert_ptr_equal(to->params.clone_val, mock_clone);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	spmap_free(from);
	spmap_free(to);
}

static void spmap_clone_deep__clone_val(void **state) {
	const struct SPmapParams params = { .clone_val = mock_clone, };
	const struct SPmap *from = spmap_init_with(params);

	assert_nul(spmap_put(from, "a", V0));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	const struct SPmap *to = spmap_clone_deep(from);

	assert_non_nul(to);

	assert_int_equal(spmap_size(to), 1);

	assert_spmap_equal(from, to);

	spmap_free(from);
	spmap_free(to);
}

static void spmap_clone_deep__no_clone_val(void **state) {
	const struct SPmap *from = spmap_init();

	assert_nul(spmap_put(from, 0, V0));

	const struct SPmap *to = spmap_clone_deep(from);
	assert_non_nul(to);
	assert_int_equal(spmap_size(to), 0);

	spmap_free(from);
	spmap_free(to);
}

static void spmap__null_inputs(void **state) {
	const struct SPmap *map = spmap_init();

	assert_nul(spmap_clone(NULL));
	assert_nul(spmap_clone_deep(NULL));
	spmap_free(NULL);
	spmap_free_vals(NULL);
	spmap_it_free(NULL);
	assert_false(spmap_get(NULL, NULL));
	assert_false(spmap_get(map, NULL));
	assert_false(spmap_contains_key(NULL, NULL));
	assert_false(spmap_contains_key(map, NULL));
	assert_false(spmap_contains_val(NULL, NULL));
	assert_false(spmap_contains_val(map, NULL));
	assert_nul(spmap_at(NULL, 0).val);
	spmap_find(NULL, NULL, NULL);
	spmap_find(map, NULL, NULL);
	spmap_find_key(NULL, NULL, NULL);
	spmap_find_key(map, NULL, NULL);
	spmap_find_val(NULL, NULL, NULL);
	spmap_find_val(map, NULL, NULL);
	assert_nul(spmap_it(NULL));
	assert_nul(spmap_filter_it(NULL, NULL, NULL));
	assert_nul(spmap_filter_it(map, NULL, NULL));
	assert_nul(spmap_key_filter_it(NULL, NULL, NULL));
	assert_nul(spmap_key_filter_it(map, NULL, NULL));
	assert_nul(spmap_val_filter_it(NULL, NULL, NULL));
	assert_nul(spmap_val_filter_it(map, NULL, NULL));
	assert_nul(spmap_it_next(NULL));
	assert_nul(spmap_put(NULL, NULL, NULL));
	assert_nul(spmap_put(map, NULL, NULL));
	assert_nul(spmap_put_if_absent(NULL, NULL, NULL));
	assert_nul(spmap_put_if_absent(map, NULL, NULL));
	assert_false(spmap_put_free(NULL, NULL, NULL));
	assert_false(spmap_put_free(map, NULL, NULL));
	assert_int_equal(spmap_put_all(NULL, NULL), 0);
	assert_int_equal(spmap_put_all(map, NULL), 0);
	assert_int_equal(spmap_put_all_free(NULL, NULL), 0);
	assert_int_equal(spmap_put_all_free(map, NULL), 0);
	assert_int_equal(spmap_put_all_clone(NULL, NULL), 0);
	assert_int_equal(spmap_put_all_clone(map, NULL), 0);
	assert_int_equal(spmap_put_all_clone_free(NULL, NULL), 0);
	assert_int_equal(spmap_put_all_clone_free(map, NULL), 0);
	assert_int_equal(spmap_put_many(NULL, NULL), 0);
	assert_nul(spmap_remove(NULL, NULL));
	assert_nul(spmap_remove(map, NULL));
	assert_false(spmap_remove_free(NULL, NULL));
	assert_false(spmap_remove_free(map, NULL));
	assert_int_equal(spmap_remove_from(NULL, NULL), 0);
	assert_int_equal(spmap_remove_from(map, NULL), 0);
	assert_int_equal(spmap_remove_from(NULL, map), 0);
	assert_int_equal(spmap_remove_from_free(NULL, NULL), 0);
	assert_int_equal(spmap_remove_from_free(map, NULL), 0);
	assert_int_equal(spmap_remove_from_free(NULL, map), 0);
	spmap_it_remove(NULL);
	spmap_it_remove_free(NULL);
	assert_false(spmap_equal(NULL, NULL));
	assert_false(spmap_equal(map, NULL));
	assert_nul(spmap_keys_pslist(NULL));
	assert_nul(spmap_keys_sset(NULL));
	assert_nul(spmap_vals_pslist(NULL));
	assert_nul(spmap_vals_pslist_clone(NULL));
	assert_nul(spmap_vals_pset(NULL));
	assert_nul(spmap_vals_pset_clone(NULL));
	assert_nul(spmap_str(NULL));
	assert_int_equal(spmap_size(NULL), 0);

	spmap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(spmap_put_get_remove__case_sensitive),
		TEST(spmap_put_get_remove__case_insensitive),

		TEST(spmap_free_vals__),

		TEST(spmap_find__matches),
		TEST(spmap_find_key__matches),
		TEST(spmap_find_val__matches),

		TEST(spmap_it__many),
		TEST(spmap_it__empty),

		TEST(spmap_it_free__partial),

		TEST(spmap_it_next__partial),

		TEST(spmap_filter_it__many),
		TEST(spmap_key_filter_it__many),
		TEST(spmap_val_filter_it__many),

		TEST(spmap_equal__case_sensitive),
		TEST(spmap_equal__case_insensitive),

		TEST(spmap_contains_key__),

		TEST(spmap_contains_val__),

		TEST(spmap_at__),

		TEST(spmap_put_free__),

		TEST(spmap_put_all__variants),

		TEST(spmap_put_many__many),

		TEST(spmap_put_if_absent__),

		TEST(spmap_remove_free__),

		TEST(spmap_remove_from__),

		TEST(spmap_remove_from_free__),

		TEST(spmap_it_remove__many),
		TEST(spmap_it_remove__partial),

		TEST(spmap_it_remove_free__many),
		TEST(spmap_it_remove_free__partial),

		TEST(spmap_str__),

		TEST(spmap_keys_pslist__many),

		TEST(spmap_keys_sset__many),
		TEST(spmap_keys_sset__params),

		TEST(spmap_vals_pslist_clone__many),
		TEST(spmap_vals_pslist__many),

		TEST(spmap_vals_pset__many),
		TEST(spmap_vals_pset_clone__many),

		TEST(spmap_clone__many),
		TEST(spmap_clone__params),

		TEST(spmap_clone_deep__clone_val),
		TEST(spmap_clone_deep__no_clone_val),

		TEST(spmap__null_inputs),
	};

	return RUN(tests);
}


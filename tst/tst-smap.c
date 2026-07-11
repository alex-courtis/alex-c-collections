#include "assert-pset.h"
#include "assert-smap.h"
#include "assert-sset.h"
#include "asserts.h"
#include "expects.h"
#include "mock-fn.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pmap.h"
#include "pset.h"
#include "slist.h"
#include "sset.h"
#include "str.h"

#include "smap.h"

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

struct SMap {
	const struct SMapParams params;
	const struct PMap *pmap;
};

static int vals[4] = { 20, 21, 22, 23, };
static void *V0 = &vals[0];
static void *V1 = &vals[1];
static void *V2 = &vals[2];
static void *V3 = &vals[3];

static int datas[1] = { 30, };
static void *D0 = &datas[0];

static void smap_put_get_remove__case_sensitive(void **state) {

	const struct SMap *map = smap_init();
	assert_nul(smap_put(map, "a", V0));
	assert_nul(smap_put(map, "b", V1));
	assert_nul(smap_put(map, "c", V2));

	assert_int_equal(smap_size(map), 3);

	assert_ptr_equal(smap_get(map, "b"), V1);

	assert_nul(smap_get(map, "x"));

	assert_ptr_equal(smap_remove(map, "b"), V1);

	assert_nul(smap_get(map, "b"));

	smap_free(map);
}

static void smap_put_get_remove__case_insensitive(void **state) {
	const struct SMapParams params = { .case_insensitive = true, };
	const struct SMap *map = smap_init_with(params);

	assert_nul(smap_put(map, "A", V0));
	assert_nul(smap_put(map, "B", V1));

	assert_ptr_equal(smap_get(map, "b"), V1);

	assert_nul(smap_get(map, "x"));

	assert_ptr_equal(smap_remove(map, "b"), V1);

	assert_nul(smap_get(map, "b"));

	smap_free(map);
}

static void smap_free_vals__(void **state) {
	const struct SMap *map = smap_init();
	assert_nul(smap_put(map, "a", strdup("zero")));

	smap_free_vals(map);
}

static void smap_match__matches(void **state) {
	const struct SMap *map = smap_init();

	assert_nul(smap_put(map, "0", V0));
	assert_nul(smap_put(map, "1", V1));
	assert_nul(smap_put(map, "2", V2));

	// skip 0
	expect_string(mock_match_str_ptr, key, "0");
	expect_ptr(mock_match_str_ptr, val, V0);
	expect_ptr(mock_match_str_ptr, data, V2);
	will_return(mock_match_str_ptr, false);

	// get 1
	expect_string(mock_match_str_ptr, key, "1");
	expect_ptr(mock_match_str_ptr, val, V1);
	expect_ptr(mock_match_str_ptr, data, V2);
	will_return(mock_match_str_ptr, true);

	const struct SMapPair kv_pair = smap_match(map, mock_match_str_ptr, V2);
	assert_str_equal(kv_pair.key, "1");
	assert_ptr_equal(kv_pair.val, V1);

	smap_free(map);
}

static void smap_match_key__matches(void **state) {
	const struct SMap *map = smap_init();

	assert_nul(smap_put(map, "0", V0));
	assert_nul(smap_put(map, "1", V1));
	assert_nul(smap_put(map, "2", V2));

	// skip 0
	expect_string(mock_match_str, val, "0");
	expect_ptr(mock_match_str, data, V2);
	will_return(mock_match_str, false);

	// get 1
	expect_string(mock_match_str, val, "1");
	expect_ptr(mock_match_str, data, V2);
	will_return(mock_match_str, true);

	const struct SMapPair k_pair = smap_match_key(map, mock_match_str, V2);
	assert_str_equal(k_pair.key, "1");
	assert_ptr_equal(k_pair.val, V1);

	smap_free(map);
}

static void smap_match_val__matches(void **state) {
	const struct SMap *map = smap_init();

	assert_nul(smap_put(map, "0", V0));
	assert_nul(smap_put(map, "1", V1));
	assert_nul(smap_put(map, "2", V2));

	// skip 0
	expect_ptr(mock_match_ptr, val, V0);
	expect_ptr(mock_match_ptr, data, V2);
	will_return(mock_match_ptr, false);

	// get 1
	expect_ptr(mock_match_ptr, val, V1);
	expect_ptr(mock_match_ptr, data, V2);
	will_return(mock_match_ptr, true);

	const struct SMapPair v_pair = smap_match_val(map, mock_match_ptr, V2);
	assert_str_equal(v_pair.key, "1");
	assert_ptr_equal(v_pair.val, V1);

	smap_free(map);
}

static void smap_it__many(void **state) {
	const struct SMapParams params = { .allow_null_val = true, };
	const struct SMap *map = smap_init_with(params);

	assert_nul(smap_put(map, "a", V0));
	assert_nul(smap_put(map, "b", NULL));
	assert_nul(smap_put(map, "c", V2));

	const struct SMapIt *it = smap_it(map);

	assert_non_nul(it);
	assert_str_equal(it->key, "a");
	assert_ptr_equal(it->val, V0);

	it = smap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "b");
	assert_nul(it->val);

	smap_it_free(it);

	smap_free(map);
}

static void smap_it_free__partial(void **state) {
	const struct SMapIt *it = calloc(1, sizeof(struct SMapIt));

	smap_it_free(it);
}

static void smap_it_next__partial(void **state) {
	const struct SMapIt *it = calloc(1, sizeof(struct SMapIt));

	assert_nul(smap_it_next(it));
}

static void smap_it__empty(void **state) {

	const struct SMap *map = smap_init();

	const struct SMapIt *it = smap_it(map);

	assert_nul(it);

	smap_free(map);
}

static void smap_match_it__many(void **state) {
	const struct SMap *map = smap_init();

	assert_nul(smap_put(map, "0", V0));
	assert_nul(smap_put(map, "1", V1));
	assert_nul(smap_put(map, "2", V2));

	// skip "0"
	expect_string(mock_match_str_ptr, key, "0");
	expect_ptr(mock_match_str_ptr, val, V0);
	expect_ptr(mock_match_str_ptr, data, D0);
	will_return(mock_match_str_ptr, false);

	// get "1"
	expect_string(mock_match_str_ptr, key, "1");
	expect_ptr(mock_match_str_ptr, val, V1);
	expect_ptr(mock_match_str_ptr, data, D0);
	will_return(mock_match_str_ptr, true);

	const struct SMapIt *it = smap_match_it(map, mock_match_str_ptr, D0);
	assert_non_nul(it);
	assert_str_equal(it->key, "1");
	assert_ptr_equal(it->val, V1);

	// skip "2"
	expect_string(mock_match_str_ptr, key, "2");
	expect_ptr(mock_match_str_ptr, val, V2);
	expect_ptr(mock_match_str_ptr, data, D0);
	will_return(mock_match_str_ptr, false);

	// done
	it = smap_it_next(it);
	assert_nul(it);

	smap_free(map);
}

static void smap_match_key_it__many(void **state) {
	const struct SMap *map = smap_init();

	assert_nul(smap_put(map, "0", V0));
	assert_nul(smap_put(map, "1", V1));
	assert_nul(smap_put(map, "2", V2));

	// skip V0
	expect_string(mock_match_str, val, "0");
	expect_ptr(mock_match_str, data, D0);
	will_return(mock_match_str, false);

	// get V1
	expect_string(mock_match_str, val, "1");
	expect_ptr(mock_match_str, data, D0);
	will_return(mock_match_str, true);

	const struct SMapIt *it = smap_match_key_it(map, mock_match_str, D0);
	assert_non_nul(it);
	assert_str_equal(it->key, "1");
	assert_ptr_equal(it->val, V1);

	// skip V2
	expect_string(mock_match_str, val, "2");
	expect_ptr(mock_match_str, data, D0);
	will_return(mock_match_str, false);

	// done
	it = smap_it_next(it);
	assert_nul(it);

	smap_free(map);
}

static void smap_match_val_it__many(void **state) {
	const struct SMap *map = smap_init();

	assert_nul(smap_put(map, "0", V0));
	assert_nul(smap_put(map, "1", V1));
	assert_nul(smap_put(map, "2", V2));

	// skip V0
	expect_ptr(mock_match_ptr, val, V0);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// get V1
	expect_ptr(mock_match_ptr, val, V1);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, true);

	const struct SMapIt *it = smap_match_val_it(map, mock_match_ptr, D0);
	assert_non_nul(it);
	assert_str_equal(it->key, "1");
	assert_ptr_equal(it->val, V1);

	// skip V2
	expect_ptr(mock_match_ptr, val, V2);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// done
	it = smap_it_next(it);
	assert_nul(it);

	smap_free(map);
}

static void smap_equal__case_sensitive(void **state) {

	const struct SMap *actual = smap_init();
	assert_nul(smap_put(actual, "a", V0));
	assert_nul(smap_put(actual, "b", V1));

	assert_smap_not_equal(actual, NULL);

	const struct SMap *expected = smap_init();
	assert_nul(smap_put(expected, "a", V0));
	assert_nul(smap_put(expected, "b", V1));

	assert_smap_equal(actual, expected);

	smap_free(actual);
	smap_free(expected);
}

static void smap_equal__case_insensitive(void **state) {

	const struct SMapParams params = { .case_insensitive = true, };
	const struct SMap *actual = smap_init_with(params);

	assert_nul(smap_put(actual, "a", V0));
	assert_nul(smap_put(actual, "b", V1));

	const struct SMap *expected = smap_init();
	assert_nul(smap_put(expected, "A", V0));
	assert_nul(smap_put(expected, "B", V1));

	assert_smap_equal(actual, expected);

	assert_nul(smap_put(actual, "c", V2));

	assert_smap_not_equal(actual, expected);

	smap_free(actual);
	smap_free(expected);
}

static void smap_contains_key__(void **state) {
	const struct SMap *map = smap_init();

	assert_false(smap_contains_key(map, "a"));

	assert_nul(smap_put(map, "a", V0));
	assert_nul(smap_put(map, "b", V1));

	assert_true(smap_contains_key(map, "a"));
	assert_true(smap_contains_key(map, "b"));

	assert_false(smap_contains_key(map, "c"));

	assert_false(smap_contains_key(map, NULL));

	smap_free(map);
}

static void smap_contains_val__(void **state) {
	const struct SMap *map = smap_init();

	assert_false(smap_contains_key(map, V0));

	assert_nul(smap_put(map, "a", V0));
	assert_nul(smap_put(map, "b", V1));

	assert_true(smap_contains_val(map, V0));
	assert_true(smap_contains_val(map, V1));

	assert_false(smap_contains_val(map, V2));

	assert_false(smap_contains_val(map, NULL));

	smap_free(map);
}

static void smap_put_free__(void **state) {
	const struct SMapParams params = { .free_val = mock_free, };
	const struct SMap *map = smap_init_with(params);

	assert_nul(smap_put(map, "a", V0));

	assert_false(smap_put_free(map, "b", V1));

	expect_ptr(mock_free, ptr, V0);
	assert_true(smap_put_free(map, "a", V0));

	smap_free(map);
}

static void smap_put_all__variants(void **state) {
	const struct SMap *from = smap_init();
	assert_nul(smap_put(from, "a", V0));
	assert_nul(smap_put(from, "b", V1));

	const struct SMap *expected = smap_init();
	assert_nul(smap_put(expected, "a", V0));
	assert_nul(smap_put(expected, "b", V1));

	const struct SMapParams params = {
		.free_val = mock_free,
		.clone_val = mock_clone,
	};

	const struct SMap *to = smap_init_with(params);
	assert_nul(smap_put(to, "a", V3));

	// put_all
	const struct SMap *actual = smap_clone(to);

	assert_int_equal(smap_put_all(actual, from), 1);

	assert_smap_equal(actual, expected);
	smap_free(actual);

	// put_all_free
	actual = smap_clone(to);
	expect_ptr(mock_free, ptr, V3);

	assert_int_equal(smap_put_all_free(actual, from), 1);

	assert_smap_equal(actual, expected);
	smap_free(actual);

	// put_all_clone
	actual = smap_clone(to);
	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);
	expect_ptr(mock_clone, ptr, V1);
	will_return_ptr_type(mock_clone, V1, void*);

	assert_int_equal(smap_put_all_clone(actual, from), 1);

	assert_smap_equal(actual, expected);
	smap_free(actual);

	// put_all_clone_free
	actual = smap_clone(to);
	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);
	expect_ptr(mock_clone, ptr, V1);
	will_return_ptr_type(mock_clone, V1, void*);
	expect_ptr(mock_free, ptr, V3);

	assert_int_equal(smap_put_all_clone_free(actual, from), 1);

	assert_smap_equal(actual, expected);
	smap_free(actual);

	smap_free(to);
	smap_free(from);
	smap_free(expected);
}

static void smap_put_many__many(void **state) {
	const struct SMap *to = smap_init();
	assert_nul(smap_put(to, "a", V0));
	assert_nul(smap_put(to, "b", strdup("replaced")));

	const struct SMap *expected = smap_init();
	assert_nul(smap_put(expected, "a", V0));
	assert_nul(smap_put(expected, "b", V1));
	assert_nul(smap_put(expected, "c", V2));

	assert_int_equal(smap_put_many(to,
				"b", V1,
				"c", V2,
				NULL),
			1);

	assert_smap_equal(to, expected);

	smap_free(to);
	smap_free(expected);
}

static void smap_put_if_absent__(void **state) {
	const struct SMap *map = smap_init();

	assert_nul(smap_put_if_absent(map, "a", V0));
	assert_ptr_equal(smap_get(map, "a"), V0);

	const void *existing = smap_put_if_absent(map, "a", V1);
	assert_ptr_equal(existing, V0);

	smap_free(map);
}

static void smap_remove_free__(void **state) {
	const struct SMapParams params = { .free_val = mock_free, };
	const struct SMap *map = smap_init_with(params);

	assert_nul(smap_put(map, "a", V0));

	assert_false(smap_remove_free(map, "b"));

	expect_ptr(mock_free, ptr, V0);
	assert_true(smap_remove_free(map, "a"));

	smap_free(map);
}

static void smap_remove_all__(void **state) {
	const struct SMap *map = smap_init();

	assert_nul(smap_put(map, "a", V0));
	assert_nul(smap_put(map, "b", V1));
	assert_nul(smap_put(map, "c", V2));

	const struct SMap *from = smap_init();

	assert_nul(smap_put(from, "b", V1));
	assert_nul(smap_put(from, "d", V3));

	const struct SMap *expected = smap_init();

	assert_nul(smap_put(expected, "a", V0));
	assert_nul(smap_put(expected, "c", V2));

	assert_int_equal(smap_remove_all(map, from), 1);

	assert_smap_equal(map, expected);

	smap_free(map);
	smap_free(from);
	smap_free(expected);
}

static void smap_remove_all_free__(void **state) {
	const struct SMapParams params = { .free_val = mock_free, };
	const struct SMap *map = smap_init_with(params);

	assert_nul(smap_put(map, "a", V0));

	const struct SMap *from = smap_init();

	assert_nul(smap_put(from, "a", V0));

	expect_ptr(mock_free, ptr, V0);

	assert_int_equal(smap_remove_all_free(map, from), 1);

	smap_free(map);
	smap_free(from);
}

static void smap_str__(void **state) {
	const struct SMapParams params = { .allow_null_val = true, };
	const struct SMap *map = smap_init_with(params);

	assert_nul(smap_put(map, "a", V0));
	assert_nul(smap_put(map, "b", NULL));
	assert_nul(smap_put(map, "c", V2));

	char *expected = sprintf_alloc(
			"a = %p\n"
			"b = (null)\n"
			"c = %p\n",
			V0,
			V2
			);

	char *actual = smap_str(map);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	smap_free(map);
}

static void smap_keys_slist__many(void **state) {
	const struct SMap *map = smap_init();

	smap_put(map, "a", V0);
	smap_put(map, "b", V1);

	struct SList *list = smap_keys_slist(map);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "a");
	assert_str_equal(slist_at(list, 1), "b");

	smap_free(map);
	slist_free_vals(&list, NULL);
}

static void smap_keys_sset__many(void **state) {
	const struct SMap *map = smap_init();

	smap_put(map, "a", V0);
	smap_put(map, "b", V1);

	const struct SSet *expected = sset_init();
	sset_add(expected, "a");
	sset_add(expected, "b");

	const struct SSet *actual = smap_keys_sset(map);

	assert_sset_equal(actual, expected);

	smap_free(map);
	sset_free(expected);
	sset_free(actual);
}

static void smap_keys_sset__params(void **state) {
	const struct SMapParams params = {
		.case_insensitive = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SMap *map = smap_init_with(params);

	const struct SSet *set = smap_keys_sset(map);

	assert_true(set->params.case_insensitive);
	assert_int_equal(set->params.initial, 99);
	assert_int_equal(set->params.grow, 1);

	smap_free(map);

	sset_free(set);
}

static void smap_vals_slist__many(void **state) {
	const struct SMapParams params = { .allow_null_val = true, };
	const struct SMap *map = smap_init_with(params);

	smap_put(map, "a", V0);
	smap_put(map, "b", NULL);
	smap_put(map, "c", V2);

	struct SList *list = smap_vals_slist(map);

	assert_int_equal(slist_length(list), 3);
	assert_ptr_equal(slist_at(list, 0), V0);
	assert_nul(slist_at(list, 1));
	assert_ptr_equal(slist_at(list, 2), V2);

	slist_free(&list);
	smap_free(map);
}

static void smap_vals_pset__many(void **state) {
	const struct SMapParams params = { .allow_null_val = true, };
	const struct SMap *map = smap_init_with(params);

	smap_put(map, "a", V0);
	smap_put(map, "b", NULL);
	smap_put(map, "c", V2);

	const struct PSet *expected = pset_init();
	pset_add(expected, V0);
	pset_add(expected, V2);

	const struct PSet *actual = smap_vals_pset(map);

	assert_pset_equal(actual, expected);

	smap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void smap_vals_pset_clone__many(void **state) {
	const struct SMapParams params = { .clone_val = mock_clone, };
	const struct SMap *map = smap_init_with(params);

	smap_put(map, "a", V0);

	const struct PSet *expected = pset_init();
	pset_add(expected, V0);

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	const struct PSet *actual = smap_vals_pset_clone(map);

	assert_pset_equal(actual, expected);

	smap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void smap_vals_slist_clone__many(void **state) {
	const struct SMapParams params = {
		.allow_null_val = true,
		.clone_val = (fn_clone)clone_strdup,
	};
	const struct SMap *map = smap_init_with(params);

	smap_put(map, "a", "aa");
	smap_put(map, "b", NULL);
	smap_put(map, "c", "bb");

	struct SList *list = smap_vals_slist_clone(map);

	assert_int_equal(slist_length(list), 3);
	assert_str_equal(slist_at(list, 0), "aa");
	assert_nul(slist_at(list, 1));
	assert_str_equal(slist_at(list, 2), "bb");

	slist_free_vals(&list, NULL);
	smap_free(map);
}

static void smap_clone__many(void **state) {
	const struct SMapParams params = { .allow_null_val = true, };
	const struct SMap *from = smap_init_with(params);

	assert_nul(smap_put(from, "a", V0));
	assert_nul(smap_put(from, "b", NULL));
	assert_nul(smap_put(from, "c", V2));

	const struct SMap *to = smap_clone(from);

	assert_non_nul(to);

	assert_int_equal(smap_size(to), 3);

	assert_smap_equal(from, to);

	smap_free(from);
	smap_free(to);
}

// also tests constructor
static void smap_clone__params(void **state) {
	const struct SMapParams params = {
		.case_insensitive = true,
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.free_val = mock_free,
		.clone_val = mock_clone,
		.allow_null_val = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SMap *from = smap_init_with(params);

	const struct SMap *to = smap_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->pmap->size, 0);
	assert_int_equal(to->pmap->capacity, 99);
	assert_true(to->pmap->params.allow_null_val);
	assert_int_equal(to->pmap->params.grow, 1);
	assert_ptr_equal(to->pmap->params.equal_key, equal_strcasecmp);
	assert_ptr_equal(to->pmap->params.equal_val, mock_equal);
	assert_ptr_equal(to->pmap->params.alloc_key, clone_strdup);
	assert_ptr_equal(to->pmap->params.alloc_val, mock_alloc);
	assert_ptr_equal(to->pmap->params.free_key, (fn_free)free);
	assert_ptr_equal(to->pmap->params.free_val, mock_free);
	assert_ptr_equal(to->pmap->params.clone_val, mock_clone);

	assert_true(to->params.case_insensitive);
	assert_ptr_equal(to->params.equal_val, mock_equal);
	assert_ptr_equal(to->params.alloc_val, mock_alloc);
	assert_ptr_equal(to->params.free_val, mock_free);
	assert_ptr_equal(to->params.clone_val, mock_clone);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	smap_free(from);
	smap_free(to);
}

static void smap_clone_deep__clone_val(void **state) {
	const struct SMapParams params = { .clone_val = mock_clone, };
	const struct SMap *from = smap_init_with(params);

	assert_nul(smap_put(from, "a", V0));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	const struct SMap *to = smap_clone_deep(from);

	assert_non_nul(to);

	assert_int_equal(smap_size(to), 1);

	assert_smap_equal(from, to);

	smap_free(from);
	smap_free(to);
}

static void smap_clone_deep__no_clone_val(void **state) {
	const struct SMap *from = smap_init();

	assert_nul(smap_put(from, 0, V0));

	const struct SMap *to = smap_clone_deep(from);
	assert_non_nul(to);
	assert_int_equal(smap_size(to), 0);

	smap_free(from);
	smap_free(to);
}

static void smap__null_inputs(void **state) {
	const struct SMap *map = smap_init();

	assert_nul(smap_clone(NULL));
	assert_nul(smap_clone_deep(NULL));
	smap_free(NULL);
	smap_free_vals(NULL);
	smap_it_free(NULL);
	assert_false(smap_get(NULL, NULL));
	assert_false(smap_get(map, NULL));
	assert_false(smap_contains_key(NULL, NULL));
	assert_false(smap_contains_key(map, NULL));
	assert_false(smap_contains_val(NULL, NULL));
	assert_false(smap_contains_val(map, NULL));
	smap_match(NULL, NULL, NULL);
	smap_match(map, NULL, NULL);
	smap_match_key(NULL, NULL, NULL);
	smap_match_key(map, NULL, NULL);
	smap_match_val(NULL, NULL, NULL);
	smap_match_val(map, NULL, NULL);
	assert_nul(smap_it(NULL));
	assert_nul(smap_match_it(NULL, NULL, NULL));
	assert_nul(smap_match_it(map, NULL, NULL));
	assert_nul(smap_match_key_it(NULL, NULL, NULL));
	assert_nul(smap_match_key_it(map, NULL, NULL));
	assert_nul(smap_match_val_it(NULL, NULL, NULL));
	assert_nul(smap_match_val_it(map, NULL, NULL));
	assert_nul(smap_it_next(NULL));
	assert_nul(smap_put(NULL, NULL, NULL));
	assert_nul(smap_put(map, NULL, NULL));
	assert_nul(smap_put_if_absent(NULL, NULL, NULL));
	assert_nul(smap_put_if_absent(map, NULL, NULL));
	assert_false(smap_put_free(NULL, NULL, NULL));
	assert_false(smap_put_free(map, NULL, NULL));
	assert_int_equal(smap_put_all(NULL, NULL), 0);
	assert_int_equal(smap_put_all(map, NULL), 0);
	assert_int_equal(smap_put_all_free(NULL, NULL), 0);
	assert_int_equal(smap_put_all_free(map, NULL), 0);
	assert_int_equal(smap_put_all_clone(NULL, NULL), 0);
	assert_int_equal(smap_put_all_clone(map, NULL), 0);
	assert_int_equal(smap_put_all_clone_free(NULL, NULL), 0);
	assert_int_equal(smap_put_all_clone_free(map, NULL), 0);
	assert_int_equal(smap_put_many(NULL, NULL), 0);
	assert_nul(smap_remove(NULL, NULL));
	assert_nul(smap_remove(map, NULL));
	assert_false(smap_remove_free(NULL, NULL));
	assert_false(smap_remove_free(map, NULL));
	assert_int_equal(smap_remove_all(NULL, NULL), 0);
	assert_int_equal(smap_remove_all(map, NULL), 0);
	assert_int_equal(smap_remove_all(NULL, map), 0);
	assert_int_equal(smap_remove_all_free(NULL, NULL), 0);
	assert_int_equal(smap_remove_all_free(map, NULL), 0);
	assert_int_equal(smap_remove_all_free(NULL, map), 0);
	assert_false(smap_equal(NULL, NULL));
	assert_false(smap_equal(map, NULL));
	assert_nul(smap_keys_slist(NULL));
	assert_nul(smap_keys_sset(NULL));
	assert_nul(smap_vals_slist(NULL));
	assert_nul(smap_vals_slist_clone(NULL));
	assert_nul(smap_vals_pset(NULL));
	assert_nul(smap_vals_pset_clone(NULL));
	assert_nul(smap_str(NULL));
	assert_int_equal(smap_size(NULL), 0);

	smap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(smap_put_get_remove__case_sensitive),
		TEST(smap_put_get_remove__case_insensitive),

		TEST(smap_free_vals__),

		TEST(smap_match__matches),
		TEST(smap_match_key__matches),
		TEST(smap_match_val__matches),

		TEST(smap_it__many),
		TEST(smap_it__empty),

		TEST(smap_it_free__partial),

		TEST(smap_it_next__partial),

		TEST(smap_match_it__many),
		TEST(smap_match_key_it__many),
		TEST(smap_match_val_it__many),

		TEST(smap_equal__case_sensitive),
		TEST(smap_equal__case_insensitive),

		TEST(smap_contains_key__),

		TEST(smap_contains_val__),

		TEST(smap_put_free__),

		TEST(smap_put_all__variants),

		TEST(smap_put_many__many),

		TEST(smap_put_if_absent__),

		TEST(smap_remove_free__),

		TEST(smap_remove_all__),

		TEST(smap_remove_all_free__),

		TEST(smap_str__),

		TEST(smap_keys_slist__many),

		TEST(smap_keys_sset__many),
		TEST(smap_keys_sset__params),

		TEST(smap_vals_slist_clone__many),
		TEST(smap_vals_slist__many),

		TEST(smap_vals_pset__many),
		TEST(smap_vals_pset_clone__many),

		TEST(smap_clone__many),
		TEST(smap_clone__params),

		TEST(smap_clone_deep__clone_val),
		TEST(smap_clone_deep__no_clone_val),

		TEST(smap__null_inputs),
	};

	return RUN(tests);
}


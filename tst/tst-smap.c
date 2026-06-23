#include "tst.h"
#include "asserts.h"
#include "assert-smap.h"
#include "expects.h"
#include "mock-fn.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pmap.h"
#include "slist.h"
#include "str.h"

#include "smap.h"

struct PMap {
	const struct PMapParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct SMap {
	const struct SMapParams params;
	const struct PMap *pmap;
};

static int vals[3] = { 20, 21, 22, };
static void *V0 = &vals[0];
static void *V1 = &vals[1];
static void *V2 = &vals[2];

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

static void smap_find__matches(void **state) {
	const struct SMap *map = smap_init();

	assert_nul(smap_put(map, "0", V0));
	assert_nul(smap_put(map, "1", V1));
	assert_nul(smap_put(map, "2", V2));

	// skip 0
	expect_string(mock_match_key_val, key, "0");
	expect_ptr(mock_match_key_val, val, V0);
	expect_ptr(mock_match_key_val, data, V2);
	will_return(mock_match_key_val, false);

	// get 1
	expect_string(mock_match_key_val, key, "1");
	expect_ptr(mock_match_key_val, val, V1);
	expect_ptr(mock_match_key_val, data, V2);
	will_return(mock_match_key_val, true);

	const struct SMapPair pair = smap_find(map, mock_match_key_val, V2);
	assert_str_equal(pair.key, "1");
	assert_ptr_equal(pair.val, V1);

	smap_free(map);
}

static void smap_iter__many(void **state) {

	const struct SMap *map = smap_init();
	assert_nul(smap_put(map, "a", V0));
	assert_nul(smap_put(map, "b", NULL));
	assert_nul(smap_put(map, "c", V2));

	const struct SMapIter *iter = smap_iter(map);

	assert_non_nul(iter);
	assert_str_equal(iter->key, "a");
	assert_ptr_equal(iter->val, V0);

	iter = smap_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "b");
	assert_nul(iter->val);

	smap_iter_free(iter);

	smap_free(map);
}

static void smap_iter_free__partial(void **state) {
	const struct SMapIter *iter = calloc(1, sizeof(struct SMapIter));

	smap_iter_free(iter);
}

static void smap_iter_next__partial(void **state) {
	const struct SMapIter *iter = calloc(1, sizeof(struct SMapIter));

	assert_nul(smap_iter_next(iter));
}

static void smap_iter__empty(void **state) {

	const struct SMap *map = smap_init();

	const struct SMapIter *iter = smap_iter(map);

	assert_nul(iter);

	smap_free(map);
}

static void smap_match_iter__many(void **state) {
	const struct SMap *map = smap_init();

	assert_nul(smap_put(map, "0", V0));
	assert_nul(smap_put(map, "1", V1));
	assert_nul(smap_put(map, "2", V2));

	// skip "0"
	expect_string(mock_match_key_val, key, "0");
	expect_ptr(mock_match_key_val, val, V0);
	expect_ptr(mock_match_key_val, data, D0);
	will_return(mock_match_key_val, false);

	// get "1"
	expect_string(mock_match_key_val, key, "1");
	expect_ptr(mock_match_key_val, val, V1);
	expect_ptr(mock_match_key_val, data, D0);
	will_return(mock_match_key_val, true);

	const struct SMapIter *iter = smap_match_iter(map, mock_match_key_val, D0);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "1");
	assert_ptr_equal(iter->val, V1);

	// skip V2
	expect_string(mock_match_key_val, key, "2");
	expect_ptr(mock_match_key_val, val, V2);
	expect_ptr(mock_match_key_val, data, D0);
	will_return(mock_match_key_val, false);

	// done
	iter = smap_iter_next(iter);
	assert_nul(iter);

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

static void smap_put_free__(void **state) {
	const struct SMapParams params = { .free_val = mock_free, };
	const struct SMap *map = smap_init_with(params);

	assert_nul(smap_put(map, "a", V0));

	assert_false(smap_put_free(map, "b", V1));

	expect_ptr(mock_free, val, V0);
	assert_true(smap_put_free(map, "a", V0));

	smap_free(map);
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

	expect_ptr(mock_free, val, V0);
	assert_true(smap_remove_free(map, "a"));

	smap_free(map);
}

static void smap_str__(void **state) {

	const struct SMap *map = smap_init();
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

static void smap_keys_slist_deep__many(void **state) {
	const struct SMap *map = smap_init();

	smap_put(map, "a", V0);
	smap_put(map, "b", V1);

	struct SList *list = smap_keys_slist_deep(map);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "a");
	assert_str_equal(slist_at(list, 1), "b");

	smap_free(map);
	slist_free_vals(&list, NULL);
}

static void smap_vals_slist_shallow__many(void **state) {
	const struct SMap *map = smap_init();

	smap_put(map, "a", V0);
	smap_put(map, "b", NULL);
	smap_put(map, "c", V2);

	struct SList *list = smap_vals_slist_shallow(map);

	assert_int_equal(slist_length(list), 3);
	assert_ptr_equal(slist_at(list, 0), V0);
	assert_nul(slist_at(list, 1));
	assert_ptr_equal(slist_at(list, 2), V2);

	slist_free(&list);
	smap_free(map);
}

static void smap_vals_slist_deep__many(void **state) {
	const struct SMapParams params = { .clone_val = fn_clone_strdup, };
	const struct SMap *map = smap_init_with(params);

	smap_put(map, "a", "aa");
	smap_put(map, "b", NULL);
	smap_put(map, "c", "bb");

	struct SList *list = smap_vals_slist_deep(map);

	assert_int_equal(slist_length(list), 3);
	assert_str_equal(slist_at(list, 0), "aa");
	assert_nul(slist_at(list, 1));
	assert_str_equal(slist_at(list, 2), "bb");

	slist_free_vals(&list, NULL);
	smap_free(map);
}

static void smap_clone_shallow__many(void **state) {
	const struct SMap *from = smap_init();

	assert_nul(smap_put(from, "a", V0));
	assert_nul(smap_put(from, "b", NULL));
	assert_nul(smap_put(from, "c", V2));

	const struct SMap *to = smap_clone_shallow(from);

	assert_non_nul(to);

	assert_int_equal(smap_size(to), 3);

	assert_smap_equal(from, to);

	smap_free(from);
	smap_free(to);
}

// also tests constructor
static void smap_clone_shallow__params(void **state) {
	const struct SMapParams params = {
		.case_insensitive = true,
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.free_val = mock_free,
		.clone_val = mock_clone,
		.initial = 99,
		.grow = 1,
	};
	const struct SMap *from = smap_init_with(params);

	const struct SMap *to = smap_clone_shallow(from);

	assert_non_nul(to);

	assert_int_equal(to->pmap->size, 0);
	assert_int_equal(to->pmap->capacity, 99);
	assert_int_equal(to->pmap->params.grow, 1);
	assert_ptr_equal(to->pmap->params.equal_key, fn_equal_strcasecmp);
	assert_ptr_equal(to->pmap->params.equal_val, mock_equal);
	assert_ptr_equal(to->pmap->params.alloc_key, fn_clone_strdup);
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

	expect_ptr(mock_clone, val, V0);
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

	assert_nul(smap_clone_shallow(NULL));
	assert_nul(smap_clone_deep(NULL));
	smap_free(NULL);
	smap_free_vals(NULL);
	smap_iter_free(NULL);
	assert_false(smap_get(NULL, NULL));
	assert_false(smap_get(map, NULL));
	assert_false(smap_contains_key(NULL, NULL));
	smap_find(NULL, NULL, NULL);
	smap_find(NULL, mock_match_key_val, NULL);
	assert_nul(smap_iter(NULL));
	assert_nul(smap_match_iter(NULL, NULL, NULL));
	assert_nul(smap_iter_next(NULL));
	assert_nul(smap_put(NULL, NULL, NULL));
	assert_nul(smap_put(map, NULL, NULL));
	assert_nul(smap_put_if_absent(NULL, NULL, NULL));
	assert_nul(smap_put_if_absent(map, NULL, NULL));
	assert_false(smap_put_free(NULL, NULL, NULL));
	assert_false(smap_put_free(map, NULL, NULL));
	assert_nul(smap_remove(NULL, NULL));
	assert_nul(smap_remove(map, NULL));
	assert_false(smap_remove_free(NULL, NULL));
	assert_false(smap_remove_free(map, NULL));
	assert_false(smap_equal(NULL, NULL));
	assert_false(smap_equal(map, NULL));
	assert_nul(smap_keys_slist_deep(NULL));
	assert_nul(smap_vals_slist_shallow(NULL));
	assert_nul(smap_vals_slist_deep(NULL));
	assert_nul(smap_str(NULL));
	assert_int_equal(smap_size(NULL), 0);

	smap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(smap_put_get_remove__case_sensitive),
		TEST(smap_put_get_remove__case_insensitive),

		TEST(smap_free_vals__),

		TEST(smap_find__matches),

		TEST(smap_iter__many),
		TEST(smap_iter__empty),

		TEST(smap_iter_free__partial),

		TEST(smap_iter_next__partial),

		TEST(smap_match_iter__many),

		TEST(smap_equal__case_sensitive),
		TEST(smap_equal__case_insensitive),

		TEST(smap_contains_key__),

		TEST(smap_put_free__),

		TEST(smap_put_if_absent__),

		TEST(smap_remove_free__),

		TEST(smap_str__),

		TEST(smap_keys_slist_deep__many),

		TEST(smap_vals_slist_deep__many),
		TEST(smap_vals_slist_shallow__many),

		TEST(smap_clone_shallow__many),
		TEST(smap_clone_shallow__params),

		TEST(smap_clone_deep__clone_val),
		TEST(smap_clone_deep__no_clone_val),

		TEST(smap__null_inputs),
	};

	return RUN(tests);
}


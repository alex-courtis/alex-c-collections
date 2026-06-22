#include "tst.h"
#include "asserts.h"
#include "assert-imap.h"
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

#include "imap.h"

struct PMap {
	const struct PMapParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct IMap {
	const struct IMapParams params;
	const struct PMap *pmap;
};

static int vals[3] = { 20, 21, 22, };
static void *V0 = &vals[0];
static void *V1 = &vals[1];
static void *V2 = &vals[2];

static int datas[1] = { 30, };
static void *D0 = &datas[0];

static void imap_put_get_remove(void **state) {
	const struct IMapParams params = { 0 };
	const struct IMap *map = imap_init_with(params);

	assert_nul(imap_put(map, 0, V0));
	assert_nul(imap_put(map, 1, V1));
	assert_nul(imap_put(map, 2, V2));

	assert_int_equal(imap_size(map), 3);

	assert_ptr_equal(imap_get(map, 1), V1);

	assert_nul(imap_get(map, 999));

	assert_ptr_equal(imap_remove(map, 1), V1);

	assert_nul(imap_get(map, 1));

	imap_free(map);
}

static void imap_free_vals__(void **state) {
	const struct IMap *map = imap_init();
	assert_nul(imap_put(map, 0, strdup("zero")));

	imap_free_vals(map);
}

static void imap_iter__many(void **state) {

	const struct IMap *map = imap_init();
	assert_nul(imap_put(map, 0, V0));
	assert_nul(imap_put(map, 1, NULL));
	assert_nul(imap_put(map, 2, V2));

	const struct IMapIter *iter = imap_iter(map);

	assert_non_nul(iter);
	assert_int_equal(iter->key, 0);
	assert_ptr_equal(iter->val, V0);

	iter = imap_iter_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, 1);
	assert_nul(iter->val);

	imap_iter_free(iter);

	imap_free(map);
}

static void imap_iter__empty(void **state) {

	const struct IMap *map = imap_init();

	const struct IMapIter *iter = imap_iter(map);

	assert_nul(iter);

	imap_free(map);
}

static void imap_iter_free__partial(void **state) {
	const struct IMapIter *iter = calloc(1, sizeof(struct IMapIter));

	imap_iter_free(iter);
}

static void imap_iter_next__partial(void **state) {
	const struct IMapIter *iter = calloc(1, sizeof(struct IMapIter));

	assert_nul(imap_iter_next(iter));
}

static void imap_match_iter__many(void **state) {
	const struct IMap *map = imap_init();

	assert_nul(imap_put(map, 0, V0));
	assert_nul(imap_put(map, 1, V1));
	assert_nul(imap_put(map, 2, V2));

	// skip K0
	expect_int_value(mock_match_size_t_val, key, 0);
	expect_ptr(mock_match_size_t_val, val, V0);
	expect_ptr(mock_match_size_t_val, data, D0);
	will_return(mock_match_size_t_val, false);

	// pass K1
	expect_int_value(mock_match_size_t_val, key, 1);
	expect_ptr(mock_match_size_t_val, val, V1);
	expect_ptr(mock_match_size_t_val, data, D0);
	will_return(mock_match_size_t_val, true);

	const struct IMapIter *iter = imap_match_iter(map, mock_match_size_t_val, D0);
	assert_non_nul(iter);
	assert_int_equal(iter->key, 1);
	assert_ptr_equal(iter->val, V1);

	// skip K2
	expect_int_value(mock_match_size_t_val, key, 2);
	expect_ptr(mock_match_size_t_val, val, V2);
	expect_ptr(mock_match_size_t_val, data, D0);
	will_return(mock_match_size_t_val, false);

	// done
	iter = imap_iter_next(iter);
	assert_nul(iter);

	imap_free(map);
}

static void imap_equal__(void **state) {

	const struct IMap *actual = imap_init();
	assert_nul(imap_put(actual, 0, V0));
	assert_nul(imap_put(actual, 1, V1));

	assert_imap_not_equal(actual, NULL);

	const struct IMap *expected = imap_init();
	assert_nul(imap_put(expected, 0, V0));
	assert_nul(imap_put(expected, 1, V1));

	assert_imap_equal(actual, expected);

	assert_nul(imap_put(actual, 2, V2));

	assert_imap_not_equal(actual, expected);

	imap_free(actual);
	imap_free(expected);
}

static void imap_equal__key_removed(void **state) {
	const struct IMap *a = imap_init();
	assert_nul(imap_put(a, 0, V0));
	assert_nul(imap_put(a, 1, V1));

	const struct IMap *b = imap_init();
	assert_nul(imap_put(b, 0, V0));
	assert_nul(imap_put(b, 1, V1));

	int *removed_key = (int*)b->pmap->keys[0];
	b->pmap->keys[0] = NULL;

	assert_imap_not_equal(a, b);

	free(removed_key);
	imap_free(a);
	imap_free(b);
}

static void imap_contains_key__(void **state) {
	const struct IMap *map = imap_init();

	assert_false(imap_contains_key(map, 0));

	assert_nul(imap_put(map, 0, V0));
	assert_nul(imap_put(map, 1, V1));

	assert_true(imap_contains_key(map, 0));
	assert_true(imap_contains_key(map, 1));

	assert_false(imap_contains_key(map, 2));

	imap_free(map);
}

static void imap_get__key_removed(void **state) {

	const struct IMap *actual = imap_init();
	assert_nul(imap_put(actual, 0, V0));

	int *removed_key = (int*)actual->pmap->keys[0];
	actual->pmap->keys[0] = NULL;

	assert_nul(imap_get(actual, 0));

	free(removed_key);
	imap_free(actual);
}

static void imap_put_free__(void **state) {
	const struct IMap *map = imap_init();

	const char *val = strdup("val");

	assert_nul(imap_put(map, 0, val));

	assert_false(imap_put_free(map, 1, V1));

	assert_true(imap_put_free(map, 0, V0));

	imap_free(map);
}

static void imap_put_if_absent__(void **state) {
	const struct IMap *map = imap_init();

	assert_nul(imap_put_if_absent(map, 0, V0));
	assert_ptr_equal(imap_get(map, 0), V0);

	const void *existing = imap_put_if_absent(map, 0, V1);
	assert_ptr_equal(existing, V0);

	imap_free(map);
}

static void imap_remove_free__(void **state) {
	const struct IMap *map = imap_init();

	const char *val = strdup("val");

	assert_nul(imap_put(map, 0, val));

	assert_true(imap_remove_free(map, 0));

	assert_false(imap_remove_free(map, 1));

	imap_free(map);
}

static void imap_str__(void **state) {

	const struct IMap *map = imap_init();
	assert_nul(imap_put(map, 0, V0));
	assert_nul(imap_put(map, 1, NULL));
	assert_nul(imap_put(map, 999, V2));

	char *expected = sprintf_alloc(
			"0 = %p\n"
			"1 = (null)\n"
			"999 = %p\n",
			V0,
			V2
			);

	char *actual = imap_str(map);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	imap_free(map);
}

static void imap_vals_slist_shallow__many(void **state) {
	const struct IMap *map = imap_init();

	imap_put(map, 0, V0);
	imap_put(map, 1, NULL);
	imap_put(map, 2, V2);

	struct SList *list = imap_vals_slist_shallow(map);

	assert_int_equal(slist_length(list), 3);
	assert_ptr_equal(slist_at(list, 0), V0);
	assert_nul(slist_at(list, 1));
	assert_ptr_equal(slist_at(list, 2), V2);

	slist_free(&list);
	imap_free(map);
}

static void imap_vals_slist_deep__many(void **state) {
	const struct IMapParams params = { .clone_val = fn_clone_strdup, };
	const struct IMap *map = imap_init_with(params);

	imap_put(map, 0, "0");
	imap_put(map, 1, NULL);
	imap_put(map, 2, "2");

	struct SList *list = imap_vals_slist_deep(map);

	assert_int_equal(slist_length(list), 3);
	assert_str_equal(slist_at(list, 0), "0");
	assert_nul(slist_at(list, 1));
	assert_str_equal(slist_at(list, 2), "2");

	slist_free_vals(&list, NULL);
	imap_free(map);
}

static void imap_clone_shallow__many(void **state) {
	const struct IMap *from = imap_init();

	assert_nul(imap_put(from, 0, V0));
	assert_nul(imap_put(from, 1, NULL));
	assert_nul(imap_put(from, 2, V2));

	const struct IMap *to = imap_clone_shallow(from);

	assert_non_nul(to);

	assert_int_equal(imap_size(to), 3);

	assert_imap_equal(from, to);

	imap_free(from);
	imap_free(to);
}

// also tests constructor
static void imap_clone_shallow__params(void **state) {
	const struct IMapParams params = {
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.free_val = mock_free,
		.clone_val = mock_clone,
		.initial = 99,
		.grow = 1,
	};
	const struct IMap *from = imap_init_with(params);

	const struct IMap *to = imap_clone_shallow(from);

	assert_non_nul(to);

	// commented out are tested elsewhere
	assert_int_equal(to->pmap->size, 0);
	assert_int_equal(to->pmap->capacity, 99);
	assert_int_equal(to->pmap->params.grow, 1);
	assert_ptr_equal(to->pmap->params.equal_val, mock_equal);
	assert_ptr_equal(to->pmap->params.alloc_val, mock_alloc);
	assert_ptr_equal(to->pmap->params.free_key, (fn_free)free);
	assert_ptr_equal(to->pmap->params.free_val, mock_free);
	assert_ptr_equal(to->pmap->params.clone_val, mock_clone);

	assert_ptr_equal(to->params.equal_val, mock_equal);
	assert_ptr_equal(to->params.free_val, mock_free);
	assert_ptr_equal(to->params.clone_val, mock_clone);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	imap_free(from);
	imap_free(to);
}

static void imap_clone_deep__clone_val(void **state) {
	const struct IMapParams params = { .clone_val = mock_clone, };
	const struct IMap *from = imap_init_with(params);

	assert_nul(imap_put(from, 0, V0));

	expect_ptr(mock_clone, val, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	const struct IMap *to = imap_clone_deep(from);

	assert_non_nul(to);

	assert_int_equal(imap_size(to), 1);

	assert_imap_equal(from, to);

	imap_free(from);
	imap_free(to);
}

static void imap_clone_deep__no_clone_val(void **state) {
	const struct IMap *from = imap_init();

	assert_nul(imap_put(from, 0, V0));

	const struct IMap *to = imap_clone_deep(from);
	assert_non_nul(to);
	assert_int_equal(imap_size(to), 0);

	imap_free(from);
	imap_free(to);
}

static void imap__null_inputs(void **state) {
	const struct IMap *map = imap_init();

	assert_nul(imap_clone_shallow(NULL));
	assert_nul(imap_clone_deep(NULL));
	imap_free(NULL);
	imap_free_vals(NULL);
	imap_iter_free(NULL);
	assert_false(imap_get(NULL, 0));
	assert_false(imap_contains_key(NULL, 0));
	assert_nul(imap_iter(NULL));
	assert_nul(imap_match_iter(NULL, NULL, NULL));
	assert_nul(imap_iter_next(NULL));
	assert_false(imap_put(NULL, 0, NULL));
	assert_nul(imap_put_if_absent(NULL, 0, NULL));
	assert_nul(imap_put_if_absent(map, 0, NULL));
	assert_false(imap_put_free(NULL, 0, NULL));
	assert_false(imap_put_free(map, 0, NULL));
	assert_nul(imap_remove(NULL, 0));
	assert_nul(imap_remove(map, 0));
	assert_false(imap_remove_free(NULL, 0));
	assert_false(imap_remove_free(map, 0));
	assert_false(imap_equal(NULL, NULL));
	assert_false(imap_equal(map, NULL));
	assert_nul(imap_vals_slist_shallow(NULL));
	assert_nul(imap_vals_slist_deep(NULL));
	assert_nul(imap_str(NULL));
	assert_int_equal(imap_size(NULL), 0);

	imap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(imap_put_get_remove),

		TEST(imap_free_vals__),

		TEST(imap_iter__many),
		TEST(imap_iter__empty),

		TEST(imap_iter_free__partial),

		TEST(imap_iter_next__partial),

		TEST(imap_match_iter__many),

		TEST(imap_equal__),
		TEST(imap_equal__key_removed),

		TEST(imap_contains_key__),

		TEST(imap_get__key_removed),

		TEST(imap_put_free__),

		TEST(imap_put_if_absent__),

		TEST(imap_remove_free__),

		TEST(imap_str__),

		TEST(imap_vals_slist_shallow__many),
		TEST(imap_vals_slist_deep__many),

		TEST(imap_clone_shallow__many),
		TEST(imap_clone_shallow__params),

		TEST(imap_clone_deep__clone_val),
		TEST(imap_clone_deep__no_clone_val),

		TEST(imap__null_inputs),
	};

	return RUN(tests);
}


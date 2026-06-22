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
	const struct PMap *ptab;
};

static int vals[3] = { 20, 21, 22, };
static void *V0 = &vals[0];
static void *V1 = &vals[1];
static void *V2 = &vals[2];

static int datas[1] = { 30, };
static void *D0 = &datas[0];

static void imap_put_get_remove(void **state) {
	const struct IMapParams params = { 0 };
	const struct IMap *tab = imap_init_with(params);

	assert_nul(imap_put(tab, 0, V0));
	assert_nul(imap_put(tab, 1, V1));
	assert_nul(imap_put(tab, 2, V2));

	assert_int_equal(imap_size(tab), 3);

	assert_ptr_equal(imap_get(tab, 1), V1);

	assert_nul(imap_get(tab, 999));

	assert_ptr_equal(imap_remove(tab, 1), V1);

	assert_nul(imap_get(tab, 1));

	imap_free(tab);
}

static void imap_free_vals__(void **state) {
	const struct IMap *tab = imap_init();
	assert_nul(imap_put(tab, 0, strdup("zero")));

	imap_free_vals(tab);
}

static void imap_iter__many(void **state) {

	const struct IMap *tab = imap_init();
	assert_nul(imap_put(tab, 0, V0));
	assert_nul(imap_put(tab, 1, NULL));
	assert_nul(imap_put(tab, 2, V2));

	const struct IMapIter *iter = imap_iter(tab);

	assert_non_nul(iter);
	assert_int_equal(iter->key, 0);
	assert_ptr_equal(iter->val, V0);

	iter = imap_iter_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, 1);
	assert_nul(iter->val);

	imap_iter_free(iter);

	imap_free(tab);
}

static void imap_iter__empty(void **state) {

	const struct IMap *tab = imap_init();

	const struct IMapIter *iter = imap_iter(tab);

	assert_nul(iter);

	imap_free(tab);
}

static void imap_iter_free__partial(void **state) {
	const struct IMapIter *iter = calloc(1, sizeof(struct IMapIter));

	imap_iter_free(iter);
}

static void imap_iter_next__partial(void **state) {
	const struct IMapIter *iter = calloc(1, sizeof(struct IMapIter));

	assert_nul(imap_iter_next(iter));
}

static void imap_filter_iter__(void **state) {
	const struct IMap *tab = imap_init();

	assert_nul(imap_put(tab, 0, V0));
	assert_nul(imap_put(tab, 1, V1));
	assert_nul(imap_put(tab, 2, V2));

	// skip K0
	expect_int_value(mock_equal_size_t, a, 0);
	expect_ptr(mock_equal_size_t, b, D0);
	will_return(mock_equal_size_t, false);

	// pass K1
	expect_int_value(mock_equal_size_t, a, 1);
	expect_ptr(mock_equal_size_t, b, D0);
	will_return(mock_equal_size_t, true);

	// pass V1
	expect_ptr(mock_equal, a, V1);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, true);

	const struct IMapIter *iter = imap_filter_iter(tab, mock_equal_size_t, mock_equal, D0);
	assert_non_nul(iter);
	assert_int_equal(iter->key, 1);
	assert_ptr_equal(iter->val, V1);

	// pass K2
	expect_int_value(mock_equal_size_t, a, 2);
	expect_ptr(mock_equal_size_t, b, D0);
	will_return(mock_equal_size_t, true);

	// skip V2
	expect_ptr(mock_equal, a, V2);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, false);

	// done
	iter = imap_iter_next(iter);
	assert_nul(iter);

	imap_free(tab);
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

	int *removed_key = (int*)b->ptab->keys[0];
	b->ptab->keys[0] = NULL;

	assert_imap_not_equal(a, b);

	free(removed_key);
	imap_free(a);
	imap_free(b);
}

static void imap_contains_key__(void **state) {
	const struct IMap *tab = imap_init();

	assert_false(imap_contains_key(tab, 0));

	assert_nul(imap_put(tab, 0, V0));
	assert_nul(imap_put(tab, 1, V1));

	assert_true(imap_contains_key(tab, 0));
	assert_true(imap_contains_key(tab, 1));

	assert_false(imap_contains_key(tab, 2));

	imap_free(tab);
}

static void imap_get__key_removed(void **state) {

	const struct IMap *actual = imap_init();
	assert_nul(imap_put(actual, 0, V0));

	int *removed_key = (int*)actual->ptab->keys[0];
	actual->ptab->keys[0] = NULL;

	assert_nul(imap_get(actual, 0));

	free(removed_key);
	imap_free(actual);
}

static void imap_put_free__(void **state) {
	const struct IMap *tab = imap_init();

	const char *val = strdup("val");

	assert_nul(imap_put(tab, 0, val));

	assert_false(imap_put_free(tab, 1, V1));

	assert_true(imap_put_free(tab, 0, V0));

	imap_free(tab);
}

static void imap_put_if_absent__(void **state) {
	const struct IMap *tab = imap_init();

	assert_nul(imap_put_if_absent(tab, 0, V0));
	assert_ptr_equal(imap_get(tab, 0), V0);

	const void *existing = imap_put_if_absent(tab, 0, V1);
	assert_ptr_equal(existing, V0);

	imap_free(tab);
}

static void imap_remove_free__(void **state) {
	const struct IMap *tab = imap_init();

	const char *val = strdup("val");

	assert_nul(imap_put(tab, 0, val));

	assert_true(imap_remove_free(tab, 0));

	assert_false(imap_remove_free(tab, 1));

	imap_free(tab);
}

static void imap_str__(void **state) {

	const struct IMap *tab = imap_init();
	assert_nul(imap_put(tab, 0, V0));
	assert_nul(imap_put(tab, 1, NULL));
	assert_nul(imap_put(tab, 999, V2));

	char *expected = sprintf_alloc(
			"0 = %p\n"
			"1 = (null)\n"
			"999 = %p\n",
			V0,
			V2
			);

	char *actual = imap_str(tab);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	imap_free(tab);
}

static void imap_vals_slist_shallow__many(void **state) {
	const struct IMap *tab = imap_init();

	imap_put(tab, 0, V0);
	imap_put(tab, 1, NULL);
	imap_put(tab, 2, V2);

	struct SList *list = imap_vals_slist_shallow(tab);

	assert_int_equal(slist_length(list), 3);
	assert_ptr_equal(slist_at(list, 0), V0);
	assert_nul(slist_at(list, 1));
	assert_ptr_equal(slist_at(list, 2), V2);

	slist_free(&list);
	imap_free(tab);
}

static void imap_vals_slist_deep__many(void **state) {
	const struct IMapParams params = { .clone_val = fn_clone_strdup, };
	const struct IMap *tab = imap_init_with(params);

	imap_put(tab, 0, "0");
	imap_put(tab, 1, NULL);
	imap_put(tab, 2, "2");

	struct SList *list = imap_vals_slist_deep(tab);

	assert_int_equal(slist_length(list), 3);
	assert_str_equal(slist_at(list, 0), "0");
	assert_nul(slist_at(list, 1));
	assert_str_equal(slist_at(list, 2), "2");

	slist_free_vals(&list, NULL);
	imap_free(tab);
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
	assert_int_equal(to->ptab->size, 0);
	assert_int_equal(to->ptab->capacity, 99);
	assert_int_equal(to->ptab->params.grow, 1);
	assert_ptr_equal(to->ptab->params.equal_val, mock_equal);
	assert_ptr_equal(to->ptab->params.alloc_val, mock_alloc);
	assert_ptr_equal(to->ptab->params.free_key, (fn_free)free);
	assert_ptr_equal(to->ptab->params.free_val, mock_free);
	assert_ptr_equal(to->ptab->params.clone_val, mock_clone);

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
	const struct IMap *tab = imap_init();

	assert_nul(imap_clone_shallow(NULL));
	assert_nul(imap_clone_deep(NULL));
	imap_free(NULL);
	imap_free_vals(NULL);
	imap_iter_free(NULL);
	assert_false(imap_get(NULL, 0));
	assert_false(imap_contains_key(NULL, 0));
	assert_nul(imap_iter(NULL));
	assert_nul(imap_filter_iter(NULL, NULL, NULL, NULL));
	assert_nul(imap_iter_next(NULL));
	assert_false(imap_put(NULL, 0, NULL));
	assert_nul(imap_put_if_absent(NULL, 0, NULL));
	assert_nul(imap_put_if_absent(tab, 0, NULL));
	assert_false(imap_put_free(NULL, 0, NULL));
	assert_false(imap_put_free(tab, 0, NULL));
	assert_nul(imap_remove(NULL, 0));
	assert_nul(imap_remove(tab, 0));
	assert_false(imap_remove_free(NULL, 0));
	assert_false(imap_remove_free(tab, 0));
	assert_false(imap_equal(NULL, NULL));
	assert_false(imap_equal(tab, NULL));
	assert_nul(imap_vals_slist_shallow(NULL));
	assert_nul(imap_vals_slist_deep(NULL));
	assert_nul(imap_str(NULL));
	assert_int_equal(imap_size(NULL), 0);

	imap_free(tab);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(imap_put_get_remove),

		TEST(imap_free_vals__),

		TEST(imap_iter__many),
		TEST(imap_iter__empty),

		TEST(imap_iter_free__partial),

		TEST(imap_iter_next__partial),

		TEST(imap_filter_iter__),

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


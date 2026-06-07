#include "tst.h"
#include "asserts.h"
#include "assert-ptable.h"
#include "expects.h"

#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"

#include "ptable.h"

/*
   diff -u \
   <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' tst/tst-stable.c) \
   <(sed -e 's/ptable/xtable/g ; s/PTable/XTable/g' tst/tst-ptable.c)
   */

static int keys[6] = { 10, 11, 12, 13, 14, 15 };
static void *K0 = &keys[0];
static void *K1 = &keys[1];
static void *K2 = &keys[2];
static void *K3 = &keys[3];
static void *K4 = &keys[4];
static void *K5 = &keys[5];

static int vals[6] = { 20, 21, 22, 23, 24, 25, };
static void *V0 = &vals[0];
static void *V1 = &vals[1];
static void *V2 = &vals[2];
static void *V3 = &vals[3];
static void *V4 = &vals[4];
static void *V5 = &vals[5];

static int before_all(void **state) {
	return 0;
}

static int after_all(void **state) {
	return 0;
}

static int before_each(void **state) {
	return 0;
}

static int after_each(void **state) {
	return 0;
}

static void mock_free_val(const void* const val) {
	check_expected_ptr(val);
}

static char* fn_str_first(const void *val) {
	return strndup(val, 1);
}

static void ptable_init__size(void **state) {
	const struct PTable *tab = ptable_init_with(NULL, NULL, NULL, NULL, 5, 50);

	assert_non_nul(tab);

	assert_int_equal(ptable_size(tab), 0);
	assert_int_equal(ptable_capacity(tab), 5);

	ptable_free(tab);
}

static void ptable_init__invalid(void **state) {
	const struct PTable *tab = ptable_init_with(NULL, NULL, NULL, NULL, 0, 0);

	assert_nul(tab);
}

static void ptable_free_vals__null_fn_free(void **state) {
	const struct PTable *tab = ptable_init();

	const char *val = strdup("0");

	ptable_put(tab, K0, val);

	assert_int_equal(ptable_size(tab), 1);

	// valgrind will indicate that val has been free'd
	ptable_free_vals(tab, NULL);
}

static void ptable_free_vals__fn_free(void **state) {
	const struct PTable *tab = ptable_init();

	ptable_put(tab, K0, V0);
	ptable_put(tab, K1, NULL);
	ptable_put(tab, K2, V2);

	assert_int_equal(ptable_size(tab), 3);

	expect_ptr(mock_free_val, val, V0);
	expect_ptr(mock_free_val, val, V2);

	ptable_free_vals(tab, mock_free_val);
}

static void fn_free_ptable(const void *val) {
	ptable_free_vals(val, mock_free_val);
}

static void ptable_free_vals__fn_free_hierarchical(void **state) {
	const struct PTable *outer = ptable_init();
	const struct PTable *inner1 = ptable_init();
	const struct PTable *inner2 = ptable_init();

	ptable_put(outer, K0, (void*)inner1);
	ptable_put(outer, K1, (void*)inner2);

	ptable_put(inner1, K2, V2);
	ptable_put(inner1, K3, V3);

	ptable_put(inner2, K4, V4);
	ptable_put(inner2, K5, V5);

	assert_int_equal(ptable_size(outer), 2);

	expect_ptr(mock_free_val, val, V2);
	expect_ptr(mock_free_val, val, V3);
	expect_ptr(mock_free_val, val, V4);
	expect_ptr(mock_free_val, val, V5);

	ptable_free_vals(outer, fn_free_ptable);
}

static void ptable_put__new(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));

	assert_int_equal(ptable_size(tab), 2);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_ptr_equal(ptable_get(tab, K1), V1);

	ptable_free(tab);
}

static void ptable_put__overwrite(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));
	assert_nul(ptable_put(tab, K2, V2));
	assert_nul(ptable_put(tab, K3, V3));

	assert_ptr_equal(ptable_put(tab, K1, V4), V1);

	assert_ptr_equal(ptable_put(tab, K3, V5), V3);

	assert_int_equal(ptable_size(tab), 4);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_ptr_equal(ptable_get(tab, K1), V4);
	assert_ptr_equal(ptable_get(tab, K2), V2);
	assert_ptr_equal(ptable_get(tab, K3), V5);

	ptable_free(tab);
}

static void ptable_put__null(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_int_equal(ptable_size(tab), 1);

	assert_nul(ptable_put(tab, K1, NULL));
	assert_int_equal(ptable_size(tab), 2);

	assert_nul(ptable_put(tab, K2, V2));
	assert_int_equal(ptable_size(tab), 3);

	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_nul(ptable_get(tab, K1));
	assert_ptr_equal(ptable_get(tab, K2), V2);

	ptable_free(tab);
}

static void ptable_put__null_overwrite(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));

	assert_ptr_equal(ptable_get(tab, K0), V0);

	assert_ptr_equal(ptable_put(tab, K0, NULL), V0);

	assert_int_equal(ptable_size(tab), 1);
	assert_nul(ptable_get(tab, K0));

	ptable_free(tab);
}

static void ptable_put__grow(void **state) {
	const struct PTable *tab = ptable_init_with(NULL, NULL, NULL, NULL, 3, 5);

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));
	assert_nul(ptable_put(tab, K2, V2));

	assert_int_equal(ptable_size(tab), 3);
	assert_int_equal(ptable_capacity(tab), 3);

	assert_nul(ptable_put(tab, K3, V3));
	assert_int_equal(ptable_size(tab), 4);
	assert_int_equal(ptable_capacity(tab), 8);

	assert_nul(ptable_put(tab, K4, V4));
	assert_nul(ptable_put(tab, K5, V5));

	assert_int_equal(ptable_size(tab), 6);
	assert_int_equal(ptable_capacity(tab), 8);

	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_ptr_equal(ptable_get(tab, K1), V1);
	assert_ptr_equal(ptable_get(tab, K2), V2);

	assert_ptr_equal(ptable_get(tab, K3), V3);
	assert_ptr_equal(ptable_get(tab, K4), V4);
	assert_ptr_equal(ptable_get(tab, K5), V5);

	ptable_free(tab);
}

static void ptable_iter__empty(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_iter(tab));

	ptable_free(tab);
}

static void ptable_iter__free(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));

	const struct PTableIter *iter = ptable_iter(tab);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K0);
	assert_ptr_equal(ptable_iter_val(iter), V0);

	// valgrind will indicate that iter has been free'd
	ptable_iter_free(iter);

	ptable_free(tab);
}

static void ptable_iter__vals(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, NULL));
	assert_nul(ptable_put(tab, K1, V1));
	assert_nul(ptable_put(tab, K2, NULL));
	assert_nul(ptable_put(tab, K3, V3));
	assert_nul(ptable_put(tab, K4, NULL));

	assert_int_equal(ptable_size(tab), 5);

	// zero
	const struct PTableIter *iter = ptable_iter(tab);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K0);
	assert_nul(ptable_iter_val(iter));

	// one
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K1);
	assert_ptr_equal(ptable_iter_val(iter), V1);

	// two
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K2);
	assert_nul(ptable_iter_val(iter));

	// three
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K3);
	assert_ptr_equal(ptable_iter_val(iter), V3);

	// four
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K4);
	assert_nul(ptable_iter_val(iter));

	// end
	iter = ptable_iter_next(iter);
	assert_nul(iter);

	ptable_free(tab);
}

static void ptable_iter__removed(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));
	assert_nul(ptable_put(tab, K2, V2));
	assert_nul(ptable_put(tab, K3, V3));
	assert_nul(ptable_put(tab, K4, V4));

	assert_ptr_equal(ptable_remove(tab, K0), V0);

	assert_ptr_equal(ptable_remove(tab, K2), V2);

	assert_ptr_equal(ptable_remove(tab, K4), V4);

	assert_int_equal(ptable_size(tab), 2);

	// one
	const struct PTableIter *iter = ptable_iter(tab);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K1);
	assert_ptr_equal(ptable_iter_val(iter), V1);

	// three
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K3);
	assert_ptr_equal(ptable_iter_val(iter), V3);

	// end
	iter = ptable_iter_next(iter);
	assert_nul(iter);

	ptable_free(tab);
}

static void ptable_put__again(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));

	assert_int_equal(ptable_size(tab), 2);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_ptr_equal(ptable_get(tab, K1), V1);

	// remove zero
	assert_ptr_equal(ptable_remove(tab, K0), V0);

	assert_int_equal(ptable_size(tab), 1);
	assert_nul(ptable_get(tab, K0));

	// put zero again afterwards
	assert_nul(ptable_put(tab, K0, V0));
	assert_int_equal(ptable_size(tab), 2);

	// one
	const struct PTableIter *iter = ptable_iter(tab);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K1);
	assert_ptr_equal(ptable_iter_val(iter), V1);

	// zero moved later
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K0);
	assert_ptr_equal(ptable_iter_val(iter), V0);

	// end
	iter = ptable_iter_next(iter);
	assert_nul(iter);

	ptable_free(tab);
}

static void ptable_remove__existing(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));
	assert_nul(ptable_put(tab, K2, V2));

	assert_int_equal(ptable_size(tab), 3);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_ptr_equal(ptable_get(tab, K1), V1);
	assert_ptr_equal(ptable_get(tab, K2), V2);

	// K1
	assert_ptr_equal(ptable_remove(tab, K1), V1);
	assert_int_equal(ptable_size(tab), 2);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_nul(ptable_get(tab, K1));
	assert_ptr_equal(ptable_get(tab, K2), V2);

	// K2
	assert_ptr_equal(ptable_remove(tab, K2), V2);
	assert_int_equal(ptable_size(tab), 1);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_nul(ptable_get(tab, K1));
	assert_nul(ptable_get(tab, K2));

	// K0
	assert_ptr_equal(ptable_remove(tab, K0), V0);
	assert_int_equal(ptable_size(tab), 0);
	assert_nul(ptable_get(tab, K0));
	assert_nul(ptable_get(tab, K1));
	assert_nul(ptable_get(tab, K2));

	ptable_free(tab);
}

static void ptable_remove__inexistent(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));
	assert_nul(ptable_put(tab, K2, V2));

	assert_int_equal(ptable_size(tab), 3);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_ptr_equal(ptable_get(tab, K1), V1);
	assert_ptr_equal(ptable_get(tab, K2), V2);

	assert_nul(ptable_remove(tab, K3));
	assert_int_equal(ptable_size(tab), 3);

	ptable_free(tab);
}

static void ptable_equal__length_different(void **state) {
	const struct PTable *a = ptable_init();
	const struct PTable *b = ptable_init();

	assert_nul(ptable_put(a, K0, V0));
	assert_nul(ptable_put(a, K1, V1));

	assert_nul(ptable_put(b, K1, V2));

	assert_ptable_not_equal(a, b, NULL, NULL);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__keys_different(void **state) {
	const struct PTable *a = ptable_init();
	const struct PTable *b = ptable_init();

	assert_nul(ptable_put(a, K0, NULL));
	assert_nul(ptable_put(a, K1, NULL));

	assert_nul(ptable_put(b, K0, NULL));
	assert_nul(ptable_put(b, K2, NULL));

	assert_ptable_not_equal(a, b, NULL, NULL);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__pointers_ok(void **state) {
	const struct PTable *a = ptable_init();
	const struct PTable *b = ptable_init();

	assert_nul(ptable_put(a, K0, V0));
	assert_nul(ptable_put(a, K1, V1));
	assert_nul(ptable_put(a, K2, V2));

	assert_nul(ptable_put(b, K0, V0));
	assert_nul(ptable_put(b, K1, V1));
	assert_nul(ptable_put(b, K2, V2));

	assert_ptable_equal(a, b, NULL, NULL);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__pointers_different(void **state) {
	const struct PTable *a = ptable_init();
	const struct PTable *b = ptable_init();

	assert_nul(ptable_put(a, K0, V0));
	assert_nul(ptable_put(a, K1, V1));
	assert_nul(ptable_put(a, K2, V2));

	assert_nul(ptable_put(b, K0, V0));
	assert_nul(ptable_put(b, K1, V0));
	assert_nul(ptable_put(b, K2, V0));

	assert_ptable_not_equal(a, b, NULL, NULL);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__comparison_ok(void **state) {
	const struct PTable *a = ptable_init();
	const struct PTable *b = ptable_init();

	assert_nul(ptable_put(a, K0, "a"));

	assert_nul(ptable_put(b, K0, "a"));

	assert_ptable_equal(a, b, fn_equal_strcmp, NULL);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__comparison_different(void **state) {
	const struct PTable *a = ptable_init();
	const struct PTable *b = ptable_init();

	assert_nul(ptable_put(a, K0, "a"));

	assert_nul(ptable_put(b, K0, "b"));

	assert_false(ptable_equal(a, b, fn_equal_strcmp));

	ptable_free(a);
	ptable_free(b);
}

static void ptable_keys_slist__empty(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_keys_slist(tab));

	ptable_free(tab);
}

static void ptable_keys_slist__many(void **state) {
	const struct PTable *tab = ptable_init();

	ptable_put(tab, K0, V0);
	ptable_put(tab, K1, V1);

	struct SList *list = ptable_keys_slist(tab);

	assert_int_equal(slist_length(list), 2);
	assert_ptr_equal(slist_at(list, 0), K0);
	assert_ptr_equal(slist_at(list, 1), K1);

	slist_free(&list);
	ptable_free(tab);
}

static void ptable_vals_slist__empty(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_vals_slist(tab));

	ptable_free(tab);
}

static void ptable_vals_slist__many(void **state) {
	const struct PTable *tab = ptable_init();

	ptable_put(tab, K0, V1);
	ptable_put(tab, K1, NULL);
	ptable_put(tab, K2, V3);

	struct SList *list = ptable_vals_slist(tab);

	assert_int_equal(slist_length(list), 3);
	assert_ptr_equal(slist_at(list, 0), V1);
	assert_nul(slist_at(list, 1));
	assert_ptr_equal(slist_at(list, 2), V3);

	slist_free(&list);
	ptable_free(tab);
}

static void ptable_str__null(void **state) {
	assert_nul(ptable_str(NULL, NULL));
}

static void ptable_str__empty(void **state) {
	const struct PTable *tab = ptable_init();

	char *actual = ptable_str(tab, NULL);
	assert_str_equal(actual, "");

	free(actual);
	ptable_free(tab);
}

static void ptable_str__pointers(void **state) {
	const struct PTable *tab = ptable_init();

	ptable_put(tab, K0, V0);
	ptable_put(tab, K1, NULL);
	ptable_put(tab, K2, V2);

	char expected[2048];
	snprintf(expected, sizeof(expected),
			"%p = %p\n"
			"%p = (null)\n"
			"%p = %p\n",
			K0, V0,
			K1,
			K2, V2
			);

	char *actual = ptable_str(tab, NULL);

	assert_str_equal(actual, expected);

	free(actual);
	ptable_free(tab);
}

static void ptable_str__fn_str(void **state) {
	const struct PTable *tab = ptable_init();

	ptable_put(tab, K0, "AAA");
	ptable_put(tab, K1, NULL);
	ptable_put(tab, K2, "BBB");

	char expected[2048];
	snprintf(expected, sizeof(expected),
			"%p = A\n"
			"%p = (null)\n"
			"%p = B\n",
			K0,
			K1,
			K2
			);

	char *actual = ptable_str(tab, fn_str_first);
	assert_str_equal(actual, expected);

	free(actual);
	ptable_free(tab);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(ptable_init__size),
		TEST(ptable_init__invalid),

		TEST(ptable_free_vals__null_fn_free),
		TEST(ptable_free_vals__fn_free),
		TEST(ptable_free_vals__fn_free_hierarchical),

		TEST(ptable_put__new),
		TEST(ptable_put__overwrite),
		TEST(ptable_put__null),
		TEST(ptable_put__null_overwrite),
		TEST(ptable_put__grow),

		TEST(ptable_iter__empty),
		TEST(ptable_iter__free),
		TEST(ptable_iter__vals),
		TEST(ptable_iter__removed),

		TEST(ptable_put__again),

		TEST(ptable_remove__existing),
		TEST(ptable_remove__inexistent),

		TEST(ptable_equal__length_different),
		TEST(ptable_equal__keys_different),
		TEST(ptable_equal__pointers_ok),
		TEST(ptable_equal__pointers_different),
		TEST(ptable_equal__comparison_ok),
		TEST(ptable_equal__comparison_different),

		TEST(ptable_keys_slist__empty),
		TEST(ptable_keys_slist__many),

		TEST(ptable_vals_slist__empty),
		TEST(ptable_vals_slist__many),

		TEST(ptable_str__null),
		TEST(ptable_str__empty),
		TEST(ptable_str__pointers),
		TEST(ptable_str__fn_str),
	};

	return RUN(tests);
}


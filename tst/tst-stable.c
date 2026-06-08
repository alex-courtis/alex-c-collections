#include "tst.h"
#include "asserts.h"
#include "assert-stable.h"
#include "expects.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "slist.h"
#include "str.h"

#include "stable.h"

/*
   diff --color=always -U 10000 <(sed -e 's/itable/xtable/g ; s/ITable/XTable/g' tst/tst-itable.c) <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' tst/tst-stable.c) | less
   */

static int vals[3] = { 20, 21, 22, };
static void *V0 = &vals[0];
static void *V1 = &vals[1];
static void *V2 = &vals[2];

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

static bool mock_test_key_str(const char* const key) {
	check_expected_ptr(key);

	return mock_type(bool);
}

static bool mock_test_val(const void* const val) {
	check_expected_ptr(val);

	return mock_type(bool);
}

static void stable_put_get_remove__case_sensitive(void **state) {

	const struct STable *tab = stable_init();
	assert_nul(stable_put(tab, "a", V0));
	assert_nul(stable_put(tab, "b", V1));
	assert_nul(stable_put(tab, "c", V2));

	assert_int_equal(stable_size(tab), 3);

	assert_ptr_equal(stable_get(tab, "b"), V1);

	assert_nul(stable_get(tab, "x"));

	assert_ptr_equal(stable_remove(tab, "b"), V1);

	assert_nul(stable_get(tab, "b"));

	stable_free(tab);
}

static void stable_put_get_remove__case_insensitive(void **state) {

	const struct STable *tab = stable_init_with(10, 10, true);
	assert_nul(stable_put(tab, "A", V0));
	assert_nul(stable_put(tab, "B", V1));

	assert_ptr_equal(stable_get(tab, "b"), V1);

	assert_nul(stable_get(tab, "x"));

	assert_ptr_equal(stable_remove(tab, "b"), V1);

	assert_nul(stable_get(tab, "b"));

	stable_free(tab);
}

static void stable_free_vals__(void **state) {
	const struct STable *tab = stable_init();
	assert_nul(stable_put(tab, "a", strdup("zero")));

	// valgrind will indicate that key and val have been free'd
	stable_free_vals(tab, NULL);
}

static void stable_iter__(void **state) {

	const struct STable *tab = stable_init();
	assert_nul(stable_put(tab, "a", V0));
	assert_nul(stable_put(tab, "b", NULL));
	assert_nul(stable_put(tab, "c", V2));

	const struct STableIter *iter = stable_iter(tab);

	assert_non_nul(iter);
	assert_str_equal(stable_iter_key(iter), "a");
	assert_ptr_equal(stable_iter_val(iter), V0);

	iter = stable_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(stable_iter_key(iter), "b");
	assert_nul(stable_iter_val(iter));

	stable_iter_free(iter);

	stable_free(tab);
}

static void stable_filter_iter__(void **state) {
	const struct STable *tab = stable_init();

	assert_nul(stable_put(tab, "0", V0));
	assert_nul(stable_put(tab, "1", V1));
	assert_nul(stable_put(tab, "2", V2));

	// skip "0"
	expect_string(mock_test_key_str, key, "0");
	will_return(mock_test_key_str, false);

	// get 1
	expect_string(mock_test_key_str, key, "1");
	will_return(mock_test_key_str, true);
	expect_ptr(mock_test_val, val, V1);
	will_return(mock_test_val, true);

	const struct STableIter *iter = stable_filter_iter(tab, (fn_test)mock_test_key_str, mock_test_val);
	assert_non_nul(iter);
	assert_str_equal(stable_iter_key(iter), "1");
	assert_ptr_equal(stable_iter_val(iter), V1);

	// skip V2
	expect_string(mock_test_key_str, key, "2");
	will_return(mock_test_key_str, true);
	expect_ptr(mock_test_val, val, V2);
	will_return(mock_test_val, false);

	// done
	iter = stable_iter_next(iter);
	assert_nul(iter);

	stable_free(tab);
}

static void stable_equal__case_sensitive(void **state) {

	const struct STable *actual = stable_init();
	assert_nul(stable_put(actual, "a", V0));
	assert_nul(stable_put(actual, "b", V1));

	const struct STable *expected = stable_init();
	assert_nul(stable_put(expected, "a", V0));
	assert_nul(stable_put(expected, "b", V1));

	assert_stable_equal(actual, expected, NULL, NULL);

	assert_nul(stable_put(actual, "c", V2));

	assert_stable_not_equal(actual, expected, NULL, NULL);

	stable_free(actual);
	stable_free(expected);
}

static void stable_equal__case_insensitive(void **state) {

	const struct STable *actual = stable_init_with(10, 10, true);
	assert_nul(stable_put(actual, "a", V0));
	assert_nul(stable_put(actual, "b", V1));

	const struct STable *expected = stable_init();
	assert_nul(stable_put(expected, "A", V0));
	assert_nul(stable_put(expected, "B", V1));

	assert_stable_equal(actual, expected, NULL, NULL);

	assert_nul(stable_put(actual, "c", V2));

	assert_stable_not_equal(actual, expected, NULL, NULL);

	stable_free(actual);
	stable_free(expected);
}

static void stable_str__(void **state) {

	const struct STable *tab = stable_init();
	assert_nul(stable_put(tab, "a", V0));
	assert_nul(stable_put(tab, "b", NULL));
	assert_nul(stable_put(tab, "c", V2));

	char *expected = sprintf_alloc(
			"a = %p\n"
			"b = (null)\n"
			"c = %p\n",
			V0,
			V2
			);

	char *actual = stable_str(tab, NULL);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	stable_free(tab);
}

static void stable_keys_slist__many(void **state) {
	const struct STable *tab = stable_init();

	stable_put(tab, "a", V0);
	stable_put(tab, "b", V1);

	struct SList *list = stable_keys_slist(tab);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "a");
	assert_str_equal(slist_at(list, 1), "b");

	slist_free(&list);
	stable_free(tab);
}

static void stable_vals_slist__many(void **state) {
	const struct STable *tab = stable_init();

	stable_put(tab, "a", V0);
	stable_put(tab, "b", NULL);
	stable_put(tab, "c", V2);

	struct SList *list = stable_vals_slist(tab);

	assert_int_equal(slist_length(list), 3);
	assert_ptr_equal(slist_at(list, 0), V0);
	assert_nul(slist_at(list, 1));
	assert_ptr_equal(slist_at(list, 2), V2);

	slist_free(&list);
	stable_free(tab);
}

static void stable_clone__shallow(void **state) {
	const struct STable *from = stable_init();

	assert_nul(stable_put(from, "a", V0));
	assert_nul(stable_put(from, "b", NULL));
	assert_nul(stable_put(from, "c", V2));

	const struct STable *to = stable_clone(from, NULL);

	assert_non_nul(to);

	assert_int_equal(stable_size(to), 3);

	assert_stable_equal(from, to, NULL, NULL);

	stable_free(from);
	stable_free(to);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(stable_put_get_remove__case_sensitive),
		TEST(stable_put_get_remove__case_insensitive),

		TEST(stable_free_vals__),

		TEST(stable_iter__),

		TEST(stable_filter_iter__),

		TEST(stable_equal__case_sensitive),
		TEST(stable_equal__case_insensitive),

		TEST(stable_str__),

		TEST(stable_keys_slist__many),

		TEST(stable_vals_slist__many),

		TEST(stable_clone__shallow),
	};

	return RUN(tests);
}


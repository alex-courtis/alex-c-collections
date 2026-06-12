#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdlib.h>

#include "fn.h"

static int ptrs[2] = { 20, 21, };
static void *P0 = &ptrs[0];
static void *P1 = &ptrs[1];

static void fn_equal_ptr__(void **state) {
	assert_true(fn_equal_ptr(P0, P0));
	assert_false(fn_equal_ptr(P0, P1));
}

static void fn_equal_strcmp__(void **state) {
	assert_true(fn_equal_strcmp(P0, P0));
	assert_false(fn_equal_strcmp(P0, NULL));
	assert_false(fn_equal_strcmp(NULL, P0));
	assert_true(fn_equal_strcmp("a", "a"));
	assert_false(fn_equal_strcmp("a", "b"));
}

static void fn_equal_strcasecmp__(void **state) {
	assert_true(fn_equal_strcasecmp(P0, P0));
	assert_false(fn_equal_strcasecmp(P0, NULL));
	assert_false(fn_equal_strcasecmp(NULL, P0));
	assert_true(fn_equal_strcasecmp("a", "a"));
	assert_true(fn_equal_strcasecmp("a", "A"));
	assert_false(fn_equal_strcasecmp("a", "b"));
}

static void fn_equal_strstr__(void **state) {
	assert_true(fn_equal_strstr(P0, P0));
	assert_false(fn_equal_strstr(P0, NULL));
	assert_false(fn_equal_strstr(NULL, P0));
	assert_true(fn_equal_strstr("aabb", "bb"));
	assert_false(fn_equal_strstr("aabb", "xx"));
}

static void fn_less_than_strcmp__(void **state) {
	assert_true(fn_less_than_strcmp(P0, P0));
	assert_false(fn_less_than_strcmp(P0, NULL));
	assert_false(fn_less_than_strcmp(NULL, P0));
	assert_true(fn_less_than_strcmp("a", "b"));
}

static void fn_less_than_strcasecmp__(void **state) {
	assert_true(fn_less_than_strcasecmp(P0, P0));
	assert_false(fn_less_than_strcasecmp(P0, NULL));
	assert_false(fn_less_than_strcasecmp(NULL, P0));
	assert_true(fn_less_than_strcasecmp("a", "B"));
}

static void fn_clone_strdup__(void **state) {
	assert_nul(fn_clone_strdup(NULL));

	char *str = fn_clone_strdup("foo");
	assert_str_equal(str, "foo");
	free(str);
}

static void fn_str_or_null__(void **state) {
	char *str = fn_str_or_null(NULL);
	assert_str_equal(str, "(null)");
	free(str);

	str = fn_str_or_null("foo");
	assert_str_equal(str, "foo");
	free(str);
}

int main(void) {

	const struct CMUnitTest tests[] = {
		TEST(fn_equal_ptr__),
		TEST(fn_equal_strcmp__),
		TEST(fn_equal_strcasecmp__),
		TEST(fn_equal_strstr__),
		TEST(fn_less_than_strcmp__),
		TEST(fn_less_than_strcasecmp__),
		TEST(fn_clone_strdup__),
		TEST(fn_str_or_null__),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}

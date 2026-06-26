#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdlib.h>

#include "fn.h"

static int ptrs[2] = { 20, 21, };
static void *P0 = &ptrs[0];
static void *P1 = &ptrs[1];

static void equal_ptr__(void **state) {
	assert_true(equal_ptr(P0, P0));
	assert_false(equal_ptr(P0, P1));
}

static void equal_strcmp__(void **state) {
	assert_true(equal_strcmp(P0, P0));
	assert_false(equal_strcmp(P0, NULL));
	assert_false(equal_strcmp(NULL, P0));
	assert_true(equal_strcmp("a", "a"));
	assert_false(equal_strcmp("a", "b"));
}

static void equal_strcasecmp__(void **state) {
	assert_true(equal_strcasecmp(P0, P0));
	assert_false(equal_strcasecmp(P0, NULL));
	assert_false(equal_strcasecmp(NULL, P0));
	assert_true(equal_strcasecmp("a", "a"));
	assert_true(equal_strcasecmp("a", "A"));
	assert_false(equal_strcasecmp("a", "b"));
}

static void equal_strstr__(void **state) {
	assert_true(equal_strstr(P0, P0));
	assert_false(equal_strstr(P0, NULL));
	assert_false(equal_strstr(NULL, P0));
	assert_true(equal_strstr("aabb", "bb"));
	assert_false(equal_strstr("aabb", "xx"));
}

static void less_than_strcmp__(void **state) {
	assert_true(less_than_strcmp(P0, P0));
	assert_false(less_than_strcmp(P0, NULL));
	assert_false(less_than_strcmp(NULL, P0));
	assert_true(less_than_strcmp("a", "b"));
}

static void less_than_strcasecmp__(void **state) {
	assert_true(less_than_strcasecmp(P0, P0));
	assert_false(less_than_strcasecmp(P0, NULL));
	assert_false(less_than_strcasecmp(NULL, P0));
	assert_true(less_than_strcasecmp("a", "B"));
}

static void clone_strdup__(void **state) {
	assert_nul(clone_strdup(NULL));

	char *str = clone_strdup("foo");
	assert_str_equal(str, "foo");
	free(str);
}

static void str_or_null__(void **state) {
	char *str = str_or_null(NULL);
	assert_str_equal(str, "(null)");
	free(str);

	str = str_or_null("foo");
	assert_str_equal(str, "foo");
	free(str);
}

int main(void) {

	const struct CMUnitTest tests[] = {
		TEST(equal_ptr__),
		TEST(equal_strcmp__),
		TEST(equal_strcasecmp__),
		TEST(equal_strstr__),
		TEST(less_than_strcmp__),
		TEST(less_than_strcasecmp__),
		TEST(clone_strdup__),
		TEST(str_or_null__),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}

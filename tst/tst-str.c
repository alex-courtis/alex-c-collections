#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdlib.h>

#include "str.h"

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	return 0;
}

int after_each(void **state) {
	return 0;
}

static void sprintf_alloc__ok(void **state) {

	char *actual = sprintf_alloc("%d %s", 1, "bar");

	assert_str_equal(actual, "1 bar");

	free(actual);
}

static void snprintf_alloc__longer(void **state) {
	char *actual = snprintf_alloc(10, "%d %s", 1, "bar");

	assert_str_equal(actual, "1 bar");

	free(actual);
}

static void snprintf_alloc__shorter(void **state) {
	char *actual = snprintf_alloc(3, "%d %s", 1, "bar");

	assert_str_equal(actual, "1 b");

	free(actual);
}

static void snprintf_alloc__equal(void **state) {
	char *actual = snprintf_alloc(5, "%d %s", 1, "bar");

	assert_str_equal(actual, "1 bar");

	free(actual);
}

static void sprintf_append__ok(void **state) {
	char *actual = strdup("foo ");

	actual = sprintf_append(actual, "%d %s", 1, "bar");

	assert_str_equal(actual, "foo 1 bar");

	free(actual);
}

static void sprintf_append__null(void **state) {
	char *actual = sprintf_append(NULL, "%d %s", 1, "bar");

	assert_str_equal(actual, "1 bar");

	free(actual);
}

static void snprintf_append__longer(void **state) {
	char *actual = strdup("foo ");

	actual = snprintf_append(actual, 100, "%d %s", 1, "bar");

	assert_str_equal(actual, "foo 1 bar");

	free(actual);
}

static void snprintf_append__shorter(void **state) {
	char *actual = strdup("foo ");

	actual = snprintf_append(actual, 2, "%d %s", 1, "bar");

	assert_str_equal(actual, "fo");

	free(actual);
}

static void snprintf_append__equal(void **state) {
	char *actual = strdup("foo ");

	actual = snprintf_append(actual, 9, "%d %s", 1, "bar");

	assert_str_equal(actual, "foo 1 bar");

	free(actual);
}

static void snprintf_append__null(void **state) {
	char *actual = snprintf_append(NULL, 10, "%d %s", 1, "bar");

	assert_str_equal(actual, "1 bar");

	free(actual);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"

static void sprintf_alloc__empty(void **state) {


	char *actual = sprintf_alloc("", 1, "bar");

	assert_str_equal(actual, "");

	free(actual);
}

static void snprintf_alloc__empty(void **state) {
	char *actual = snprintf_alloc(10, "", 1, "bar");

	assert_str_equal(actual, "");

	free(actual);
}

static void sprintf_append__empty(void **state) {
	char *actual = strdup("foo ");

	actual = sprintf_append(actual, "", 1, "bar");

	assert_str_equal(actual, "foo ");

	free(actual);
}

static void snprintf_append__empty(void **state) {
	char *actual = strdup("foo ");

	actual = snprintf_append(actual, 900, "", 1, "bar");

	assert_str_equal(actual, "foo ");

	free(actual);
}

#pragma GCC diagnostic pop // "-Wformat-zero-length"


int main(void) {

	// dummy usages of the implicitly tested functions, for cppcheck unusedFunction
	if (false) {
		vsprintf_alloc("", NULL);
		vsnprintf_alloc(0, "", NULL);
		vsprintf_append(0, "", NULL);
		vsnprintf_append(NULL, 0, "", NULL);
	}

	const struct CMUnitTest tests[] = {

		// tests vs versions
		TEST(sprintf_alloc__ok),
		TEST(sprintf_alloc__empty),

		// tests vsn versions
		TEST(snprintf_alloc__longer),
		TEST(snprintf_alloc__shorter),
		TEST(snprintf_alloc__equal),
		TEST(snprintf_alloc__empty),

		// tests vs versions
		TEST(sprintf_append__ok),
		TEST(sprintf_append__empty),
		TEST(sprintf_append__null),

		// tests vsn versions
		TEST(snprintf_append__longer),
		TEST(snprintf_append__shorter),
		TEST(snprintf_append__equal),
		TEST(snprintf_append__empty),
		TEST(snprintf_append__null),
	};

	return RUN(tests);
}

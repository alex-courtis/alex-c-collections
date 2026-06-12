#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"

static void sprintf_alloc__ok(void **state) {

	char *actual = sprintf_alloc("%d_%s", 1, "bar");

	assert_str_equal(actual, "1_bar");

	free(actual);
}

static void snprintf_alloc__longer(void **state) {
	char *actual = snprintf_alloc(10, "%d_%s", 1, "bar");

	assert_str_equal(actual, "1_bar");

	free(actual);
}

static void snprintf_alloc__shorter(void **state) {
	char *actual = snprintf_alloc(3, "%d_%s", 1, "bar");

	assert_str_equal(actual, "1_b");

	free(actual);
}

static void snprintf_alloc__equal(void **state) {
	char *actual = snprintf_alloc(5, "%d_%s", 1, "bar");

	assert_str_equal(actual, "1_bar");

	free(actual);
}

static void snprintf_alloc__zero(void **state) {
	char *actual = snprintf_alloc(0, "%d_%s", 1, "bar");

	assert_str_equal(actual, "");

	free(actual);
}

static void sprintf_append__ok(void **state) {
	char *actual = strdup("foo-");

	actual = sprintf_append(actual, "%d_%s", 1, "bar");

	assert_str_equal(actual, "foo-1_bar");

	free(actual);
}

static void sprintf_append__null(void **state) {
	char *actual = sprintf_append(NULL, "%d_%s", 1, "bar");

	assert_str_equal(actual, "1_bar");

	free(actual);
}

static void snprintf_append__longer(void **state) {
	char *actual = strdup("foo-");

	actual = snprintf_append(actual, 100, "%d_%s", 1, "bar");

	assert_str_equal(actual, "foo-1_bar");

	free(actual);
}

static void snprintf_append__shorter_left(void **state) {
	char *actual = strdup("foo-");

	actual = snprintf_append(actual, 2, "%d_%s", 1, "bar");

	assert_str_equal(actual, "fo");

	free(actual);
}

static void snprintf_append__shorter_right(void **state) {
	char *actual = strdup("foo-");

	actual = snprintf_append(actual, 6, "%d_%s", 1, "bar");

	assert_str_equal(actual, "foo-1_");

	free(actual);
}

static void snprintf_append__equal_total(void **state) {
	char *actual = strdup("foo-");

	actual = snprintf_append(actual, 9, "%d_%s", 1, "bar");

	assert_str_equal(actual, "foo-1_bar");

	free(actual);
}

static void snprintf_append__equal_left(void **state) {
	char *actual = strdup("foo-");

	actual = snprintf_append(actual, 4, "%d_%s", 1, "bar");

	assert_str_equal(actual, "foo-");

	free(actual);
}

static void snprintf_append__zero(void **state) {
	char *actual = strdup("foo-");

	actual = snprintf_append(actual, 0, "%d_%s", 1, "bar");

	assert_str_equal(actual, "");

	free(actual);
}

static void snprintf_append__null(void **state) {
	char *actual = snprintf_append(NULL, 10, "%d_%s", 1, "bar");

	assert_str_equal(actual, "1_bar");

	free(actual);
}

static void snprintf_append__null_shorter(void **state) {
	char *actual = snprintf_append(NULL, 3, "%d_%s", 1, "bar");

	assert_str_equal(actual, "1_b");

	free(actual);
}

static void snprintf_append__null_zero(void **state) {
	char *actual = snprintf_append(NULL, 0, "%d_%s", 1, "bar");

	assert_str_equal(actual, "");

	free(actual);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"

static void sprintf_alloc__empty_format(void **state) {

	char *actual = sprintf_alloc("", 1, "bar");

	assert_str_equal(actual, "");

	free(actual);
}

static void snprintf_alloc__empty_format(void **state) {
	char *actual = snprintf_alloc(10, "", 1, "bar");

	assert_str_equal(actual, "");

	free(actual);
}

static void sprintf_append__empty_format(void **state) {
	char *actual = strdup("foo ");

	actual = sprintf_append(actual, "", 1, "bar");

	assert_str_equal(actual, "foo ");

	free(actual);
}

static void snprintf_append__empty_format(void **state) {
	char *actual = strdup("foo ");

	actual = snprintf_append(actual, 900, "", 1, "bar");

	assert_str_equal(actual, "foo ");

	free(actual);
}

#pragma GCC diagnostic pop // "-Wformat-zero-length"

static void null_args(void **state) {
	assert_nul(vsprintf_alloc("", NULL));
	assert_nul(vsnprintf_alloc(0, "", NULL));

	vsprintf_append(0, "", NULL);
	vsnprintf_append(NULL, 0, "", NULL);
}

int main(void) {

	const struct CMUnitTest tests[] = {

		// tests vs versions
		TEST(sprintf_alloc__ok),

		// tests vsn versions
		TEST(snprintf_alloc__longer),
		TEST(snprintf_alloc__shorter),
		TEST(snprintf_alloc__equal),
		TEST(snprintf_alloc__zero),

		// tests vs versions
		TEST(sprintf_append__ok),
		TEST(sprintf_append__null),

		// tests vsn versions
		TEST(snprintf_append__longer),
		TEST(snprintf_append__shorter_left),
		TEST(snprintf_append__shorter_right),
		TEST(snprintf_append__equal_total),
		TEST(snprintf_append__equal_left),
		TEST(snprintf_append__zero),
		TEST(snprintf_append__null),
		TEST(snprintf_append__null_shorter),
		TEST(snprintf_append__null_zero),

		TEST(sprintf_alloc__empty_format),
		TEST(snprintf_alloc__empty_format),
		TEST(sprintf_append__empty_format),
		TEST(snprintf_append__empty_format),

		TEST(null_args),
	};

	return RUN(tests);
}

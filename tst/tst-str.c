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

static char *vsprintf_alloc_harness(const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	char *out = vsprintf_alloc(__format, args);
	va_end(args);
	return out;
}

static char *vsnprintf_alloc_harness(size_t __maxlen, const char *__restrict __format, ...) {
	va_list args;
	va_start(args, __format);
	char *out = vsnprintf_alloc(__maxlen, __format, args);
	va_end(args);
	return out;
}

static void sprintf_alloc__ok(void **state) {

	char *actual = sprintf_alloc("%d %s", 1, "bar");

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

static void sprintf_append__empty(void **state) {
	char *actual = strdup("foo ");

	actual = sprintf_append(actual, "", 1, "bar");

	assert_str_equal(actual, "foo ");

	free(actual);
}

static void vsprintf_alloc__empty(void **state) {
	char *actual = vsprintf_alloc_harness("", 1, "bar");

	assert_str_equal(actual, "");

	free(actual);
}

static void vsnprintf_alloc__empty(void **state) {
	char *actual = vsnprintf_alloc_harness(10, "", 1, "bar");

	assert_str_equal(actual, "");

	free(actual);
}

#pragma GCC diagnostic pop // "-Wformat-zero-length"

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

static void vsprintf_alloc__ok(void **state) {
	char *actual = vsprintf_alloc_harness("%d %s", 1, "bar");

	assert_str_equal(actual, "1 bar");

	free(actual);
}

static void vsnprintf_alloc__longer(void **state) {
	char *actual = vsnprintf_alloc_harness(10, "%d %s", 1, "bar");

	assert_str_equal(actual, "1 bar");

	free(actual);
}

static void vsnprintf_alloc__shorter(void **state) {
	char *actual = vsnprintf_alloc_harness(3, "%d %s", 1, "bar");

	assert_str_equal(actual, "1 b");

	free(actual);
}

static void vsnprintf_alloc__equal(void **state) {
	char *actual = vsnprintf_alloc_harness(5, "%d %s", 1, "bar");

	assert_str_equal(actual, "1 bar");

	free(actual);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(sprintf_alloc__ok),
		TEST(sprintf_alloc__empty),

		TEST(sprintf_append__ok),
		TEST(sprintf_append__empty),
		TEST(sprintf_append__null),

		TEST(vsprintf_alloc__ok),
		TEST(vsprintf_alloc__empty),

		TEST(vsnprintf_alloc__longer),
		TEST(vsnprintf_alloc__shorter),
		TEST(vsnprintf_alloc__equal),
		TEST(vsnprintf_alloc__empty),
	};

	return RUN(tests);
}

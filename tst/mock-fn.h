#ifndef MOCK_FN
#define MOCK_FN

#include <stdbool.h>
#include <stdint.h>

bool mock_equal(const void* const val, const void* const data);

bool mock_less_than(const void* const a, const void* const b);

bool mock_test(const void* const val, const void* const data);

bool mock_test_str(const char* const val, const void* const data);

bool mock_test_uint64_t(const uint64_t val, const void* const data);

const void *mock_alloc(const void* const val);

void mock_free(const void* const val);

void *mock_clone(const void* const val);

char* mock_str(const void* const val);

#endif // MOCK_FN

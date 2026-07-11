#ifndef MOCK_FN
#define MOCK_FN

#include <stdbool.h>
#include <stddef.h>

bool mock_equal(const void* const a, const void* const b);

bool mock_less_than(const void* const a, const void* const b);

bool mock_pred(const void* const ptr);

bool mock_2pred(const void* const val, const void* const data);

bool mock_2pred_str(const char* const val, const void* const data);

bool mock_2pred_szt(const size_t val, const void* const data);

bool mock_3pred(const void* const key, const void* const val, const void* const data);

bool mock_3pred_str_ptr(const char *key, const void* const val, const void* const data);

bool mock_3pred_str_str(const char *key, const char* const val, const void* const data);

bool mock_3pred_str_szt(const char *key, const size_t val, const void* const data);

bool mock_3pred_szt_ptr(const size_t key, const void* const val, const void* const data);

void *mock_clone(const void* const ptr);

void *mock_alloc(const void* const ptr);

void mock_free(const void* const ptr);

char* mock_str(const void* const ptr);

#endif // MOCK_FN

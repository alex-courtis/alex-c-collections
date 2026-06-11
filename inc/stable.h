#ifndef STABLE_H
#define STABLE_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Array backed string indexed table.
 * Entries preserve insertion order.
 * Operations linearly traverse keys.
 * NULL values permitted.
 * Not thread safe.
 */
struct STable; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct STableIterState; // IWYU pragma: keep
struct STableIter {
	const char *key;
	const void *val;
	struct STableIterState *st;
};

/*
 * Optional constructor params, defaults noted
 */
struct STableParams {
	const bool case_insensitive; // false
	fn_equal equal_val;          // compare val pointers
	fn_free free_val;            // free
	fn_str str_val;              // "%p"
	fn_clone clone_val;          // shallow clone
	const size_t initial;        // 10
	const size_t grow;           // 10
};

/*
 * Lifecycle
 */

// construct a table with STableParams defaults
const struct STable *stable_init(void);

// construct a table with params
const struct STable *stable_init_with(const struct STableParams params);

// clone a table
const struct STable *stable_clone(const struct STable* const from);

// free table
void stable_free(const struct STable* const tab);

// free table and vals
void stable_free_vals(const struct STable* const tab);

// free iter
void stable_iter_free(const struct STableIter* const iter);

/*
 * Access
 */

// return val, NULL if not present
const void *stable_get(const struct STable* const tab, const char* const key);

// create an iterator, caller must stable_iter_free or invoke stable_next until NULL
const struct STableIter *stable_iter(const struct STable* const tab);

// create an iterator filtering by test_key and test_val, NULL tests match all
const struct STableIter *stable_filter_iter(const struct STable* const tab, fn_test_str test_key, fn_test test_val, const void* const data);

// next iterator entry, NULL at end of table
const struct STableIter *stable_iter_next(const struct STableIter* const iter);

/*
 * Mutate
 */

// set key/val, return old val if overwritten
const void *stable_put(const struct STable* const tab, const char* const key, const void* const val);

// remove key, return old val if present
const void *stable_remove(const struct STable* const tab, const char* const key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses case_insensitive and equal_val from a
bool stable_equal(const struct STable* const a, const struct STable* const b);

/*
 * Conversion
 */

// ordered key pointers, caller frees list only
struct SList *stable_keys_slist(const struct STable* const tab);

// ordered val pointers, caller frees list only
struct SList *stable_vals_slist(const struct STable* const tab);

/*
 * Info
 */

// to string, user frees, format "k = str_val\n"
char *stable_str(const struct STable* const tab);

// number of entries
size_t stable_size(const struct STable* const tab);

#endif // STABLE_H


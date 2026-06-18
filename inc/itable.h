#ifndef ITABLE_H
#define ITABLE_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `PTable` with `size_t` keys
 */
struct ITable; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct ITableIterState; // IWYU pragma: keep
struct ITableIter {
	size_t key;
	const void *val;
	struct ITableIterState *st;
};

/*
 * Optional constructor params (default)
 */
struct ITableParams {
	const fn_equal equal_val; // _get, _put, _equal, _clone_    (compare key pointers)
	const fn_alloc alloc_val; // _put, _vals_slist, _clone_deep (use key pointer)
	const fn_free free_val;   // _put_free, _free_vals          (free)
	const fn_str str_val;     // _str                           (%p)
	const size_t initial;     // initial capacity               (10)
	const size_t grow;        // grow capacity by               (10)
};

/*
 * Test for a key
 */
typedef bool (*fn_equal_size_t)(const size_t a, const void* const b);

/*
 * Lifecycle
 */

// construct a table with ITableParams defaults
const struct ITable *itable_init(void);

// construct a table with params
const struct ITable *itable_init_with(const struct ITableParams params);

// clone a table, setting val pointers
const struct ITable *itable_clone_shallow(const struct ITable* const from);

// clone a table, NOP when NULL alloc_val
const struct ITable *itable_clone_deep(const struct ITable* const from);

// free table
void itable_free(const struct ITable* const tab);

// free table and vals
void itable_free_vals(const struct ITable* const tab);

// free iter
void itable_iter_free(const struct ITableIter* const iter);

/*
 * Access
 */

// return val, NULL if not present
const void *itable_get(const struct ITable* const tab, const size_t key);

// true if key is present
bool itable_contains_key(const struct ITable* const tab, const size_t key);

// create an iterator, caller must itable_iter_free or invoke itable_next until NULL
const struct ITableIter *itable_iter(const struct ITable* const tab);

// create an iterator filtering by equal_key and equal_val, NULL tests match all
const struct ITableIter *itable_filter_iter(const struct ITable* const tab, fn_equal_size_t equal_key, fn_equal equal_val, const void* const data);

// next iterator entry, NULL at end of table
const struct ITableIter *itable_iter_next(const struct ITableIter* const iter);

/*
 * Mutate
 */

// set key/val, return old val if overwritten
const void *itable_put(const struct ITable* const tab, const size_t key, const void* const val);

// set key/val if not present, return existing val if present
const void *itable_put_if_absent(const struct ITable* const tab, const size_t key, const void* const val);

// set key/val, free old val, return true if overwritten
bool itable_put_free(const struct ITable* const tab, const size_t key, const char* const val);

// remove key/val, return old val if present
const void *itable_remove(const struct ITable* const tab, const size_t key);

// remove key/val, if removed free val and return true
bool itable_remove_free(const struct ITable* const tab, const size_t key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses equal_val from a
bool itable_equal(const struct ITable* const a, const struct ITable* const b);

/*
 * Conversion
 */

// ordered val pointers, caller frees list only
struct SList *itable_vals_slist(const struct ITable* const tab);

/*
 * Info
 */

// to string, user frees, format "k = str_val\n"
char *itable_str(const struct ITable* const tab);

// number of entries
size_t itable_size(const struct ITable* const tab);

#endif // ITABLE_H


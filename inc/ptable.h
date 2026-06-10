#ifndef PTABLE_H
#define PTABLE_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Array backed pointer indexed table.
 * Entries preserve insertion order.
 * Operations linearly traverse keys.
 * NULL values permitted.
 * Not thread safe.
 */
struct PTable; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct PTableIter; // IWYU pragma: keep

/*
 * Optional constructor params, defaults noted
 */
struct PTableParams {
	fn_equal equal_key;   // compare key pointers
	fn_equal equal_val;   // compare val pointers
	fn_alloc alloc_key;   // set the key pointer, result be idempotent
	fn_free free_key;     // nop
	fn_str str_key;       // "%p"
	fn_clone clone_val;   // shallow clone
	const size_t initial; // 10
	const size_t grow;    // 10
};

/*
 * Lifecycle
 */

// construct a table with PTableParams defaults
const struct PTable *ptable_init(void);

// construct a table with params
const struct PTable *ptable_init_with(const struct PTableParams params);

// clone a table with clone_val
const struct PTable *ptable_clone(const struct PTable* const from);

// free table
void ptable_free(const void* const tab);

// free table and vals, null free_val uses free()
void ptable_free_vals(const struct PTable* const tab, fn_free free_val);

// free iter
void ptable_iter_free(const struct PTableIter* const iter);

/*
 * Access
 */

// return val, NULL if not present
const void *ptable_get(const struct PTable* const tab, const void* const key);

// create an iterator, caller must ptable_iter_free or invoke ptable_next until NULL
const struct PTableIter *ptable_iter(const struct PTable* const tab);

// create an iterator filtering by test_key and test_val, NULL tests match all
const struct PTableIter *ptable_filter_iter(const struct PTable* const tab, fn_test test_key, fn_test test_val, const void* const data);

// next iterator entry, NULL at end of table
const struct PTableIter *ptable_iter_next(const struct PTableIter* const iter);

// iterator key, NULL on NULL iter
const void *ptable_iter_key(const struct PTableIter* const iter);

// iterator value, NULL on NULL iter
const void *ptable_iter_val(const struct PTableIter* const iter);

/*
 * Mutate
 */

// set key/val, return old val if overwritten
const void *ptable_put(const struct PTable* const tab, const void* const key, const void* const val);

// remove key, return old val if present
const void *ptable_remove(const struct PTable* const tab, const void* const key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses equal_key and equal_val from a
bool ptable_equal(const struct PTable* const a, const struct PTable* const b);

/*
 * Conversion
 */

// ordered key pointers, caller frees list only
struct SList *ptable_keys_slist(const struct PTable* const tab);

// ordered val pointers, caller frees list only
struct SList *ptable_vals_slist(const struct PTable* const tab);

/*
 * Info
 */

// TODO move str_val to member
// to string, user frees, format "%p = %p\n", "%s" when str_val
char *ptable_str(const struct PTable* const tab, fn_str str_val);

// number of entries
size_t ptable_size(const struct PTable* const tab);

#endif // PTABLE_H


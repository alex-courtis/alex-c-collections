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
 * Lifecycle
 */

// construct a table with initial size 10, growing by 10 as necessary
const struct PTable *ptable_init(void);

// construct a table with initial size, growing as necessary, NULL on zero initial or grow
// NULL equal_key: compares pointers
// NULL alloc_key: key pointer
// NULL free_key: NOP
// NULL str_key: "%p"
const struct PTable *ptable_init_with(fn_equal equal_key, fn_alloc alloc_key, fn_free free_key, fn_str str_key, const size_t initial, const size_t grow);

// free table
void ptable_free(const void* const tab);

// free table and vals, null fn_free_val uses free()
void ptable_free_vals(const struct PTable* const tab, fn_free);

// free iter
void ptable_iter_free(const struct PTableIter* const iter);

/*
 * Access
 */

// return val, NULL not present
const void *ptable_get(const struct PTable* const tab, const void* const key);

// create an iterator, caller must ptable_iter_free or invoke ptable_next until NULL
const struct PTableIter *ptable_iter(const struct PTable* const tab);

// next iterator value, NULL at end of table
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

// same length, keys and vals equal in order, NULL equal compares pointers
bool ptable_equal(const struct PTable* const a, const struct PTable* const b, fn_equal);

/*
 * Conversion
 */

// ordered key pointers to list, caller frees list only
struct SList *ptable_keys_slist(const struct PTable* const tab);

// ordered val pointers to list, caller frees list only
struct SList *ptable_vals_slist(const struct PTable* const tab);

/*
 * Info
 */

// to string, user frees
// fn_str NULL: "%p = %p\n"
// fn_str:      "%p = %s\n"
// NULL vals always printed as "(null)"
char *ptable_str(const struct PTable* const tab, fn_str);

// number of entries with val
size_t ptable_size(const struct PTable* const tab);

// current capacity: initial + n * grow
size_t ptable_capacity(const struct PTable* const tab);

#endif // PTABLE_H


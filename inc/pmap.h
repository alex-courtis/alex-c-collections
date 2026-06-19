#ifndef PMAP_H
#define PMAP_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Array backed pointer indexed map.
 * Entries preserve insertion order.
 * Operations linearly traverse keys.
 * NULL values permitted.
 */
struct PMap; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct PMapIterState; // IWYU pragma: keep
struct PMapIter {
	const void *key;
	const void *val;
	struct PMapIterState *st;
};

// TODO move the usages back into function comments

/*
 * Optional constructor params (default)
 */
struct PMapParams {
	const fn_equal equal_key;   // _get, _contains_key, _put, _equal, _clone_ (compare key pointers)
	const fn_equal equal_val;   // _equal                                     (compare val pointers)
	const fn_alloc alloc_key;   // _clone_, _put, must be idempotent          (use key pointer)
	const fn_alloc alloc_val;   // _put, _vals_slist, _clone_deep             (use key pointer)
	const fn_free free_key;     // _remove, _free, _free_vals                 (NOP)
	const fn_free free_val;     // _put_free, _free_vals                      (free)
	const fn_str str_key;       // _str                                       (%p)
	const fn_str str_val;       // _str                                       (%p)
	const size_t initial;       // initial capacity                           (10)
	const size_t grow;          // grow capacity by                           (10)
};

/*
 * Lifecycle
 */

// construct with PMapParams defaults
const struct PMap *pmap_init(void);

// construct with params
const struct PMap *pmap_init_with(const struct PMapParams params);

// clone, setting val pointers
const struct PMap *pmap_clone_shallow(const struct PMap* const from);

// clone, NOP when NULL alloc_val
const struct PMap *pmap_clone_deep(const struct PMap* const from);

// free map
void pmap_free(const struct PMap* const tab);

// free map and vals
void pmap_free_vals(const struct PMap* const tab);

// free iter
void pmap_iter_free(const struct PMapIter* const iter);

/*
 * Access
 */

// return val, NULL if not present
const void *pmap_get(const struct PMap* const tab, const void* const key);

// true if key is present
bool pmap_contains_key(const struct PMap* const tab, const void* const key);

// create an iterator, caller must pmap_iter_free or invoke pmap_next until NULL
const struct PMapIter *pmap_iter(const struct PMap* const tab);

// create an iterator filtering by equal_key and equal_val, NULL tests match all
const struct PMapIter *pmap_filter_iter(const struct PMap* const tab, fn_equal equal_key, fn_equal equal_val, const void* const data);

// next iterator entry, NULL at end of map
const struct PMapIter *pmap_iter_next(const struct PMapIter* const iter);

/*
 * Mutate
 */

// set key/val, return old val if overwritten
const void *pmap_put(const struct PMap* const tab, const void* const key, const void* const val);

// set key/val if not present, return existing val if present
const void *pmap_put_if_absent(const struct PMap* const tab, const void* const key, const void* const val);

// set key/val, free old val, return true if overwritten
bool pmap_put_free(const struct PMap* const tab, const void* const key, const void* const val);

// remove val, return old val if present
const void *pmap_remove(const struct PMap* const tab, const void* const key);

// remove val, if removed free val and return true
bool pmap_remove_free(const struct PMap* const tab, const void* const key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses equal_key and equal_val from a
bool pmap_equal(const struct PMap* const a, const struct PMap* const b);

/*
 * Conversion
 */

// ordered keys, caller frees list only
struct SList *pmap_keys_slist_shallow(const struct PMap* const tab);

// ordered keys, caller frees list list and vals, NOP when NULL alloc_key
struct SList *pmap_keys_slist_deep(const struct PMap* const tab);

// ordered vals, caller frees list only
struct SList *pmap_vals_slist_shallow(const struct PMap* const tab);

// ordered vals, caller frees list and vals, NOP when NULL alloc_val
struct SList *pmap_vals_slist_deep(const struct PMap* const tab);

/*
 * Info
 */

// to string, user frees, format "str_key = str_val\n"
char *pmap_str(const struct PMap* const tab);

// number of entries
size_t pmap_size(const struct PMap* const tab);

#endif // PMAP_H


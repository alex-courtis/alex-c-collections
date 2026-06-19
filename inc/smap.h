#ifndef SMAP_H
#define SMAP_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `PMap` with string keys.
 * Keys are memory managed.
 */
struct SMap; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct SMapIterState; // IWYU pragma: keep
struct SMapIter {
	const char *key;
	const void *val;
	struct SMapIterState *st;
};

/*
 * Optional constructor params (default)
 */
struct SMapParams {
	const bool case_insensitive; //                                (false)
	const fn_equal equal_val;    // _get, _put, _equal, _clone_    (compare key pointers)
	const fn_alloc alloc_val;    // _put, _vals_slist, _clone_deep (use key pointer)
	const fn_free free_val;      // _put_free, _free_vals          (free)
	const fn_str str_val;        // _str                           (%p)
	const size_t initial;        // initial capacity               (10)
	const size_t grow;           // grow capacity by               (10)
};

/*
 * Lifecycle
 */

// construct a table with SMapParams defaults
const struct SMap *smap_init(void);

// construct a table with params
const struct SMap *smap_init_with(const struct SMapParams params);

// clone a table, setting val pointers
const struct SMap *smap_clone_shallow(const struct SMap* const from);

// clone a table, NOP when NULL alloc_val
const struct SMap *smap_clone_deep(const struct SMap* const from);

// free table
void smap_free(const struct SMap* const tab);

// free table and vals
void smap_free_vals(const struct SMap* const tab);

// free iter
void smap_iter_free(const struct SMapIter* const iter);

/*
 * Access
 */

// return val, NULL if not present
const void *smap_get(const struct SMap* const tab, const char* const key);

// true if key is present
bool smap_contains_key(const struct SMap* const tab, const char* const key);

// create an iterator, caller must smap_iter_free or invoke smap_next until NULL
const struct SMapIter *smap_iter(const struct SMap* const tab);

// create an iterator filtering by equal_key and equal_val, NULL tests match all
const struct SMapIter *smap_filter_iter(const struct SMap* const tab, fn_equal equal_key, fn_equal equal_val, const void* const data);

// next iterator entry, NULL at end of table
const struct SMapIter *smap_iter_next(const struct SMapIter* const iter);

/*
 * Mutate
 */

// set key/val, return old val if overwritten
const void *smap_put(const struct SMap* const tab, const char* const key, const void* const val);

// set key/val if not present, return existing val if present
const void *smap_put_if_absent(const struct SMap* const tab, const char* const key, const void* const val);

// set key/val, free old val, return true if overwritten
bool smap_put_free(const struct SMap* const tab, const  char* const key, const void* const val);

// remove val, return old val if present
const void *smap_remove(const struct SMap* const tab, const char* const key);

// remove val, if removed free val and return true
bool smap_remove_free(const struct SMap* const tab, const char* const key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses case_insensitive and equal_val from a
bool smap_equal(const struct SMap* const a, const struct SMap* const b);

/*
 * Conversion
 */

// ordered keys, caller frees list and vals
struct SList *smap_keys_slist_deep(const struct SMap* const tab);

// ordered vals, caller frees list only
struct SList *smap_vals_slist_shallow(const struct SMap* const tab);

// ordered vals, caller frees list and vals, NOP when NULL alloc_val
struct SList *smap_vals_slist_deep(const struct SMap* const tab);

/*
 * Info
 */

// to string, user frees, format "k = str_val\n"
char *smap_str(const struct SMap* const tab);

// number of entries
size_t smap_size(const struct SMap* const tab);

#endif // SMAP_H


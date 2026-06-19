#ifndef IMAP_H
#define IMAP_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `PMap` with `size_t` keys
 */
struct IMap; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct IMapIterState; // IWYU pragma: keep
struct IMapIter {
	size_t key;
	const void *val;
	struct IMapIterState *st;
};

/*
 * Optional constructor params (default)
 */
struct IMapParams {
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

// construct with IMapParams defaults
const struct IMap *imap_init(void);

// construct with params
const struct IMap *imap_init_with(const struct IMapParams params);

// clone, setting val pointers
const struct IMap *imap_clone_shallow(const struct IMap* const from);

// clone, NOP when NULL alloc_val
const struct IMap *imap_clone_deep(const struct IMap* const from);

// free map
void imap_free(const struct IMap* const tab);

// free map and vals
void imap_free_vals(const struct IMap* const tab);

// free iter
void imap_iter_free(const struct IMapIter* const iter);

/*
 * Access
 */

// return val, NULL if not present
const void *imap_get(const struct IMap* const tab, const size_t key);

// true if key is present
bool imap_contains_key(const struct IMap* const tab, const size_t key);

// create an iterator, caller must imap_iter_free or invoke imap_next until NULL
const struct IMapIter *imap_iter(const struct IMap* const tab);

// create an iterator filtering by equal_key and equal_val, NULL tests match all
const struct IMapIter *imap_filter_iter(const struct IMap* const tab, fn_equal_size_t equal_key, fn_equal equal_val, const void* const data);

// next iterator entry, NULL at end of map
const struct IMapIter *imap_iter_next(const struct IMapIter* const iter);

/*
 * Mutate
 */

// set key/val, return old val if overwritten
const void *imap_put(const struct IMap* const tab, const size_t key, const void* const val);

// set key/val if not present, return existing val if present
const void *imap_put_if_absent(const struct IMap* const tab, const size_t key, const void* const val);

// set key/val, free old val, return true if overwritten
bool imap_put_free(const struct IMap* const tab, const size_t key, const char* const val);

// remove val, return old val if present
const void *imap_remove(const struct IMap* const tab, const size_t key);

// remove val, if removed free val and return true
bool imap_remove_free(const struct IMap* const tab, const size_t key);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses equal_val from a
bool imap_equal(const struct IMap* const a, const struct IMap* const b);

/*
 * Conversion
 */

// ordered vals, caller frees list only
struct SList *imap_vals_slist_shallow(const struct IMap* const tab);

// ordered vals, caller frees list and vals, NOP when NULL alloc_val
struct SList *imap_vals_slist_deep(const struct IMap* const tab);

/*
 * Info
 */

// to string, user frees, format "k = str_val\n"
char *imap_str(const struct IMap* const tab);

// number of entries
size_t imap_size(const struct IMap* const tab);

#endif // IMAP_H


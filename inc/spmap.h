#ifndef SPMAP_H
#define SPMAP_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * `PPmap` with string keys.
 * Keys are memory managed.
 */
struct SPMap; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct SImaptState; // IWYU pragma: keep
struct SImapt {
	const char *key;
	const void *val;
	struct SImaptState *st;
};

/*
 * Optional constructor params (default)
 */
struct SPMapParams {
	const bool case_insensitive; // false
	const fn_equal equal_val;    // compare key pointers
	const fn_clone alloc_val;    // assign val pointer
	const fn_free free_val;      // free
	const fn_clone clone_val;    // NOP
	const fn_str str_val;        // %p
	const bool allow_null_val;   // false
	const size_t initial;        // 10
	const size_t grow;           // 10
};

/*
 * Key/Val
 */
struct SPMapPair {
	const char *key;
	const void *val;
};

/*
 * Lifecycle
 */

// construct with SPMapParams defaults
const struct SPMap *spmap_init(void);

// construct with params
const struct SPMap *spmap_init_with(const struct SPMapParams params);

// same params, caller frees vals when alloc_val present [alloc_val]
const struct SPMap *spmap_clone(const struct SPMap* const from);

// same params, caller frees vals, NULL on NULL clone_val, alloc_val overrides clone_val [alloc_key, alloc_val, clone_val]
const struct SPMap *spmap_clone_deep(const struct SPMap* const from);

// free map
void spmap_free(const struct SPMap* const map);

// free map and vals [free_val]
void spmap_free_vals(const struct SPMap* const map);

// free iterator
void spmap_it_free(const struct SImapt* const it);

/*
 * Access
 */

// return val, NULL if not present
const void *spmap_get(const struct SPMap* const map, const char* const key);

// true if key is present
bool spmap_contains_key(const struct SPMap* const map, const char* const key);

// true if val is present [equal_val]
bool spmap_contains_val(const struct SPMap* const map, const void* const val);

// find the first key/val match, {NULL,NULL} when no matches or NULL match
struct SPMapPair spmap_match(const struct SPMap* const map, fn_3pred_str_ptr match, const void* const data);

// find the first key match, {NULL,NULL} when no matches or NULL match
struct SPMapPair spmap_match_key(const struct SPMap* const map, fn_2pred_str match, const void* const data);

// find the first val match, {NULL,NULL} when no matches or NULL match
struct SPMapPair spmap_match_val(const struct SPMap* const map, fn_2pred match, const void* const data);

// create an iterator, caller must spmap_it_free or invoke spmap_next until NULL
const struct SImapt *spmap_it(const struct SPMap* const map);

// create an iterator filtering by key/val match, return NULL when no matches or NULL match
const struct SImapt *spmap_match_it(const struct SPMap* const map, fn_3pred_str_ptr match, const void* const data);

// create an iterator filtering by key match, return NULL when no matches or NULL match
const struct SImapt *spmap_match_key_it(const struct SPMap* const map, fn_2pred_str match, const void* const data);

// create an iterator filtering by val match, return NULL when no matches or NULL match
const struct SImapt *spmap_match_val_it(const struct SPMap* const map, fn_2pred match, const void* const data);

// next iterator entry, NULL at end of map
const struct SImapt *spmap_it_next(const struct SImapt* const it);

/*
 * Mutate
 */

// set key/val, return old val if overwritten [alloc_val]
const void *spmap_put(const struct SPMap* const map, const char* const key, const void* const val);

// set key/val if not present, return existing val if present [alloc_val]
const void *spmap_put_if_absent(const struct SPMap* const map, const char* const key, const void* const val);

// set key/val, free old val, return true if overwritten [alloc_val, free_val]
bool spmap_put_free(const struct SPMap* const map, const  char* const key, const void* const val);

// set all from key/val, returning number overwritten [alloc_val]
size_t spmap_put_all(const struct SPMap* const map, const struct SPMap* const from);

// set all from key/val, returning number overwritten, freeing overwritten vals [alloc_val, free_val]
size_t spmap_put_all_free(const struct SPMap* const map, const struct SPMap* const from);

// set all from key/val, returning number overwritten, NOP when NULL clone_val  [clone_val]
size_t spmap_put_all_clone(const struct SPMap* const map, const struct SPMap* const from);

// set all from key/val, returning number overwritten, freeing overwritten vals, NOP when NULL clone_val [free_val, clone_val]
size_t spmap_put_all_clone_free(const struct SPMap* const map, const struct SPMap* const from);

// remove entry, if removed return old val
const void *spmap_remove(const struct SPMap* const map, const char* const key);

// remove and free entry, if removed free it and return true [free_val]
bool spmap_remove_free(const struct SPMap* const map, const char* const key);

// remove entries matching from keys, return number removed [equal_key]
size_t spmap_remove_all(const struct SPMap* const map, const struct SPMap* const from);

// remove and free entries matching from keys, return number removed [equal_key, free_val]
size_t spmap_remove_all_free(const struct SPMap* const map, const struct SPMap* const from);

/*
 * Comparison
 */

// same length, keys and vals equal in order, uses params from a [equal_val]
bool spmap_equal(const struct SPMap* const a, const struct SPMap* const b);

/*
 * Conversion
 */

// map ordered keys, caller frees list and contents
struct Pslist *spmap_keys_pslist(const struct SPMap* const map);

// map ordered keys, same params
const struct SSet *spmap_keys_sset(const struct SPMap* const map);

// map ordered vals, caller frees list, caller frees contents when alloc_val present [alloc_val]
struct Pslist *spmap_vals_pslist(const struct SPMap* const map);

// map ordered vals, caller frees list and vals, NULL when NULL clone_val [clone_val]
struct Pslist *spmap_vals_pslist_clone(const struct SPMap* const map);

// map ordered vals, same params, caller frees set, caller frees vals when alloc_val present [alloc_val]
const struct PSet *spmap_vals_pset(const struct SPMap* const map);

// map ordered vals, same params, caller frees set and vals, NULL on NULL clone_val or both alloc_val and clone_val [clone_val]
const struct PSet *spmap_vals_pset_clone(const struct SPMap* const map);


/*
 * Info
 */

// to string, user frees, format "k = str_val\n"
char *spmap_str(const struct SPMap* const map);

// number of entries
size_t spmap_size(const struct SPMap* const map);

#endif // SPMAP_H


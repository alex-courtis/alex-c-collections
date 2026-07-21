#ifndef PLIST_H
#define PLIST_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Array backed pointer list.
 * Entries preserve insertion order.
 * Operations linearly traverse values.
 * NULL not permitted.
 */
struct Plist; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct PlistItState; // IWYU pragma: keep
struct PlistIt {
	const void* val;
	struct PlistItState *st;
};

/*
 * Filter, must match all when multiple predicates specified, empty filter matches anything
 */
struct PlistFilter {
	// test vals
	fn_pred_p val;

	// test vals against user data
	const void *data;
	fn_pred_pp val_data;
};

/*
 * Optional constructor params (default)
 */
struct PlistParams {
	const fn_equal equal_val; // compare val pointers
	const fn_clone alloc_val; // use val pointer
	const fn_free free_val;   // free
	const fn_clone clone_val; // use val pointer
	const fn_str str_val;     // %p
	const size_t initial;     // 10
	const size_t grow;        // 10
};

/*
 * Lifecycle
 */

// construct with PlistParams defaults
const struct Plist *plist_init(void);

// construct with params
const struct Plist *plist_init_with(const struct PlistParams params);

// same params, caller frees vals when alloc_val present [alloc_val]
const struct Plist *plist_clone(const struct Plist* const from);

// list ordered vals, caller frees vals, NULL when NULL clone_val [clone_val]
const struct Plist *plist_clone_deep(const struct Plist* const from);

// free list
void plist_free(const struct Plist* const list);

// free list and vals [free_val]
void plist_free_vals(const struct Plist* const list);

// free iterator
void plist_it_free(const struct PlistIt* const it);

/*
 * Access
 */

// true if this list contains the specified element [equal_val]
bool plist_contains(const struct Plist* const list, const void* const val);

// element at zero indexed position
const void *plist_at(const struct Plist* const list, const size_t i);

// find the first, NULL when no match or NULL match
const void *plist_find(const struct Plist* const list, const struct PlistFilter filter);

// create an iterator, caller must plist_it_free or invoke plist_next until NULL
const struct PlistIt *plist_it(const struct Plist* const list);

// create an iterator at the end, of the list caller must plist_it_free or invoke plist_next until NULL
const struct PlistIt *plist_it_end(const struct Plist* const list);

// create a filtering iterator, return NULL when no matches, caller must plist_it_free or invoke plist_next until NULL
const struct PlistIt *plist_filter_it(const struct Plist* const list, const struct PlistFilter filter);

// create a filtering iterator at the end of the list, return NULL when no matches, caller must plist_it_free or invoke plist_next until NULL
const struct PlistIt *plist_filter_it_end(const struct Plist* const list, const struct PlistFilter filter);

// next iterator val, NULL at end of list
const struct PlistIt *plist_it_next(const struct PlistIt* const it);

// prev iterator val, NULL at beginning of list
const struct PlistIt *plist_it_prev(const struct PlistIt* const it);

/*
 * Mutate
 */

// add if the list does not contain val, return true if added [equal_val, alloc_val]
bool plist_add(const struct Plist* const list, const void* const val);

// add from vals not contained in the list, return number added [equal_val, alloc_val]
size_t plist_add_all(const struct Plist* const list, const struct Plist* const from);

// add from vals not contained in the list, return number added, NOP when NULL clone_val [equal_val, clone_val]
size_t plist_add_all_clone(const struct Plist* const list, const struct Plist* const from);

// if the list contains val, remove it and return true [equal_val, alloc_val]
bool plist_remove(const struct Plist* const list, const void* const val);

// if the list contains val, remove it, free it and return true [equal_val, alloc_val, free_val]
bool plist_remove_free(const struct Plist* const list, const void* const val);

// remove all vals, returning number removed
size_t plist_remove_all(const struct Plist* const list);

// remove all vals and free, returning number removed [free_val]
size_t plist_remove_all_free(const struct Plist* const list);

// remove vals contained in, return number removed [equal_val]
size_t plist_remove_in(const struct Plist* const list, const struct Plist* const in);

// remove and free vals contained in, return number removed [equal_val, free_val]
size_t plist_remove_in_free(const struct Plist* const list, const struct Plist* const in);

// remove the it.val, it is unusable, plist_it_next must be called
void plist_it_remove(const struct PlistIt* const it);

// remove and free the it.val, it is unusable, plist_it_next must be called [free_val]
void plist_it_remove_free(const struct PlistIt* const it);

// shell sort in place, NULL less_than_val NOP
void plist_sort(const struct Plist* const list, fn_less_than less_than_val);

/*
 * Comparison
 */

// same length, vals equal in order, uses params from a [equal_val]
bool plist_equal(const struct Plist* const a, const struct Plist* const b);

/*
 * Conversion
 */

// list ordered vals, caller frees slist, caller frees contents when alloc_val present [alloc_val]
struct Pslist *plist_pslist(const struct Plist* const list);

// list ordered vals, caller frees slist and vals, NULL when NULL clone_val [clone_val]
struct Pslist *plist_pslist_clone(const struct Plist* const list);

/*
 * Info
 */

// to string, user frees, format "str_val\n"
char *plist_str(const struct Plist* const list);

// number of values
size_t plist_size(const struct Plist* const list);

#endif // PLIST_H


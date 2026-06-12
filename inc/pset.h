#ifndef PSET_H
#define PSET_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Array backed ordered set.
 * Operations linearly traverse values.
 * NULL not permitted.
 * Not thread safe.
 */
struct PSet; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct PSetIterState; // IWYU pragma: keep
struct PSetIter {
	const void* val;
	struct PSetIterState *st;
};

// todo note which functions use which

/*
 * Optional constructor params, defaults noted
 */
struct PSetParams {
	const fn_equal equal_val;         // compare val pointers
	const fn_less_than less_than_val; // no sorting
	const fn_free free_val;           // free
	const fn_clone clone_val;         // shallow clone
	const size_t initial;             // 10
	const size_t grow;                // 10
};

/*
 * Lifecycle
 */

// construct a set with PSetParams defaults
const struct PSet *pset_init(void);

// construct a set with params
const struct PSet *pset_init_with(const struct PSetParams params);

// clone a set
const struct PSet *pset_clone(const struct PSet* const from);

// free set
void pset_free(const struct PSet* const set);

// free table and vals
void pset_free_vals(const struct PSet* const set);

// free iter
void pset_iter_free(const struct PSetIter* const iter);

/*
 * Access
 */

// true if this set contains the specified element
bool pset_contains(const struct PSet* const set, const void* const val);

// todo maybe add a find first

// create an iterator, caller must pset_iter_free or invoke pset_next until NULL
const struct PSetIter *pset_iter(const struct PSet* const set);

// create an iterator filtering by test_val, NULL test_val matches all
const struct PSetIter *pset_filter_iter(const struct PSet* const set, fn_test test_val, const void* const data);

// next iterator value, NULL at end of set
const struct PSetIter *pset_iter_next(const struct PSetIter* const iter);

/*
 * Mutate
 */

// true if this set did not already contain the specified element
bool pset_add(const struct PSet* const set, const void* const val);

// true if this set contained the element
const void *pset_remove(const struct PSet* const set, const void* const val);

// shell sort in place
void pset_sort(const struct PSet* const set);

/*
 * Comparison
 */

// same length, vals equal in order, uses equal_val from a
bool pset_equal(const struct PSet* const a, const struct PSet* const b);

/*
 * Conversion
 */

// ordered val pointers, caller frees list only
struct SList *pset_slist(const struct PSet* const set);

/*
 * Info
 */

// to string, user frees, format "str_val\n", "%p" for NULL fn_str
char *pset_str(const struct PSet* const set, fn_str str_val);

// number of values
size_t pset_size(const struct PSet* const set);

#endif // PSET_H


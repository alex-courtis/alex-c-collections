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
struct PSetIter; // IWYU pragma: keep

/*
 * Lifecycle
 */

// construct a set with initial size 10, growing by 10 as necessary
const struct PSet *pset_init(void);

// construct a set with initial size, grow as needed, NULL on zero initial or grow
const struct PSet *pset_init_with(const size_t initial, const size_t grow);

// clone a set, NULL clone_val for shallow clone
const struct PSet *pset_clone(const struct PSet* const set, fn_clone clone_val);

// free set
void pset_free(const void* const set);

// free set and vals, NULL free_val uses free()
void pset_free_vals(const struct PSet* const set, fn_free free_val);

// free iter
void pset_iter_free(const struct PSetIter* const iter);

/*
 * Access
 */

// true if this set contains the specified element, null equal_val to compare pointers
bool pset_contains(const struct PSet* const set, const void* const val, fn_equal equal_val);

// create an iterator, caller must pset_iter_free or invoke pset_next until NULL
const struct PSetIter *pset_iter(const struct PSet* const set);

// next iterator value, NULL at end of set
const struct PSetIter *pset_iter_next(const struct PSetIter* const iter);

// iterator value, NULL on NULL iter
const void *pset_iter_val(const struct PSetIter* const iter);

/*
 * Mutate
 */

// true if this set did not already contain the specified element, NULL equal_val compares pointer
bool pset_add(const struct PSet* const set, const void* const val, fn_equal equal_val);

// true if this set contained the element, NULL equal_val compares pointer
const void *pset_remove(const struct PSet* const set, const void* const val, fn_equal equal_val);

// shell sort in place
void pset_sort(const struct PSet* const set, fn_less_than less_than_val);

/*
 * Comparison
 */

// same length, vals equal in order, NULL equal compares pointers
bool pset_equal(const struct PSet* const a, const struct PSet* const b, fn_equal equal_val);

/*
 * Conversion
 */

// ordered val pointers, caller frees list only
struct SList *pset_slist(const struct PSet* const set);

/*
 * Info
 */

// to string, user frees, format "%p\n", "%s" when str_val
char *pset_str(const struct PSet* const set, fn_str str_val);

// number of values
size_t pset_size(const struct PSet* const set);

// current capacity: initial + n * grow
size_t pset_capacity(const struct PSet* const set);

#endif // PSET_H


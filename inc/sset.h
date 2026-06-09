#ifndef SSET_H
#define SSET_H

#include <stdbool.h>
#include <stddef.h>

#include "fn.h"

/*
 * Array backed ordered string set.
 * Operations linearly traverse values.
 * NULL not permitted.
 * Not thread safe.
*/
struct SSet; // IWYU pragma: keep

/*
 * Entry iterator.
 */
struct SSetIter; // IWYU pragma: keep

/*
 * Optional constructor params, defaults noted
 */
struct SSetParams {
	const bool case_insensitive; // false
	const size_t initial;        // 10
	const size_t grow;           // 10
};

/*
 * Lifecycle
 */

// construct a set with defaults
const struct SSet *sset_init(void);

// construct a set with params
const struct SSet *sset_init_with(const struct SSetParams params);

// deep clone
const struct SSet *sset_clone(const struct SSet* const from);

// free set and vals
void sset_free(const struct SSet* const set);

// free iter
void sset_iter_free(const struct SSetIter* const iter);

/*
 * Access
 */

// true if this set contains the specified element
bool sset_contains(const struct SSet* const set, const char* const val);

// create an iterator, caller must sset_iter_free or invoke sset_next until NULL
const struct SSetIter *sset_iter(const struct SSet* const set);

// next iterator value, NULL at end of set
const struct SSetIter *sset_iter_next(const struct SSetIter* const iter);

// iterator value, NULL on NULL iter
const char *sset_iter_val(const struct SSetIter* const iter);

/*
 * Mutate
 */

// true if this set did not already contain the specified element
bool sset_add(const struct SSet* const set, const char* const val);

// true if this set contained the element
bool sset_remove(const struct SSet* const set, const char* const val);

// shell sort in place
void sset_sort(const struct SSet* const set);

/*
 * Comparison
 */

// same length, vals equal in order, case sensitivity is from a
bool sset_equal(const struct SSet* const a, const struct SSet* const b);

/*
 * Conversion
 */

// ordered strings, caller frees list
struct SList *sset_slist(const struct SSet* const set);

/*
 * Info
 */

// to string, user frees, format "%s\n"
char *sset_str(const struct SSet* const set);

// number of values
size_t sset_size(const struct SSet* const set);

#endif // SSET_H


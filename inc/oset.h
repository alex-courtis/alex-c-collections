#ifndef OSET_H
#define OSET_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Array backed ordered set.
 * Operations linearly traverse values.
 * Non NULL values only.
 */
struct OSet;

/*
 * Entry iterator.
 */
struct OSetIter {
	const void *val;
};

/*
 * Lifecycle
 */

// construct a set with initial size, grow as needed, NULL on zero param
const struct OSet *oset_init(const size_t initial, const size_t grow);

// free set
void oset_free(const void *set);

// free map and vals, NULL free_val uses free()
void oset_free_vals(const struct OSet *set, void (*free_val)(void *val));

// free iter
void oset_iter_free(const struct OSetIter *iter);

/*
 * Access
 */

// true if this set contains the specified element
bool oset_contains(const struct OSet *set, const void *val);

// true if this set contained the element
bool oset_remove(const struct OSet *set, const void *val);

// number of values
size_t oset_size(const struct OSet *set);

// create an iterator, caller must oset_iter_free or invoke oset_next until NULL
const struct OSetIter *oset_iter(const struct OSet *set);

// next iterator value, NULL at end of set
const struct OSetIter *oset_next(const struct OSetIter *iter);

/*
 * Mutate
 */

// true if this set did not already contain the specified element
bool oset_add(const struct OSet *set, const void *val);

#endif // OSET_H


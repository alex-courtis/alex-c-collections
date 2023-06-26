#ifndef PTABLE_H
#define PTABLE_H

#include <stddef.h>

/*
 * Convenience wrapper around ITable with pointer key.
 */
struct PTable;

/*
 * Entry iterator.
 */
struct PTableIter {
	const void *key;
	const void *val;
};

/*
 * Lifecycle
 */

// construct a table with initial size, growing as necessary, NULL on zero param
const struct PTable *ptable_init(const size_t initial, const size_t grow);

// free table
void ptable_free(const void *tab);

// free table and vals, null free_val uses free()
void ptable_free_vals(const struct PTable *tab, void (*free_val)(void *val));

// free iter
void ptable_iter_free(const struct PTableIter *iter);

/*
 * Access
 */

// return val, NULL not present
void *ptable_get(const struct PTable *tab, const void* key);

// create an iterator, caller must ptable_iter_free or invoke ptable_next until NULL
const struct PTableIter *ptable_iter(const struct PTable *tab);

// next iterator value, NULL at end of list
const struct PTableIter *ptable_next(const struct PTableIter *iter);

// number of entries with val
size_t ptable_size(const struct PTable *tab);

/*
 * Mutate
 */

// set key/val, return old val if overwritten, NULL val to remove
void *ptable_put(const struct PTable *tab, void* key, void *val);

#endif // PTABLE_H


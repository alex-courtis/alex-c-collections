#ifndef ITABLE_H
#define ITABLE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Array backed integer indexed table.
 * Entries preserve insertion order.
 * Operations linearly traverse keys.
 * Not thread safe.
 * Non NULL values only.
 */
struct ITable;

/*
 * Entry iterator.
 */
struct ITableIter {
	uint64_t key;
	const void *val;
};

/*
 * Lifecycle
 */

// construct a table with initial size, growing as necessary, NULL on zero param
const struct ITable *itable_init(const size_t initial, const size_t grow);

// free table
void itable_free(const void *tab);

// free table and vals, null free_val uses free()
void itable_free_vals(const struct ITable *tab, void (*free_val)(void *val));

// free iter
void itable_iter_free(const struct ITableIter *iter);

/*
 * Access
 */

// return val, NULL not present
void *itable_get(const struct ITable *tab, const uint64_t key);

// create an iterator, caller must itable_iter_free or invoke itable_next until NULL
const struct ITableIter *itable_iter(const struct ITable *tab);

// next iterator value, NULL at end of list
const struct ITableIter *itable_next(const struct ITableIter *iter);

// number of entries with val
size_t itable_size(const struct ITable *tab);

/*
 * Mutate
 */

// set key/val, return old val if overwritten, NULL val to remove
void *itable_put(const struct ITable *tab, const uint64_t key, void *val);

#endif // ITABLE_H


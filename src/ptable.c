#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "itable.h"

#include "ptable.h"

const struct PTable *ptable_init(const size_t initial, const size_t grow) {

	// pointer has to fit into ITable key
	assert(sizeof(void*) <= sizeof(uint64_t));

	return (struct PTable *)itable_init(initial, grow);
}

void ptable_free(const void *tab) {
	itable_free(tab);
}

void ptable_free_vals(const struct PTable *tab, void (*free_val)(void *val)) {
	itable_free_vals((struct ITable*)tab, free_val);
}

void *ptable_get(const struct PTable *tab, const void* key) {
	return itable_get((struct ITable*)tab, (uint64_t)key);
}

const struct PTableIter *ptable_iter(const struct PTable *tab) {
	return (struct PTableIter*)itable_iter((struct ITable*)tab);
}

const struct PTableIter *ptable_next(const struct PTableIter *iter) {
	return (struct PTableIter*)itable_next((struct ITableIter*)iter);
}

size_t ptable_size(const struct PTable *tab) {
	return itable_size((struct ITable*)tab);
}

void *ptable_put(const struct PTable *tab, void* key, void *val) {
	return itable_put((struct ITable*)tab, (uint64_t)key, val);
}


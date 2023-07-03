#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "itable.h"

struct ITable {
	uint64_t *keys;
	void **vals;
	size_t capacity;
	size_t grow;
	size_t next;
};

struct ITableIterP {
	struct ITableIter iter;
	const struct ITable *tab;
	uint64_t *k;
	void **v;
};

// grow to capacity + grow
void grow_itable(struct ITable *tab) {

	// grow new arrays
	uint64_t *new_keys = calloc(tab->capacity + tab->grow, sizeof(uint64_t));
	void **new_vals = calloc(tab->capacity + tab->grow, sizeof(void*));

	// copy old arrays
	memcpy(new_keys, tab->keys, tab->capacity * sizeof(uint64_t));
	memcpy(new_vals, tab->vals, tab->capacity * sizeof(void*));

	// free old arrays
	free(tab->keys);
	free(tab->vals);

	// lock in new
	tab->keys = new_keys;
	tab->vals = new_vals;
	tab->capacity += tab->grow;
}

const struct ITable *itable_init(const size_t initial, const size_t grow) {
	if (initial == 0 || grow == 0)
		return NULL;

	struct ITable *tab = calloc(1, sizeof(struct ITable));
	tab->capacity = initial;
	tab->grow = grow;
	tab->keys = calloc(tab->capacity, sizeof(uint64_t));
	tab->vals = calloc(tab->capacity, sizeof(void*));

	return tab;
}

void itable_free(const void *cvtab) {
	if (!cvtab)
		return;

	struct ITable *tab = (struct ITable*)cvtab;

	free(tab->keys);
	free(tab->vals);

	free(tab);
}

void itable_free_vals(const struct ITable *ctab, void (*free_val)(void *val)) {
	if (!ctab)
		return;

	struct ITable *tab = (struct ITable*)ctab;

	for (void **v = tab->vals; v < tab->vals + tab->capacity; v++) {
		if (*v) {
			if (free_val) {
				free_val(*v);
			} else {
				free(*v);
			}
		}
	}

	itable_free(tab);
}

void itable_iter_free(const struct ITableIter *iter) {
	if (!iter)
		return;

	free((void*)iter);
}

void *itable_get(const struct ITable *tab, const uint64_t key) {
	if (!tab)
		return NULL;

	// loop over keys
	uint64_t *k;
	void **v;
	for (k = tab->keys, v = tab->vals;
			k < tab->keys + tab->next;
			k++, v++) {
		if (*k == key) {
			return *v;
		}
	}

	return NULL;
}

const struct ITableIter *itable_iter(const struct ITable *tab) {
	if (!tab)
		return NULL;

	// loop over keys and vals
	uint64_t *k;
	void **v;
	for (k = tab->keys, v = tab->vals;
			v < tab->vals + tab->next && k < tab->keys + tab->next;
			k++, v++) {
		if (*v) {
			struct ITableIterP *iterp = calloc(1, sizeof(struct ITableIterP));

			iterp->tab = tab;
			iterp->iter.key = *k;
			iterp->iter.val = *v;
			iterp->k = k;
			iterp->v = v;

			return (struct ITableIter*)iterp;
		}
	}

	return NULL;
}

const struct ITableIter *itable_next(const struct ITableIter *iter) {
	if (!iter)
		return NULL;

	struct ITableIterP *iterp = (struct ITableIterP*)iter;

	if (!iterp || !iterp->tab) {
		itable_iter_free(iter);
		return NULL;
	}

	// loop over keys and vals
	while (++iterp->v < iterp->tab->vals + iterp->tab->next &&
			++iterp->k < iterp->tab->keys + iterp->tab->next) {
		if (*iterp->v) {
			iterp->iter.key = *(iterp->k);
			iterp->iter.val = *(iterp->v);
			return iter;
		}
	}

	itable_iter_free(iter);
	return NULL;
}

size_t itable_size(const struct ITable *tab) {
	if (!tab)
		return 0;

	// loop over vals
	void **v;
	size_t size;
	for (v = tab->vals, size = 0; v < tab->vals + tab->next; v++) {
		if (*v) {
			size++;
		}
	}

	return size;
}

void *itable_put(const struct ITable *ctab, const uint64_t key, void *val) {
	if (!ctab)
		return NULL;

	struct ITable *tab = (struct ITable*)ctab;

	// loop over existing keys
	uint64_t *k;
	void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->next; k++, v++) {

		// overwrite existing values, skip NULL holes
		if (*k == key && *v) {
			void *prev = *v;
			*v = val;
			return prev;
		}
	}

	// grow for new entry
	if (tab->next >= tab->capacity) {
		grow_itable(tab);
		k = &tab->keys[tab->next];
		v = &tab->vals[tab->next];
	}

	// new
	*k = key;
	*v = val;
	tab->next++;

	return NULL;
}


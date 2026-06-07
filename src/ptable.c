#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"
#include "str.h"

#include "ptable.h"

/*
   diff -u \
   <(sed -e ' s/stable/xtable/g ; s/STable/XTable/g ' src/stable.c) \
   <(sed -e 's/ptable/xtable/g ; s/PTable/XTable/g' src/ptable.c)
   */

struct PTable {
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t grow;
	size_t size;
	fn_equal equal_key;
	fn_alloc alloc_key;
	fn_free free_key;
	fn_str str_key;
};

struct PTableIter {
	const void *key;
	const void *val;
	const struct PTable *tab;
	size_t position;
};

// grow to capacity + grow
static void grow_ptable(struct PTable *tab) {

	// grow new arrays
	const void **new_keys = calloc(tab->capacity + tab->grow, sizeof(void*));
	const void **new_vals = calloc(tab->capacity + tab->grow, sizeof(void*));

	// copy old arrays
	memcpy(new_keys, tab->keys, tab->capacity * sizeof(void*));
	memcpy(new_vals, tab->vals, tab->capacity * sizeof(void*));

	// free old arrays
	free(tab->keys);
	free(tab->vals);

	// lock in new
	tab->keys = new_keys;
	tab->vals = new_vals;
	tab->capacity += tab->grow;
}

const struct PTable *ptable_init(void) {
	return ptable_init_with(NULL, NULL, NULL, NULL, 10, 10);
}

const struct PTable *ptable_init_with(fn_equal equal_key, fn_alloc alloc_key, fn_free free_key, fn_str str_key, const size_t initial, const size_t grow) {
	if (initial == 0 || grow == 0)
		return NULL;

	struct PTable *tab = calloc(1, sizeof(struct PTable));
	tab->capacity = initial;
	tab->grow = grow;
	tab->keys = calloc(tab->capacity, sizeof(void*));
	tab->vals = calloc(tab->capacity, sizeof(void*));
	tab->equal_key = equal_key;
	tab->alloc_key = alloc_key;
	tab->free_key = free_key;
	tab->str_key = str_key;

	return tab;
}

void ptable_free(const void* const cvtab) {
	if (!cvtab)
		return;

	struct PTable *tab = (struct PTable*)cvtab;

	if (tab->free_key) {
		for (const void **k = tab->keys; k < tab->keys + tab->capacity; k++) {
			tab->free_key(*k);
		}
	}

	free(tab->keys);
	free(tab->vals);

	free(tab);
}

void ptable_free_vals(const struct PTable* const tab, fn_free free_val) {
	if (!tab)
		return;

	for (const void **v = tab->vals; v < tab->vals + tab->capacity; v++) {
		if (*v) {
			if (free_val) {
				free_val(*v);
			} else {
				free((void*)*v);
			}
		}
	}

	ptable_free(tab);
}

void ptable_iter_free(const struct PTableIter* const iter) {
	if (!iter)
		return;

	free((void*)iter);
}

const void *ptable_get(const struct PTable* const tab, const void* const key) {
	if (!tab)
		return NULL;

	// loop over keys
	const void **k;
	const void **v;
	for (k = tab->keys, v = tab->vals;
			k < tab->keys + tab->size;
			k++, v++) {
		if (tab->equal_key ? tab->equal_key(*k, key) : *k == key) {
			return *v;
		}
	}

	return NULL;
}

const struct PTableIter *ptable_iter(const struct PTable* const tab) {
	if (!tab || tab->size == 0)
		return NULL;

	// first key/val
	struct PTableIter *it = calloc(1, sizeof(struct PTableIter));
	it->tab = tab;
	it->key = *(tab->keys);
	it->val = *(tab->vals);
	it->position = 0;

	return it;
}

const struct PTableIter *ptable_iter_next(const struct PTableIter* const iter) {
	if (!iter)
		return NULL;

	struct PTableIter *it = (struct PTableIter*)iter;

	if (!it->tab) {
		ptable_iter_free(it);
		return NULL;
	}

	if (++it->position < it->tab->size) {
		it->key = *(it->tab->keys + it->position);
		it->val = *(it->tab->vals + it->position);
		return it;
	} else {
		ptable_iter_free(it);
		return NULL;
	}
}

const void *ptable_iter_key(const struct PTableIter* const iter) {
	return iter ? iter->key : NULL;
}

const void *ptable_iter_val(const struct PTableIter* const iter) {
	return iter ? iter->val : NULL;
}

const void *ptable_put(const struct PTable* const ctab, const void* const key, const void* const val) {
	if (!ctab || !key)
		return NULL;

	struct PTable *tab = (struct PTable*)ctab;

	// loop over existing keys
	const void **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {

		// overwrite existing values
		if (tab->equal_key ? tab->equal_key(*k, key) : *k == key) {
			const void *prev = *v;
			*v = val;
			return prev;
		}
	}

	// grow for new entry
	if (tab->size >= tab->capacity) {
		grow_ptable(tab);
		k = &tab->keys[tab->size];
		v = &tab->vals[tab->size];
	}

	// new
	if (tab->alloc_key) {
		*k = tab->alloc_key(key);
	} else {
		*k = key;
	}
	*v = val;
	tab->size++;

	return NULL;
}

const void *ptable_remove(const struct PTable* const ctab, const void* const key) {
	if (!ctab)
		return NULL;

	struct PTable *tab = (struct PTable*)ctab;

	// loop over existing keys
	const void **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {

		if (tab->equal_key ? tab->equal_key(*k, key) : *k == key) {
			if (tab->free_key) {
				tab->free_key((void*)*k);
			}
			*k = NULL;
			const void* prev = *v;
			*v = NULL;
			tab->size--;

			// shift down over removed
			const void **mk;
			const void **mv;
			for (mk = k, mv = v; mk < tab->keys + tab->size; mk++, mv++) {
				*mk = *(mk + 1);
				*mv = *(mv + 1);
			}
			*mk = NULL;
			*mv = NULL;

			return prev;
		}
	}

	return NULL;
}

bool ptable_equal(const struct PTable* const a, const struct PTable* const b, fn_equal equal) {
	if (!a || !b || a->size != b->size)
		return false;

	const void **ak, **bk;
	const void **av, **bv;

	for (ak = a->keys, bk = b->keys, av = a->vals, bv = b->vals;
			ak < a->keys + a->size;
			ak++, bk++, av++, bv++) {

		// key
		if (!(a->equal_key ? a->equal_key(*ak, *bk) : *ak == *bk)) {
			return false;
		}

		// value
		if (equal) {
			if (!equal(*av, *bv)) {
				return false;
			}
		} else if (*av != *bv) {
			return false;
		}
	}

	return true;
}

struct SList *ptable_keys_slist(const struct PTable* const tab) {
	if (!tab)
		return NULL;

	struct SList *list = NULL;

	const void **k;
	for (k = tab->keys; k < tab->keys + tab->size; k++) {
		slist_append(&list, (void*)*k);
	}

	return list;
}

struct SList *ptable_vals_slist(const struct PTable* const tab) {
	if (!tab)
		return NULL;

	struct SList *list = NULL;

	const void **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {
		slist_append(&list, (void*)*v);
	}

	return list;
}

char *ptable_str(const struct PTable* const tab, fn_str str_val) {
	if (!tab)
		return NULL;

	char *out = strdup("");

	const void **k;
	const void **v;
	for (k = tab->keys, v = tab->vals; k < tab->keys + tab->size; k++, v++) {

		if (tab->str_key) {
			char *key = tab->str_key(*k);
			out = sprintf_append(out, "%s = ", key ? key : "???");
			free(key);
		} else {
			out = sprintf_append(out, "%p = ", *k ? *k : "(null)");
		}

		if (*v) {
			if (str_val) {
				char *val = str_val(*v);
				out = sprintf_append(out, "%s\n", val);
				free(val);
			} else {
				out = sprintf_append(out, "%s\n", (char*)*v);
			}
		} else {
			out = sprintf_append(out, "%s", "(null)\n");
		}
	}

	return out;
}

size_t ptable_size(const struct PTable* const tab) {
	return tab ? tab->size : 0;
}

size_t ptable_capacity(const struct PTable* const tab) {
	return tab ? tab->capacity : 0;
}

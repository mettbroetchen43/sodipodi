#define __ARIKKEI_DICT_C__

/*
 * Arikkei
 *
 * Basic datatypes and code snippets
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 *
 */

#include <malloc.h>
#include <string.h>

#include "arikkei-dict.h"

#ifndef NULL
#define NULL 0L
#endif

struct _ArikkeiDictEntry {
	int next;
	const void *key;
	void *val;
};

static unsigned int
arikkei_string_hash (const void *data)
{
	const unsigned char *p;
	unsigned int hval;
	p = data;
	hval = *p;
	if (hval) {
		for (p += 1; *p; p++) hval = (hval << 5) - hval + *p;
	}
	return hval;
}

static unsigned int
arikkei_string_equal (const void *l, const void *r)
{
	return !strcmp (l, r);
}

static unsigned int
arikkei_pointer_hash (const void *data)
{
	unsigned int hval, p;
	hval = 0;
	p = (unsigned int) data;
	while (p) {
		hval ^= p;
		p /= 17;
	}
	return hval;
}

static unsigned int
arikkei_pointer_equal (const void *l, const void *r)
{
	return l == r;
}

static unsigned int
arikkei_int_hash (const void *data)
{
	unsigned int hval, p;
	hval = 0;
	p = (unsigned int) data;
	while (p) {
		hval ^= p;
		p /= 17;
	}
	return hval;
}

static unsigned int
arikkei_int_equal (const void *l, const void *r)
{
	return (unsigned int) l == (unsigned int) r;
}

void
arikkei_dict_setup_full (ArikkeiDict *dict, unsigned int hashsize,
			 unsigned int (* hash) (const void *),
			 unsigned int (* equal) (const void *, const void *))
{
	unsigned int i;
	if (hashsize < 1) hashsize = 1;
	dict->size = hashsize << 1;
	dict->hashsize = hashsize;
	dict->entries = (ArikkeiDictEntry *) malloc (dict->size * sizeof (ArikkeiDictEntry));
	for (i = 0; i < dict->hashsize; i++) dict->entries[i].key = NULL;
	for (i = dict->hashsize; i < dict->size - 1; i++) dict->entries[i].next = i + 1;
	dict->entries[dict->size - 1].next = -1;
	dict->free = dict->hashsize;
	dict->hash = hash;
	dict->equal = equal;
}

void
arikkei_dict_setup_string (ArikkeiDict *dict, unsigned int hashsize)
{
	arikkei_dict_setup_full (dict, hashsize, arikkei_string_hash, arikkei_string_equal);
}

void
arikkei_dict_setup_pointer (ArikkeiDict *dict, unsigned int hashsize)
{
	arikkei_dict_setup_full (dict, hashsize, arikkei_pointer_hash, arikkei_pointer_equal);
}

void
arikkei_dict_setup_int (ArikkeiDict *dict, unsigned int hashsize)
{
	arikkei_dict_setup_full (dict, hashsize, arikkei_int_hash, arikkei_int_equal);
}

void
arikkei_dict_release (ArikkeiDict *dict)
{
	free (dict->entries);
}

void
arikkei_dict_insert (ArikkeiDict *dict, const void *key, void *val)
{
	unsigned int hval;
	int pos;
	if (!key) return;
	hval = dict->hash (key) % dict->hashsize;
	if (dict->entries[hval].key) {
		for (pos = hval; pos >= 0; pos = dict->entries[pos].next) {
			if (dict->equal (dict->entries[pos].key, key)) {
				dict->entries[pos].val = val;
				return;
			}
		}
		/* Have to prepend new element */
		if (dict->free < 0) {
			int newsize, i;
			newsize = dict->size << 1;
			dict->entries = (ArikkeiDictEntry *) realloc (dict->entries, newsize * sizeof (ArikkeiDictEntry));
			for (i = dict->size; i < newsize - 1; i++) dict->entries[i].next = i + 1;
			dict->entries[newsize - 1].next = -1;
			dict->free = dict->size;
			dict->size = newsize;
		}
		pos = dict->free;
		dict->free = dict->entries[pos].next;
		dict->entries[pos].next = dict->entries[hval].next;
		dict->entries[pos].key = key;
		dict->entries[pos].val = val;
		dict->entries[hval].next = pos;
	} else {
		/* Insert root element */
		dict->entries[hval].next = -1;
		dict->entries[hval].key = key;
		dict->entries[hval].val = val;
		return;
	}
}

void
arikkei_dict_remove (ArikkeiDict *dict, const void *key)
{
	unsigned int hval;
	if (!key) return;
	hval = dict->hash (key) % dict->hashsize;
	if (!dict->entries[hval].key) return;
	if (dict->equal (dict->entries[hval].key, key)) {
		/* Have to remove root key */
		if (dict->entries[hval].next) {
			int pos;
			pos = dict->entries[hval].next;
			dict->entries[hval] = dict->entries[pos];
			dict->entries[pos].next = dict->free;
			dict->free = pos;
		} else {
			dict->entries[hval].key = NULL;
		}
	} else {
		int pos, prev;
		prev = hval;
		for (pos = dict->entries[hval].next; pos >= 0; pos = dict->entries[pos].next) {
			if (dict->equal (dict->entries[pos].key, key)) {
				dict->entries[prev].next = dict->entries[pos].next;
				dict->entries[pos].next = dict->free;
				dict->free = pos;
				return;
			}
			prev = pos;
		}
	}
}

void *
arikkei_dict_lookup (ArikkeiDict *dict, const void *key)
{
	unsigned int hval;
	int pos;
	if (!key) return NULL;
	hval = dict->hash (key) % dict->hashsize;
	if (dict->entries[hval].key) {
		for (pos = hval; pos >= 0; pos = dict->entries[pos].next) {
			if (dict->equal (dict->entries[pos].key, key)) {
				return dict->entries[pos].val;
			}
		}
	}
	return NULL;
}



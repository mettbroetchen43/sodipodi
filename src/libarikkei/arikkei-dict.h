#ifndef __ARIKKEI_DICT_H__
#define __ARIKKEI_DICT_H__

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

typedef struct _ArikkeiDict ArikkeiDict;
typedef struct _ArikkeiDictEntry ArikkeiDictEntry;

struct _ArikkeiDict {
	unsigned int size;
	unsigned int hashsize;
	ArikkeiDictEntry *entries;
	int free;
	unsigned int (* hash) (const void *data);
	unsigned int (* equal) (const void *l, const void *r);
};

void arikkei_dict_setup_full (ArikkeiDict *dict, unsigned int hashsize,
			      unsigned int (* hash) (const void *),
			      unsigned int (* equal) (const void *, const void *));
void arikkei_dict_setup_string (ArikkeiDict *dict, unsigned int hashsize);
void arikkei_dict_setup_pointer (ArikkeiDict *dict, unsigned int hashsize);
void arikkei_dict_setup_int (ArikkeiDict *dict, unsigned int hashsize);
void arikkei_dict_release (ArikkeiDict *dict);

void arikkei_dict_insert (ArikkeiDict *dict, const void *key, void *val);
void arikkei_dict_remove (ArikkeiDict *dict, const void *key);
void *arikkei_dict_lookup (ArikkeiDict *dict, const void *key);

#endif

#define __NR_TYPE_PRIMITIVES_C__

/*
 * Typeface and script library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

/* This should be enough for approximately 10000 fonts */
#define NR_DICTSIZE 2777

#include <libarikkei/arikkei-dict.h>
#include <libnr/nr-macros.h>

#include "nr-type-primitives.h"

void
nr_name_list_release (NRNameList *list)
{
	if (list->destructor) {
		list->destructor (list);
	}
}

/* fixme: Remove typedict stuff (Lauris) */

NRTypeDict *
nr_type_dict_new (void)
{
	ArikkeiDict *dict;
	dict = nr_new (ArikkeiDict, 1);
	arikkei_dict_setup_string (dict, NR_DICTSIZE);
	return (NRTypeDict *) dict;
}


void
nr_type_dict_insert (NRTypeDict *td, const unsigned char *key, void *val)
{
	arikkei_dict_insert ((ArikkeiDict *) td, key, val);
}

void *
nr_type_dict_lookup (NRTypeDict *td, const unsigned char *key)
{
	return arikkei_dict_lookup ((ArikkeiDict *) td, key);
}


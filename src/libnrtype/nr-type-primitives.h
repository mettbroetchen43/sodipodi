#ifndef __NR_TYPE_PRIMITIVES_H__
#define __NR_TYPE_PRIMITIVES_H__

/*
 * Typeface and script library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

typedef struct _NRNameList NRNameList;
typedef struct _NRTypeDict NRTypeDict;

typedef void (* NRNameListDestructor) (NRNameList *list);

struct _NRNameList {
	unsigned long length;
	unsigned char **names;
	NRNameListDestructor destructor;
};

void nr_name_list_release (NRNameList *list);

NRTypeDict *nr_type_dict_new (void);

void nr_type_dict_insert (NRTypeDict *td, const unsigned char *key, void *val);

void *nr_type_dict_lookup (NRTypeDict *td, const unsigned char *key);

#endif

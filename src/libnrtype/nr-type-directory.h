#ifndef __NR_TYPE_DIRECTORY_H__
#define __NR_TYPE_DIRECTORY_H__

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

typedef void (* NRNameListDestructor) (NRNameList *list);

#include <libnrtype/nr-typeface.h>

struct _NRNameList {
	unsigned long length;
	unsigned char **names;
	NRNameListDestructor destructor;
	void *data;
};

void nr_name_list_release (NRNameList *list);

NRTypeFace *nr_type_directory_lookup_fuzzy (const unsigned char *family, const unsigned char *style);

NRNameList *nr_type_directory_family_list_get (NRNameList *families);
NRNameList *nr_type_directory_style_list_get (const unsigned char *family, NRNameList *styles);

#endif

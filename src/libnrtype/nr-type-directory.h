#ifndef __NR_TYPE_DIRECTORY_H__
#define __NR_TYPE_DIRECTORY_H__

/*
 * Typeface and script library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <libnrtype/nr-type-primitives.h>
#include <libnrtype/nr-typeface.h>

NRTypeFace *nr_type_directory_lookup (const unsigned char *name);
NRTypeFace *nr_type_directory_lookup_fuzzy (const unsigned char *family, const unsigned char *style);

NRNameList *nr_type_directory_family_list_get (NRNameList *flist);
NRNameList *nr_type_directory_style_list_get (const unsigned char *family, NRNameList *slist);

void nr_type_directory_forget_face (NRTypeFace *tf);

#endif

#define __NR_TYPE_DIRECTORY_C__

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

#include <string.h>
#include <libnr/nr-macros.h>
#include "nr-type-gnome.h"
#include "nr-type-directory.h"

NRTypeFace *
nr_type_directory_lookup_fuzzy (const unsigned char *family, const unsigned char *style)
{
	NRTypeFace *face;
	unsigned int weight;
	unsigned int italic;

	italic = FALSE;
	weight = GNOME_FONT_BOOK;
	if (style) {
		if (strstr (style, "italic")) italic = TRUE;
		if (strstr (style, "oblique")) italic = TRUE;
		if (strstr (style, "bold")) weight = GNOME_FONT_BOLD;
	}

	face = nr_new (NRTypeFace, 1);

	face->refcount = 1;
	face->face = gnome_font_unsized_closest (family, weight, italic);

	return face;
}



#ifndef __NR_TYPEFACE_H__
#define __NR_TYPEFACE_H__

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

typedef struct _NRTypeFace NRTypeFace;

#include <libnr/nr-types.h>
#include <libnr/nr-path.h>

enum {
	NR_TYPEFACE_METRICS_DEFAULT,
	NR_TYPEFACE_METRICS_HORIZONTAL,
	NR_TYPEFACE_METRICS_VERTICAL
};

NRTypeFace *nr_typeface_ref (NRTypeFace *tf);
NRTypeFace *nr_typeface_unref (NRTypeFace *tf);

const unsigned char *nr_typeface_get_family_name (NRTypeFace *tf);
const unsigned char *nr_typeface_get_attribute (NRTypeFace *tf, const unsigned char *key);

NRBPath *nr_typeface_get_glyph_outline (NRTypeFace *tf, int glyph, unsigned int metrics, NRBPath *d, unsigned int ref);
void nr_typeface_unref_glyph_outline (NRTypeFace *tf, int glyph);

unsigned int nr_typeface_lookup_default (NRTypeFace *tf, int unival);

#endif

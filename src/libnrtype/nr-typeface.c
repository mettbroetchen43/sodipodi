#define __NR_TYPEFACE_C__

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
#include <libnr/nr-matrix.h>
#include "nr-typeface.h"

NRTypeFace *
nr_typeface_ref (NRTypeFace *tf)
{
	tf->refcount += 1;

	return tf;
}

NRTypeFace *
nr_typeface_unref (NRTypeFace *tf)
{
	tf->refcount -= 1;

	if (tf->refcount < 1) {
		tf->vmv->free (tf);
	}

	return NULL;
}

unsigned int
nr_typeface_name_get (NRTypeFace *tf, unsigned char *str, unsigned int size)
{
	return nr_typeface_attribute_get (tf, "name", str, size);
}

unsigned int
nr_typeface_family_name_get (NRTypeFace *tf, unsigned char *str, unsigned int size)
{
	return nr_typeface_attribute_get (tf, "family", str, size);
}

unsigned int
nr_typeface_attribute_get (NRTypeFace *tf, const unsigned char *key, unsigned char *str, unsigned int size)
{
	return tf->vmv->attribute_get (tf, key, str, size);
}

NRBPath *
nr_typeface_glyph_outline_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRBPath *d, unsigned int ref)
{
	return tf->vmv->glyph_outline_get (tf, glyph, metrics, d, ref);
}

void
nr_typeface_glyph_outline_unref (NRTypeFace *tf, unsigned int glyph)
{
	return tf->vmv->glyph_outline_unref (tf, glyph);
}

NRPointF *
nr_typeface_glyph_advance_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv)
{
	return tf->vmv->glyph_advance_get (tf, glyph, metrics, adv);
}

unsigned int
nr_typeface_lookup_default (NRTypeFace *tf, unsigned int unival)
{
	return tf->vmv->lookup (tf, NR_TYPEFACE_LOOKUP_RULE_DEFAULT, unival);
}

NRFont *
nr_font_new_default (NRTypeFace *tf, unsigned int metrics, float size)
{
	NRMatrixF scale;

	nr_matrix_f_set_scale (&scale, size, size);

	return tf->vmv->font_new (tf, metrics, &scale);
}



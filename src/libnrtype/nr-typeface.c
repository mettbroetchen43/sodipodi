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
#include "nr-type-gnome.h"
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
		gnome_font_face_unref (tf->face);
		nr_free (tf);
	}

	return NULL;
}

const unsigned char *
nr_typeface_get_family_name (NRTypeFace *tf)
{
	return gnome_font_face_get_family_name (tf->face);
}

const unsigned char *
nr_typeface_get_attribute (NRTypeFace *tf, const unsigned char *key)
{
	if (!strcmp (key, "weight")) {
		guint wc;
		wc = gnome_font_face_get_weight_code (tf->face);
		return (wc >= GNOME_FONT_BOLD) ? "bold" : "normal";
	} else if (!strcmp (key, "style")) {
		return gnome_font_face_is_italic (tf->face) ? "italic" : "normal";
	}

	return "normal";
}

NRBPath *
nr_typeface_get_glyph_outline (NRTypeFace *tf, int glyph, unsigned int metrics, NRBPath *d, unsigned int ref)
{
	if (metrics == NR_TYPEFACE_METRICS_VERTICAL) {
		/* fixme: Really should implement this */
		return NULL;
	} else {
		d->path = (ArtBpath *) gnome_font_face_get_glyph_stdoutline (tf->face, glyph);
		if (ref) gnome_font_face_ref (tf->face);
	}

	return d;
}

void
nr_typeface_unref_glyph_outline (NRTypeFace *tf, int glyph)
{
	/* fixme: This is somewhat dangerous, but OK... */
	gnome_font_face_unref (tf->face);
}

unsigned int
nr_typeface_lookup_default (NRTypeFace *tf, int unival)
{
	return gnome_font_face_lookup_default (tf->face, unival);
}


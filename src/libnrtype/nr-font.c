#define __NR_FONT_C__

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

#include <libnr/nr-macros.h>
#include "nr-type-gnome.h"
#include "nr-font.h"

NRFont *
nr_font_new_default (NRTypeFace *tf, unsigned int metrics, float size)
{
	NRFont *font;

	font = nr_new (NRFont, 1);

	font->refcount = 1;
	font->metrics = metrics;
	font->face = nr_typeface_ref (tf);
	font->font = gnome_font_face_get_font_default (tf->face, size);
	font->nglyphs = tf->nglyphs;

	return font;
}

NRFont *
nr_font_ref (NRFont *font)
{
	font->refcount += 1;

	return font;
}

NRFont *
nr_font_unref (NRFont *font)
{
	font->refcount -= 1;

	if (font->refcount < 1) {
		nr_typeface_unref (font->face);
		gnome_font_unref (font->font);
		nr_free (font);
	}

	return NULL;
}

NRBPath *
nr_font_get_glyph_outline (NRFont *font, int glyph, NRBPath *d, unsigned int ref)
{
	if (font->metrics == NR_TYPEFACE_METRICS_VERTICAL) {
		/* fixme: Really should implement this */
		return NULL;
	} else {
		d->path = (ArtBpath *) gnome_font_get_glyph_stdoutline (font->font, glyph);
		if (ref) gnome_font_ref (font->font);
	}

	return d;
}

void
nr_font_unref_glyph_outline (NRFont *font, int glyph)
{
	/* fixme: This is somewhat dangerous, but OK... */
	gnome_font_unref (font->font);
}

NRPointF *
nr_font_get_glyph_advance (NRFont *font, int glyph, NRPointF *adv)
{
	if (font->metrics == NR_TYPEFACE_METRICS_VERTICAL) {
		/* fixme: Really should implement this */
		return NULL;
	} else {
		ArtPoint p;
		if (gnome_font_get_glyph_stdadvance (font->font, glyph, &p)) {
			adv->x = p.x;
			adv->y = p.y;
			return adv;
		} else {
			return NULL;
		}
	}

	return adv;
}

float
nr_font_get_size (NRFont *font)
{
	return gnome_font_get_size (font->font);
}

NRTypeFace *
nr_font_get_typeface (NRFont *font)
{
	return font->face;
}


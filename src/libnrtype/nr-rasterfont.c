#define __NR_RASTERFONT_C__

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
#include <libnr/nr-matrix.h>
#include "nr-rasterfont.h"

struct _NRRasterFont {
	unsigned int refcount;
	NRFont *font;
	NRMatrixF transform;
};

NRRasterFont *
nr_rasterfont_new (NRFont *font, NRMatrixF *transform)
{
	NRRasterFont *rf;

	rf = nr_new (NRRasterFont, 1);

	rf->refcount = 1;
	rf->font = nr_font_ref (font);
	rf->transform = *transform;

	return rf;
}

NRRasterFont *
nr_rasterfont_ref (NRRasterFont *rf)
{
	rf->refcount += 1;

	return rf;
}

NRRasterFont *
nr_rasterfont_unref (NRRasterFont *rf)
{
	rf->refcount -= 1;

	if (rf->refcount < 1) {
		nr_font_unref (rf->font);
		nr_free (rf);
	}

	return NULL;
}

NRPointF *
nr_rasterfont_get_glyph_advance (NRRasterFont *rf, int glyph, unsigned int metric, NRPointF *adv)
{
	NRPointF a;

	if (nr_font_get_glyph_advance (rf->font, glyph, metric, &a)) {
		adv->x = NR_MATRIX_DF_TRANSFORM_X (&rf->transform, a.x, a.y);
		adv->y = NR_MATRIX_DF_TRANSFORM_Y (&rf->transform, a.x, a.y);
		return adv;
	}

	return NULL;
}

NRFont *
nr_rasterfont_get_font (NRRasterFont *rf)
{
	return rf->font;
}

NRTypeFace *
nr_rasterfont_get_typeface (NRRasterFont *rf)
{
	nr_font_get_typeface (rf->font);
}


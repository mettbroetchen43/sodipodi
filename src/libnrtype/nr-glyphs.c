#define __NR_GLYPHS_C__

/*
 * Typeface and script library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <libnr/nr-rect.h>

#include "nr-glyphs.h"

/* Transform incorporates initial position as well */
NRPGL *
nr_pgl_new_from_string (NRUShort *chars, unsigned int length, NRFont *font, NRMatrixF *transform)
{
	NRPGL *pgl;
	unsigned int i;

	pgl = (NRPGL *) malloc (sizeof (NRPGL) + length * sizeof (struct _NRPosGlyph));

	pgl->rfont = nr_rasterfont_new (font, transform);
	nr_rect_f_set_empty (&pgl->area);
	pgl->advance.x = transform->c[4];
	pgl->advance.y = transform->c[5];
	pgl->length = length;

	for (i = 0; i < length; i++) {
		int glyph;
		NRPointF adv;
		NRRectF area;
		glyph = nr_typeface_lookup_default (font->face, chars[i]);
		pgl->glyphs[i].glyph = glyph;
		pgl->glyphs[i].x = pgl->advance.x;
		pgl->glyphs[i].y = pgl->advance.y;
		nr_rasterfont_glyph_advance_get (pgl->rfont, glyph, &adv);
		pgl->advance.x += adv.x;
		pgl->advance.y += adv.y;
		nr_rasterfont_glyph_area_get (pgl->rfont, glyph, &area);
		nr_rect_f_union (&pgl->area, &pgl->area, &area);
	}

	/* terminator */
	pgl->glyphs[i].glyph = 0;
	pgl->glyphs[i].x = pgl->advance.x;
	pgl->glyphs[i].y = pgl->advance.y;

	return pgl;
}

NRPGL *
nr_pgl_free (NRPGL *pgl)
{
	nr_rasterfont_unref (pgl->rfont);

	return NULL;
}


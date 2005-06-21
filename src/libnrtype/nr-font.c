#define __NR_FONT_C__

/*
 * Typeface and script library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <string.h>

#include <libnr/nr-macros.h>
#include <libnr/nr-rect.h>
#include <libnr/nr-matrix.h>
#include "nr-font.h"

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
		((NRTypeFaceClass *) ((NRObject *) font->face)->klass)->font_free (font);
	}

	return NULL;
}

NRPath *
nr_font_glyph_outline_get (NRFont *font, unsigned int glyph, unsigned int ref)
{
	return ((NRTypeFaceClass *) ((NRObject *) font->face)->klass)->font_glyph_outline_get (font, glyph, ref);
}

void
nr_font_glyph_outline_unref (NRFont *font, unsigned int glyph)
{
	((NRTypeFaceClass *) ((NRObject *) font->face)->klass)->font_glyph_outline_unref (font, glyph);
}

NRPointF *
nr_font_glyph_advance_get (NRFont *font, unsigned int glyph, NRPointF *adv)
{
	return ((NRTypeFaceClass *) ((NRObject *) font->face)->klass)->font_glyph_advance_get (font, glyph, adv);
}

NRRectF *
nr_font_glyph_area_get (NRFont *font, unsigned int glyph, NRRectF *area)
{
	return ((NRTypeFaceClass *) ((NRObject *) font->face)->klass)->font_glyph_area_get (font, glyph, area);
}

NRRasterFont *
nr_rasterfont_new (NRFont *font, const NRMatrixF *transform)
{
	return ((NRTypeFaceClass *) ((NRObject *) font->face)->klass)->rasterfont_new (font, transform);
}

/* Generic implementation */

typedef struct _NRFontGeneric NRFontGeneric;

struct _NRFontGeneric {
	NRFont font;

	NRRasterFont *rfonts;

	NRPath **outlines;
};

NRFont *
nr_font_generic_new (NRTypeFace *tf, unsigned int metrics, NRMatrixF *transform)
{
	NRFontGeneric *fg;

	fg = nr_new (NRFontGeneric, 1);

	fg->font.refcount = 1;
	fg->font.next = NULL;
	fg->font.face = nr_typeface_ref (tf);
	fg->font.metrics = metrics;
	fg->font.size = (float) NR_MATRIX_DF_EXPANSION (transform);

	fg->rfonts = NULL;
	fg->outlines = NULL;

	return (NRFont *) fg;
}

void
nr_font_generic_free (NRFont *font)
{
	NRFontGeneric *fg;

	fg = (NRFontGeneric *) font;

	if (fg->outlines) {
		unsigned int i;
		for (i = 0; i < font->face->nglyphs; i++) {
			if (fg->outlines[i]) free (fg->outlines[i]);
		}
		nr_free (fg->outlines);
	}

	nr_typeface_unref (font->face);
	nr_free (font);
}

NRPath *
nr_font_generic_glyph_outline_get (NRFont *font, unsigned int glyph, unsigned int ref)
{
	NRFontGeneric *fg;

	fg = (NRFontGeneric *) font;

	if (!fg->outlines) {
		fg->outlines = nr_new (NRPath *, font->face->nglyphs);
		memset (fg->outlines, 0x0, font->face->nglyphs * sizeof (NRPath *));
	}

	if (!fg->outlines[glyph]) {
		NRPath *tfgol;
		tfgol = nr_typeface_glyph_outline_get (font->face, glyph, font->metrics, 0);
		if (tfgol) {
			NRMatrixF scale;
			nr_matrix_f_set_scale (&scale, font->size / 1000.0F, font->size / 1000.0F);
			fg->outlines[glyph] = nr_path_duplicate_transform (tfgol, &scale);
		}
	}

	return fg->outlines[glyph];
}

void
nr_font_generic_glyph_outline_unref (NRFont *font, unsigned int glyph)
{
	/* NOP by now */
}

NRPointF *
nr_font_generic_glyph_advance_get (NRFont *font, unsigned int glyph, NRPointF *adv)
{
	((NRTypeFaceClass *) ((NRObject *) font->face)->klass)->glyph_advance_get (font->face, glyph, font->metrics, adv);

	adv->x *= (font->size / 1000.0F);
	adv->y *= (font->size / 1000.0F);

	return adv;
}

NRRectF *
nr_font_generic_glyph_area_get (NRFont *font, unsigned int glyph, NRRectF *area)
{
	NRPath *path;

	path = nr_font_glyph_outline_get (font, glyph, 0);
	if (!path) return NULL;
	area->x0 = area->y0 = NR_HUGE_F;
	area->x1 = area->y1 = -NR_HUGE_F;
	nr_path_matrix_f_bbox_f_union (path, NULL, area, 0.25);

	return !nr_rect_f_test_empty (area) ? area : NULL;
}

NRRasterFont *
nr_font_generic_rasterfont_new (NRFont *font, const NRMatrixF *transform)
{
	NRFontGeneric *fg;
	NRRasterFont *rf;
	double mexp;

	fg = (NRFontGeneric *) font;

	mexp = NR_MATRIX_DF_EXPANSION (transform);
	rf = fg->rfonts;
	while (rf != NULL) {
		double fmexp;
		fmexp = NR_MATRIX_DF_EXPANSION (&rf->transform);
		if (NR_DF_TEST_CLOSE (mexp, fmexp, 0.001 * mexp)) {
			double rmexp;
			if (NR_DF_TEST_CLOSE (mexp, 0.0, 0.0001)) return nr_rasterfont_ref (rf);
			rmexp = mexp * 0.001;
			if (NR_DF_TEST_CLOSE (transform->c[0], rf->transform.c[0], rmexp) &&
			    NR_DF_TEST_CLOSE (transform->c[1], rf->transform.c[1], rmexp) &&
			    NR_DF_TEST_CLOSE (transform->c[2], rf->transform.c[2], rmexp) &&
			    NR_DF_TEST_CLOSE (transform->c[3], rf->transform.c[3], rmexp)) {
				return nr_rasterfont_ref (rf);
			}
		}
		rf = rf->next;
	}

	rf = nr_rasterfont_generic_new (font, transform);

	rf->next = fg->rfonts;
	fg->rfonts = rf;

	return rf;
}

void
nr_font_generic_rasterfont_free (NRRasterFont *rf)
{
	NRFontGeneric *fg;

	fg = (NRFontGeneric *) rf->font;

	if (fg->rfonts == rf) {
		fg->rfonts = rf->next;
	} else {
		NRRasterFont *ref;
		ref = fg->rfonts;
		while (ref->next != rf) ref = ref->next;
		ref->next = rf->next;
	}

	rf->next = NULL;

	nr_rasterfont_generic_free (rf);
}

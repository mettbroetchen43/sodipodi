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

#define noRFDEBUG

#include <string.h>

#include <libnr/nr-macros.h>
#include <libnr/nr-rect.h>
#include <libnr/nr-matrix.h>
#include <libnr/nr-pixops.h>

#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_vpath_bpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath.h>
#include <libart_lgpl/art_svp_wind.h>
#include <libart_lgpl/art_rect_svp.h>
#include <libart_lgpl/art_gray_svp.h>

#include "nr-rasterfont.h"

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
		rf->font->face->vmv->rasterfont_free (rf);
	}

	return NULL;
}

NRPointF *
nr_rasterfont_glyph_advance_get (NRRasterFont *rf, int glyph, NRPointF *adv)
{
	return rf->font->face->vmv->rasterfont_glyph_advance_get (rf, glyph, adv);
}

NRRectF *
nr_rasterfont_glyph_area_get (NRRasterFont *rf, int glyph, NRRectF *area)
{
	return rf->font->face->vmv->rasterfont_glyph_area_get (rf, glyph, area);
}

void
nr_rasterfont_glyph_mask_render (NRRasterFont *rf, int glyph, NRPixBlock *mask, float x, float y)
{
	rf->font->face->vmv->rasterfont_glyph_mask_render (rf, glyph, mask, x, y);
}

/* Generic implementation */

#define NRRF_PAGEBITS 6
#define NRRF_PAGE_SIZE (1 << NRRF_PAGEBITS)
#define NRRF_PAGE_MASK ((1 << NRRF_PAGEBITS) - 1)

#define NR_RASTERFONT_ADVANCE_FLAG (1 << 0)
#define NR_RASTERFONT_BBOX_FLAG (1 << 0)
#define NR_RASTERFONT_GMAP_FLAG (1 << 0)

#define NRRF_TINY_SIZE (sizeof (unsigned char *))
#define NRRF_MAX_GLYPH_DIMENSION 256
#define NRRF_MAX_GLYPH_SIZE 32 * 32

#define NRRF_COORD_INT_LOWER(i) ((i) >> 6)
#define NRRF_COORD_INT_UPPER(i) (((i) + 63) >> 6)
#define NRRF_COORD_INT_SIZE(i0,i1) (NRRF_COORD_INT_UPPER (i1) - NRRF_COORD_INT_LOWER (i0))
#define NRRF_COORD_TO_FLOAT(i) ((double) (i) / 64.0)
#define NRRF_COORD_FROM_FLOAT_LOWER(f) ((int) (f * 64.0))
#define NRRF_COORD_FROM_FLOAT_UPPER(f) ((int) (f * 64.0 + 63.999999))

enum {
	NRRF_GMAP_NONE,
	NRRF_GMAP_TINY,
	NRRF_GMAP_IMAGE,
	NRRF_GMAP_SVP
};

struct _NRRFGlyphSlot {
	unsigned int has_advance : 1;
	unsigned int has_bbox : 1;
	unsigned int has_gmap : 2;
	NRPointF advance;
	NRRectS bbox;
	union {
		unsigned char d[NRRF_TINY_SIZE];
		unsigned char *px;
		ArtSVP *svp;
	} gmap;
};

static NRRFGlyphSlot *nr_rasterfont_ensure_glyph_slot (NRRasterFont *rf, unsigned int glyph, unsigned int flags);

NRRasterFont *
nr_rasterfont_generic_new (NRFont *font, NRMatrixF *transform)
{
	NRRasterFont *rf;

	rf = nr_new (NRRasterFont, 1);

	rf->refcount = 1;
	rf->next = NULL;
	rf->font = nr_font_ref (font);
	rf->transform = *transform;
	/* fixme: How about subpixel positioning */
	rf->transform.c[4] = 0.0;
	rf->transform.c[5] = 0.0;
	rf->nglyphs = NR_FONT_NUM_GLYPHS (font);
	rf->pages = NULL;

	return rf;
}

void
nr_rasterfont_generic_free (NRRasterFont *rf)
{
	if (rf->pages) {
		int npages, p;
		npages = rf->nglyphs / NRRF_PAGE_SIZE;
		for (p = 0; p < npages; p++) {
			if (rf->pages[p]) {
				NRRFGlyphSlot *slots;
				int s;
				slots = rf->pages[p];
				for (s = 0; s < NRRF_PAGE_SIZE; s++) {
					if (slots[s].has_gmap == NRRF_GMAP_IMAGE) {
						nr_free (slots[s].gmap.px);
					} else if (slots[s].has_gmap == NRRF_GMAP_SVP) {
						art_svp_free (slots[s].gmap.svp);
					}
				}
				nr_free (rf->pages[p]);
			}
		}
		nr_free (rf->pages);
	}
	nr_font_unref (rf->font);
	nr_free (rf);
}

NRPointF *
nr_rasterfont_generic_glyph_advance_get (NRRasterFont *rf, unsigned int glyph, NRPointF *adv)
{
	NRPointF a;

	if (nr_font_glyph_advance_get (rf->font, glyph, &a)) {
		adv->x = NR_MATRIX_DF_TRANSFORM_X (&rf->transform, a.x, a.y);
		adv->y = NR_MATRIX_DF_TRANSFORM_Y (&rf->transform, a.x, a.y);
		return adv;
	}

	return NULL;
}

NRRectF *
nr_rasterfont_generic_glyph_area_get (NRRasterFont *rf, unsigned int glyph, NRRectF *area)
{
	NRRFGlyphSlot *slot;

	glyph = CLAMP (glyph, 0, rf->nglyphs);

	slot = nr_rasterfont_ensure_glyph_slot (rf, glyph, NR_RASTERFONT_BBOX_FLAG);

	area->x0 = NRRF_COORD_TO_FLOAT (slot->bbox.x0);
	area->y0 = NRRF_COORD_TO_FLOAT (slot->bbox.y0);
	area->x1 = NRRF_COORD_TO_FLOAT (slot->bbox.x1);
	area->y1 = NRRF_COORD_TO_FLOAT (slot->bbox.y1);

	return area;
}

void
nr_rasterfont_generic_glyph_mask_render (NRRasterFont *rf, unsigned int glyph, NRPixBlock *m, float x, float y)
{
	NRRFGlyphSlot *slot;
	NRRectS area;
	int sx, sy;
	unsigned char *spx;
	int srs;
	NRPixBlock spb;

	glyph = CLAMP (glyph, 0, rf->nglyphs);

	slot = nr_rasterfont_ensure_glyph_slot (rf, glyph, NR_RASTERFONT_BBOX_FLAG | NR_RASTERFONT_GMAP_FLAG);

	if (nr_rect_s_test_empty (&slot->bbox)) return;

	sx = (int) floor (x + 0.5);
	sy = (int) floor (y + 0.5);

	area.x0 = NRRF_COORD_INT_LOWER (slot->bbox.x0) + sx;
	area.y0 = NRRF_COORD_INT_LOWER (slot->bbox.y0) + sy;
	area.x1 = NRRF_COORD_INT_UPPER (slot->bbox.x1) + sx;
	area.y1 = NRRF_COORD_INT_UPPER (slot->bbox.y1) + sy;

	spb.empty = 1;
	if (slot->has_gmap == NRRF_GMAP_IMAGE) {
		spx = slot->gmap.px;
		srs = NRRF_COORD_INT_SIZE (slot->bbox.x0, slot->bbox.x1);
	} else if (slot->has_gmap == NRRF_GMAP_TINY) {
		spx = slot->gmap.d;
		srs = NRRF_COORD_INT_SIZE (slot->bbox.x0, slot->bbox.x1);
	} else {
		nr_pixblock_setup_fast (&spb, NR_PIXBLOCK_MODE_A8, area.x0, area.y0, area.x1, area.y1, 0);
		art_gray_svp_aa (slot->gmap.svp, area.x0 - sx, area.y0 - sy, area.x1 - sx, area.y1 - sy,
				 NR_PIXBLOCK_PX (&spb), spb.rs);
		spb.empty = 0;
		spx = NR_PIXBLOCK_PX (&spb);
		srs = spb.rs;
	}

	if (nr_rect_s_test_intersect (&area, &m->area)) {
		NRRectS clip;
		int x, y;
		nr_rect_s_intersect (&clip, &area, &m->area);
		for (y = clip.y0; y < clip.y1; y++) {
			unsigned char *d, *s;
			s = spx + (y - area.y0) * srs + (clip.x0 - area.x0);
			d = NR_PIXBLOCK_PX (m) + (y - m->area.y0) * m->rs + (clip.x0 - m->area.x0);
			for (x = clip.x0; x < clip.x1; x++) {
				*d = (NR_A7 (*s, *d) + 127) / 255;
				s += 1;
				d += 1;
			}
		}
	}

	if (!spb.empty) nr_pixblock_release (&spb);
}

static NRRFGlyphSlot *
nr_rasterfont_ensure_glyph_slot (NRRasterFont *rf, unsigned int glyph, unsigned int flags)
{
	NRRFGlyphSlot *slot;
	unsigned int page, code;

	page = glyph / NRRF_PAGE_SIZE;
	code = glyph % NRRF_PAGE_SIZE;

	if (!rf->pages) {
		rf->pages = nr_new (NRRFGlyphSlot *, rf->nglyphs / NRRF_PAGE_SIZE + 1);
		memset (rf->pages, 0x0, (rf->nglyphs / NRRF_PAGE_SIZE + 1) * sizeof (NRRFGlyphSlot *));
	}

	if (!rf->pages[page]) {
		rf->pages[page] = nr_new (NRRFGlyphSlot, NRRF_PAGE_SIZE);
		memset (rf->pages[page], 0x0, NRRF_PAGE_SIZE * sizeof (NRRFGlyphSlot));
	}

	slot = rf->pages[page] + code;

	if ((flags & NR_RASTERFONT_ADVANCE_FLAG) && !slot->has_advance) {
		NRPointF a;
		if (nr_font_glyph_advance_get (rf->font, glyph, &a)) {
			slot->advance.x = NR_MATRIX_DF_TRANSFORM_X (&rf->transform, a.x, a.y);
			slot->advance.y = NR_MATRIX_DF_TRANSFORM_Y (&rf->transform, a.x, a.y);
		}
		slot->has_advance = 1;
	}

	if (((flags & NR_RASTERFONT_BBOX_FLAG) && !slot->has_bbox) ||
	    ((flags & NR_RASTERFONT_GMAP_FLAG) && !slot->has_gmap)) {
		NRBPath gbp;
		if (nr_font_glyph_outline_get (rf->font, glyph, &gbp, 0)) {

		    if (gbp.path && (gbp.path->code == ART_MOVETO)) {
			ArtBpath *abp;
			ArtVpath *vp, *pvp;
			ArtSVP *svpa, *svpb;
			ArtDRect bbox;
			double a[6];
			int w, h;
			a[0] = rf->transform.c[0];
			a[1] = rf->transform.c[1];
			a[2] = rf->transform.c[2];
			a[3] = rf->transform.c[3];
			a[4] = 0.0;
			a[5] = 0.0;
			abp = art_bpath_affine_transform (gbp.path, a);
			vp = art_bez_path_to_vec (abp, 0.25);
			art_free (abp);
			pvp = art_vpath_perturb (vp);
			art_free (vp);
			svpa = art_svp_from_vpath (pvp);
			art_free (pvp);
			svpb = art_svp_uncross (svpa);
			art_svp_free (svpa);
			svpa = art_svp_rewind_uncrossed (svpb, ART_WIND_RULE_ODDEVEN);
			art_svp_free (svpb);
			art_drect_svp (&bbox, svpa);
			slot->bbox.x0 = NRRF_COORD_FROM_FLOAT_LOWER (bbox.x0);
			slot->bbox.y0 = NRRF_COORD_FROM_FLOAT_LOWER (bbox.y0);
			slot->bbox.x1 = NRRF_COORD_FROM_FLOAT_UPPER (bbox.x1);
			slot->bbox.y1 = NRRF_COORD_FROM_FLOAT_UPPER (bbox.y1);
			w = NRRF_COORD_INT_SIZE (slot->bbox.x0, slot->bbox.x1);
			h = NRRF_COORD_INT_SIZE (slot->bbox.y0, slot->bbox.y1);
#if 0
			if ((w * h) > NRRF_TINY_SIZE) {
				art_gray_svp_aa (svpa,
						 NRRF_COORD_INT_LOWER (slot->bbox.x0),
						 NRRF_COORD_INT_LOWER (slot->bbox.y0),
						 NRRF_COORD_INT_UPPER (slot->bbox.x1),
						 NRRF_COORD_INT_UPPER (slot->bbox.x1),
						 slot->gmap.d,
						 w);
				art_svp_free (svpa);
				slot->has_gmap = NRRF_GMAP_TINY;
			}
#endif
			if ((w >= NRRF_MAX_GLYPH_DIMENSION) ||
			    (h >= NRRF_MAX_GLYPH_DIMENSION) ||
			    ((w * h) > NRRF_MAX_GLYPH_SIZE)) {
				slot->gmap.svp = svpa;
				slot->has_gmap = NRRF_GMAP_SVP;
			} else {
				slot->gmap.px = nr_new (unsigned char, w * h);
				art_gray_svp_aa (svpa,
						 NRRF_COORD_INT_LOWER (slot->bbox.x0),
						 NRRF_COORD_INT_LOWER (slot->bbox.y0),
						 NRRF_COORD_INT_UPPER (slot->bbox.x1),
						 NRRF_COORD_INT_UPPER (slot->bbox.y1),
						 slot->gmap.px,
						 w);
				art_svp_free (svpa);
				slot->has_gmap = NRRF_GMAP_IMAGE;
			}}
		} else {
			slot->bbox.x0 = 0;
			slot->bbox.y0 = 0;
			slot->bbox.x0 = 0;
			slot->bbox.x0 = 0;
			slot->gmap.d[0] = 0;
			slot->has_gmap = NRRF_GMAP_TINY;
		}
		slot->has_bbox = 1;
	}

	return slot;
}


#ifndef __NR_TYPE_GNOME_H__
#define __NR_TYPE_GNOME_H__

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

#include <libnrtype/nr-rasterfont.h>
#include <libgnomeprint/gnome-font.h>

typedef struct _NRTypeFaceVMV NRTypeFaceVMV;

struct _NRTypeFaceVMV {
	void (* free) (NRTypeFace *tf);
	const unsigned char *(* get_attribute) (NRTypeFace *tf);
	NRBPath *(* glyph_outline_get) (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRBPath *d, unsigned int ref);
	void (* glyph_outline_unref) (NRTypeFace *tf, unsigned int glyph);
	unsigned int (* lookup) (NRTypeFace *tf, unsigned int rule, unsigned int glyph);

	NRFont *(* font_new) (NRTypeFace *tf, unsigned int metrics, NRMatrixF *transform);
	void (* font_free) (NRFont *font);
	NRBPath *(* font_glyph_outline_get) (NRFont *font, unsigned int glyph, NRBPath *d, unsigned int ref);
	void (* font_glyph_outline_unref) (NRFont *font, unsigned int glyph);
	NRPointF (* font_glyph_advance_get) (NRFont *font, unsigned int glyph, NRPointF *adv);

	NRRasterFont *(* rasterfont_new) (NRFont *font, NRMatrixF *transform);
	void (* rasterfont_free) (NRRasterFont *rfont);
	NRPointF (* rasterfont_glyph_advance_get) (NRRasterFont *rfont, unsigned int glyph, NRPointF *adv);
	NRRectF (* rasterfont_glyph_area_get) (NRRasterFont *rfont, unsigned int glyph, NRPointF *adv);
	void (* rasterfont_render_glyph_mask) (NRRasterFont *rfont, unsigned int glyph, NRPixBlock *m, float x, float y);
};

struct _NRTypeFace {
	unsigned int refcount;

	NRTypeFaceVMV *vmv;

	NRTypeFace *prev, *next;
	GnomeFontFace *face;
	unsigned int nglyphs;
	NRFont *fonts;
};

struct _NRFont {
	NRFont *prev, *next;
	unsigned int refcount;
	NRTypeFace *face;
	unsigned int metrics : 2;
	float size;
	GnomeFont *font;
	unsigned int nglyphs;

	NRRasterFont *rfonts;
};

#endif

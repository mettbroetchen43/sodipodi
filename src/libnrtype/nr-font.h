#ifndef __NR_FONT_H__
#define __NR_FONT_H__

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

typedef struct _NRFont NRFont;

#include <libnrtype/nr-typeface.h>

NRFont *nr_font_new_default (NRTypeFace *tf, float size);

NRFont *nr_font_ref (NRFont *font);
NRFont *nr_font_unref (NRFont *font);

NRBPath *nr_font_get_glyph_outline (NRFont *font, int glyph, unsigned int metric, NRBPath *d, unsigned int ref);
void nr_font_unref_glyph_outline (NRFont *font, int glyph);

NRPointF *nr_font_get_glyph_advance (NRFont *font, int glyph, unsigned int metric, NRPointF *adv);

NRTypeFace *nr_font_get_typeface (NRFont *font);
float nr_font_get_size (NRFont *font);

#endif

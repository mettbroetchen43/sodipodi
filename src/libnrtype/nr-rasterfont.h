#ifndef __NR_RASTERFONT_H__
#define __NR_RASTERFONT_H__

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

typedef struct _NRRasterFont NRRasterFont;

#include <libnrtype/nr-font.h>

NRRasterFont *nr_rasterfont_new (NRFont *font, NRMatrixF *transform);

NRRasterFont *nr_rasterfont_ref (NRRasterFont *rf);
NRRasterFont *nr_rasterfont_unref (NRRasterFont *rf);

NRPointF *nr_rasterfont_get_glyph_advance (NRRasterFont *rf, int glyph, unsigned int metric, NRPointF *adv);

NRFont *nr_rasterfont_get_font (NRRasterFont *rf);
NRTypeFace *nr_rasterfont_get_typeface (NRRasterFont *rf);

#endif

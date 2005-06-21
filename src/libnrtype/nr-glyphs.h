#ifndef __NR_GLYPHS_H__
#define __NR_GLYPHS_H__

/*
 * Typeface and script library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

typedef struct _NRPosGlyph NRPosGlyph;
typedef struct _NRPGL NRPGL;

#include <libnr/nr-types.h>

#include <libnrtype/nr-rasterfont.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _NRPosGlyph {
	NRULong glyph;
	float x, y;
};

struct _NRPGL {
	NRRasterFont *rfont;
	NRPointF origin;
	NRRectF area;
	NRPointF advance;
	unsigned int length;
	struct _NRPosGlyph glyphs[1];
};

/* Transform incorporates initial position as well */
NRPGL *nr_pgl_new_from_string (NRUShort *text, unsigned int length, NRFont *font, NRMatrixF *transform);
NRPGL *nr_pgl_free (NRPGL *pgl);

void nr_pgl_set_origin (NRPGL *pgl, float x, float y);

#ifdef __cplusplus
};
#endif

#endif


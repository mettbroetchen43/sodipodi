#ifndef __NR_TYPEFACE_H__
#define __NR_TYPEFACE_H__

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

typedef struct _NRTypeFace NRTypeFace;
typedef struct _NRTypeFaceDef NRTypeFaceDef;
typedef struct _NRTypeFaceVMV NRTypeFaceVMV;
typedef struct _NRTypePosDef NRTypePosDef;

#include <libnr/nr-types.h>
#include <libnr/nr-path.h>
#include <libnrtype/nr-font.h>
#include <libnrtype/nr-rasterfont.h>

enum {
	NR_TYPEFACE_METRICS_DEFAULT,
	NR_TYPEFACE_METRICS_HORIZONTAL,
	NR_TYPEFACE_METRICS_VERTICAL
};

enum {
	NR_TYPEFACE_LOOKUP_RULE_DEFAULT
};

struct _NRTypeFaceDef {
	NRTypeFaceDef *next;
	NRTypeFaceVMV *vmv;
	NRTypePosDef *pdef;
	unsigned int idx;
	unsigned char *name;
	unsigned char *family;
	NRTypeFace *typeface;
};

struct _NRTypeFaceVMV {
	NRTypeFace *(* new) (NRTypeFaceDef *def);

	void (* free) (NRTypeFace *tf);
	unsigned int (* attribute_get) (NRTypeFace *tf, const unsigned char *key, unsigned char *str, unsigned int size);
	NRBPath *(* glyph_outline_get) (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRBPath *path, unsigned int ref);
	void (* glyph_outline_unref) (NRTypeFace *tf, unsigned int glyph, unsigned int metrics);
	NRPointF *(* glyph_advance_get) (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv);
	unsigned int (* lookup) (NRTypeFace *tf, unsigned int rule, unsigned int glyph);
	NRFont *(* font_new) (NRTypeFace *tf, unsigned int metrics, NRMatrixF *transform);

	void (* font_free) (NRFont *font);
	NRBPath *(* font_glyph_outline_get) (NRFont *font, unsigned int glyph, NRBPath *path, unsigned int ref);
	void (* font_glyph_outline_unref) (NRFont *font, unsigned int glyph);
	NRPointF *(* font_glyph_advance_get) (NRFont *font, unsigned int glyph, NRPointF *adv);
	NRRectF *(* font_glyph_area_get) (NRFont *font, unsigned int glyph, NRRectF *area);
	NRRasterFont *(* rasterfont_new) (NRFont *font, NRMatrixF *transform);

	void (* rasterfont_free) (NRRasterFont *rfont);
	NRPointF *(* rasterfont_glyph_advance_get) (NRRasterFont *rfont, unsigned int glyph, NRPointF *adv);
	NRRectF *(* rasterfont_glyph_area_get) (NRRasterFont *rfont, unsigned int glyph, NRRectF *area);
	void (* rasterfont_glyph_mask_render) (NRRasterFont *rfont, unsigned int glyph, NRPixBlock *m, float x, float y);
};

struct _NRTypeFace {
	NRTypeFaceVMV *vmv;
	unsigned int refcount;
	NRTypeFaceDef *def;
	unsigned int nglyphs;
};

NRTypeFace *nr_typeface_ref (NRTypeFace *tf);
NRTypeFace *nr_typeface_unref (NRTypeFace *tf);

unsigned int nr_typeface_name_get (NRTypeFace *tf, unsigned char *str, unsigned int size);
unsigned int nr_typeface_family_name_get (NRTypeFace *tf, unsigned char *str, unsigned int size);
unsigned int nr_typeface_attribute_get (NRTypeFace *tf, const unsigned char *key, unsigned char *str, unsigned int size);

NRBPath *nr_typeface_glyph_outline_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRBPath *d, unsigned int ref);
void nr_typeface_glyph_outline_unref (NRTypeFace *tf, unsigned int glyph, unsigned int metrics);
NRPointF *nr_typeface_glyph_advance_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv);

unsigned int nr_typeface_lookup_default (NRTypeFace *tf, unsigned int unival);

NRFont *nr_font_new_default (NRTypeFace *tf, unsigned int metrics, float size);

void nr_type_empty_build_def (NRTypeFaceDef *def, const unsigned char *name, const unsigned char *family);

#endif

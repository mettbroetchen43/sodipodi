#ifndef __NR_TYPEFACE_H__
#define __NR_TYPEFACE_H__

/*
 * Typeface and script library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#define NR_TYPE_TYPEFACE (nr_typeface_get_type ())
#define NR_TYPEFACE(o) (NR_CHECK_INSTANCE_CAST ((o), NR_TYPE_TYPEFACE, NRTypeFace))
#define NR_IS_TYPEFACE(o) (NR_CHECK_INSTANCE_TYPE ((o), NR_TYPE_TYPEFACE))

typedef struct _NRTypeFace NRTypeFace;
typedef struct _NRTypeFaceClass NRTypeFaceClass;

typedef struct _NRTypeFaceDef NRTypeFaceDef;
typedef struct _NRTypePosDef NRTypePosDef;

#include <libnr/nr-types.h>
#include <libnr/nr-path.h>
#include <libnr/nr-object.h>
#include <libnrtype/nr-font.h>
#include <libnrtype/nr-rasterfont.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	NR_TYPEFACE_METRICS_DEFAULT,
	NR_TYPEFACE_METRICS_HORIZONTAL,
	NR_TYPEFACE_METRICS_VERTICAL
};

enum {
	NR_TYPEFACE_LOOKUP_RULE_DEFAULT
};

/* We want to keep 1:1 mapping with CSS stuff */

enum {
	/* NR_TYPEFACE_SLANT_UNKNOWN, */
	NR_TYPEFACE_SLANT_ROMAN,
	NR_TYPEFACE_SLANT_ITALIC,
	NR_TYPEFACE_SLANT_OBLIQUE
};

enum {
	/* NR_TYPEFACE_WEIGHT_UNKNOWN, */
	NR_TYPEFACE_WEIGHT_THIN,
	NR_TYPEFACE_WEIGHT_ULTRALIGHT,
	NR_TYPEFACE_WEIGHT_LIGHT,
	NR_TYPEFACE_WEIGHT_NORMAL,
	NR_TYPEFACE_WEIGHT_MEDIUM,
	NR_TYPEFACE_WEIGHT_DEMIBOLD,
	NR_TYPEFACE_WEIGHT_BOLD,
	NR_TYPEFACE_WEIGHT_ULTRABOLD,
	NR_TYPEFACE_WEIGHT_BLACK
};

struct _NRTypeFaceDef {
	NRTypeFaceDef *next;
	unsigned int type;
	NRTypePosDef *pdef;
	unsigned int idx;
	unsigned char *name;
	unsigned char *family;
	/* Preparsed properties */
	unsigned int weight : 4;
	unsigned int slant : 2;
	/* Existing typeface if present */
	NRTypeFace *typeface;
};

struct _NRTypeFaceClass {
	NRObjectClass parent_class;

	void (* setup) (NRTypeFace *tface, NRTypeFaceDef *def);

	unsigned int (* attribute_get) (NRTypeFace *tf, const unsigned char *key, unsigned char *str, unsigned int size);
	NRPath *(* glyph_outline_get) (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, unsigned int ref);
	void (* glyph_outline_unref) (NRTypeFace *tf, unsigned int glyph, unsigned int metrics);
	NRPointF *(* glyph_advance_get) (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv);
	unsigned int (* lookup) (NRTypeFace *tf, unsigned int rule, unsigned int glyph);
	NRFont *(* font_new) (NRTypeFace *tf, unsigned int metrics, NRMatrixF *transform);

	void (* font_free) (NRFont *font);
	NRPath *(* font_glyph_outline_get) (NRFont *font, unsigned int glyph, unsigned int ref);
	void (* font_glyph_outline_unref) (NRFont *font, unsigned int glyph);
	NRPointF *(* font_glyph_advance_get) (NRFont *font, unsigned int glyph, NRPointF *adv);
	NRRectF *(* font_glyph_area_get) (NRFont *font, unsigned int glyph, NRRectF *area);
	NRRasterFont *(* rasterfont_new) (NRFont *font, const NRMatrixF *transform);

	void (* rasterfont_free) (NRRasterFont *rfont);
	NRPointF *(* rasterfont_glyph_advance_get) (NRRasterFont *rfont, unsigned int glyph, NRPointF *adv);
	NRRectF *(* rasterfont_glyph_area_get) (NRRasterFont *rfont, unsigned int glyph, NRRectF *area);
	void (* rasterfont_glyph_mask_render) (NRRasterFont *rfont, unsigned int glyph, NRPixBlock *m, float x, float y);
};

struct _NRTypeFace {
	NRObject object;

	NRTypeFaceDef *def;
	unsigned int nglyphs;
};

typedef struct _NRFamilyDef NRFamilyDef;

struct _NRFamilyDef {
	NRFamilyDef *next;
	unsigned char *name;
	NRTypeFaceDef *faces;
};

unsigned int nr_typeface_get_type (void);

NRTypeFace *nr_typeface_new (NRTypeFaceDef *def);

#define nr_typeface_ref(t) (NRTypeFace *) nr_object_ref ((NRObject *) (t))
#define nr_typeface_unref(t) (NRTypeFace *) nr_object_unref ((NRObject *) (t))

unsigned int nr_typeface_name_get (NRTypeFace *tf, unsigned char *str, unsigned int size);
unsigned int nr_typeface_family_name_get (NRTypeFace *tf, unsigned char *str, unsigned int size);
unsigned int nr_typeface_attribute_get (NRTypeFace *tf, const unsigned char *key, unsigned char *str, unsigned int size);

NRPath *nr_typeface_glyph_outline_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, unsigned int ref);
void nr_typeface_glyph_outline_unref (NRTypeFace *tf, unsigned int glyph, unsigned int metrics);
NRPointF *nr_typeface_glyph_advance_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv);

unsigned int nr_typeface_lookup_default (NRTypeFace *tf, unsigned int unival);

NRFont *nr_font_new_default (NRTypeFace *tf, unsigned int metrics, float size);

void nr_type_empty_build_def (NRTypeFaceDef *def, const unsigned char *name, const unsigned char *family);

/* Preparsed properties */
#define nr_type_get_weight (t) (((NRTypeFace *) (t))->def->weight)
#define nr_type_get_slant (t) (((NRTypeFace *) (t))->def->slant)

/* Helpers for preparsed stuff */
unsigned int nr_type_string_to_weight (const unsigned char *wstr);
const unsigned char *nr_type_weight_to_string (unsigned int weight);

unsigned int nr_type_string_to_slant (const unsigned char *sstr);
const unsigned char *nr_type_slant_to_string (unsigned int slant);

const unsigned char *nr_type_make_style_name_dumb (const NRTypeFaceDef *def);

#ifdef __cplusplus
};
#endif

#endif

#define __NR_TYPEFACE_C__

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
#include <libnr/nr-matrix.h>
#include "nr-typeface.h"
#include "nr-type-directory.h"

static void nr_typeface_class_init (NRTypeFaceClass *klass);
static void nr_typeface_init (NRTypeFace *tface);
static void nr_typeface_finalize (NRObject *object);

static void nr_typeface_setup (NRTypeFace *tface, NRTypeFaceDef *def);

static NRObjectClass *parent_class;

unsigned int
nr_typeface_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		type = nr_object_register_type (NR_TYPE_OBJECT,
						"NRTypeFace",
						sizeof (NRTypeFaceClass),
						sizeof (NRTypeFace),
						(void (*) (NRObjectClass *)) nr_typeface_class_init,
						(void (*) (NRObject *)) nr_typeface_init);
	}
	return type;
}

static void
nr_typeface_class_init (NRTypeFaceClass *klass)
{
	NRObjectClass *object_class;

	object_class = (NRObjectClass *) klass;

	parent_class = ((NRObjectClass *) klass)->parent;

	object_class->finalize = nr_typeface_finalize;

	klass->setup = nr_typeface_setup;

	klass->font_glyph_outline_get = nr_font_generic_glyph_outline_get;
	klass->font_glyph_outline_unref = nr_font_generic_glyph_outline_unref;
	klass->font_glyph_advance_get = nr_font_generic_glyph_advance_get;
	klass->font_glyph_area_get = nr_font_generic_glyph_area_get;

	klass->rasterfont_new = nr_font_generic_rasterfont_new;
	klass->rasterfont_free = nr_font_generic_rasterfont_free;

	klass->rasterfont_glyph_advance_get = nr_rasterfont_generic_glyph_advance_get;
	klass->rasterfont_glyph_area_get = nr_rasterfont_generic_glyph_area_get;
	klass->rasterfont_glyph_mask_render = nr_rasterfont_generic_glyph_mask_render;
}

static void
nr_typeface_init (NRTypeFace *tface)
{
}

static void
nr_typeface_finalize (NRObject *object)
{
	NRTypeFace *tface;

	tface = (NRTypeFace *) object;

	nr_type_directory_forget_face (tface);

	((NRObjectClass *) (parent_class))->finalize (object);
}

static void
nr_typeface_setup (NRTypeFace *tface, NRTypeFaceDef *def)
{
	tface->def = def;
}

NRTypeFace *
nr_typeface_new (NRTypeFaceDef *def)
{
	NRTypeFace *tface;

	tface = (NRTypeFace *) nr_object_new (def->type);

	((NRTypeFaceClass *) ((NRObject *) tface)->klass)->setup (tface, def);

	return tface;
}

unsigned int
nr_typeface_name_get (NRTypeFace *tf, unsigned char *str, unsigned int size)
{
	return nr_typeface_attribute_get (tf, "name", str, size);
}

unsigned int
nr_typeface_family_name_get (NRTypeFace *tf, unsigned char *str, unsigned int size)
{
	return nr_typeface_attribute_get (tf, "family", str, size);
}

unsigned int
nr_typeface_attribute_get (NRTypeFace *tf, const unsigned char *key, unsigned char *str, unsigned int size)
{
	return ((NRTypeFaceClass *) ((NRObject *) tf)->klass)->attribute_get (tf, key, str, size);
}

NRBPath *
nr_typeface_glyph_outline_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRBPath *d, unsigned int ref)
{
	return ((NRTypeFaceClass *) ((NRObject *) tf)->klass)->glyph_outline_get (tf, glyph, metrics, d, ref);
}

void
nr_typeface_glyph_outline_unref (NRTypeFace *tf, unsigned int glyph, unsigned int metrics)
{
	((NRTypeFaceClass *) ((NRObject *) tf)->klass)->glyph_outline_unref (tf, glyph, metrics);
}

NRPointF *
nr_typeface_glyph_advance_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv)
{
	return ((NRTypeFaceClass *) ((NRObject *) tf)->klass)->glyph_advance_get (tf, glyph, metrics, adv);
}

unsigned int
nr_typeface_lookup_default (NRTypeFace *tf, unsigned int unival)
{
	return ((NRTypeFaceClass *) ((NRObject *) tf)->klass)->lookup (tf, NR_TYPEFACE_LOOKUP_RULE_DEFAULT, unival);
}

NRFont *
nr_font_new_default (NRTypeFace *tf, unsigned int metrics, float size)
{
	NRMatrixF scale;

	nr_matrix_f_set_scale (&scale, size, size);

	return ((NRTypeFaceClass *) ((NRObject *) tf)->klass)->font_new (tf, metrics, &scale);
}

/* NRTypeFaceEmpty */

#define NR_TYPE_TYPEFACE_EMPTY (nr_typeface_empty_get_type ())

typedef struct _NRTypeFaceEmpty NRTypeFaceEmpty;
typedef struct _NRTypeFaceEmptyClass NRTypeFaceEmptyClass;

struct _NRTypeFaceEmpty {
	NRTypeFace typeface;
};

struct _NRTypeFaceEmptyClass {
	NRTypeFaceClass typeface_class;
};

static unsigned int nr_typeface_empty_get_type (void);

static void nr_typeface_empty_class_init (NRTypeFaceEmptyClass *klass);
static void nr_typeface_empty_init (NRTypeFaceEmpty *tfe);
static void nr_typeface_empty_finalize (NRObject *object);

static unsigned int nr_typeface_empty_attribute_get (NRTypeFace *tf, const unsigned char *key, unsigned char *str, unsigned int size);
static NRBPath *nr_typeface_empty_glyph_outline_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRBPath *d, unsigned int ref);
static void nr_typeface_empty_glyph_outline_unref (NRTypeFace *tf, unsigned int glyph, unsigned int metrics);
static NRPointF *nr_typeface_empty_glyph_advance_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv);
static unsigned int nr_typeface_empty_lookup (NRTypeFace *tf, unsigned int rule, unsigned int unival);

static NRFont *nr_typeface_empty_font_new (NRTypeFace *tf, unsigned int metrics, NRMatrixF *transform);
static void nr_typeface_empty_font_free (NRFont *font);

static NRTypeFaceClass *empty_parent_class;

static unsigned int
nr_typeface_empty_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		type = nr_object_register_type (NR_TYPE_TYPEFACE,
						"NRTypeFaceEmpty",
						sizeof (NRTypeFaceEmptyClass),
						sizeof (NRTypeFaceEmpty),
						(void (*) (NRObjectClass *)) nr_typeface_empty_class_init,
						(void (*) (NRObject *)) nr_typeface_empty_init);
	}
	return type;
}

static void
nr_typeface_empty_class_init (NRTypeFaceEmptyClass *klass)
{
	NRObjectClass *object_class;
	NRTypeFaceClass *tface_class;

	object_class = (NRObjectClass *) klass;
	tface_class = (NRTypeFaceClass *) klass;

	empty_parent_class = (NRTypeFaceClass *) (((NRObjectClass *) klass)->parent);

	object_class->finalize = nr_typeface_empty_finalize;

	tface_class->attribute_get = nr_typeface_empty_attribute_get;
	tface_class->glyph_outline_get = nr_typeface_empty_glyph_outline_get;
	tface_class->glyph_outline_unref = nr_typeface_empty_glyph_outline_unref;
	tface_class->glyph_advance_get = nr_typeface_empty_glyph_advance_get;
	tface_class->lookup = nr_typeface_empty_lookup;

	tface_class->font_new = nr_typeface_empty_font_new;
	tface_class->font_free = nr_typeface_empty_font_free;
}

static void
nr_typeface_empty_init (NRTypeFaceEmpty *tfe)
{
	NRTypeFace *tface;

	tface = (NRTypeFace *) tfe;

	tface->nglyphs = 1;
}

static void
nr_typeface_empty_finalize (NRObject *object)
{
	NRTypeFace *tface;

	tface = (NRTypeFace *) object;

	((NRObjectClass *) (parent_class))->finalize (object);
}

static NRFont *empty_fonts = NULL;

static unsigned int
nr_typeface_empty_attribute_get (NRTypeFace *tf, const unsigned char *key, unsigned char *str, unsigned int size)
{
	const unsigned char *val;
	int len;

	if (!strcmp (key, "name")) {
		val = tf->def->name;
	} else if (!strcmp (key, "family")) {
		val = tf->def->family;
	} else if (!strcmp (key, "weight")) {
		val = "normal";
	} else if (!strcmp (key, "style")) {
		val = "normal";
	} else {
		val = "";
	}

	len = MIN (size - 1, strlen (val));
	if (len > 0) {
		memcpy (str, val, len);
	}
	if (size > 0) {
		str[len] = '\0';
	}

	return strlen (val);
}

static NRBPath *
nr_typeface_empty_glyph_outline_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRBPath *d, unsigned int ref)
{
	static const ArtBpath gol[] = {
		{ART_MOVETO, 0, 0, 0, 0, 100.0, 100.0},
		{ART_LINETO, 0, 0, 0, 0, 100.0, 900.0},
		{ART_LINETO, 0, 0, 0, 0, 900.0, 900.0},
		{ART_LINETO, 0, 0, 0, 0, 900.0, 100.0},
		{ART_LINETO, 0, 0, 0, 0, 100.0, 100.0},
		{ART_MOVETO, 0, 0, 0, 0, 150.0, 150.0},
		{ART_LINETO, 0, 0, 0, 0, 850.0, 150.0},
		{ART_LINETO, 0, 0, 0, 0, 850.0, 850.0},
		{ART_LINETO, 0, 0, 0, 0, 150.0, 850.0},
		{ART_LINETO, 0, 0, 0, 0, 150.0, 150.0},
		{ART_END, 0, 0, 0, 0, 0, 0}
	};

	d->path = (ArtBpath *) gol;

	return d;
}

static void
nr_typeface_empty_glyph_outline_unref (NRTypeFace *tf, unsigned int glyph, unsigned int metrics)
{
}

static NRPointF *
nr_typeface_empty_glyph_advance_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv)
{
	if (metrics == NR_TYPEFACE_METRICS_VERTICAL) {
		adv->x = 0.0;
		adv->y = -1000.0;
	} else {
		adv->x = 1000.0;
		adv->y = 0.0;
	}

	return adv;
}

static unsigned int
nr_typeface_empty_lookup (NRTypeFace *tf, unsigned int rule, unsigned int unival)
{
	return 0;
}

static NRFont *
nr_typeface_empty_font_new (NRTypeFace *tf, unsigned int metrics, NRMatrixF *transform)
{
	NRFont *font;
	float size;

	size = (float) NR_MATRIX_DF_EXPANSION (transform);

	font = empty_fonts;
	while (font != NULL) {
		if (NR_DF_TEST_CLOSE (size, font->size, 0.001 * size) && (font->metrics == metrics)) {
			return nr_font_ref (font);
		}
		font = font->next;
	}
	
	font = nr_font_generic_new (tf, metrics, transform);

	font->next = empty_fonts;
	empty_fonts = font;

	return font;
}

static void
nr_typeface_empty_font_free (NRFont *font)
{
	if (empty_fonts == font) {
		empty_fonts = font->next;
	} else {
		NRFont *ref;
		ref = empty_fonts;
		while (ref->next != font) ref = ref->next;
		ref->next = font->next;
	}

	font->next = NULL;

	nr_font_generic_free (font);
}

void
nr_type_empty_build_def (NRTypeFaceDef *def, const unsigned char *name, const unsigned char *family)
{
	def->type = NR_TYPE_TYPEFACE_EMPTY;
	def->name = strdup (name);
	def->family = strdup (family);
	def->typeface = NULL;
        def->weight = NR_TYPEFACE_WEIGHT_NORMAL;
        def->slant = NR_TYPEFACE_SLANT_ROMAN;
}

NRTypeFaceWeight nr_type_get_weight (NRTypeFace *tf)
{
	if (!(tf && tf->def))
		return NR_TYPEFACE_WEIGHT_UNKNOWN;

        return tf->def->weight;
}

NRTypeFaceSlant nr_type_get_slant (NRTypeFace *tf)
{
	if (!(tf && tf->def))
		return NR_TYPEFACE_SLANT_UNKNOWN;

        return tf->def->slant;
}

NRTypeFaceWeight nrTypefaceStrToWeight (const unsigned char *weightStr)
{
  if (!strcmp(weightStr,"Normal")) return NR_TYPEFACE_WEIGHT_NORMAL;
  if (!strcmp(weightStr,"Bold")) return NR_TYPEFACE_WEIGHT_BOLD;
  if (!strcmp(weightStr,"Ultrabold")) return NR_TYPEFACE_WEIGHT_ULTRABOLD;
  if (!strcmp(weightStr,"Demibold")) return NR_TYPEFACE_WEIGHT_DEMIBOLD;
  if (!strcmp(weightStr,"Thin")) return NR_TYPEFACE_WEIGHT_THIN;
  if (!strcmp(weightStr,"Light")) return NR_TYPEFACE_WEIGHT_LIGHT;
  if (!strcmp(weightStr,"Ultralight")) return NR_TYPEFACE_WEIGHT_ULTRALIGHT;
  if (!strcmp(weightStr,"Medium")) return NR_TYPEFACE_WEIGHT_MEDIUM;
  if (!strcmp(weightStr,"Black")) return NR_TYPEFACE_WEIGHT_BLACK;
  return NR_TYPEFACE_WEIGHT_UNKNOWN;
}

const unsigned char *nrTypefaceWeightToStr (NRTypeFaceWeight weight)
{
  switch (weight)
    {
    case NR_TYPEFACE_WEIGHT_NORMAL:
      return "Normal";
    case NR_TYPEFACE_WEIGHT_BOLD:
      return "Bold";
    case NR_TYPEFACE_WEIGHT_ULTRABOLD:
      return "Ultrabold";
    case NR_TYPEFACE_WEIGHT_DEMIBOLD:
      return "Demibold";
    case NR_TYPEFACE_WEIGHT_THIN:
      return "Thin";
    case NR_TYPEFACE_WEIGHT_LIGHT:
      return "Light";
    case NR_TYPEFACE_WEIGHT_ULTRALIGHT:
      return "Ultralight";
    case NR_TYPEFACE_WEIGHT_MEDIUM:
      return "Medium";
    case NR_TYPEFACE_WEIGHT_BLACK:
      return "Black";
    case NR_TYPEFACE_WEIGHT_UNKNOWN:
    default:
      return "?";
    }
}

NRTypeFaceSlant nrTypefaceStrToSlant (const unsigned char *slantStr)
{
  if (!strcmp(slantStr,"Roman")) return NR_TYPEFACE_SLANT_ROMAN;
  if (!strcmp(slantStr,"Italic")) return NR_TYPEFACE_SLANT_ITALIC;
  if (!strcmp(slantStr,"Oblique")) return NR_TYPEFACE_SLANT_OBLIQUE;
  return NR_TYPEFACE_SLANT_UNKNOWN;
}

const unsigned char *nrTypefaceSlantToStr (NRTypeFaceSlant slant)
{
  switch (slant)
    {
    case NR_TYPEFACE_SLANT_ROMAN:
      return "Roman";
    case NR_TYPEFACE_SLANT_ITALIC:
      return "Italic";
    case NR_TYPEFACE_SLANT_OBLIQUE:
      return "Oblique";
    case NR_TYPEFACE_SLANT_UNKNOWN:
    default:
      return "?";
    }
}

const unsigned char *nrTypefaceMkName(const NRTypeFaceDef *def)
{
  const unsigned char* slantStr;
  const unsigned char* weightStr;
  unsigned char* res;
  int len;

  if (!def)
    return NULL;

  slantStr=nrTypefaceSlantToStr(def->slant);
  weightStr=nrTypefaceWeightToStr(def->weight);

  len=strlen(slantStr)+strlen(weightStr)+1;
  res=(unsigned char*)(malloc(len+1));
  if (!res)
    return NULL;

  sprintf(res,"%s %s",weightStr,slantStr);
  return res;
}

#define __NR_TYPEFACE_C__

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

#include <string.h>
#include <libnr/nr-macros.h>
#include <libnr/nr-matrix.h>
#include "nr-typeface.h"
#include "nr-type-directory.h"

NRTypeFace *
nr_typeface_ref (NRTypeFace *tf)
{
	tf->refcount += 1;

	return tf;
}

NRTypeFace *
nr_typeface_unref (NRTypeFace *tf)
{
	tf->refcount -= 1;

	if (tf->refcount < 1) {
		nr_type_directory_forget_face (tf);
		tf->vmv->free (tf);
	}

	return NULL;
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
	return tf->vmv->attribute_get (tf, key, str, size);
}

NRBPath *
nr_typeface_glyph_outline_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRBPath *d, unsigned int ref)
{
	return tf->vmv->glyph_outline_get (tf, glyph, metrics, d, ref);
}

void
nr_typeface_glyph_outline_unref (NRTypeFace *tf, unsigned int glyph, unsigned int metrics)
{
	return tf->vmv->glyph_outline_unref (tf, glyph, metrics);
}

NRPointF *
nr_typeface_glyph_advance_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv)
{
	return tf->vmv->glyph_advance_get (tf, glyph, metrics, adv);
}

unsigned int
nr_typeface_lookup_default (NRTypeFace *tf, unsigned int unival)
{
	return tf->vmv->lookup (tf, NR_TYPEFACE_LOOKUP_RULE_DEFAULT, unival);
}

NRFont *
nr_font_new_default (NRTypeFace *tf, unsigned int metrics, float size)
{
	NRMatrixF scale;

	nr_matrix_f_set_scale (&scale, size, size);

	return tf->vmv->font_new (tf, metrics, &scale);
}

static NRTypeFace *nr_typeface_empty_new (NRTypeFaceDef *def);
void nr_typeface_empty_free (NRTypeFace *tf);

unsigned int nr_typeface_empty_attribute_get (NRTypeFace *tf, const unsigned char *key, unsigned char *str, unsigned int size);
NRBPath *nr_typeface_empty_glyph_outline_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRBPath *d, unsigned int ref);
void nr_typeface_empty_glyph_outline_unref (NRTypeFace *tf, unsigned int glyph, unsigned int metrics);
NRPointF *nr_typeface_empty_glyph_advance_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv);
unsigned int nr_typeface_empty_lookup (NRTypeFace *tf, unsigned int rule, unsigned int unival);

NRFont *nr_typeface_empty_font_new (NRTypeFace *tf, unsigned int metrics, NRMatrixF *transform);
void nr_typeface_empty_font_free (NRFont *font);

static NRTypeFaceVMV nr_type_empty_vmv = {
	nr_typeface_empty_new,

	nr_typeface_empty_free,
	nr_typeface_empty_attribute_get,
	nr_typeface_empty_glyph_outline_get,
	nr_typeface_empty_glyph_outline_unref,
	nr_typeface_empty_glyph_advance_get,
	nr_typeface_empty_lookup,

	nr_typeface_empty_font_new,
	nr_typeface_empty_font_free,

	nr_font_generic_glyph_outline_get,
	nr_font_generic_glyph_outline_unref,
	nr_font_generic_glyph_advance_get,
	nr_font_generic_glyph_area_get,

	nr_font_generic_rasterfont_new,
	nr_font_generic_rasterfont_free,

	nr_rasterfont_generic_glyph_advance_get,
	nr_rasterfont_generic_glyph_area_get,
	nr_rasterfont_generic_glyph_mask_render
};

static NRFont *empty_fonts = NULL;

void
nr_type_empty_build_def (NRTypeFaceDef *def, const unsigned char *name, const unsigned char *family)
{
	def->vmv = &nr_type_empty_vmv;
	def->name = strdup (name);
	def->family = strdup (family);
	def->typeface = NULL;
}

static NRTypeFace *
nr_typeface_empty_new (NRTypeFaceDef *def)
{
	NRTypeFace *tf;

	tf = nr_new (NRTypeFace, 1);

	tf->vmv = &nr_type_empty_vmv;
	tf->refcount = 1;
	tf->def = def;

	tf->nglyphs = 1;

	return tf;
}

void
nr_typeface_empty_free (NRTypeFace *tf)
{
	nr_free (tf);
}

unsigned int
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

NRBPath *
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

void
nr_typeface_empty_glyph_outline_unref (NRTypeFace *tf, unsigned int glyph, unsigned int metrics)
{
}

NRPointF *
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

unsigned int
nr_typeface_empty_lookup (NRTypeFace *tf, unsigned int rule, unsigned int unival)
{
	return 0;
}

NRFont *
nr_typeface_empty_font_new (NRTypeFace *tf, unsigned int metrics, NRMatrixF *transform)
{
	NRFont *font;
	float size;

	size = NR_MATRIX_DF_EXPANSION (transform);

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

void
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


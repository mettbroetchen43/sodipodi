#define __NR_TYPE_GNOME_C__

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
#include "nr-type-gnome.h"

void nr_typeface_gnome_free (NRTypeFace *tf);

unsigned int nr_typeface_gnome_attribute_get (NRTypeFace *tf, const unsigned char *key, unsigned char *str, unsigned int size);
NRBPath *nr_typeface_gnome_glyph_outline_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRBPath *d, unsigned int ref);
void nr_typeface_gnome_glyph_outline_unref (NRTypeFace *tf, unsigned int glyph);
NRPointF *nr_typeface_gnome_glyph_advance_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv);
unsigned int nr_typeface_gnome_lookup (NRTypeFace *tf, unsigned int rule, unsigned int unival);

NRFont *nr_typeface_gnome_font_new (NRTypeFace *tf, unsigned int metrics, NRMatrixF *transform);
void nr_typeface_gnome_font_free (NRFont *font);

static NRTypeFaceVMV nr_type_gnome_vmv = {
	nr_typeface_gnome_free,
	nr_typeface_gnome_attribute_get,
	nr_typeface_gnome_glyph_outline_get,
	nr_typeface_gnome_glyph_outline_unref,
	nr_typeface_gnome_glyph_advance_get,
	nr_typeface_gnome_lookup,

	nr_typeface_gnome_font_new,
	nr_typeface_gnome_font_free,

	nr_font_generic_glyph_outline_get,
	nr_font_generic_glyph_outline_unref,
	nr_font_generic_glyph_advance_get,

	nr_font_generic_rasterfont_new,
	nr_font_generic_rasterfont_free,

	nr_rasterfont_generic_glyph_advance_get,
	nr_rasterfont_generic_glyph_area_get,
	nr_rasterfont_generic_glyph_mask_render
};

static NRTypeFaceGnome *typefaces = NULL;

NRTypeFace *
nr_typeface_gnome_new (GnomeFontFace *gff)
{
	NRTypeFaceGnome *tfg;

	for (tfg = typefaces; tfg != NULL; tfg = tfg->next) {
		if (tfg->face == gff) {
			return nr_typeface_ref ((NRTypeFace *) tfg);
		}
	}

	tfg = nr_new (NRTypeFaceGnome, 1);

	tfg->typeface.vmv = &nr_type_gnome_vmv;
	tfg->typeface.refcount = 1;
	tfg->typeface.nglyphs = gnome_font_face_get_num_glyphs (gff);

	gnome_font_face_ref (gff);
	tfg->face = gff;
	tfg->fonts = NULL;

	tfg->prev = NULL;
	tfg->next = typefaces;
	if (tfg->next) tfg->next->prev = tfg;
	typefaces = tfg;

	return (NRTypeFace *) tfg;
}

void
nr_typeface_gnome_free (NRTypeFace *tf)
{
	NRTypeFaceGnome *tfg;

	tfg = (NRTypeFaceGnome *) tf;

	gnome_font_face_unref (tfg->face);

	if (tfg->prev) {
		tfg->prev->next = tfg->next;
	} else {
		typefaces = tfg->next;
	}
	if (tfg->next) tfg->next->prev = tfg->prev;

	nr_free (tfg);
}

unsigned int
nr_typeface_gnome_attribute_get (NRTypeFace *tf, const unsigned char *key, unsigned char *str, unsigned int size)
{
	NRTypeFaceGnome *tfg;
	const unsigned char *val;
	int len;

	tfg = (NRTypeFaceGnome *) tf;

	if (!strcmp (key, "name")) {
		val = gnome_font_face_get_name (tfg->face);
	} else if (!strcmp (key, "family")) {
		val = gnome_font_face_get_family_name (tfg->face);
	} else if (!strcmp (key, "weight")) {
		guint wc;
		wc = gnome_font_face_get_weight_code (tfg->face);
		val = (wc >= GNOME_FONT_BOLD) ? "bold" : "normal";
	} else if (!strcmp (key, "style")) {
		val = gnome_font_face_is_italic (tfg->face) ? "italic" : "normal";
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
nr_typeface_gnome_glyph_outline_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRBPath *d, unsigned int ref)
{
	NRTypeFaceGnome *tfg;

	tfg = (NRTypeFaceGnome *) tf;

	if (metrics == NR_TYPEFACE_METRICS_VERTICAL) {
		/* fixme: Really should implement this */
		return NULL;
	} else {
		d->path = (ArtBpath *) gnome_font_face_get_glyph_stdoutline (tfg->face, glyph);
	}

	return d;
}

void
nr_typeface_gnome_glyph_outline_unref (NRTypeFace *tf, unsigned int glyph)
{
	NRTypeFaceGnome *tfg;

	tfg = (NRTypeFaceGnome *) tf;
}

NRPointF *
nr_typeface_gnome_glyph_advance_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv)
{
	NRTypeFaceGnome *tfg;
	ArtPoint gfa;

	tfg = (NRTypeFaceGnome *) tf;

	gnome_font_face_get_glyph_stdadvance (tfg->face, glyph, &gfa);

	adv->x = gfa.x;
	adv->y = gfa.y;

	return adv;
}

unsigned int
nr_typeface_gnome_lookup (NRTypeFace *tf, unsigned int rule, unsigned int unival)
{
	NRTypeFaceGnome *tfg;

	tfg = (NRTypeFaceGnome *) tf;

	if (rule == NR_TYPEFACE_LOOKUP_RULE_DEFAULT) {
		return gnome_font_face_lookup_default (tfg->face, unival);
	}

	return 0;
}

NRFont *
nr_typeface_gnome_font_new (NRTypeFace *tf, unsigned int metrics, NRMatrixF *transform)
{
	NRTypeFaceGnome *tfg;
	NRFont *font;
	float size;

	tfg = (NRTypeFaceGnome *) tf;
	size = NR_MATRIX_DF_EXPANSION (transform);

	font = tfg->fonts;
	while (font != NULL) {
		if (NR_DF_TEST_CLOSE (size, font->size, 0.001 * size)) {
			return nr_font_ref (font);
		}
		font = font->next;
	}
	
	font = nr_font_generic_new (tf, metrics, transform);

	font->next = tfg->fonts;
	tfg->fonts = font;

	return font;
}

void
nr_typeface_gnome_font_free (NRFont *font)
{
	NRTypeFaceGnome *tfg;

	tfg = (NRTypeFaceGnome *) font->face;

	if (tfg->fonts == font) {
		tfg->fonts = font->next;
	} else {
		NRFont *ref;
		ref = tfg->fonts;
		while (ref->next != font) ref = ref->next;
		ref->next = font->next;
	}

	font->next = NULL;

	nr_font_generic_free (font);
}

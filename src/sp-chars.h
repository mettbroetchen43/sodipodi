#ifndef __SP_CHARS_H__
#define __SP_CHARS_H__

/*
 * SPChars - parent class for text objects
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <libgnomeprint/gnome-font.h>
#include "sp-shape.h"

#define SP_TYPE_CHARS (sp_chars_get_type ())
#define SP_CHARS(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_CHARS, SPChars))
#define SP_CHARS_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CHARS, SPCharsClass))
#define SP_IS_CHARS(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_CHARS))
#define SP_IS_CHARS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CHARS))

typedef struct _SPCharElement SPCharElement;

struct _SPCharElement {
	guint glyph;
	GnomeFontFace *face;
	gfloat affine[6];
};

struct _SPChars {
	SPShape shape;
	GList *element;
};

struct _SPCharsClass {
	SPShapeClass parent_class;
};

GtkType sp_chars_get_type (void);

void sp_chars_clear (SPChars *chars);

void sp_chars_add_element (SPChars *chars, guint glyph, GnomeFontFace *face, const gdouble *affine);

#endif



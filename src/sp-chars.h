#ifndef __SP_CHARS_H__
#define __SP_CHARS_H__

/*
 * SPChars - parent class for text objects
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libgnomeprint/gnome-font.h>
#include "helper/curve.h"
#include "sp-item.h"

#define SP_TYPE_CHARS (sp_chars_get_type ())
#define SP_CHARS(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_CHARS, SPChars))
#define SP_CHARS_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CHARS, SPCharsClass))
#define SP_IS_CHARS(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_CHARS))
#define SP_IS_CHARS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CHARS))

typedef struct _SPCharElement SPCharElement;

struct _SPCharElement {
	SPCharElement *next;
	guint glyph;
	GnomeFontFace *face;
	gfloat affine[6];
};

struct _SPChars {
	SPItem item;
	SPCharElement *elements;
	ArtDRect paintbox;
};

struct _SPCharsClass {
	SPItemClass parent_class;
};

GtkType sp_chars_get_type (void);

void sp_chars_clear (SPChars *chars);

void sp_chars_add_element (SPChars *chars, guint glyph, GnomeFontFace *face, const gdouble *affine);

SPCurve *sp_chars_normalized_bpath (SPChars *chars);

/* This is completely unrelated to SPItem::print */
void sp_chars_do_print (SPChars *chars, GnomePrintContext *gpc, const gdouble *ctm,
			const ArtDRect *pbox, const ArtDRect *dbox, const ArtDRect *bbox);
void sp_chars_set_paintbox (SPChars *chars, ArtDRect *paintbox);

#endif



#ifndef __SP_TEXT_H__
#define __SP_TEXT_H__

/*
 * SPText - a SVG <text> element
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define SP_TYPE_TEXT (sp_text_get_type ())
#define SP_TEXT(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_TEXT, SPText))
#define SP_TEXT_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_TEXT, SPTextClass))
#define SP_IS_TEXT(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_TEXT))
#define SP_IS_TEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_TEXT))

#define SP_TYPE_TSPAN (sp_tspan_get_type ())
#define SP_TSPAN(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_TSPAN, SPTSpan))
#define SP_TSPAN_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_TSPAN, SPTSpanClass))
#define SP_IS_TSPAN(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_TSPAN))
#define SP_IS_TSPAN_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_TSPAN))

#define SP_TYPE_STRING (sp_string_get_type ())
#define SP_STRING(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_STRING, SPString))
#define SP_STRING_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_STRING, SPStringClass))
#define SP_IS_STRING(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_STRING))
#define SP_IS_STRING_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_STRING))

#include "sp-chars.h"

typedef struct _SPLayoutData SPLayoutData;

struct _SPLayoutData {
	/* fixme: Vectors */
	guint x_set : 1;
	guint y_set : 1;
	guint dx_set : 1;
	guint dy_set : 1;
	guint rotate_set : 1;
	gfloat x, y;
	gfloat dx, dy;
	gfloat rotate;
};

/*
 * The ultimate source of current mess is, that we have to derive string <- chars
 * This will be changed as soon as we have NRArenaGlyphList class
 */

/* SPString */

struct _SPString {
	SPChars chars;

	/* fixme: We probably do not need it, as string cannot have attributes */
	SPLayoutData *ly;

	guchar *text;
};

struct _SPStringClass {
	SPCharsClass parent_class;
};

GtkType sp_string_get_type (void);

/* SPTSpan */

struct _SPTSpan {
	SPItem item;

	SPLayoutData ly;

	SPObject *string;
};

struct _SPTSpanClass {
	SPItemClass parent_class;
};

GtkType sp_tspan_get_type (void);

/* SPText */

struct _SPText {
	SPItem item;

	SPLayoutData ly;

	SPObject *children;
};

struct _SPTextClass {
	SPItemClass parent_class;
};

GtkType sp_text_get_type (void);

gchar *sp_text_get_string_multiline (SPText *text);
void sp_text_set_repr_text_multiline (SPText *text, const guchar *str);

SPItem *sp_text_get_last_string (SPText *text);

END_GNOME_DECLS

#endif

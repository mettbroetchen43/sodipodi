#ifndef __SP_TEXT_H__
#define __SP_TEXT_H__

/*
 * SVG <text> and <tspan> implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
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

/* Text specific flags */
#define SP_TEXT_CONTENT_MODIFIED_FLAG SP_OBJECT_USER_MODIFIED_FLAG_A
#define SP_TEXT_LAYOUT_MODIFIED_FLAG SP_OBJECT_USER_MODIFIED_FLAG_A

#define SP_TSPAN_STRING(t) ((SPString *) SP_TSPAN (t)->string)

#include <libnr/nr-types.h>
#include "svg/svg-types.h"
#include "sp-chars.h"

typedef struct _SPLayoutData SPLayoutData;

struct _SPLayoutData {
	/* fixme: Vectors */
	SPSVGLength x;
	SPSVGLength y;
	SPSVGLength dx;
	SPSVGLength dy;
	guint rotate_set : 1;
	gfloat rotate;
};

/*
 * The ultimate source of current mess is, that we have to derive string <- chars
 * This will be changed as soon as we have NRArenaGlyphList class
 */

/* SPString */

struct _SPString {
	SPChars chars;
	/* Link to parent layout */
	SPLayoutData *ly;
	/* Content */
	guchar *text;
	NRPointF *p;
	/* Bookkeeping */
	guint start;
	guint length;
	/* Using current direction and style */
	ArtDRect bbox;
	ArtPoint advance;
};

struct _SPStringClass {
	SPCharsClass parent_class;
};

#define SP_STRING_TEXT(s) (SP_STRING (s)->text)

GtkType sp_string_get_type (void);

/* SPTSpan */

enum {
	SP_TSPAN_ROLE_UNSPECIFIED,
	SP_TSPAN_ROLE_PARAGRAPH,
	SP_TSPAN_ROLE_LINE
};

struct _SPTSpan {
	SPItem item;

	guint role : 2;

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

	guint relayout : 1;
};

struct _SPTextClass {
	SPItemClass parent_class;
};

#define SP_TEXT_CHILD_STRING(c) (SP_IS_TSPAN (c) ? SP_TSPAN_STRING (c) : SP_STRING (c))

GtkType sp_text_get_type (void);

int sp_text_is_empty (SPText *text);
gchar *sp_text_get_string_multiline (SPText *text);
void sp_text_set_repr_text_multiline (SPText *text, const guchar *str);

SPCurve *sp_text_normalized_bpath (SPText *text);

#if 0
/* fixme: Better place for these */
gint sp_text_font_weight_to_gp (gint weight);
#define sp_text_font_italic_to_gp(s) ((s) != SP_CSS_FONT_STYLE_NORMAL)
#endif

/* fixme: Think about these (Lauris) */

SPTSpan *sp_text_append_line (SPText *text);
SPTSpan *sp_text_insert_line (SPText *text, gint pos);

/* This gives us SUM (strlen (STRING)) + (LINES - 1) */
gint sp_text_get_length (SPText *text);
gint sp_text_append (SPText *text, const guchar *utf8);
/* Returns position after inserted */
gint sp_text_insert (SPText *text, gint pos, const guchar *utf8, gboolean preservews);
/* Returns start position */
gint sp_text_delete (SPText *text, gint start, gint end);

gint sp_text_up (SPText *text, gint pos);
gint sp_text_down (SPText *text, gint pos);

void sp_text_get_cursor_coords (SPText *text, gint position, ArtPoint *p0, ArtPoint *p1);

END_GNOME_DECLS

#endif

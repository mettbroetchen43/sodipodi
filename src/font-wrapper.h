#ifndef SP_FONT_WRAPPER_H
#define SP_FONT_WRAPPER_H

#include <glib.h>
#include <libgnomeprint/gnome-font.h>
#include "helper/curve.h"

typedef struct _SPFont SPFont;

typedef struct {
	GnomeFontWeight weight;
	gboolean italic;
} SPFontInfo;

typedef struct {
	gchar * name;
	gint num_fonts;
	SPFontInfo * fonts;
} SPFontFamilyInfo;

SPFont * sp_font_get (const gchar * family, GnomeFontWeight weight, gboolean italic);
void sp_font_ref (SPFont * font);
void sp_font_unref (SPFont * font);

SPFont * sp_font_default (void);

SPCurve * sp_font_glyph_outline (SPFont * font, guint glyph);

gint sp_font_glyph_width (SPFont * font, guint glyph);

GList * sp_font_family_list (void);

#endif

#ifndef __SP_SVG_H__
#define __SP_SVG_H__

/*
 * SVG data parser
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib.h>
#include <libart_lgpl/art_bpath.h>
#include <libgnomeprint/gnome-font.h>
#include "svg-types.h"
#include "../helper/units.h"
#include "../display/stroke.h"

/* Generic */

/*
 * These are very-very simple:
 * - they accept everything libc strtod accepts
 * - no valid end character checking
 * Return FALSE and let val untouched on error
 */
 
gboolean sp_svg_read_number_f (const guchar *str, gfloat *val);
gboolean sp_svg_read_number_d (const guchar *str, gdouble *val);

/* Length */

/*
 * Parse number with optional unit specifier:
 * - for px, pt, pc, mm, cm, computed is final value accrding to SVG spec
 * - for em, ex, and % computed is left untouched
 * - % is divided by 100 (i.e. 100% is 1.0)
 * !isalnum check is done at the end
 * Any return value pointer can be NULL
 */

gboolean sp_svg_length_read (const unsigned char *str, SPSVGLength *length);
gboolean sp_svg_length_read_lff (const unsigned char *str, unsigned long *unit, float *value, float *computed);
void sp_svg_length_unset (SPSVGLength *length, unsigned long unit, float value, float computed);

#if 0
gdouble sp_svg_read_length (const SPUnit **unit, const gchar *str, gdouble def);
#endif
gint sp_svg_write_length (gchar *buf, gint buflen, gdouble val, const SPUnit *unit);

gdouble sp_svg_read_percentage (const gchar * str, gdouble def);
gint sp_svg_write_percentage (gchar * buf, gint buflen, gdouble val);

guint32 sp_svg_read_color (const guchar * str, guint32 def);
gint sp_svg_write_color (gchar * buf, gint buflen, guint32 color);

gdouble * sp_svg_read_affine (gdouble affine[], const gchar * str);
gint sp_svg_write_affine (gchar * buf, gint buflen, gdouble affine[]);

/* NB! As paths can be long, we use here dynamic string */

ArtBpath * sp_svg_read_path (const gchar * str);
gchar * sp_svg_write_path (const ArtBpath * bpath);

SPStrokeType sp_svg_read_stroke_type (const gchar * str);
ArtPathStrokeJoinType sp_svg_read_stroke_join (const gchar * str);
ArtPathStrokeCapType sp_svg_read_stroke_cap (const gchar * str);

GnomeFontWeight sp_svg_read_font_weight (const gchar * str);
gboolean sp_svg_read_font_italic (const gchar * str);

#endif

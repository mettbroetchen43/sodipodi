#ifndef __SP_SVG_H__
#define __SP_SVG_H__

#include <glib.h>
#include <libart_lgpl/art_bpath.h>
#include <libgnomeprint/gnome-font.h>
#include "../helper/units.h"
#include "../display/stroke.h"

/* General CSS Properties */
/* length */

gdouble sp_svg_read_length (const SPUnit **unit, const gchar *str, gdouble def);
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

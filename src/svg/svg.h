#ifndef SP_SVG_H
#define SP_SVG_H

#include <glib.h>
#include <libart_lgpl/art_bpath.h>
#include <libgnomeprint/gnome-font.h>
#include "../style.h"
#include "../display/fill.h"
#include "../display/stroke.h"

/* fixme: Currently we copy SPUnit, but this should be removed */

typedef enum {
	SP_SVG_UNIT_PIXELS   = (1 << 4),
	SP_SVG_UNIT_ABSOLUTE = (1 << 5),
	SP_SVG_UNIT_USER     = (1 << 6),
	SP_SVG_UNIT_PERCENT  = (1 << 7),
	SP_SVG_UNIT_EM       = (1 << 8),
	SP_SVG_UNIT_EX       = (1 << 9)
} SPSVGUnit;

#define SP_SVG_UNIT_MASK 0x3f0

/* General CSS Properties */
/* length */

gdouble sp_svg_read_length (SPSVGUnit * unit, const gchar * str, gdouble def);
gint sp_svg_write_length (gchar * buf, gint buflen, gdouble val, SPSVGUnit unit);

gdouble sp_svg_read_percentage (const gchar * str, gdouble def);
gint sp_svg_write_percentage (gchar * buf, gint buflen, gdouble val);

guint32 sp_svg_read_color (const guchar * str, guint32 def);
gint sp_svg_write_color (gchar * buf, gint buflen, guint32 color);

gdouble * sp_svg_read_affine (gdouble affine[], const gchar * str);
gint sp_svg_write_affine (gchar * buf, gint buflen, gdouble affine[]);

/* NB! As paths can be long, we use here dynamic string */

ArtBpath * sp_svg_read_path (const gchar * str);
gchar * sp_svg_write_path (const ArtBpath * bpath);

SPFillType sp_svg_read_fill_type (const gchar * str);
gint sp_svg_write_fill_type (gchar * buf, gint buflen, SPFillType type, guint32 color);

SPStrokeType sp_svg_read_stroke_type (const gchar * str);
ArtPathStrokeJoinType sp_svg_read_stroke_join (const gchar * str);
ArtPathStrokeCapType sp_svg_read_stroke_cap (const gchar * str);

GnomeFontWeight sp_svg_read_font_weight (const gchar * str);
gboolean sp_svg_read_font_italic (const gchar * str);

#endif

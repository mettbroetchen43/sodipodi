#ifndef SP_SVG_H
#define SP_SVG_H

#include <glib.h>
#include <libart_lgpl/art_bpath.h>
#include "../display/fill.h"
#include "../display/stroke.h"

typedef enum {
	SP_SVG_UNIT_USER,
	SP_SVG_UNIT_ABSOLUTE,
	SP_SVG_UNIT_PIXELS,
	SP_SVG_UNIT_PERCENT,
	SP_SVG_UNIT_EM,
	SP_SVG_UNIT_EX
} SPSVGUnit;

/* General CSS Properties */
/* length */

gdouble sp_svg_read_length (SPSVGUnit * unit, const gchar * str);
gint sp_svg_write_length (gchar * buf, gint buflen, gdouble val, SPSVGUnit unit);

gdouble sp_svg_read_percentage (const gchar * str);
gint sp_svg_write_percentage (gchar * buf, gint buflen, gdouble val);

guint32 sp_svg_read_color (const gchar * str);
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

#endif

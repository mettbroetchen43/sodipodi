#define SP_SHAPE_STYLE_C

#include <string.h>
#include "svg/svg.h"
#include "sp-shape-style.h"

SPFill *
sp_fill_read (SPFill * fill, SPCSSAttr * css)
{
	const gchar * prop;

	g_return_val_if_fail (fill != NULL, NULL);
	g_return_val_if_fail (css != NULL, NULL);

	prop = sp_repr_css_property (css, "fill", NULL);
	if (prop != NULL) {
		if (strcmp (prop, "none") == 0) {
			fill->type = SP_FILL_NONE;
		} else {
			fill->type = SP_FILL_COLOR;
			fill->color = (fill->color & 0xff) |
				sp_svg_read_color (prop);
		}
	}
	prop = sp_repr_css_property (css, "fill-opacity", NULL);
	if (prop != NULL) {
		fill->color = (fill->color & 0xffffff00) |
			(guint) (255.0 * sp_svg_read_percentage (prop) + 0.5);
	}

	return fill;
}

SPStroke *
sp_stroke_read (SPStroke * stroke, SPCSSAttr * css)
{
	const gchar * prop;
	SPSVGUnit unit;

	g_return_val_if_fail (stroke != NULL, NULL);
	g_return_val_if_fail (css != NULL, NULL);

	prop = sp_repr_css_property (css, "stroke", NULL);
	if (prop != NULL) {
		if (strcmp (prop, "none") == 0) {
			stroke->type = SP_STROKE_NONE;
		} else {
			stroke->type = SP_STROKE_COLOR;
			stroke->color = (stroke->color & 0xff) |
				sp_svg_read_color (prop);
		}
	}
	prop = sp_repr_css_property (css, "stroke-opacity", NULL);
	if (prop != NULL) {
		stroke->color = (stroke->color & 0xffffff00) |
			(guint) (255.0 * sp_svg_read_percentage (prop) + 0.5);
	}
	prop = sp_repr_css_property (css, "stroke-linecap", NULL);
	if (prop != NULL) {
		if (strcmp (prop, "round") == 0) {
			stroke->cap = ART_PATH_STROKE_CAP_ROUND;
		} else if (strcmp (prop, "square") == 0) {
			stroke->cap = ART_PATH_STROKE_CAP_SQUARE;
		} else {
			stroke->cap = ART_PATH_STROKE_CAP_BUTT;
		}
	}
	prop = sp_repr_css_property (css, "stroke-linejoin", NULL);
	if (prop != NULL) {
		if (strcmp (prop, "round") == 0) {
			stroke->join = ART_PATH_STROKE_JOIN_ROUND;
		} else if (strcmp (prop, "bevel") == 0) {
			stroke->join = ART_PATH_STROKE_JOIN_BEVEL;
		} else {
			stroke->join = ART_PATH_STROKE_JOIN_MITER;
		}
	}
	/* fixme: implement units */
	prop = sp_repr_css_property (css, "stroke-width", NULL);
	if (prop != NULL) {
		stroke->width = sp_svg_read_length (&unit, prop);
		stroke->scaled = (unit != SP_SVG_UNIT_PIXELS);
	}

	return stroke;
}

#define SP_SVG_FILL_C

#include "svg/svg.h"

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


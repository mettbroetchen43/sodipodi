#define SP_SVG_FILL_C

/* fixme: remove this */

#if 0

#include <string.h>
#include <stdio.h>
#include "svg.h"

SPFillType
sp_svg_read_fill_type (const gchar * str)
{
	g_return_val_if_fail (str != NULL, SP_FILL_NONE);

	if (strcmp (str, "none") == 0)
		return SP_FILL_NONE;
	
	return SP_FILL_COLOR;
}

gint
sp_svg_write_fill_type (gchar * buf, gint buflen, SPFillType type, guint32 color)
{
	g_return_val_if_fail (buf != NULL, 0);

	switch (type) {
	case SP_FILL_NONE:
		return snprintf (buf, buflen, "none");
	case SP_FILL_COLOR:
		return sp_svg_write_color (buf, buflen, color);
	default:
		g_assert_not_reached ();
	}

	return 0;
}

#endif

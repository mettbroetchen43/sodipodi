#define SP_SVG_LENGTH_C

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "svg.h"

/* Length */

gdouble
sp_svg_read_length (SPSVGUnit * unit, const gchar * str)
{
	char * u;
	gdouble v;

	*unit = SP_SVG_UNIT_USER;
	if (str == NULL) return 0.0;

	v = strtod (str, &u);
	while (isspace (*u)) {
		if (*u == '\0') return v;
		u++;
	}
	if (strncmp (u, "px", 2) == 0) {
		*unit = SP_SVG_UNIT_PIXELS;
		return v;
	}
	if (strncmp (u, "pt", 2) == 0) {
		*unit = SP_SVG_UNIT_ABSOLUTE;
		return v;
	}
	if (strncmp (u, "mm", 2) == 0) {
		*unit = SP_SVG_UNIT_ABSOLUTE;
		return v * 72.0 / 25.4;
	}
	if (strncmp (u, "cm", 2) == 0) {
		*unit = SP_SVG_UNIT_ABSOLUTE;
		return v * 72.0 / 2.54;
	}
	if (strncmp (u, "in", 2) == 0) {
		*unit = SP_SVG_UNIT_ABSOLUTE;
		return v * 72.0;
	}
	if (strncmp (u, "em", 2) == 0) {
		*unit = SP_SVG_UNIT_EM;
		return v;
	}
	if (strncmp (u, "ex", 2) == 0) {
		*unit = SP_SVG_UNIT_EX;
		return v;
	}
	if (*u == '%') {
		*unit = SP_SVG_UNIT_PERCENT;
		return v / 100.0;
	}
	return v;
}

gint
sp_svg_write_length (gchar * buf, gint buflen, gdouble val, SPSVGUnit unit)
{
	switch (unit) {
	case SP_SVG_UNIT_USER:
		return snprintf (buf, buflen, "%g", val);
	case SP_SVG_UNIT_ABSOLUTE:
		return snprintf (buf, buflen, "%gpt", val);
	case SP_SVG_UNIT_PIXELS:
		return snprintf (buf, buflen, "%gpx", val);
	case SP_SVG_UNIT_PERCENT:
		return snprintf (buf, buflen, "%g%%", val * 100.0);
	case SP_SVG_UNIT_EM:
		return snprintf (buf, buflen, "%gem", val);
	case SP_SVG_UNIT_EX:
		return snprintf (buf, buflen, "%gex", val);
	default:
		g_assert_not_reached ();
	}
	g_assert_not_reached ();
	return 0;
}

gdouble
sp_svg_read_percentage (const gchar * str)
{
	char * u;
	gdouble v;

	if (str == NULL) return 0.0;

	v = strtod (str, &u);
	while (isspace (*u)) {
		if (*u == '\0') return v;
		u++;
	}
	if (*u == '%') v /= 100.0;

	return v;
}

gint
sp_svg_write_percentage (gchar * buf, gint buflen, gdouble val)
{
	return sp_svg_write_length (buf, buflen, val, SP_SVG_UNIT_PERCENT);
}


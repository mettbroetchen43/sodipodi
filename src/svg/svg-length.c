#define __SP_SVG_LENGTH_C__

/*
 * SVG data parser
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "svg.h"

gboolean
sp_svg_read_number_f (const guchar *str, gfloat *val)
{
	gchar *e;
	gfloat v;

	if (!str) return FALSE;

	v = strtod (str, &e);
	if (e) *val = v;

	return (e != NULL);
}

gboolean
sp_svg_read_number_d (const guchar *str, gdouble *val)
{
	gchar *e;
	gdouble v;

	if (!str) return FALSE;

	v = strtod (str, &e);
	if (e) *val = v;

	return (e != NULL);
}

/* Length */
/* NB! px is absolute in SVG, but we prefer to keep it separate for UI reasons */

gdouble
sp_svg_read_length (const SPUnit **unit, const gchar *str, gdouble def)
{
	char * u;
	gdouble v;

	*unit = sp_unit_get_identity (SP_UNIT_USERSPACE);
	if (str == NULL) return def;

	v = strtod (str, &u);
	while (isspace (*u)) {
		if (*u == '\0') return v;
		u++;
	}

	if (strncmp (u, "px", 2) == 0) {
		*unit = sp_unit_get_identity (SP_UNIT_DEVICE);
	} else if (strncmp (u, "pt", 2) == 0) {
		*unit = sp_unit_get_identity (SP_UNIT_ABSOLUTE);
	} else if (strncmp (u, "pc", 2) == 0) {
		*unit = sp_unit_get_identity (SP_UNIT_ABSOLUTE);
		v *= 12.0;
	} else if (strncmp (u, "mm", 2) == 0) {
		*unit = sp_unit_get_identity (SP_UNIT_ABSOLUTE);
		v *= (72.0 / 25.4);
	} else if (strncmp (u, "cm", 2) == 0) {
		*unit = sp_unit_get_identity (SP_UNIT_ABSOLUTE);
		v *= (72.0 / 2.54);
	} else if (strncmp (u, "m", 1) == 0) {
		*unit = sp_unit_get_identity (SP_UNIT_ABSOLUTE);
		v *= (72.0 / 0.0254);
	} else if (strncmp (u, "in", 2) == 0) {
		*unit = sp_unit_get_identity (SP_UNIT_ABSOLUTE);
		v *= 72.0;
	} else if (strncmp (u, "em", 2) == 0) {
		*unit = sp_unit_get_by_abbreviation ("em");
	} else if (strncmp (u, "ex", 2) == 0) {
		*unit = sp_unit_get_by_abbreviation ("ex");
	} else if (*u == '%') {
		*unit = sp_unit_get_by_abbreviation ("%");
	}

	return v;
}

gint
sp_svg_write_length (gchar * buf, gint buflen, gdouble val, const SPUnit *unit)
{
	if (unit->base != SP_UNIT_USERSPACE) {
		return snprintf (buf, buflen, "%g%s", val, unit->abbr);
	} else {
		return snprintf (buf, buflen, "%g", val);
	}
}

gdouble
sp_svg_read_percentage (const gchar * str, gdouble def)
{
	char * u;
	gdouble v;

	if (str == NULL) return def;

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
	return sp_svg_write_length (buf, buflen, val * 100.0, sp_unit_get_by_abbreviation ("%"));
}


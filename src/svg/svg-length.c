#define __SP_SVG_LENGTH_C__

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
	if ((const guchar *) e == str) return FALSE;
	*val = v;
	return TRUE;
}

gboolean
sp_svg_read_number_d (const guchar *str, gdouble *val)
{
	gchar *e;
	gdouble v;

	if (!str) return FALSE;
	v = strtod (str, &e);
	if ((const guchar *) e == str) return FALSE;
	*val = v;
	return TRUE;
}

/* Length */

gboolean
sp_svg_length_read (const unsigned char *str, SPSVGLength *length)
{
	unsigned long unit;
	float value, computed;

	if (!str) return FALSE;

	if (!sp_svg_length_read_lff (str, &unit, &value, &computed)) return FALSE;

	length->set = TRUE;
	length->unit = unit;
	length->value = value;
	length->computed = computed;

	return TRUE;
}

#define UVAL(a,b) (((guint) (a) << 8) | (guint) (b))

gboolean
sp_svg_length_read_lff (const unsigned char *str, unsigned long *unit, float *val, float *computed)
{
	const guchar *e;
	gfloat v;

	if (!str) return FALSE;
	v = strtod (str, (gchar **) &e);
	if (e == str) return FALSE;
	if (!e[0]) {
		/* Unitless */
		if (unit) *unit = SP_SVG_UNIT_NONE;
		if (val) *val = v;
		if (computed) *computed = v;
		return TRUE;
	} else if (!isalnum (e[0])) {
		/* Unitless or percent */
		if (e[0] == '%') {
			/* Percent */
			if (e[1] && isalnum (e[1])) return FALSE;
			if (unit) *unit = SP_SVG_UNIT_PERCENT;
			if (val) *val = v * 0.01;
			return TRUE;
		} else {
			if (unit) *unit = SP_SVG_UNIT_NONE;
			if (val) *val = v;
			if (computed) *computed = v;
			return TRUE;
		}
	} else if (e[1] && !isalnum (e[2])) {
		guint uval;
		/* Units */
		uval = UVAL (e[0], e[1]);
		switch (uval) {
		case UVAL('p','x'):
			if (unit) *unit = SP_SVG_UNIT_PX;
			if (computed) *computed = v * 1.0;
			break;
		case UVAL('p','t'):
			if (unit) *unit = SP_SVG_UNIT_PT;
			if (computed) *computed = v * 1.25;
			break;
		case UVAL('p','c'):
			if (unit) *unit = SP_SVG_UNIT_PC;
			if (computed) *computed = v * 15.0;
			break;
		case UVAL('m','m'):
			if (unit) *unit = SP_SVG_UNIT_MM;
			if (computed) *computed = v * 3.543307;
			break;
		case UVAL('c','m'):
			if (unit) *unit = SP_SVG_UNIT_CM;
			if (computed) *computed = v * 35.43307;
			break;
		case UVAL('i','n'):
			if (unit) *unit = SP_SVG_UNIT_IN;
			if (computed) *computed = v * 90.0;
			break;
		case UVAL('e','m'):
			if (unit) *unit = SP_SVG_UNIT_EM;
			break;
		case UVAL('e','x'):
			if (unit) *unit = SP_SVG_UNIT_EX;
			break;
		default:
			/* Invalid */
			return FALSE;
			break;
		}
		if (val) *val = v;
		return TRUE;
	}

	/* Invalid */
	return FALSE;
}

void
sp_svg_length_unset (SPSVGLength *length, unsigned long unit, float value, float computed)
{
	length->set = FALSE;
	length->unit = unit;
	length->value = value;
	length->computed = computed;
}

#if 0
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
#endif

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


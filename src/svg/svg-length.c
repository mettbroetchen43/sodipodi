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

unsigned int
sp_svg_number_read_f (const unsigned char *str, float *val)
{
	char *e;
	float v;

	if (!str) return 0;
	v = strtod (str, &e);
	if ((const unsigned char *) e == str) return 0;
	*val = v;
	return 1;
}

unsigned int
sp_svg_number_read_d (const unsigned char *str, double *val)
{
	char *e;
	double v;

	if (!str) return 0;
	v = strtod (str, &e);
	if ((const unsigned char *) e == str) return 0;
	*val = v;
	return 1;
}

/* Length */

unsigned int
sp_svg_length_read (const unsigned char *str, SPSVGLength *length)
{
	unsigned long unit;
	float value, computed;

	if (!str) return 0;

	if (!sp_svg_length_read_lff (str, &unit, &value, &computed)) return 0;

	length->set = 1;
	length->unit = unit;
	length->value = value;
	length->computed = computed;

	return 1;
}

#define UVAL(a,b) (((unsigned int) (a) << 8) | (unsigned int) (b))

unsigned int
sp_svg_length_read_lff (const unsigned char *str, unsigned long *unit, float *val, float *computed)
{
	const unsigned char *e;
	float v;

	if (!str) return 0;
	v = strtod (str, (char **) &e);
	if (e == str) return 0;
	if (!e[0]) {
		/* Unitless */
		if (unit) *unit = SP_SVG_UNIT_NONE;
		if (val) *val = v;
		if (computed) *computed = v;
		return 1;
	} else if (!isalnum (e[0])) {
		/* Unitless or percent */
		if (e[0] == '%') {
			/* Percent */
			if (e[1] && isalnum (e[1])) return 0;
			if (unit) *unit = SP_SVG_UNIT_PERCENT;
			if (val) *val = v * 0.01;
			return 1;
		} else {
			if (unit) *unit = SP_SVG_UNIT_NONE;
			if (val) *val = v;
			if (computed) *computed = v;
			return 1;
		}
	} else if (e[1] && !isalnum (e[2])) {
		unsigned int uval;
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
			return 0;
			break;
		}
		if (val) *val = v;
		return 1;
	}

	/* Invalid */
	return 0;
}

void
sp_svg_length_unset (SPSVGLength *length, unsigned long unit, float value, float computed)
{
	length->set = 0;
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

#if 0
int
sp_svg_write_length (char *buf, gint buflen, gdouble val, const SPUnit *unit)
{
	if (unit->base != SP_UNIT_USERSPACE) {
		return snprintf (buf, buflen, "%g%s", val, unit->abbr);
	} else {
		return snprintf (buf, buflen, "%g", val);
	}
}
#endif

double
sp_svg_read_percentage (const char * str, double def)
{
	char * u;
	double v;

	if (str == NULL) return def;

	v = strtod (str, &u);
	while (isspace (*u)) {
		if (*u == '\0') return v;
		u++;
	}
	if (*u == '%') v /= 100.0;

	return v;
}

int
sp_svg_write_percentage (char * buf, int buflen, double val)
{
	return snprintf (buf, buflen, "%g%%", val);
}


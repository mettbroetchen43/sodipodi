#define __SP_SVG_LENGTH_C__

/*
 * SVG data parser
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * This code is in public domain
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <libarikkei/arikkei-strlib.h>

#include "svg.h"

#ifdef WIN32
#define strcasecmp _stricmp
#endif

unsigned int
sp_svg_boolean_read (const unsigned char *str, unsigned int *val)
{
	unsigned int v;
	char *e;
	if (!str || !*str) return 0;
	if (!val) return 0;
	if (!strcasecmp (str, "true") || !strcasecmp (str, "yes")) {
		*val = 1;
		return 1;
	}
	if (!strcasecmp (str, "false") || !strcasecmp (str, "no")) {
		*val = 0;
		return 1;
	}
	v = strtoul (str, &e, 10);
	if ((const unsigned char *) e != str) {
		*val = v;
		return 1;
	}
	return 0;
}

unsigned int
sp_svg_number_read_d (const unsigned char *str, double *val)
{
	unsigned int len;
	len = arikkei_strtod_exp (str, 256, val);
	return len != 0;
}

unsigned int
sp_svg_number_read_f (const unsigned char *str, float *val)
{
	double dval;
	unsigned int len;
	len = arikkei_strtod_exp (str, 256, &dval);
	if (!len) return 0;
	*val = dval;
	return 1;
}

double
sp_svg_atof (const unsigned char *str)
{
	double val;
	val = 0.0;
	arikkei_strtod_exp (str, 256, &val);
	return val;
}

unsigned int
sp_svg_number_write_i (unsigned char *buf, int val)
{
	return arikkei_itoa (buf, 256, val);
}

unsigned int
sp_svg_number_write_d (unsigned char *buf, double val, unsigned int tprec, unsigned int fprec, unsigned int padf)
{
	return arikkei_dtoa_simple (buf, 256, val, tprec, fprec, padf);
}

unsigned int
sp_svg_number_write_de (unsigned char *buf, double val, unsigned int tprec, unsigned int padf)
{
	return arikkei_dtoa_exp (buf, 256, val, tprec, padf);
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
	double dval;
	unsigned int len;
	float v;

	if (!str) return 0;
	len = arikkei_strtod_exp (str, 256, &dval);
	if (!len) return 0;
	v = dval;
	e = str + len;
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
sp_svg_length_set (SPSVGLength *length, unsigned long unit, float value, float computed)
{
	length->set = 1;
	length->unit = unit;
	length->value = value;
	length->computed = computed;
}

void
sp_svg_length_unset (SPSVGLength *length, unsigned long unit, float value, float computed)
{
	length->set = 0;
	length->unit = unit;
	length->value = value;
	length->computed = computed;
}

void
sp_svg_length_update (SPSVGLength *length, double em, double ex, double scale)
{
	if (length->unit == SP_SVG_UNIT_EM) {
		length->computed = length->value * em;
	} else if (length->unit == SP_SVG_UNIT_EX) {
		length->computed = length->value * ex;
	} else if (length->unit == SP_SVG_UNIT_PERCENT) {
		length->computed = length->value * scale;
	}
}

double
sp_svg_read_percentage (const char * str, double def)
{
	unsigned int len;
	const char * u;
	double v;

	if (str == NULL) return def;

	len = arikkei_strtod_exp ((const unsigned char *) str, 256, &v);
	if (!len) return def;
	u = str + len;
	while (isspace (*u)) {
		if (*u == '\0') return v;
		u++;
	}
	if (*u == '%') v /= 100.0;

	return v;
}

int
sp_svg_write_percentage (char *buf, int buflen, double val)
{
	unsigned char c[32];
	unsigned int len;
	if (buflen < 1) return 0;
	len = arikkei_dtoa_simple (c, 32, val * 100.0, 4, 1, 0);
	if (len < (buflen - 1)) {
		strcpy (buf, c);
		buf[len++] = '%';
		buf[len] = '\0';
	} else {
		buf[len] = '\0';
	}
	return len;
}

/* Read list of coords (unitless numbers) */

static unsigned int
sp_svg_coords_read (const unsigned char *str, float *coords, unsigned int *ncoords, unsigned int maxcoords)
{
	const unsigned char *p;
	unsigned int count, valid;
	p = str;
	count = 0;
	/* Skip initial whitespace */
	while (*p && isspace (*p)) p += 1;
	valid = 1;
	while (*p) {
		const unsigned char *p0;
		unsigned int len;
		double val;
		if (count > 0) {
			/* We are in the middle of coords list */
			/* Skip comma-wsp */
			p0 = p;
			while (*p && isspace (*p)) p += 1;
			if (!*p) break;
			valid = 0;
			if (*p == ',') p += 1;
			while (*p && isspace (*p)) p += 1;
			if (p == p0) break;
		}
		valid = 0;
		/* Parse coordinate */
		len = arikkei_strtod_exp (p, 256, &val);
		if (len < 1) break;
		p += len;
		/* If we got here everything was valid */
		if (count >= maxcoords) return 0;
		valid = 1;
		if (coords) coords[count] = val;
		count += 1;
	}
	if (valid && ncoords) *ncoords = count;
	return valid;
}

/* Read list of points for <polyline> or <polygon> */

unsigned int
sp_svg_points_read (const unsigned char *str, float *coords, unsigned int *ncoords)
{
	unsigned int count;
	if (!str) return 0;
	if (!sp_svg_coords_read (str, coords, &count, 0x80000000)) return 0;
	if (count & 0x1) return 0;
	if (ncoords) *ncoords = count;
	return 1;
}

unsigned int
sp_svg_viewbox_read (const unsigned char *str, SPSVGViewBox *viewbox)
{
	unsigned int count;
	float coords[4];
	if (!str) return 0;
	if (!sp_svg_coords_read (str, coords, &count, 4)) return 0;
	if (count != 4) return 0;
	viewbox->x0 = coords[0];
	viewbox->y0 = coords[1];
	viewbox->x1 = viewbox->x0 + coords[2];
	viewbox->y1 = viewbox->y0 + coords[3];
	return 1;
}


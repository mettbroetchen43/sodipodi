#define __ARIKKEI_STRLIB_C__

/*
 * Arikkei
 *
 * Basic datatypes and code snippets
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 *
 */

#include <math.h>
#include <string.h>
#include <assert.h>

#include "arikkei-strlib.h"

unsigned int
arikkei_strtod_simple (const unsigned char *str, unsigned int len, double *val)
{
	const unsigned char *p;
	double sign, integra, fract;
	unsigned int valid;
	p = str;
	valid = 0;
	/* NULL string */
	if (!p) return 0;
	/* Skip space */
	while (*p && (*p == ' ')) p += 1;
	/* Empty string */
	if (!*p) return 0;
	/* Sign */
	if (*p == '-') {
		sign = -1.0;
		p += 1;
	} else {
		if (*p == '+') p += 1;
		sign = 1.0;
	}
	/* Value */
	/* Integra */
	integra = 0.0;
	fract = 0.0;
	while (*p && (*p >= '0') && (*p <= '9')) {
		integra = integra * 10.0 + (double) (*p - '0');
		valid = 1;
		p += 1;
	}
	if (*p == '.') {
		double denom;
		/* Fract */
		p += 1;
		denom = 1.0;
		while (*p && (*p >= '0') && (*p <= '9')) {
			fract = fract * 10.0 + (double) (*p - '0');
			denom *= 10.0;
			valid = 1;
			p += 1;
		}
		fract = fract / denom;
	}
	if (!valid) return 0;
	*val = sign * (integra + fract);

	assert ((*val > -1e17) && (*val < 1e17));

	return p - str;
}

unsigned int
arikkei_strtod_exp (const unsigned char *str, unsigned int len, double *val)
{
	const unsigned char *p;
	double rval;
	unsigned int slen;
	p = str;
	slen = arikkei_strtod_simple (p, len, &rval);
	if (!slen) return 0;
	p += slen;
	if ((*p == 'e') || (*p == 'E')) {
		double exval;
		unsigned int exlen;
		exlen = arikkei_strtod_simple (p + 1, (str + len) - (p + 1), &exval);
		if (exlen) {
			p += 1;
			p += exlen;
			rval = rval * pow (10.0, exval);
		}
	}
	*val = rval;

	assert ((*val > -1e17) && (*val < 1e17));

	return p - str;
}

unsigned int
arikkei_itoa (unsigned char *buf, unsigned int len, int val)
{
	char c[32];
	int p, i;
	p = 0;
	if (val < 0) {
		buf[p++] = '-';
		val = -val;
	}
	i = 0;
	do {
		c[32 - (++i)] = '0' + (val % 10);
		val /= 10;
	} while (val > 0);
	memcpy (buf + p, &c[32 - i], i);
	p += i;
	buf[p] = 0;
	return p;
}

unsigned int
arikkei_dtoa_simple (unsigned char *buf, unsigned int len, double val,
		     unsigned int tprec, unsigned int fprec, unsigned int padf)
{
	double dival, fval, epsilon;
	int idigits, ival, ilen, i;
	i = 0;
	if (val < 0.0) {
		buf[i++] = '-';
		val = fabs (val);
	}
	/* Determine number of integral digits */
	if (val >= 1.0) {
		idigits = (int) floor (log10 (val));
	} else {
		idigits = 0;
	}

	/* Determine the actual number of fractional digits */
	if ((tprec - idigits) > fprec) fprec = tprec - idigits;
	/* Find epsilon */
	epsilon = 0.5 * pow (10.0, - (double) fprec);
	/* Round value */
	val += epsilon;
	/* Extract integral and fractional parts */
	dival = floor (val);
	ival = (int) dival;
	fval = val - dival;
	/* Write integra */
	ilen = arikkei_itoa (buf + i, len - i, ival);
	i += ilen;
	tprec -= ilen;
	if ((fprec > 0) && (padf || (fval > epsilon))) {
		buf[i++] = '.';
		while ((fprec > 0) && (padf || (fval > epsilon))) {
			fval *= 10.0;
			dival = floor (fval);
			fval -= dival;
			buf[i++] = '0' + (int) dival;
			fprec -= 1;
		}

	}
	buf[i] = 0;
	return i;
}

unsigned int
arikkei_dtoa_exp (unsigned char *buf, unsigned int len, double val,
		  unsigned int tprec, unsigned int padf)
{
	if ((val == 0.0) || ((fabs (val) >= 0.1) && (fabs(val) < 10000000))) {
		return arikkei_dtoa_simple (buf, len, val, tprec, 0, padf);
	} else {
		double eval;
		int p;
		eval = floor (log10 (fabs (val)));
		val = val / pow (10.0, eval);
		p = arikkei_dtoa_simple (buf, len, val, tprec, 0, padf);
		buf[p++] = 'e';
		p += arikkei_itoa (buf + p, len - p, (int) eval);
		return p;
	}
}


#ifndef __ARIKKEI_STRLIB_H__
#define __ARIKKEI_STRLIB_H__

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

unsigned int arikkei_strtod_simple (const unsigned char *str, unsigned int len, double *val);
unsigned int arikkei_strtod_exp (const unsigned char *str, unsigned int len, double *val);

/*
 * tprec - minimum number of significant digits
 * fprec - minimum number of fractional digits
 * padf - pad end with zeroes to get tprec or fprec
 *
 */

unsigned int arikkei_itoa (unsigned char *buf, unsigned int len, int val);
unsigned int arikkei_dtoa_simple (unsigned char *buf, unsigned int len, double val,
				  unsigned int tprec, unsigned int fprec, unsigned int padf);
unsigned int arikkei_dtoa_exp (unsigned char *buf, unsigned int len, double val,
			       unsigned int tprec, unsigned int padf);

#endif

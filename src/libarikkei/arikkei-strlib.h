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


unsigned int arikkei_unicode_utf8_bytes (unsigned int uval);
unsigned int arikkei_utf8_strlen (const unsigned char *str);
unsigned int arikkei_utf8_ucs2_strcpy (const unsigned char *s, unsigned short *d);
unsigned short *arikkei_utf8_ucs2_strdup (const unsigned char *s);
unsigned int arikkei_ucs2_strlen (const unsigned short *str);
unsigned int arikkei_ucs2_utf8_strcpy (const unsigned short *s, unsigned char *d);
unsigned char *arikkei_ucs2_utf8_strdup (const unsigned short *s);
unsigned int arikkei_ucs2_utf8_bytelen (const unsigned short *str);

unsigned int arikkei_ucs2_strncpy (const unsigned short *s, unsigned short *d, unsigned int maxlen);

#endif

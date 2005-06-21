#ifndef __NR_TYPES_H__
#define __NR_TYPES_H__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_INTTYPES_H
/* ISO C99 headers */
#include <inttypes.h>
typedef int8_t NRByte;
typedef uint8_t NRUByte;
typedef int16_t NRShort;
typedef uint16_t NRUShort;
typedef int32_t NRLong;
typedef uint32_t NRULong;
#else
#ifdef HAVE_NR_CONFIG_H
/* Automake based systems */
#include <libnr/nr_config.h>
#else
/* Currently only Win32 */
typedef signed char NRByte;
typedef unsigned char NRUByte;
typedef signed short NRShort;
typedef unsigned short NRUShort;
typedef signed int NRLong;
typedef unsigned int NRULong;
#endif
#endif

typedef struct _NRMatrixD NRMatrixD;
typedef struct _NRMatrixF NRMatrixF;
typedef struct _NRPointD NRPointD;
typedef struct _NRPointF NRPointF;
typedef struct _NRPointL NRPointL;
typedef struct _NRPointS NRPointS;
typedef struct _NRRectD NRRectD;
typedef struct _NRRectF NRRectF;
typedef struct _NRRectL NRRectL;
typedef struct _NRRectS NRRectS;

struct _NRMatrixD {
	double c[6];
};

struct _NRMatrixF {
	float c[6];
};

struct _NRPointD {
	double x, y;
};

struct _NRPointF {
	float x, y;
};

struct _NRPointL {
	NRLong x, y;
};

struct _NRPointS {
	NRShort x, y;
};

struct _NRRectD {
	double x0, y0, x1, y1;
};

struct _NRRectF {
	float x0, y0, x1, y1;
};

struct _NRRectL {
	NRLong x0, y0, x1, y1;
};

struct _NRRectS {
	NRShort x0, y0, x1, y1;
};

#ifdef __cplusplus
};
#endif

#endif

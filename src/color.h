#ifndef __SP_COLOR_H__
#define __SP_COLOR_H__

/*
 * SPColor, SPColorSpace
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#include <glib.h>
#include "forward.h"

typedef enum {
	SP_COLORSPACE_INVALID,
	SP_COLORSPACE_NONE,
	SP_COLORSPACE_UNKNOWN,
	SP_COLORSPACE_RGB,
	SP_COLORSPACE_CMYK
} SPColorSpaceType;

struct _SPColor {
	const SPColorSpace *colorspace;
	union {
		gfloat c[4];
	} v;
};

SPColorSpaceType sp_color_get_colorspace_type (SPColor *color);

void sp_color_copy (SPColor *dst, SPColor *src);

void sp_color_set_rgb_float (SPColor *color, gfloat r, gfloat g, gfloat b);
void sp_color_set_rgb_rgba32 (SPColor *color, guint32 value);

void sp_color_set_cmyk_float (SPColor *color, gfloat c, gfloat m, gfloat y, gfloat k);

guint32 sp_color_get_rgba32_ualpha (SPColor *color, guint alpha);
guint32 sp_color_get_rgba32_falpha (SPColor *color, gfloat alpha);

void sp_color_get_rgb_floatv (SPColor *color, gfloat *rgb);
void sp_color_get_cmyk_floatv (SPColor *color, gfloat *cmyk);

END_GNOME_DECLS

#endif


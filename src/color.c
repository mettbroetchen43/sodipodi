#define __SP_COLOR_C__

/*
 * SPColor, SPColorSpace
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <math.h>
#include "color.h"

struct _SPColorSpace {
	guchar *name;
};

static void sp_color_rgb_from_cmyk (gfloat *rgb, gfloat *cmyk);
static void sp_color_cmyk_from_rgb (gfloat *rgb, gfloat *cmyk);

static const SPColorSpace RGB = {"RGB"};
static const SPColorSpace CMYK = {"CMYK"};

SPColorSpaceType
sp_color_get_colorspace_type (SPColor *color)
{
	g_return_val_if_fail (color != NULL, SP_COLORSPACE_INVALID);

	if (color->colorspace == &RGB) return SP_COLORSPACE_RGB;
	if (color->colorspace == &CMYK) return SP_COLORSPACE_CMYK;

	return SP_COLORSPACE_UNKNOWN;
}

void
sp_color_copy (SPColor *dst, SPColor *src)
{
	g_return_if_fail (dst != NULL);
	g_return_if_fail (src != NULL);

	*dst = *src;
}

void
sp_color_set_rgb_float (SPColor *color, gfloat r, gfloat g, gfloat b)
{
	g_return_if_fail (color != NULL);
	g_return_if_fail (r >= 0.0);
	g_return_if_fail (r <= 1.0);
	g_return_if_fail (g >= 0.0);
	g_return_if_fail (g <= 1.0);
	g_return_if_fail (b >= 0.0);
	g_return_if_fail (b <= 1.0);

	color->colorspace = &RGB;
	color->v.c[0] = r;
	color->v.c[1] = g;
	color->v.c[2] = b;
}

void
sp_color_set_rgb_rgba32 (SPColor *color, guint32 value)
{
	g_return_if_fail (color != NULL);

	color->colorspace = &RGB;
	color->v.c[0] = (value >> 24) / 255.0;
	color->v.c[1] = ((value >> 16) & 0xff) / 255.0;
	color->v.c[2] = ((value >> 8) & 0xff) / 255.0;
}

void
sp_color_set_cmyk_float (SPColor *color, gfloat c, gfloat m, gfloat y, gfloat k)
{
	g_return_if_fail (color != NULL);
	g_return_if_fail (c >= 0.0);
	g_return_if_fail (c <= 1.0);
	g_return_if_fail (m >= 0.0);
	g_return_if_fail (m <= 1.0);
	g_return_if_fail (y >= 0.0);
	g_return_if_fail (y <= 1.0);
	g_return_if_fail (k >= 0.0);
	g_return_if_fail (k <= 1.0);

	color->colorspace = &CMYK;
	color->v.c[0] = c;
	color->v.c[1] = m;
	color->v.c[2] = y;
	color->v.c[3] = k;
}

guint32
sp_color_get_rgba32_ualpha (SPColor *color, guint alpha)
{
	guint32 rgba;

	g_return_val_if_fail (color != NULL, 0x0);
	g_return_val_if_fail (alpha <= 0xff, 0x0);

	if (color->colorspace == &RGB) {
		rgba = ((guint) floor (color->v.c[0] * 255.9999) << 24) |
			((guint) floor (color->v.c[1] * 255.9999) << 16) |
			((guint) floor (color->v.c[2] * 255.9999) << 8) |
			alpha;
	} else {
		gfloat rgb[3];
		sp_color_get_rgb_floatv (color, rgb);
		rgba = ((guint) floor (rgb[0] * 255.9999) << 24) |
			((guint) floor (rgb[1] * 255.9999) << 16) |
			((guint) floor (rgb[2] * 255.9999) << 8) |
			alpha;
	}

	return rgba;
}

guint32
sp_color_get_rgba32_falpha (SPColor *color, gfloat alpha)
{
	g_return_val_if_fail (color != NULL, 0x0);
	g_return_val_if_fail (alpha >= 0.0, 0x0);
	g_return_val_if_fail (alpha <= 1.0, 0x0);

	return sp_color_get_rgba32_ualpha (color, (guint) floor (alpha * 255.9999));
}

void
sp_color_get_rgb_floatv (SPColor *color, gfloat *rgb)
{
	g_return_if_fail (color != NULL);
	g_return_if_fail (rgb != NULL);

	if (color->colorspace == &RGB) {
		rgb[0] = color->v.c[0];
		rgb[1] = color->v.c[1];
		rgb[2] = color->v.c[2];
	} else if (color->colorspace == &CMYK) {
		sp_color_rgb_from_cmyk (rgb, color->v.c);
	}
}

void
sp_color_get_cmyk_floatv (SPColor *color, gfloat *cmyk)
{
	g_return_if_fail (color != NULL);
	g_return_if_fail (cmyk != NULL);

	if (color->colorspace == &CMYK) {
		cmyk[0] = color->v.c[0];
		cmyk[1] = color->v.c[1];
		cmyk[2] = color->v.c[2];
		cmyk[3] = color->v.c[3];
	} else if (color->colorspace == &RGB) {
		sp_color_cmyk_from_rgb (cmyk, color->v.c);
	}
}

static void
sp_color_rgb_from_cmyk (gfloat *rgb, gfloat *cmyk)
{
	gdouble c, m, y, k, kd;

	c = cmyk[0];
	m = cmyk[1];
	y = cmyk[2];
	k = cmyk[3];

	kd = 1.0 - k;

	c = c * kd;
	m = m * kd;
	y = y * kd;

	c = c + k;
	m = m + k;
	y = y + k;

	rgb[0] = 1.0 - c;
	rgb[1] = 1.0 - m;
	rgb[2] = 1.0 - y;
}

static void
sp_color_cmyk_from_rgb (gfloat *rgb, gfloat *cmyk)
{
	gfloat c, m, y, k, kd;

	c = 1.0 - rgb[0];
	m = 1.0 - rgb[1];
	y = 1.0 - rgb[2];
	k = MIN (MIN (c, m), y);

	c = c - k;
	m = m - k;
	y = y - k;

	kd = 1.0 - k;

	if (kd > 1e-9) {
		c = c / kd;
		m = m / kd;
		y = y / kd;
	}

	cmyk[0] = c;
	cmyk[1] = m;
	cmyk[2] = y;
	cmyk[3] = k;
}




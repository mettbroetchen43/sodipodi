#ifndef __NR_PLAIN_STUFF_H__
#define __NR_PLAIN_STUFF_H__

/*
 * Miscellaneous simple rendering utilities
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <glib.h>
#include "nr-buffers.h"

#define NR_RGBA32_R(v) (((v) >> 24) & 0xff)
#define NR_RGBA32_G(v) (((v) >> 16) & 0xff)
#define NR_RGBA32_B(v) (((v) >> 8) & 0xff)
#define NR_RGBA32_A(v) ((v) & 0xff)

/* Pixel compositing algorithms */
#define PREMUL(c,a) (((c) * (a) + 127) / 255)
#define COMPOSENNN_A7(fc,fa,bc,ba,a) (((255 - (fa)) * (bc) * (ba) + (fa) * (fc) * 255 + 127) / a)
#define COMPOSEPNN_A7(fc,fa,bc,ba,a) (((255 - (fa)) * (bc) * (ba) + (fc) * 65025 + 127) / a)
#define COMPOSENNP(fc,fa,bc,ba) (((255 - (fa)) * (bc) * (ba) + (fa) * (fc) * 255 + 32512) / 65025)
#define COMPOSEPNP(fc,fa,bc,ba) (((255 - (fa)) * (bc) * (ba) + (fc) * 65025 + 32512) / 65025)
#define COMPOSENPP(fc,fa,bc,ba) (((255 - (fa)) * (bc) + (fa) * (fc) + 127) / 255)
#define COMPOSEPPP(fc,fa,bc,ba) (((255 - (fa)) * (bc) + (fc) * 255 + 127) / 255)
#define COMPOSEP11(fc,fa,bc) (((255 - (fa)) * (bc) + (fc) * 255 + 127) / 255)
#define COMPOSEN11(fc,fa,bc) (((255 - (fa)) * (bc) + (fc) * (fa) + 127) / 255)

/* Buffers */
/* Renders buffer into another with clip */
void nr_render_buf_buf (NRBuffer *d, gint x, gint y, gint w, gint h, NRBuffer *s, gint sx, gint sy);
void nr_render_buf_buf_alpha (NRBuffer *d, gint x, gint y, gint w, gint h, NRBuffer *s, gint sx, gint sy, guint alpha);
void nr_render_buf_buf_mask (NRBuffer *d, gint x, gint y, gint w, gint h, NRBuffer *s, gint sx, gint sy, NRBuffer *m, gint mx, gint my);
void nr_render_buf_mask_rgba32 (NRBuffer *d, gint x, gint y, gint w, gint h, NRBuffer *m, gint sx, gint sy, guint32 rgba);

void nr_render_r8g8b8_buf (guchar *px, gint rs, gint w, gint h, NRBuffer *s, gint sx, gint sy);

void nr_render_checkerboard_rgb (guchar *px, gint w, gint h, gint rs, gint xoff, gint yoff);
void nr_render_checkerboard_rgb_custom (guchar *px, gint w, gint h, gint rs, gint xoff, gint yoff, guint32 c0, guint32 c1, gint sizep2);

void nr_render_rgba32_rgb (guchar *px, gint w, gint h, gint rs, gint xoff, gint yoff, guint32 c);

void nr_render_rgba32_rgba32 (guchar *px, gint w, gint h, gint rs, const guchar *src, gint srcrs);

/* RGBA target */
/* These are all not premultiplied */
void nr_render_r8g8b8a8_r8g8b8a8_alpha (guchar *px, gint w, gint h, gint rs, const guchar *src, gint srcrs, guint alpha);
void nr_render_r8g8b8a8_rgba32_mask_a8 (guchar *px, gint w, gint h, gint rs, guint32 rgba, const guchar *src, gint srcrs);
void nr_render_r8g8b8a8_r8g8b8a8_mask_a8 (guchar *px, gint w, gint h, gint rs, const guchar *src, gint srcrs, const guchar *mask, gint maskrs);
void nr_render_r8g8b8a8_gray_garbage (guchar *px, gint w, gint h, gint rs);

/* RGB target */
void nr_render_r8g8b8_r8g8b8a8 (guchar *px, gint w, gint h, gint rs, const guchar *src, gint srcrs);

void nr_render_gray_garbage_rgb (guchar *px, gint w, gint h, gint rs);

#endif

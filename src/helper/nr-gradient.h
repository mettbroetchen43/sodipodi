#ifndef __NR_GRADIENT_H__
#define __NR_GRADIENT_H__

/*
 * Gradient rendering utilities
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <glib.h>

#define NR_GRADIENT_VECTOR_LENGTH 1024

typedef enum {
	NR_GRADIENT_SPREAD_PAD,
	NR_GRADIENT_SPREAD_REFLECT,
	NR_GRADIENT_SPREAD_REPEAT
} NRGradientSpreadType;

typedef struct _NRLGradientRenderer NRLGradientRenderer;

typedef void (* NRLGradientRenderFunc) (NRLGradientRenderer *, guchar *, gint, gint, gint, gint, gint);

struct _NRLGradientRenderer {
	NRLGradientRenderFunc render;
	guchar *vector;
	NRGradientSpreadType spread;
	gdouble x0, y0;
	gdouble dx, dy;
};

NRLGradientRenderer *nr_lgradient_renderer_new_r8g8b8a8 (guchar *vector, NRGradientSpreadType spread, gdouble *n2b);
NRLGradientRenderer *nr_lgradient_renderer_new_r8g8b8 (guchar *vector, NRGradientSpreadType spread, gdouble *n2b);
void nr_lgradient_renderer_destroy (NRLGradientRenderer *lgr);
void nr_lgradient_render (NRLGradientRenderer *lgr, guchar *px, gint x0, gint y0, gint width, gint height, gint rs);

#endif

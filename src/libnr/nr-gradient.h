#ifndef __NR_GRADIENT_H__
#define __NR_GRADIENT_H__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

typedef struct _NRLGradientRenderer NRLGradientRenderer;
typedef struct _NRRGradientRenderer NRRGradientRenderer;

#include <libnr/nr-types.h>
#include <libnr/nr-pixblock.h>

#define NR_GRADIENT_VECTOR_LENGTH 1024

enum {
	NR_GRADIENT_SPREAD_PAD,
	NR_GRADIENT_SPREAD_REFLECT,
	NR_GRADIENT_SPREAD_REPEAT
};

/* Linear */

typedef void (* NRLGradientRenderFunc) (NRLGradientRenderer *lgr, NRPixBlock *pb);

struct _NRLGradientRenderer {
	NRLGradientRenderFunc render;
	const unsigned char *vector;
	unsigned int spread;
	float x0, y0;
	float dx, dy;
};

/* v2px is from 0,1 normalized vector space */

NRLGradientRenderer *nr_lgradient_renderer_setup (NRLGradientRenderer *lgr, const unsigned char *cv, unsigned int spread, const NRMatrixF *v2px);

void nr_lgradient_render (NRLGradientRenderer *lgr, NRPixBlock *pb);

/* Radial */

typedef void (* NRRGradientRenderFunc) (NRRGradientRenderer *lgr, NRPixBlock *pb);

struct _NRRGradientRenderer {
	NRRGradientRenderFunc render;
	const unsigned char *vector;
	unsigned int spread;
	NRMatrixF px2gs;
	float cx, cy;
	float fx, fy;
	float r;
};

NRRGradientRenderer *nr_rgradient_renderer_setup (NRRGradientRenderer *rgr,
						  const unsigned char *cv,
						  unsigned int spread,
						  const NRMatrixF *gs2px,
						  float cx, float cy,
						  float fx, float fy,
						  float r);

void nr_rgradient_render (NRRGradientRenderer *rgr, NRPixBlock *pb);

#endif

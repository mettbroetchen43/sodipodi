#ifndef __SP_BEZIER_UTILS_H__
#define __SP_BEZIER_UTILS_H__

/*
 * An Algorithm for Automatically Fitting Digitized Curves
 * by Philip J. Schneider
 * from "Graphics Gems", Academic Press, 1990
 *
 * Authors:
 *   Philip J. Schneider
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1990 Philip J. Schneider
 * Copyright (C) 2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <libnr/nr-types.h>

struct _NRSynthesizer {
	double tolerance2;
	/* Curve points (always 3n + 1) */
	unsigned int numcpts;
	unsigned int sizcpts;
	NRPointF *cpts;
	/* Vector points */
	unsigned int numvpts;
	unsigned int sizvpts;
	NRPointF *vpts;
	float *vdist;
	/* Fitted segment */
	unsigned int fsegs;
	NRPointF fpts[8];
	/* Directions */
	NRPointF d0, d1, d2;
	/* Midpoint */
	unsigned int midpt;
};

void nr_synthesizer_setup (struct _NRSynthesizer *sz, unsigned int maxvpts, double tolerance);

void nr_synthesizer_release (struct _NRSynthesizer *sz);

void nr_synthesizer_begin (struct _NRSynthesizer *sz, float x, float y, float distance);
void nr_synthesizer_add_point (struct _NRSynthesizer *sz, float x, float y, float distance);

/* Bezier approximation utils */

int sp_bezier_fit_cubic (NRPointF *bezier, const NRPointF *data, int len, double error);

int sp_bezier_fit_cubic_r (NRPointF *bezier, const NRPointF *data, int len, double error, int max_depth);

int sp_bezier_fit_cubic_full (NRPointF *bezier, const NRPointF *data, int len,
			      const NRPointF *tHat1, NRPointF *d1, const NRPointF *tHat2,
			      unsigned int *midpt, double error, int max_depth);


/* Data array */

unsigned int sp_darray_left_tangent (const NRPointF *d, int first, int length, NRPointF *tHat1);
unsigned int sp_darray_right_tangent (const NRPointF *d, int last, int length, NRPointF *tHat2);
void sp_darray_center_tangent (const NRPointF *d, int center, NRPointF *tHatCenter);

#endif /* __SP_BEZIER_UTILS_H__ */

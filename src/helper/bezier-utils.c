#define __SP_BEZIER_UTILS_C__

/*
 * Bezier interpolation for sodipodi drawing code
 *
 * Original code published in:
 *   An Algorithm for Automatically Fitting Digitized Curves
 *   by Philip J. Schneider
 *  "Graphics Gems", Academic Press, 1990
 *
 * Authors:
 *   Philip J. Schneider
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1990 Philip J. Schneider
 * Copyright (C) 2001 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define notBEZIER_DEBUG

#include <math.h>
#include <malloc.h>
#include <assert.h>

#include <libnr/nr-macros.h>
#include <libnr/nr-values.h>

#include "bezier-utils.h"

#define BLOCK_CPTS 32
#define MAXPOINTS 1024

#define V2Dot(a,b) ((a)->x * (b)->x + (a)->y * (b)->y)

#ifdef BEZIER_DEBUG
#ifdef __USE_ISOC99
#define ASSERT_VAL(v) assert (isfinite (v) && ((v) > -NR_HUGE_F) && ((v) < NR_HUGE_F))
#else
#define ASSERT_VAL(v) assert (((v) > -NR_HUGE_F) && ((v) < NR_HUGE_F))
#endif
#else
#define ASSERT_VAL(v)
#endif

/* Forward declarations */
static void GenerateBezier (NRPointF *b, const NRPointF *d, double *uPrime, int len,
			    const NRPointF *tHat1, const NRPointF *tHat2);
static void Reparameterize (const NRPointF *d, double *u, int first, int last, NRPointF *bezCurve);
static double NewtonRaphsonRootFind (NRPointF *Q, NRPointF P, double u);
static void BezierII (int degree, const NRPointF *bezCurve, double t, NRPointF *result);

static void ChordLengthParameterize (const NRPointF *d, double *u, int len);
static double ComputeMaxError (const NRPointF *d, double *u, int len, const NRPointF *bezCurve, int *splitPoint);

/* Vector operations */

static void nr_vector_d_add (NRPointD *d, const NRPointD *a, const NRPointD *b);
static void nr_vector_f_sub (NRPointF *d, const NRPointF *a, const NRPointF *b);
static void nr_vector_sub_dfd (NRPointD *d, const NRPointF *a, const NRPointD *b);
static void nr_vector_scale_df (NRPointD *d, const NRPointF *v, double s);

static unsigned int sp_vector_normalize (NRPointF *v);
static void sp_vector_negate (NRPointF *v);

void
nr_synthesizer_setup (struct _NRSynthesizer *sz, unsigned int maxvpts, double tolerance2)
{
	sz->tolerance2 = tolerance2;

	sz->numcpts = 0;
	sz->sizcpts = BLOCK_CPTS;
	sz->cpts = (NRPointF *) malloc (sz->sizcpts * sizeof (NRPointF));
	sz->numvpts = 0;
	sz->sizvpts = MAX (maxvpts, 3);
	sz->vpts = (NRPointF *) malloc (sz->sizvpts * sizeof (NRPointF));
	sz->vdist = (float *) malloc (sz->sizvpts * sizeof (float));

	sz->fsegs = 0;

	sz->midpt = 0;
}

void
nr_synthesizer_release (struct _NRSynthesizer *sz)
{
	if (sz->vdist) free (sz->vdist);
	if (sz->vpts) free (sz->vpts);
	if (sz->cpts) free (sz->cpts);
}

void
nr_synthesizer_begin (struct _NRSynthesizer *sz, float x, float y, float distance)
{
	sz->vpts[0].x = x;
	sz->vpts[0].y = y;
	sz->vdist[0] = distance;
	sz->numvpts = 1;
	sz->cpts[0].x = x;
	sz->cpts[0].y = y;
	sz->numcpts = 1;

	sz->fsegs = 0;
	sz->midpt = 0;
}

static void
nr_synthesizer_ensure_cspace (struct _NRSynthesizer *sz, unsigned int req)
{
	if ((sz->numcpts + req) >= sz->sizcpts) {
		sz->sizcpts = MAX (sz->sizcpts << 1, sz->numcpts + req);
		sz->cpts = (NRPointF *) realloc (sz->cpts, sz->sizcpts * sizeof (NRPointF));
	}
}

static void
nr_synthesizer_apply_segment (struct _NRSynthesizer *sz)
{
	assert (sz->fsegs > 0);
	assert (sz->fsegs < 3);
	if (sz->fsegs == 1) {
		nr_synthesizer_ensure_cspace (sz, 3);
		sz->cpts[sz->numcpts].x = sz->fpts[1].x;
		sz->cpts[sz->numcpts].y = sz->fpts[1].y;
		sz->cpts[sz->numcpts + 1].x = sz->fpts[2].x;
		sz->cpts[sz->numcpts + 1].y = sz->fpts[2].y;
		sz->cpts[sz->numcpts + 2].x = sz->fpts[3].x;
		sz->cpts[sz->numcpts + 2].y = sz->fpts[3].y;
		sz->numcpts += 3;
		sz->vpts[0] = sz->vpts[sz->numvpts - 1];
		sz->vdist[0] = sz->vdist[sz->numvpts - 1];
		sz->numvpts = 1;
		sz->fsegs = 0;
		sz->d0 = sz->d2;
		sz->midpt = sz->numvpts;
	} else {
		unsigned int i;
		assert (sz->midpt > 0);
		assert (sz->midpt < (sz->numvpts - 1));
		nr_synthesizer_ensure_cspace (sz, 3);
		sz->cpts[sz->numcpts].x = sz->fpts[1].x;
		sz->cpts[sz->numcpts].y = sz->fpts[1].y;
		sz->cpts[sz->numcpts + 1].x = sz->fpts[2].x;
		sz->cpts[sz->numcpts + 1].y = sz->fpts[2].y;
		sz->cpts[sz->numcpts + 2].x = sz->fpts[3].x;
		sz->cpts[sz->numcpts + 2].y = sz->fpts[3].y;
		sz->numcpts += 3;
		for (i = sz->midpt; i < sz->numvpts; i++) {
			sz->vpts[i - sz->midpt] = sz->vpts[i];
			sz->vdist[i - sz->midpt] = sz->vdist[i];
		}
		sz->numvpts -= sz->midpt;
		sz->fpts[0] = sz->fpts[4];
		sz->fpts[1] = sz->fpts[5];
		sz->fpts[2] = sz->fpts[6];
		sz->fpts[3] = sz->fpts[7];
		sz->fsegs = 1;
		sz->d0 = sz->d1;
		sz->midpt = sz->numvpts;
	}
}

static void
nr_synthesizer_get_d0 (struct _NRSynthesizer *sz, NRPointF *d0)
{
	double len;
	len = 0.0;
	if (sz->numcpts > 1) {
		d0->x = sz->cpts[sz->numcpts - 1].x - sz->cpts[sz->numcpts - 2].x;
		d0->y = sz->cpts[sz->numcpts - 1].y - sz->cpts[sz->numcpts - 2].y;
		len = hypot (d0->x, d0->y);
	}
	if (len < NR_EPSILON_F) {
		d0->x = sz->vpts[1].x - sz->vpts[0].x;
		d0->y = sz->vpts[1].y - sz->vpts[0].y;
		len = hypot (d0->x, d0->y);
	}
	if (len > NR_EPSILON_F) {
		d0->x /= len;
		d0->y /= len;
	} else {
		d0->x = 1.0;
		d0->y = 0.0;
	}
}

void
nr_synthesizer_add_point (struct _NRSynthesizer *sz, float x, float y, float distance)
{
	if (sz->numvpts == 0) {
		sz->vpts[sz->numvpts].x = x;
		sz->vpts[sz->numvpts].y = y;
		sz->vdist[sz->numvpts] = distance;
		sz->numvpts += 1;
		nr_synthesizer_ensure_cspace (sz, 1);
		sz->cpts[sz->numcpts].x = x;
		sz->cpts[sz->numcpts].y = y;
		sz->numcpts += 1;
		return;
	}
	/* Check if identical to previous point */
	if ((x == sz->vpts[sz->numvpts - 1].x) && (y == sz->vpts[sz->numvpts - 1].y)) return;
	/* Check if distance is monotonous */
	if (distance <= sz->vdist[sz->numvpts - 1]) return;
	/* Check whether there is room in vpts */
	if (sz->numvpts >= sz->sizvpts) {
		nr_synthesizer_apply_segment (sz);
	}
	/* Append vpoint */
	sz->vpts[sz->numvpts].x = x;
	sz->vpts[sz->numvpts].y = y;
	sz->vdist[sz->numvpts] = distance;
	/* sz->numvpts += 1; */

	if (sz->numcpts == 1) {
		/* Calculate initial direction */
		sz->d0.x = sz->vpts[1].x - sz->vpts[0].x;
		sz->d0.y = sz->vpts[1].y - sz->vpts[0].y;
		sp_vector_normalize (&sz->d0);
	}

	/* Fit and split */
	if (0 && (sz->numvpts == 3) && (sz->vpts[0].x == sz->vpts[2].x) && (sz->vpts[0].y == sz->vpts[2].y)) {
		/* Special case */
	} else {
		NRPointF fpts[8];
		NRPointF d0, d1, d2;
		unsigned int midpt;
		int fsegs;
		/* Initial direction is always calculated */
		d0 = sz->d0;
		/* Calculate directions */
		nr_synthesizer_get_d0 (sz, &d0);
		sp_vector_normalize (&d0);
		d2.x = sz->vpts[sz->numvpts].x - sz->vpts[sz->numvpts - 1].x;
		d2.y = sz->vpts[sz->numvpts].y - sz->vpts[sz->numvpts - 1].y;
		sp_vector_normalize (&d2);
		/* Try to fit */
		fsegs = sp_bezier_fit_cubic_full (fpts, sz->vpts, sz->numvpts + 1,
						  &d0, &d1, &d2, &midpt, sz->tolerance2, 2);
		if (fsegs < 0) {
			nr_synthesizer_apply_segment (sz);
			sz->vpts[sz->numvpts].x = x;
			sz->vpts[sz->numvpts].y = y;
			sz->vdist[sz->numvpts] = distance;
			nr_synthesizer_get_d0 (sz, &d0);
			d2.x = sz->vpts[sz->numvpts].x - sz->vpts[sz->numvpts - 1].x;
			d2.y = sz->vpts[sz->numvpts].y - sz->vpts[sz->numvpts - 1].y;
			sp_vector_normalize (&d2);
			fsegs = sp_bezier_fit_cubic_full (fpts, sz->vpts, sz->numvpts + 1,
							  &d0, &d1, &d2, &midpt, sz->tolerance2, 1);
			/* fixme: This shouldn't happen, but happens nevertheless */
			if (fsegs < 0) {
				nr_synthesizer_apply_segment (sz);
				sz->vpts[sz->numvpts].x = x;
				sz->vpts[sz->numvpts].y = y;
				sz->vdist[sz->numvpts] = distance;
				nr_synthesizer_get_d0 (sz, &d0);
				d2.x = sz->vpts[sz->numvpts].x - sz->vpts[sz->numvpts - 1].x;
				d2.y = sz->vpts[sz->numvpts].y - sz->vpts[sz->numvpts - 1].y;
				sp_vector_normalize (&d2);
				fsegs = sp_bezier_fit_cubic_full (fpts, sz->vpts, sz->numvpts + 1,
								  &d0, &d1, &d2, &midpt, sz->tolerance2, 2);
				assert (fsegs >= 0);
			}
		}
		if (fsegs == 1) {
			unsigned int i;
#ifdef BEZIER_DEBUG
			for (i = 0; i < 4; i++) {
				ASSERT_VAL (fpts[i].x);
				ASSERT_VAL (fpts[i].y);
			}
#endif
			/* Fit into single segment */
			for (i = 0; i < 4; i++) sz->fpts[i] = fpts[i];
			sz->d2 = d2;
		} else if (fsegs == 2) {
			unsigned int i;
#ifdef BEZIER_DEBUG
			for (i = 0; i < 8; i++) {
				ASSERT_VAL (fpts[i].x);
				ASSERT_VAL (fpts[i].y);
			}
#endif
			/* Fit into two segments */
			for (i = 0; i < 8; i++) sz->fpts[i] = fpts[i];
			sz->d1 = d1;
			sz->d2 = d2;
			sz->midpt = midpt;
		}
		sz->fsegs = fsegs;
		sz->numvpts += 1;

		if (fsegs > 0) {
			assert (sz->cpts[sz->numcpts - 1].x == sz->fpts[0].x);
			assert (sz->cpts[sz->numcpts - 1].y == sz->fpts[0].y);
		}
	}
}

#define SP_HUGE 1e5

#include <stdlib.h>

#include <glib.h>

/*
 *  B0, B1, B2, B3 : Bezier multipliers
 */

#define B0(u) ((1.0 - u) * (1.0 - u) * (1.0 - u))
#define B1(u) (3 * u * (1.0 - u) * (1.0 - u))
#define B2(u) (3 * u * u * (1.0 - u))
#define B3(u) (u * u * u)

/*
 * sp_bezier_fit_cubic
 *
 * Fit a single-segment Bezier curve to a set of digitized points
 *
 * Returns number of segments generated or -1 on error
 */

int
sp_bezier_fit_cubic (NRPointF *bezier, const NRPointF *data, int len, double error)
{
	NRPointF tHat1;
	NRPointF tHat2;
	int fill;
#ifdef BEZIER_DEBUG
	unsigned int i;
#endif

	g_return_val_if_fail (bezier != NULL, -1);
	g_return_val_if_fail (data != NULL, -1);
	g_return_val_if_fail (len > 0, -1);

#ifdef BEZIER_DEBUG
	for (i = 0; i < len; i++) {
		ASSERT_VAL (data[i].x);
		ASSERT_VAL (data[i].y);
	}
#endif

	if (len < 2) return 0;

	if ((len == 3) && (data[0].x == data[2].x) && (data[0].y == data[2].y)) {
		g_warning ("Gotcha\n");
		return 0;
	}

	if (!sp_darray_left_tangent (data, 0, len, &tHat1)) return 0;
	if (!sp_darray_right_tangent (data, len - 1, len, &tHat2)) return 0;
	sp_vector_negate (&tHat2);
	
	/* call fit-cubic function without recursion */
	fill = sp_bezier_fit_cubic_full (bezier, data, len, &tHat1, NULL, &tHat2, NULL, error, 1);

	return fill;
}

/*
 *  sp_bezier_fit_cubic_r
 *
 * Fit a multi-segment Bezier curve to a set of digitized points
 *
 * Maximum number of generated segments is 2 ^ (max_depth - 1)
 *
 *    return value:
 *      block size which filled bezier paths by fit-cubic fuction
 *        where 1 block size = 4 * sizeof(NRPointF)
 *      -1 if failed.
 */
int
sp_bezier_fit_cubic_r (NRPointF *bezier, const NRPointF *data, int len, double error, int max_depth)
{
	NRPointF tHat1;
	NRPointF tHat2;
#ifdef BEZIER_DEBUG
	unsigned int i;
#endif
	
	g_return_val_if_fail (bezier != NULL, -1);
	g_return_val_if_fail (data != NULL, -1);
	g_return_val_if_fail (len > 0, -1);
	g_return_val_if_fail (max_depth >= 0, -1);

#ifdef BEZIER_DEBUG
	for (i = 0; i < len; i++) {
		ASSERT_VAL (data[i].x);
		ASSERT_VAL (data[i].y);
	}
#endif

	if (len < 2) return 0;

	sp_darray_left_tangent (data, 0, len, &tHat1);
	sp_darray_right_tangent (data, len - 1, len, &tHat2);
	sp_vector_negate (&tHat2);
	
	/* call fit-cubic function with recursion */
	return sp_bezier_fit_cubic_full (bezier, data, len, &tHat1, NULL, &tHat2, NULL, error, max_depth);
}

int
sp_bezier_fit_cubic_full (NRPointF *bezier, const NRPointF *data, int len,
			  const NRPointF *d0, NRPointF *d1, const NRPointF *d2,
			  unsigned int *midpt, double error, int max_depth)
{
	double *u;		/* Parameter values for point */
	/* double *u_alloca; */ /* Just for memory management */
	/* double *uPrime; */ /* Improved parameter values */
	double maxError;	/* Maximum fitting error */
	int splitPoint;		/* Point to split point set at */
	double iterationError;  /* Error below which you try iterating (squared) */
	int maxIterations = 4;	/* Max times to try iterating */
	/* End direction inversed */
	NRPointF id2;
	
	int i;
	
	g_return_val_if_fail (bezier != NULL, -1);
	g_return_val_if_fail (data != NULL, -1);
	g_return_val_if_fail (len > 0, -1);
	g_return_val_if_fail (d0 != NULL, -1);
	g_return_val_if_fail (d2 != NULL, -1);
	g_return_val_if_fail (max_depth >= 0, -1);

#ifdef BEZIER_DEBUG
	for (i = 0; i < len; i++) {
		ASSERT_VAL (data[i].x);
		ASSERT_VAL (data[i].y);
	}
#endif

	if (len < 2) return 0;

	if ((len == 3) && (data[0].x == data[2].x) && (data[0].y == data[2].y)) {
		return -1;
	}

	id2.x = -d2->x;
	id2.y = -d2->y;

	/* fixme: Find good formula (lauris) */
	iterationError = 8 * error;
	
	if (len == 2) {
		double dist;
		/* We have 2 points, so just draw straight segment */
		dist = hypot (data[len - 1].x - data[0].x, data[len - 1].y - data[0].y) / 3.0;
		bezier[0] = data[0];
		bezier[3] = data[len - 1];
		bezier[1].x = bezier[0].x + d0->x * dist;
		bezier[1].y = bezier[0].y + d0->y * dist;
		bezier[2].x = bezier[3].x - d2->x * dist;
		bezier[2].y = bezier[3].y - d2->y * dist;
		return 1;
	}
	
	/*  Parameterize points, and attempt to fit curve */
	u = alloca (len * sizeof (double));
	ChordLengthParameterize (data, u, len);
	GenerateBezier (bezier, data, u, len, d0, &id2);
	
	/*  Find max deviation of points to fitted curve */
	maxError = ComputeMaxError (data, u, len, bezier, &splitPoint);
	
	if (maxError < error) {
#ifdef BEZIER_DEBUG
		unsigned int i;
		for (i = 0; i < 4; i++) {
			ASSERT_VAL (bezier[i].x);
			ASSERT_VAL (bezier[i].y);
		}
#endif
		return 1;
	}

	/*  If error not too large, try some reparameterization  */
	/*  and iteration */

	/* u_alloca = u; */
	if (maxError < iterationError) {
		for (i = 0; i < maxIterations; i++) {
			/* uPrime = Reparameterize (data, 0, len - 1, u, bezier); */
			Reparameterize (data, u, 0, len - 1, bezier);
			/* GenerateBezier (bezier, data, uPrime, len, tHat1, tHat2); */
			GenerateBezier (bezier, data, u, len, d0, &id2);
			/* maxError = ComputeMaxError(data, uPrime, len, bezier, &splitPoint); */
			maxError = ComputeMaxError(data, u, len, bezier, &splitPoint);
			/* if (u != u_alloca) g_free(u); */
			if (maxError < error) {
#ifdef BEZIER_DEBUG
				unsigned int i;
				for (i = 0; i < 4; i++) {
					ASSERT_VAL (bezier[i].x);
					ASSERT_VAL (bezier[i].y);
				}
#endif
				return 1;
			}
		}
	}
	
	if (max_depth > 1)
	{
		/*
		 *  Fitting failed -- split at max error point and fit recursively
		 */
		NRPointF	tHatCenter; /* Unit tangent vector at splitPoint */
		int  depth1, depth2;
		
		max_depth--;
		
		sp_darray_center_tangent (data, splitPoint, &tHatCenter);
		sp_vector_negate(&tHatCenter);
		depth1 = sp_bezier_fit_cubic_full (bezier, data, splitPoint + 1, d0, NULL, &tHatCenter, NULL,
						   error, max_depth);
		if (depth1 == -1)
		{
#ifdef BEZIER_DEBUG
			g_print ("fit_cubic[1]: fail on max_depth:%d\n", max_depth);
#endif
			return -1;
		}
		sp_vector_negate(&tHatCenter);
		depth2 = sp_bezier_fit_cubic_full (bezier + depth1*4, data + splitPoint, len - splitPoint,
						   &tHatCenter, NULL, &id2, NULL, error, max_depth);
		if (depth2 == -1)
		{
#ifdef BEZIER_DEBUG
			g_print ("fit_cubic[2]: fail on max_depth:%d\n", max_depth);
#endif
			return -1;
		}
		
#ifdef BEZIER_DEBUG
		g_print("fit_cubic: success[depth: %d+%d=%d] on max_depth:%d\n",
			depth1, depth2, depth1+depth2, max_depth+1);
#endif
		if (d1) *d1 = tHatCenter;
		if (midpt) *midpt = splitPoint;

		return depth1 + depth2;
	}
	else
		return -1;
}

/*
 *  GenerateBezier :
 *  Use least-squares method to find Bezier control points for region.
 *
 */
static void
GenerateBezier (NRPointF *bezier, const NRPointF *data, double *uPrime, int len, const NRPointF *tHat1, const NRPointF *tHat2)
{
	int i;
	/* Precomputed rhs for eqn */
	NRPointD A[MAXPOINTS][2];
	/* Matrix C */
	double C[2][2];
	/* Matrix X */
	double X[2];
	/* Determinants of matrices */
	double det_C0_C1, det_C0_X, det_X_C1;
	/* Alpha values, left and right	*/
	double 	alpha_l, alpha_r;
	/* Utility variable */
	NRPointD tmp;

	/* First and last control points of the Bezier curve are */
	/* positioned exactly at the first and last data points */
	bezier[0] = data[0];
	bezier[3] = data[len - 1];

	/* Compute the A's	*/
	for (i = 0; i < len; i++) {
		nr_vector_scale_df (&A[i][0], tHat1, B1 (uPrime[i]));
		nr_vector_scale_df (&A[i][1], tHat2, B2 (uPrime[i]));
	}
	
	/* Create the C and X matrices	*/
	C[0][0] = 0.0;
	C[0][1] = 0.0;
	C[1][0] = 0.0;
	C[1][1] = 0.0;
	X[0]    = 0.0;
	X[1]    = 0.0;
	
	for (i = 0; i < len; i++) {
		NRPointD tmp1, tmp2, tmp3, tmp4;

		C[0][0] += V2Dot(&A[i][0], &A[i][0]);
		C[0][1] += V2Dot(&A[i][0], &A[i][1]);
		C[1][0] = C[0][1];
		C[1][1] += V2Dot(&A[i][1], &A[i][1]);
		
                nr_vector_scale_df (&tmp1, &data[len - 1], B2(uPrime[i]));
                nr_vector_scale_df (&tmp2, &data[len - 1], B3(uPrime[i]));
                nr_vector_d_add (&tmp3, &tmp1, &tmp2);
                nr_vector_scale_df (&tmp1, &data[0], B0(uPrime[i]));
                nr_vector_scale_df (&tmp2, &data[0], B1(uPrime[i]));
                nr_vector_d_add (&tmp4, &tmp2, &tmp3);
                nr_vector_d_add (&tmp2, &tmp1, &tmp4);
		nr_vector_sub_dfd (&tmp, &data[i], &tmp2);
		
		
		X[0] += V2Dot(&A[i][0], &tmp);
		X[1] += V2Dot(&A[i][1], &tmp);
	}
	
	/* Compute the determinants of C and X	*/
	det_C0_C1 = C[0][0] * C[1][1] - C[1][0] * C[0][1];
	det_C0_X  = C[0][0] * X[1]    - C[0][1] * X[0];
	det_X_C1  = X[0]    * C[1][1] - X[1]    * C[0][1];
	
	/* Finally, derive alpha values	*/
	if (fabs (det_C0_C1) < 1e-6) {
		/* det_C0_C1 = (C[0][0] * C[1][1]) * 10e-12; */
		double dist;
		
		dist = hypot (data[len - 1].x - data[0].x, data[len - 1].y - data[0].y) / 3.0;
		
		bezier[1].x = tHat1->x * dist + bezier[0].x;
		bezier[1].y = tHat1->y * dist + bezier[0].y;
		bezier[2].x = tHat2->x * dist + bezier[3].x;
		bezier[2].y = tHat2->y * dist + bezier[3].y;

		return;
	}
	alpha_l = det_X_C1 / det_C0_C1;
	alpha_r = det_C0_X / det_C0_C1;
	
	/*  If alpha negative, use the Wu/Barsky heuristic (see text) */
	/* (if alpha is 0, you get coincident control points that lead to
	 * divide by zero in any subsequent NewtonRaphsonRootFind() call. */
	if ((alpha_l < 1e-6) || (alpha_r < 1e-6)) {
		double dist;
		
		dist = hypot (data[len - 1].x - data[0].x, data[len - 1].y - data[0].y) / 3.0;
		
		bezier[1].x = tHat1->x * dist + bezier[0].x;
		bezier[1].y = tHat1->y * dist + bezier[0].y;
		bezier[2].x = tHat2->x * dist + bezier[3].x;
		bezier[2].y = tHat2->y * dist + bezier[3].y;

		return;
	}
	
	/*  Control points 1 and 2 are positioned an alpha distance out */
	/*  on the tangent vectors, left and right, respectively */
	bezier[1].x = tHat1->x * alpha_l + bezier[0].x;
	bezier[1].y = tHat1->y * alpha_l + bezier[0].y;
	bezier[2].x = tHat2->x * alpha_r + bezier[3].x;
	bezier[2].y = tHat2->y * alpha_r + bezier[3].y;

	ASSERT_VAL (bezier[1].x);
	ASSERT_VAL (bezier[1].y);
	ASSERT_VAL (bezier[2].x);
	ASSERT_VAL (bezier[2].y);
}

/*
 * Given set of points and their parameterization, try to find a better parameterization.
 */

static void
Reparameterize (const NRPointF *d, double *u, int first, int last, NRPointF *bezCurve)
{
	int i;
	
	for (i = first; i <= last; i++) {
		u[i] = NewtonRaphsonRootFind (bezCurve, d[i], u[i]);
	}
}

/*
 *  NewtonRaphsonRootFind :
 *	Use Newton-Raphson iteration to find better root.
 *  Arguments:
 *      Q : Current fitted curve
 *      P : Digitized point
 *      u : Parameter value for "P"
 *  Return value:
 *      Improved u
 */
static double
NewtonRaphsonRootFind (NRPointF *Q, NRPointF P, double u)
{
	double numerator, denominator;
	NRPointF Q1[3], Q2[2];	/*  Q' and Q''			*/
	NRPointF Q_u, Q1_u, Q2_u; /*u evaluated at Q, Q', & Q''	*/
	double uPrime;		/*  Improved u			*/
	int i;

	ASSERT_VAL (u);
	
	/* Compute Q(u)	*/
	BezierII (3, Q, u, &Q_u);
	
	/* Generate control vertices for Q'	*/
	for (i = 0; i <= 2; i++) {
		Q1[i].x = (Q[i+1].x - Q[i].x) * 3.0;
		Q1[i].y = (Q[i+1].y - Q[i].y) * 3.0;
	}
	
	/* Generate control vertices for Q'' */
	for (i = 0; i <= 1; i++) {
		Q2[i].x = (Q1[i+1].x - Q1[i].x) * 2.0;
		Q2[i].y = (Q1[i+1].y - Q1[i].y) * 2.0;
	}
	
	/* Compute Q'(u) and Q''(u)	*/
	BezierII(2, Q1, u, &Q1_u);
	BezierII(1, Q2, u, &Q2_u);
	
	/* Compute f(u)/f'(u) */
	numerator = (Q_u.x - P.x) * (Q1_u.x) + (Q_u.y - P.y) * (Q1_u.y);
	denominator = (Q1_u.x) * (Q1_u.x) + (Q1_u.y) * (Q1_u.y) +
		(Q_u.x - P.x) * (Q2_u.x) + (Q_u.y - P.y) * (Q2_u.y);
	
	/* u = u - f(u)/f'(u) */
	uPrime = u - (numerator/denominator);
	ASSERT_VAL (uPrime);
	return uPrime;
}

/*
 * Evaluate a Bezier curve at a particular parameter value
 */

static void
BezierII (int degree, const NRPointF *bezCurve, double t, NRPointF *Q)
{
	int i, j;		
	NRPointF *Vtemp;
	
	Vtemp = alloca ((degree + 1) * sizeof (NRPointF));
	for (i = 0; i <= degree; i++) Vtemp[i] = bezCurve[i];
	
	/* Triangle computation	*/
	for (i = 1; i <= degree; i++) {	
		for (j = 0; j <= degree - i; j++) {
			Vtemp[j].x = (1.0 - t) * Vtemp[j].x + t * Vtemp[j + 1].x;
			Vtemp[j].y = (1.0 - t) * Vtemp[j].y + t * Vtemp[j + 1].y;
		}
	}

	*Q = Vtemp[0];
}

/*
 * ComputeLeftTangent, ComputeRightTangent, ComputeCenterTangent :
 *Approximate unit tangents at endpoints and "center" of digitized curve
 */
unsigned int
sp_darray_left_tangent (const NRPointF *d, int first, int len, NRPointF *tHat)
{
	int second, l2, i;
	second = first + 1;
	l2 = len / 2;
	tHat->x = (d[second].x - d[first].x) * l2;
	tHat->y = (d[second].y - d[first].y) * l2;
	for (i = 1; i < l2; i++) {
		tHat->x += (d[second + i].x - d[first].x) * (l2 - i);
		tHat->y += (d[second + i].y - d[first].y) * (l2 - i);
	}
	return sp_vector_normalize (tHat);
}

unsigned int
sp_darray_right_tangent (const NRPointF *d, int last, int len, NRPointF *tHat)
{
	int prev, l2, i;
	prev = last - 1;
	l2 = len / 2;
	tHat->x = (d[prev].x - d[last].x) * l2;
	tHat->y = (d[prev].y - d[last].y) * l2;
	for (i = 1; i < l2; i++) {
		tHat->x += (d[prev - i].x - d[last].x) * (l2 - i);
		tHat->y += (d[prev - i].y - d[last].y) * (l2 - i);
	}
	return sp_vector_normalize (tHat);
}

void
sp_darray_center_tangent (const NRPointF *d,
			  int            center,
			  NRPointF       *tHatCenter)
{
	NRPointF V1, V2;
	nr_vector_f_sub (&V1, &d[center-1], &d[center]);
	nr_vector_f_sub (&V2, &d[center], &d[center+1]);
	tHatCenter->x = (V1.x + V2.x)/2.0;
	tHatCenter->y = (V1.y + V2.y)/2.0;
	if ((tHatCenter->x == 0.0) && (tHatCenter->y == 0.0)) {
		tHatCenter->x = V1.y;
		tHatCenter->y = -V1.x;
	}
	sp_vector_normalize (tHatCenter);
}

/*
 *  ChordLengthParameterize :
 *	Assign parameter values to digitized points 
 *	using relative distances between points.
 *
 * Parameter array u has to be allocated with the same length as data
 */
static void
ChordLengthParameterize(const NRPointF *d, double *u, int len)
{
	int i;	
	
	u[0] = 0.0;

	for (i = 1; i < len; i++) {
		u[i] = u[i-1] + hypot (d[i].x - d[i-1].x, d[i].y - d[i-1].y);
	}

	for (i = 1; i < len; i++) {
		u[i] = u[i] / u[len - 1];
	}

#ifdef BEZIER_DEBUG
	g_assert (u[0] == 0.0 && u[len - 1] == 1.0);
	for (i = 1; i < len; i++) {
		g_assert (u[i] >= u[i-1]);
	}
#endif
}




/*
 *  ComputeMaxError :
 *	Find the maximum squared distance of digitized points
 *	to fitted curve.
*/
static double
ComputeMaxError (const NRPointF *d, double *u, int len, const NRPointF *bezCurve, int *splitPoint)
{
	int i;
	double maxDist; /* Maximum error */
	double dist; /* Current error */
	NRPointF P; /* Point on curve */
	NRPointF v; /* Vector from point to curve */

	*splitPoint = len / 2;

	maxDist = 0.0;
	for (i = 1; i < len; i++) {
		BezierII (3, bezCurve, u[i], &P);
		nr_vector_f_sub (&v, &P, &d[i]);
		dist = v.x * v.x + v.y * v.y;
		if (dist >= maxDist) {
			maxDist = dist;
			*splitPoint = i;
		}
	}

	return maxDist;
}

static void
nr_vector_d_add (NRPointD *d, const NRPointD *a, const NRPointD *b)
{
	d->x = a->x + b->x;
	d->y = a->y + b->y;
}

static void
nr_vector_f_sub (NRPointF *d, const NRPointF *a, const NRPointF *b)
{
	d->x = a->x - b->x;
	d->y = a->y - b->y;
}

static void
nr_vector_sub_dfd (NRPointD *d, const NRPointF *a, const NRPointD *b)
{
	d->x = a->x - b->x;
	d->y = a->y - b->y;
}

static void
nr_vector_scale_df (NRPointD *d, const NRPointF *v, double s)
{
	d->x = v->x * s;
	d->y = v->y * s;
}

static unsigned int
sp_vector_normalize (NRPointF *v)
{
	double len;

	len = hypot (v->x, v->y);

	v->x /= len;
	v->y /= len;

	return fabs (len) > 1e-18;
}

static void
sp_vector_negate (NRPointF *v)
{
	v->x = -v->x;
	v->y = -v->y;
}

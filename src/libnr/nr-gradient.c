#define __NR_GRADIENT_C__

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

#include <libnr/nr-macros.h>
#include <libnr/nr-matrix.h>
#include <libnr/nr-pixops.h>
#include <libnr/nr-pixblock-pixel.h>
#include <libnr/nr-gradient.h>

#define noNR_USE_GENERIC_RENDERER

#ifndef hypot
#define hypot(a,b) sqrt ((a) * (a) + (b) * (b))
#endif

#define NRG_MASK (NR_GRADIENT_VECTOR_LENGTH - 1)
#define NRG_2MASK ((NR_GRADIENT_VECTOR_LENGTH << 1) - 1)

static void nr_lgradient_render_block (NRRenderer *r, NRPixBlock *pb, NRPixBlock *m);
static void nr_lgradient_render_R8G8B8A8N (NRLGradientRenderer *lgr, unsigned char *px, int x0, int y0, int width, int height, int rs);
static void nr_lgradient_render_R8G8B8 (NRLGradientRenderer *lgr, unsigned char *px, int x0, int y0, int width, int height, int rs);
static void nr_lgradient_render_generic (NRLGradientRenderer *lgr, NRPixBlock *pb);

NRRenderer *
nr_lgradient_renderer_setup (NRLGradientRenderer *lgr,
			     const unsigned char *cv, 
			     unsigned int spread, 
			     const NRMatrixF *gs2px,
			     float x0, float y0,
			     float x1, float y1)
{
	NRMatrixF n2gs, n2px, px2n;

	lgr->renderer.render = nr_lgradient_render_block;

	lgr->vector = cv;
	lgr->spread = spread;

	n2gs.c[0] = x1 - x0;
	n2gs.c[1] = y1 - y0;
	n2gs.c[2] = y1 - y0;
	n2gs.c[3] = x0 - x1;
	n2gs.c[4] = x0;
	n2gs.c[5] = y0;

	nr_matrix_multiply_fff (&n2px, &n2gs, gs2px);
	nr_matrix_f_invert (&px2n, &n2px);

	lgr->x0 = n2px.c[4];
	lgr->y0 = n2px.c[5];
	lgr->dx = px2n.c[0] * NR_GRADIENT_VECTOR_LENGTH;
	lgr->dy = px2n.c[2] * NR_GRADIENT_VECTOR_LENGTH;

	return (NRRenderer *) lgr;
}

static void
nr_lgradient_render_block (NRRenderer *r, NRPixBlock *pb, NRPixBlock *m)
{
	NRLGradientRenderer *lgr;
	int width, height;

	lgr = (NRLGradientRenderer *) r;

	width = pb->area.x1 - pb->area.x0;
	height = pb->area.y1 - pb->area.y0;

#ifdef NR_USE_GENERIC_RENDERER
	nr_lgradient_render_generic (lgr, pb);
#else
	if (pb->empty) {
		switch (pb->mode) {
		case NR_PIXBLOCK_MODE_A8:
			nr_lgradient_render_generic (lgr, pb);
			break;
		case NR_PIXBLOCK_MODE_R8G8B8:
			nr_lgradient_render_generic (lgr, pb);
			break;
		case NR_PIXBLOCK_MODE_R8G8B8A8N:
			nr_lgradient_render_generic (lgr, pb);
			break;
		case NR_PIXBLOCK_MODE_R8G8B8A8P:
			nr_lgradient_render_generic (lgr, pb);
			break;
		default:
			break;
		}
	} else {
		switch (pb->mode) {
		case NR_PIXBLOCK_MODE_A8:
			nr_lgradient_render_generic (lgr, pb);
			break;
		case NR_PIXBLOCK_MODE_R8G8B8:
			nr_lgradient_render_R8G8B8 (lgr, NR_PIXBLOCK_PX (pb), pb->area.x0, pb->area.y0, width, height, pb->rs);
			break;
		case NR_PIXBLOCK_MODE_R8G8B8A8N:
			nr_lgradient_render_R8G8B8A8N (lgr, NR_PIXBLOCK_PX (pb), pb->area.x0, pb->area.y0, width, height, pb->rs);
			break;
		case NR_PIXBLOCK_MODE_R8G8B8A8P:
			nr_lgradient_render_generic (lgr, pb);
			break;
		default:
			break;
		}
	}
#endif
}

static void
nr_lgradient_render_R8G8B8A8N (NRLGradientRenderer *lgr, unsigned char *px, int x0, int y0, int width, int height, int rs)
{
	int x, y;
	unsigned char *d;
	double pos;

	for (y = 0; y < height; y++) {
		d = px + y * rs;
		pos = (y + y0 - lgr->y0) * lgr->dy + (0 + x0 - lgr->x0) * lgr->dx;
		for (x = 0; x < width; x++) {
			int ip, idx;
			unsigned int ca;
			const unsigned char *s;
			ip = (int) pos;
			switch (lgr->spread) {
			case NR_GRADIENT_SPREAD_PAD:
				idx = CLAMP (ip, 0, NRG_MASK);
				break;
			case NR_GRADIENT_SPREAD_REFLECT:
				idx = ip & NRG_2MASK;
				if (idx > NRG_MASK) idx = NRG_2MASK - idx;
				break;
			case NR_GRADIENT_SPREAD_REPEAT:
				idx = ip & NRG_MASK;
				break;
			default:
				idx = 0;
				break;
			}
			/* Full composition */
			s = lgr->vector + 4 * idx;
			if (s[3] != 0) {
				ca = 65025 - (255 - s[3]) * (255 - d[3]);
				d[0] = NR_COMPOSENNN_A7 (s[0], s[3], d[0], d[3], ca);
				d[1] = NR_COMPOSENNN_A7 (s[1], s[3], d[1], d[3], ca);
				d[2] = NR_COMPOSENNN_A7 (s[2], s[3], d[2], d[3], ca);
				d[3] = (ca + 127) / 255;
			}
			d += 4;
			pos += lgr->dx;
		}
	}
}

static void
nr_lgradient_render_R8G8B8 (NRLGradientRenderer *lgr, unsigned char *px, int x0, int y0, int width, int height, int rs)
{
	int x, y;
	unsigned char *d;
	double pos;

	for (y = 0; y < height; y++) {
		d = px + y * rs;
		pos = (y + y0 - lgr->y0) * lgr->dy + (0 + x0 - lgr->x0) * lgr->dx;
		for (x = 0; x < width; x++) {
			int ip, idx;
			const unsigned char *s;
			ip = (int) pos;
			switch (lgr->spread) {
			case NR_GRADIENT_SPREAD_PAD:
				idx = CLAMP (ip, 0, NRG_MASK);
				break;
			case NR_GRADIENT_SPREAD_REFLECT:
				idx = ip & NRG_2MASK;
				if (idx > NRG_MASK) idx = NRG_2MASK - idx;
				break;
			case NR_GRADIENT_SPREAD_REPEAT:
				idx = ip & NRG_MASK;
				break;
			default:
				idx = 0;
				break;
			}
			/* Full composition */
			s = lgr->vector + 4 * idx;
			d[0] = NR_COMPOSEN11 (s[0], s[3], d[0]);
			d[1] = NR_COMPOSEN11 (s[1], s[3], d[1]);
			d[2] = NR_COMPOSEN11 (s[2], s[3], d[2]);
			d += 3;
			pos += lgr->dx;
		}
	}
}

static void
nr_lgradient_render_generic (NRLGradientRenderer *lgr, NRPixBlock *pb)
{
	int x, y;
	unsigned char *d;
	double pos;
	int bpp;
	NRPixBlock spb;
	int x0, y0, width, height, rs;

	x0 = pb->area.x0;
	y0 = pb->area.y0;
	width = pb->area.x1 - pb->area.x0;
	height = pb->area.y1 - pb->area.y0;
	rs = pb->rs;

	nr_pixblock_setup_extern (&spb, NR_PIXBLOCK_MODE_R8G8B8A8N, 0, 0, NR_GRADIENT_VECTOR_LENGTH, 1,
				  (unsigned char *) lgr->vector,
				  4 * NR_GRADIENT_VECTOR_LENGTH,
				  0, 0);
	bpp = (pb->mode == NR_PIXBLOCK_MODE_A8) ? 1 : (pb->mode == NR_PIXBLOCK_MODE_R8G8B8) ? 3 : 4;

	for (y = 0; y < height; y++) {
		d = NR_PIXBLOCK_PX (pb) + y * rs;
		pos = (y + y0 - lgr->y0) * lgr->dy + (0 + x0 - lgr->x0) * lgr->dx;
		for (x = 0; x < width; x++) {
			int ip, idx;
			const unsigned char *s;
			ip = (int) pos;
			switch (lgr->spread) {
			case NR_GRADIENT_SPREAD_PAD:
				idx = CLAMP (ip, 0, NRG_MASK);
				break;
			case NR_GRADIENT_SPREAD_REFLECT:
				idx = ip & NRG_2MASK;
				if (idx > NRG_MASK) idx = NRG_2MASK - idx;
				break;
			case NR_GRADIENT_SPREAD_REPEAT:
				idx = ip & NRG_MASK;
				break;
			default:
				idx = 0;
				break;
			}
			s = lgr->vector + 4 * idx;
			nr_compose_pixblock_pixblock_pixel (pb, d, &spb, s);
			d += bpp;
			pos += lgr->dx;
		}
	}

	nr_pixblock_release (&spb);
}

/* Radial */

static void nr_rgradient_render_block_symmetric (NRRenderer *r, NRPixBlock *pb, NRPixBlock *m);
static void nr_rgradient_render_block_optimized (NRRenderer *r, NRPixBlock *pb, NRPixBlock *m);
static void nr_rgradient_render_generic (NRRGradientRenderer *rgr, NRPixBlock *pb);
static void nr_rgradient_render_generic_optimized (NRRGradientRenderer *rgr, NRPixBlock *pb);

NRRenderer *
nr_rgradient_renderer_setup (NRRGradientRenderer *rgr,
			     const unsigned char *cv,
			     unsigned int spread,
			     const NRMatrixF *gs2px,
			     float cx, float cy,
			     float fx, float fy,
			     float r)
{
	if (NR_DF_TEST_CLOSE (cx, fx, NR_EPSILON_F) &&
	    NR_DF_TEST_CLOSE (cy, fy, NR_EPSILON_F)) {
		rgr->renderer.render = nr_rgradient_render_block_symmetric;

		rgr->vector = cv;
		rgr->spread = spread;

		nr_matrix_f_invert (&rgr->px2gs, gs2px);

		rgr->cx = cx;
		rgr->cy = cy;
		rgr->fx = fx;
		rgr->fy = fy;
		rgr->r = r;
	} else {
		NRMatrixF n2gs, n2px;

		rgr->renderer.render = nr_rgradient_render_block_optimized;

		rgr->vector = cv;
		rgr->spread = spread;

		n2gs.c[0] = cx - fx;
		n2gs.c[1] = cy - fy;
		n2gs.c[2] = cy - fy;
		n2gs.c[3] = fx - cx;
		n2gs.c[4] = fx;
		n2gs.c[5] = fy;

		nr_matrix_multiply_fff (&n2px, &n2gs, gs2px);
		nr_matrix_f_invert (&rgr->px2gs, &n2px);

		rgr->cx = 1.0;
		rgr->cy = 0.0;
		rgr->fx = 0.0;
		rgr->fy = 0.0;
		rgr->r = r / hypot (fx - cx, fy - cy);
	}

	return (NRRenderer *) rgr;
}

static void
nr_rgradient_render_block_symmetric (NRRenderer *r, NRPixBlock *pb, NRPixBlock *m)
{
	NRRGradientRenderer *rgr;
	int width, height;

	rgr = (NRRGradientRenderer *) r;

	width = pb->area.x1 - pb->area.x0;
	height = pb->area.y1 - pb->area.y0;

	nr_rgradient_render_generic (rgr, pb);
}

static void
nr_rgradient_render_block_optimized (NRRenderer *r, NRPixBlock *pb, NRPixBlock *m)
{
	NRRGradientRenderer *rgr;
	int width, height;

	rgr = (NRRGradientRenderer *) r;

	width = pb->area.x1 - pb->area.x0;
	height = pb->area.y1 - pb->area.y0;

	nr_rgradient_render_generic_optimized (rgr, pb);
}

static void
nr_rgradient_render_generic (NRRGradientRenderer *rgr, NRPixBlock *pb)
{
	int x, y;
	unsigned char *d;
	const unsigned char *s;
	int idx;
	int bpp;
	NRPixBlock spb;
	int x0, y0, width, height, rs;

	x0 = pb->area.x0;
	y0 = pb->area.y0;
	width = pb->area.x1 - pb->area.x0;
	height = pb->area.y1 - pb->area.y0;
	rs = pb->rs;

	nr_pixblock_setup_extern (&spb, NR_PIXBLOCK_MODE_R8G8B8A8N, 0, 0, NR_GRADIENT_VECTOR_LENGTH, 1,
				  (unsigned char *) rgr->vector,
				  4 * NR_GRADIENT_VECTOR_LENGTH,
				  0, 0);
	bpp = (pb->mode == NR_PIXBLOCK_MODE_A8) ? 1 : (pb->mode == NR_PIXBLOCK_MODE_R8G8B8) ? 3 : 4;

	if (NR_DF_TEST_CLOSE (rgr->cx, rgr->fx, NR_EPSILON_D) &&
	    NR_DF_TEST_CLOSE (rgr->cy, rgr->fy, NR_EPSILON_D)) {
		for (y = 0; y < height; y++) {
			d = NR_PIXBLOCK_PX (pb) + y * rs;
			for (x = 0; x < width; x++) {
				double gx, gy;
				double r, pos;

				gx = rgr->px2gs.c[0] * (x + x0) + rgr->px2gs.c[2] * (y + y0) + rgr->px2gs.c[4];
				gy = rgr->px2gs.c[1] * (x + x0) + rgr->px2gs.c[3] * (y + y0) + rgr->px2gs.c[5];

				r = MAX (rgr->r, 1e-9);
				pos = hypot (gx - rgr->cx, gy - rgr->cy) * NR_GRADIENT_VECTOR_LENGTH / r;

				if (rgr->spread == NR_GRADIENT_SPREAD_REFLECT) {
					idx = ((int) pos) & NRG_2MASK;
					if (idx & NR_GRADIENT_VECTOR_LENGTH) idx = (2 * NR_GRADIENT_VECTOR_LENGTH) - idx;
				} else if (rgr->spread == NR_GRADIENT_SPREAD_REPEAT) {
					idx = ((int) pos) & NRG_MASK;
				} else {
					idx = CLAMP (((int) pos), 0, NRG_MASK);
				}
				s = rgr->vector + 4 * idx;
				nr_compose_pixblock_pixblock_pixel (pb, d, &spb, s);
				d += bpp;
			}
		}
	} else {
		for (y = 0; y < height; y++) {
			d = NR_PIXBLOCK_PX (pb) + y * rs;
			for (x = 0; x < width; x++) {
				double gx, gy;
				double r, pos;
				double D, N, A, B, C;

				gx = rgr->px2gs.c[0] * (x + x0) + rgr->px2gs.c[2] * (y + y0) + rgr->px2gs.c[4];
				gy = rgr->px2gs.c[1] * (x + x0) + rgr->px2gs.c[3] * (y + y0) + rgr->px2gs.c[5];

				/*
				 * (1)  (gx - fx) * (Py - fy) = (gy - fy) * (Px - fx)
				 * (2)  (Px - cx) * (Px - cx) + (Py - cy) * (Py - cy) = r * r
				 *
				 * (3)   Py = (Px - fx) * (gy - fy) / (gx - fx) + fy
				 * (4)  (gy - fy) / (gx - fx) = D
				 * (5)   Py = D * Px - D * fx + fy
				 *
				 * (6)   D * fx - fy + cy = N
				 * (7)   Px * Px - 2 * Px * cx + cx * cx + (D * Px) * (D * Px) - 2 * (D * Px) * N + N * N = r * r
				 * (8)  (D * D + 1) * (Px * Px) - 2 * (cx + D * N) * Px + cx * cx + N * N = r * r
				 *
				 * (9)   A = D * D + 1
				 * (10)  B = -2 * (cx + D * N)
				 * (11)  C = cx * cx + N * N - r * r
				 *
				 * (12)  Px = (-B +- SQRT (B * B - 4 * A * C)) / 2 * A
				 */

				r = MAX (rgr->r, 1e-9);

				if (NR_DF_TEST_CLOSE (gx, rgr->fx, NR_EPSILON_D)) {
					gx = rgr->fx + NR_EPSILON_D;
				}

				D = (gy - rgr->fy) / (gx - rgr->fx);
				N = D * rgr->fx - rgr->fy + rgr->cy;
				A = D * D + 1;
				B = -2 * (rgr->cx + D * N);
				C = rgr->cx * rgr->cx + N * N - r * r;
				if (NR_DF_TEST_CLOSE (A, 0.0, NR_EPSILON_D)) {
					pos = 0.0;
				} else {
					double q;
					q = B * B - 4 * A * C;
					if (q < 0.0) {
						pos = 0.0;
					} else {
						double px;
						if ((gx - rgr->fx) < 0.0) {
							px = (-B - sqrt (q)) / (2 * A);
						} else {
							px = (-B + sqrt (q)) / (2 * A);
						}
						pos = (gx - rgr->fx) / (px - rgr->fx) * NR_GRADIENT_VECTOR_LENGTH;
					}
				}

				if (rgr->spread == NR_GRADIENT_SPREAD_REFLECT) {
					idx = ((int) pos) & (2 * NR_GRADIENT_VECTOR_LENGTH - 1);
					if (idx & NR_GRADIENT_VECTOR_LENGTH) idx = (2 * NR_GRADIENT_VECTOR_LENGTH) - idx;
				} else if (rgr->spread == NR_GRADIENT_SPREAD_REPEAT) {
					idx = ((int) pos) & (NR_GRADIENT_VECTOR_LENGTH - 1);
				} else {
					idx = CLAMP (((int) pos), 0, (NR_GRADIENT_VECTOR_LENGTH - 1));
				}
				s = rgr->vector + 4 * idx;
				nr_compose_pixblock_pixblock_pixel (pb, d, &spb, s);
				d += bpp;
			}
		}
	}

	nr_pixblock_release (&spb);
}

static void
nr_rgradient_render_generic_optimized (NRRGradientRenderer *rgr, NRPixBlock *pb)
{
	int x, y;
	unsigned char *d;
	const unsigned char *s;
	int idx;
	int bpp;
	NRPixBlock spb;
	int x0, y0, width, height, rs;

	x0 = pb->area.x0;
	y0 = pb->area.y0;
	width = pb->area.x1 - pb->area.x0;
	height = pb->area.y1 - pb->area.y0;
	rs = pb->rs;

	nr_pixblock_setup_extern (&spb, NR_PIXBLOCK_MODE_R8G8B8A8N, 0, 0, NR_GRADIENT_VECTOR_LENGTH, 1,
				  (unsigned char *) rgr->vector,
				  4 * NR_GRADIENT_VECTOR_LENGTH,
				  0, 0);
	bpp = (pb->mode == NR_PIXBLOCK_MODE_A8) ? 1 : (pb->mode == NR_PIXBLOCK_MODE_R8G8B8) ? 3 : 4;

	for (y = 0; y < height; y++) {
		d = NR_PIXBLOCK_PX (pb) + y * rs;
		for (x = 0; x < width; x++) {
			double gx, gy;
			double r, pos;
			double D, A, C;

			/*
			 * cx = 1.0
			 * cy = 0.0
			 * fx = 0.0
			 * fy = 0.0
			 *
			 */

			gx = rgr->px2gs.c[0] * (x + x0) + rgr->px2gs.c[2] * (y + y0) + rgr->px2gs.c[4];
			gy = rgr->px2gs.c[1] * (x + x0) + rgr->px2gs.c[3] * (y + y0) + rgr->px2gs.c[5];

				/*
				 * (1)  (gx - fx) * (Py - fy) = (gy - fy) * (Px - fx)
				 * (2)  (Px - cx) * (Px - cx) + (Py - cy) * (Py - cy) = r * r
				 *
				 * (3)   Py = (Px - fx) * (gy - fy) / (gx - fx) + fy
				 * (4)  (gy - fy) / (gx - fx) = D
				 * (5)   Py = D * Px - D * fx + fy
				 *
				 * (6)   D * fx - fy + cy = N
				 * (7)   Px * Px - 2 * Px * cx + cx * cx + (D * Px) * (D * Px) - 2 * (D * Px) * N + N * N = r * r
				 * (8)  (D * D + 1) * (Px * Px) - 2 * (cx + D * N) * Px + cx * cx + N * N = r * r
				 *
				 * (9)   A = D * D + 1
				 * (10)  B = -2 * (cx + D * N)
				 * (11)  C = cx * cx + N * N - r * r
				 *
				 * (12)  Px = (-B +- SQRT (B * B - 4 * A * C)) / 2 * A
				 */

			r = MAX (rgr->r, 1e-9);

			if (NR_DF_TEST_CLOSE (gx, 0.0, NR_EPSILON_D)) {
				gx = 0.0 + NR_EPSILON_D;
			}

			D = gy / gx;
			A = D * D + 1;
			C = 1.0 - r * r;
			if (NR_DF_TEST_CLOSE (A, 0.0, NR_EPSILON_D)) {
				pos = 0.0;
			} else {
				double q;
				q = 4 * (1 - A * C);
				if (q < 0.0) {
					pos = 0.0;
				} else {
					double px;
					if (gx < 0.0) {
						px = (2 - sqrt (q)) / (2 * A);
					} else {
						px = (2 + sqrt (q)) / (2 * A);
					}
					pos = gx / px * NR_GRADIENT_VECTOR_LENGTH;
				}
			}

			if (rgr->spread == NR_GRADIENT_SPREAD_REFLECT) {
				idx = ((int) pos) & (2 * NR_GRADIENT_VECTOR_LENGTH - 1);
				if (idx & NR_GRADIENT_VECTOR_LENGTH) idx = (2 * NR_GRADIENT_VECTOR_LENGTH) - idx;
			} else if (rgr->spread == NR_GRADIENT_SPREAD_REPEAT) {
				idx = ((int) pos) & (NR_GRADIENT_VECTOR_LENGTH - 1);
			} else {
				idx = CLAMP (((int) pos), 0, (NR_GRADIENT_VECTOR_LENGTH - 1));
			}
			s = rgr->vector + 4 * idx;
			nr_compose_pixblock_pixblock_pixel (pb, d, &spb, s);
			d += bpp;
		}
	}

	nr_pixblock_release (&spb);
}


#define __NR_PATH_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <string.h>
#include <stdio.h>

#include "nr-matrix.h"
#include "nr-path.h"

enum {
	MULTI_NONE,
	MULTI_LINE,
	MULTI_CURVE2,
	MULTI_CURVE3
};

NRPath *
nr_path_new_from_art_bpath (const ArtBpath *bpath)
{
#if 1
	NRPath *path;
	NRDynamicPath *dpath;
	const ArtBpath *bp;
	unsigned int needclose;
	dpath = nr_dynamic_path_new (16);
	needclose = FALSE;
	for (bp = bpath; bp->code != ART_END; bp++) {
		switch (bp->code) {
		case ART_MOVETO:
		case ART_MOVETO_OPEN:
			if (needclose) nr_dynamic_path_closepath (dpath);
			nr_dynamic_path_moveto (dpath, bp->x3, bp->y3);
			needclose = (bp->code == ART_MOVETO);
			break;
		case ART_LINETO:
			nr_dynamic_path_lineto (dpath, bp->x3, bp->y3);
			break;
		case ART_CURVETO:
			nr_dynamic_path_curveto3 (dpath, bp->x1, bp->y1, bp->x2, bp->y2, bp->x3, bp->y3);
			break;
		default:
			break;
		}
	}
	if (needclose) nr_dynamic_path_closepath (dpath);
	path = dpath->path;
	free (dpath);
	return path;
#else
	NRPath *path;
	const ArtBpath *bp;
	int nsegments, nelements;
	int idx, sidx, midx;
	int multi, nmulti;

	nsegments = 0;
	nelements = 0;
	multi = MULTI_NONE;
	for (bp = bpath; bp->code != ART_END; bp++) {
		switch (bp->code) {
		case ART_MOVETO:
			/* Segment indicator and point coordinates */
			nsegments += 1;
			nelements += 3;
			multi = MULTI_NONE;
			break;
		case ART_MOVETO_OPEN:
			/* Segment indicator and point coordinates */
			nsegments += 1;
			nelements += 3;
			multi = MULTI_NONE;
			break;
		case ART_LINETO:
			/* Possible code and point coordinates */
			if (multi != MULTI_LINE) nelements += 1;
			nelements += 2;
			multi = MULTI_LINE;
			break;
		case ART_CURVETO:
			/* Possible code and point coordinates */
			if (multi != MULTI_CURVE3) nelements += 1;
			nelements += 6;
			multi = MULTI_CURVE3;
			break;
		default:
			break;
		}
	}
	path = (NRPath *) malloc (sizeof (NRPath) + nelements * sizeof (NRPathElement) - sizeof (NRPathElement));
	path->nelements = nelements;
	path->offset = 0;
	path->nsegments = nsegments;
	sidx = 0;
	midx = 0;
	idx = 0;
	multi = MULTI_NONE;
	nmulti = 0;
	for (bp = bpath; bp->code != ART_END; bp++) {
		switch (bp->code) {
		case ART_MOVETO:
			if (idx > 0) NR_PATH_ELEMENT_SET_LENGTH (path->elements + sidx, idx - sidx);
			if (nmulti > 0) NR_PATH_ELEMENT_SET_LENGTH (path->elements + midx, nmulti);
			sidx = idx;
			NR_PATH_ELEMENT_SET_CLOSED (path->elements + idx, TRUE);
			idx += 1;
			NR_PATH_ELEMENT_SET_VALUE (path->elements + idx, (float) bp->x3);
			idx += 1;
			NR_PATH_ELEMENT_SET_VALUE (path->elements + idx, (float) bp->y3);
			idx += 1;
			multi = MULTI_NONE;
			break;
		case ART_MOVETO_OPEN:
			if (idx > 0) NR_PATH_ELEMENT_SET_LENGTH (path->elements + sidx, idx - sidx);
			if (nmulti > 0) NR_PATH_ELEMENT_SET_LENGTH (path->elements + midx, nmulti);
			sidx = idx;
			NR_PATH_ELEMENT_SET_CLOSED (path->elements + idx, FALSE);
			idx += 1;
			NR_PATH_ELEMENT_SET_VALUE (path->elements + idx, (float) bp->x3);
			idx += 1;
			NR_PATH_ELEMENT_SET_VALUE (path->elements + idx, (float) bp->y3);
			idx += 1;
			multi = MULTI_NONE;
			break;
		case ART_LINETO:
			if (multi != MULTI_LINE) {
				if (nmulti > 0) NR_PATH_ELEMENT_SET_LENGTH (path->elements + midx, nmulti);
				midx = idx;
				nmulti = 0;
				multi = MULTI_LINE;
				NR_PATH_ELEMENT_SET_CODE (path->elements + idx, NR_PATH_LINETO);
				idx += 1;
			}
			NR_PATH_ELEMENT_SET_VALUE (path->elements + idx, (float) bp->x3);
			idx += 1;
			NR_PATH_ELEMENT_SET_VALUE (path->elements + idx, (float) bp->y3);
			idx += 1;
			nmulti += 1;
			break;
		case ART_CURVETO:
			if (multi != MULTI_CURVE3) {
				if (nmulti > 0) NR_PATH_ELEMENT_SET_LENGTH (path->elements + midx, nmulti);
				midx = idx;
				nmulti = 0;
				multi = MULTI_CURVE3;
				NR_PATH_ELEMENT_SET_CODE (path->elements + idx, NR_PATH_CURVETO3);
				idx += 1;
			}
			NR_PATH_ELEMENT_SET_VALUE (path->elements + idx, (float) bp->x1);
			idx += 1;
			NR_PATH_ELEMENT_SET_VALUE (path->elements + idx, (float) bp->y1);
			idx += 1;
			NR_PATH_ELEMENT_SET_VALUE (path->elements + idx, (float) bp->x2);
			idx += 1;
			NR_PATH_ELEMENT_SET_VALUE (path->elements + idx, (float) bp->y2);
			idx += 1;
			NR_PATH_ELEMENT_SET_VALUE (path->elements + idx, (float) bp->x3);
			idx += 1;
			NR_PATH_ELEMENT_SET_VALUE (path->elements + idx, (float) bp->y3);
			idx += 1;
			nmulti += 1;
			break;
		default:
			break;
		}
	}
	if (idx > 0) NR_PATH_ELEMENT_SET_LENGTH (path->elements + sidx, idx - sidx);
	if (nmulti > 0) NR_PATH_ELEMENT_SET_LENGTH (path->elements + midx, nmulti);

	return path;
#endif
}

unsigned int
nr_path_forall (const NRPath *path, NRMatrixF *transform, const NRPathGVector *gv, void *data)
{
	float x0, y0, sx, sy;
	unsigned int sstart;

	sstart = path->offset;
	while (sstart < path->nelements) {
		const NRPathElement *seg;
		unsigned int slen, cflags, fflags, lflags, idx, flags;

		seg = path->elements + sstart;
		slen = NR_PATH_ELEMENT_LENGTH (seg);
		cflags = NR_PATH_ELEMENT_CLOSED (seg) ? NR_PATH_CLOSED : 0; 
		fflags = NR_PATH_FIRST;
		lflags = 0;

		/* Start new subpath */
		if (transform) {
			x0 = sx = NR_MATRIX_DF_TRANSFORM_X (transform, seg[1].fval, seg[2].fval);
			y0 = sy = NR_MATRIX_DF_TRANSFORM_Y (transform, seg[1].fval, seg[2].fval);
		} else {
			x0 = sx = seg[1].fval;
			y0 = sy = seg[2].fval;
		}
		if (gv->moveto) {
			/* Moveto is always first */
			flags = cflags | fflags | lflags;
			if (!gv->moveto (x0, y0, flags, data)) return FALSE;
		}

		idx = 3;
		while (idx < slen) {
			int nmulti, i;
			nmulti = NR_PATH_ELEMENT_LENGTH (seg + idx);
			switch (NR_PATH_ELEMENT_CODE (seg + idx)) {
			case NR_PATH_LINETO:
				idx += 1;
				for (i = 0; i < nmulti; i++) {
					float x1, y1;
					if (transform) {
						x1 = NR_MATRIX_DF_TRANSFORM_X (transform,
									       seg[idx].fval, seg[idx + 1].fval);
						y1 = NR_MATRIX_DF_TRANSFORM_Y (transform,
									       seg[idx].fval, seg[idx + 1].fval);
					} else {
						x1 = seg[idx].fval;
						y1 = seg[idx + 1].fval;
					}
					idx += 2;
					if (idx >= slen) lflags = NR_PATH_LAST;
					flags = cflags | fflags | lflags;
					if (gv->lineto && !gv->lineto (x0, y0, x1, y1, flags, data)) {
						return FALSE;
					}
					fflags = 0;
					x0 = x1;
					y0 = y1;
				}
				break;
			case NR_PATH_CURVETO2:
				idx += 1;
				for (i = 0; i < nmulti; i++) {
					float x1, y1, x2, y2;
					if (transform) {
						x1 = NR_MATRIX_DF_TRANSFORM_X (transform,
									       seg[idx].fval, seg[idx + 1].fval);
						y1 = NR_MATRIX_DF_TRANSFORM_Y (transform,
									       seg[idx].fval, seg[idx + 1].fval);
						x2 = NR_MATRIX_DF_TRANSFORM_X (transform,
									       seg[idx + 2].fval, seg[idx + 3].fval);
						y2 = NR_MATRIX_DF_TRANSFORM_Y (transform,
									       seg[idx + 2].fval, seg[idx + 3].fval);
					} else {
						x1 = seg[idx].fval;
						y1 = seg[idx + 1].fval;
						x2 = seg[idx + 2].fval;
						y2 = seg[idx + 3].fval;
					}
					idx += 4;
					if (idx >= slen) lflags = NR_PATH_LAST;
					flags = cflags | fflags | lflags;
					if (gv->curveto2 && !gv->curveto2 (x0, y0, x1, y1, x2, y2, flags, data)) {
						return FALSE;
					}
					fflags = 0;
					x0 = x2;
					y0 = y2;
				}
				break;
			case NR_PATH_CURVETO3:
				idx += 1;
				for (i = 0; i < nmulti; i++) {
					float x1, y1, x2, y2, x3, y3;
					if (transform) {
						x1 = NR_MATRIX_DF_TRANSFORM_X (transform,
									       seg[idx].fval, seg[idx + 1].fval);
						y1 = NR_MATRIX_DF_TRANSFORM_Y (transform,
									       seg[idx].fval, seg[idx + 1].fval);
						x2 = NR_MATRIX_DF_TRANSFORM_X (transform,
									       seg[idx + 2].fval, seg[idx + 3].fval);
						y2 = NR_MATRIX_DF_TRANSFORM_Y (transform,
									       seg[idx + 2].fval, seg[idx + 3].fval);
						x3 = NR_MATRIX_DF_TRANSFORM_X (transform,
									       seg[idx + 4].fval, seg[idx + 5].fval);
						y3 = NR_MATRIX_DF_TRANSFORM_Y (transform,
									       seg[idx + 4].fval, seg[idx + 5].fval);
					} else {
						x1 = seg[idx].fval;
						y1 = seg[idx + 1].fval;
						x2 = seg[idx + 2].fval;
						y2 = seg[idx + 3].fval;
						x3 = seg[idx + 4].fval;
						y3 = seg[idx + 5].fval;
					}
					idx += 6;
					/* fixme: LAST flag may be wrong if autoclose is on */
					if (idx >= slen) lflags = NR_PATH_LAST;
					flags = cflags | fflags | lflags;
					if (gv->curveto3 && !gv->curveto3 (x0, y0, x1, y1, x2, y2, x3, y3, flags, data)) {
						return FALSE;
					}
					fflags = 0;
					x0 = x3;
					y0 = y3;
				}
				break;
			default:
				fprintf (stderr, "Invalid path code '%d'\n", NR_PATH_ELEMENT_CODE (seg + idx));
				return FALSE;
				break;
			}
		}
		/* Finish path */
		if (gv->endpath) {
			fflags = 0;
			lflags = NR_PATH_LAST;
			flags = cflags | fflags | lflags;
			if (!gv->endpath (x0, y0, sx, sy, flags, data)) return FALSE;
		}
		sstart += slen;
	}
	return TRUE;
}

struct _NRPathFlattenData {
	const NRPathGVector *pgv;
	void *data;
	double tolerance2;
};

static unsigned int
nr_curve_flatten (double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3,
		  unsigned int flags, struct _NRPathFlattenData *fdata)
{
	double dx1_0, dy1_0, dx2_0, dy2_0, dx3_0, dy3_0, dx2_3, dy2_3, d3_0_2;
	double s1_q, t1_q, s2_q, t2_q, v2_q;
	double f2, f2_q;
	double x00t, y00t, x0tt, y0tt, xttt, yttt, x1tt, y1tt, x11t, y11t;

	dx1_0 = x1 - x0;
	dy1_0 = y1 - y0;
	dx2_0 = x2 - x0;
	dy2_0 = y2 - y0;
	dx3_0 = x3 - x0;
	dy3_0 = y3 - y0;
	dx2_3 = x3 - x2;
	dy2_3 = y3 - y2;
	f2 = fdata->tolerance2;
	d3_0_2 = dx3_0 * dx3_0 + dy3_0 * dy3_0;
	if (d3_0_2 < f2) {
		double d1_0_2, d2_0_2;
		d1_0_2 = dx1_0 * dx1_0 + dy1_0 * dy1_0;
		d2_0_2 = dx2_0 * dx2_0 + dy2_0 * dy2_0;
		if ((d1_0_2 < f2) && (d2_0_2 < f2)) {
			goto nosubdivide;
		} else {
			goto subdivide;
		}
	}
	f2_q = f2 * d3_0_2;
	s1_q = dx1_0 * dx3_0 + dy1_0 * dy3_0;
	t1_q = dy1_0 * dx3_0 - dx1_0 * dy3_0;
	s2_q = dx2_0 * dx3_0 + dy2_0 * dy3_0;
	t2_q = dy2_0 * dx3_0 - dx2_0 * dy3_0;
	v2_q = dx2_3 * dx3_0 + dy2_3 * dy3_0;
	if ((t1_q * t1_q) > f2_q) goto subdivide;
	if ((t2_q * t2_q) > f2_q) goto subdivide;
	if ((s1_q < 0.0) && ((s1_q * s1_q) > f2_q)) goto subdivide;
	if ((v2_q < 0.0) && ((v2_q * v2_q) > f2_q)) goto subdivide;
	if (s1_q >= s2_q) goto subdivide;

 nosubdivide:
	if (fdata->pgv->lineto) {
		return fdata->pgv->lineto (x0, y0, x3, y3, flags, fdata->data);
	}
	return TRUE;

 subdivide:
	x00t = (x0 + x1) * 0.5;
	y00t = (y0 + y1) * 0.5;
	x0tt = (x0 + 2 * x1 + x2) * 0.25;
	y0tt = (y0 + 2 * y1 + y2) * 0.25;
	x1tt = (x1 + 2 * x2 + x3) * 0.25;
	y1tt = (y1 + 2 * y2 + y3) * 0.25;
	x11t = (x2 + x3) * 0.5;
	y11t = (y2 + y3) * 0.5;
	xttt = (x0tt + x1tt) * 0.5;
	yttt = (y0tt + y1tt) * 0.5;

	if (!nr_curve_flatten (x0, y0, x00t, y00t, x0tt, y0tt, xttt, yttt, flags & (~NR_PATH_LAST), fdata)) return FALSE;
	if (!nr_curve_flatten (xttt, yttt, x1tt, y1tt, x11t, y11t, x3, y3, flags & (~NR_PATH_FIRST), fdata)) return FALSE;
	return TRUE;
}

static unsigned int
nr_path_flatten_moveto (float x0, float y0, unsigned int flags, void *data)
{
	struct _NRPathFlattenData *fdata;
	fdata = (struct _NRPathFlattenData *) data;
	if (fdata->pgv->moveto) {
		return fdata->pgv->moveto (x0, y0, flags, fdata->data);
	}
	return TRUE;
}

static unsigned int
nr_path_flatten_lineto (float x0, float y0, float x1, float y1, unsigned int flags, void *data)
{
	struct _NRPathFlattenData *fdata;
	fdata = (struct _NRPathFlattenData *) data;
	if (fdata->pgv->lineto) {
		return fdata->pgv->lineto (x0, y0, x1, y1, flags, fdata->data);
	}
	return TRUE;
}

static unsigned int
nr_path_flatten_curveto2 (float x0, float y0, float x1, float y1, float x2, float y2,
			  unsigned int flags, void *data)
{
	struct _NRPathFlattenData *fdata;
	fdata = (struct _NRPathFlattenData *) data;
	return nr_curve_flatten (x0, y0,
				 x1 + (x0 - x1) / 3.0, y1 + (y0 - y1) / 3.0,
				 x1 + (x2 - x1) / 3.0, y1 + (y2 - y1) / 3.0, x2, y2, flags, fdata);
}

static unsigned int
nr_path_flatten_curveto3 (float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3,
			  unsigned int flags, void *data)
{
	struct _NRPathFlattenData *fdata;
	fdata = (struct _NRPathFlattenData *) data;
	return nr_curve_flatten (x0, y0, x1, y1, x2, y2, x3, y3, flags, fdata);
}

static unsigned int
nr_path_flatten_endpath (float ex, float ey, float sx, float sy, unsigned int flags, void *data)
{
	struct _NRPathFlattenData *fdata;
	fdata = (struct _NRPathFlattenData *) data;
	if (fdata->pgv->endpath) {
		return fdata->pgv->endpath (ex, ey, sx, sy, flags, fdata->data);
	}
	return TRUE;
}

static NRPathGVector fpgv = {
	nr_path_flatten_moveto,
	nr_path_flatten_lineto,
	nr_path_flatten_curveto2,
	nr_path_flatten_curveto3,
	nr_path_flatten_endpath
};

unsigned int
nr_path_forall_flat (const NRPath *path, NRMatrixF *transform, float tolerance, const NRPathGVector *gv, void *data)
{
	struct _NRPathFlattenData fdata;
	fdata.pgv = gv;
	fdata.data = data;
	fdata.tolerance2 = tolerance * tolerance;

	return nr_path_forall (path, transform, &fpgv, &fdata);
}

unsigned int
nr_path_forall_art (const ArtBpath *bpath, NRMatrixF *transform, const NRPathGVector *gv, void *data)
{
	const ArtBpath *bp;
	float x0, y0, x1, y1, sx, sy;
	float x2, y2, x3, y3;
	unsigned int cflags, fflags, lflags, flags;

	x0 = y0 = x1 = y1 = 0.0;
	sx = sy = 0.0;
	cflags = 0;
	fflags = NR_PATH_FIRST;
	lflags = 0;

	for (bp = bpath; bp->code != ART_END; bp++) {
		switch (bp->code) {
		case ART_MOVETO:
		case ART_MOVETO_OPEN:
			if (cflags && ((x0 != sx) || (y0 != sy))) {
				/* Have to close previous subpath */
				if (gv->lineto) {
					flags = NR_PATH_CLOSED | NR_PATH_LAST;
					if (!gv->lineto (x0, y0, sx, sy, flags, data)) return FALSE;
				}
			}
			if (gv->endpath) {
				flags = cflags | fflags | NR_PATH_LAST;
				if (!gv->endpath (x0, y0, sx, sy, flags, data)) return FALSE;
			}
			if (transform) {
				sx = x0 = NR_MATRIX_DF_TRANSFORM_X (transform, bp->x3, bp->y3);
				sy = y0 = NR_MATRIX_DF_TRANSFORM_Y (transform, bp->x3, bp->y3);
			} else {
				sx = x0 = bp->x3;
				sy = y0 = bp->y3;
			}
			if (gv->moveto) {
				cflags = (bp->code == ART_MOVETO) ? NR_PATH_CLOSED : 0;
				fflags = NR_PATH_FIRST;
				lflags = 0;
				flags = cflags | fflags | lflags;
				if (!gv->moveto (x0, y0, flags, data)) return FALSE;
			}
			break;
		case ART_LINETO:
			if (transform) {
				x1 = NR_MATRIX_DF_TRANSFORM_X (transform, bp->x3, bp->y3);
				y1 = NR_MATRIX_DF_TRANSFORM_Y (transform, bp->x3, bp->y3);
			} else {
				x1 = bp->x3;
				y1 = bp->y3;
			}
			if (gv->lineto) {
				lflags = ((bp[1].code != ART_LINETO) && (bp[1].code != ART_CURVETO)) ? NR_PATH_LAST : 0;
				flags = cflags | fflags | lflags;
				if (!gv->lineto (x0, y0, x1, y1, flags, data)) return FALSE;
				fflags = FALSE;
			}
			x0 = x1;
			y0 = y1;
			break;
		case ART_CURVETO:
			if (transform) {
				x1 = NR_MATRIX_DF_TRANSFORM_X (transform, bp->x1, bp->y1);
				y1 = NR_MATRIX_DF_TRANSFORM_Y (transform, bp->x1, bp->y1);
				x2 = NR_MATRIX_DF_TRANSFORM_X (transform, bp->x2, bp->y2);
				y2 = NR_MATRIX_DF_TRANSFORM_Y (transform, bp->x2, bp->y2);
				x3 = NR_MATRIX_DF_TRANSFORM_X (transform, bp->x3, bp->y3);
				y3 = NR_MATRIX_DF_TRANSFORM_Y (transform, bp->x3, bp->y3);
			} else {
				x1 = bp->x1;
				y1 = bp->y1;
				x2 = bp->x2;
				y2 = bp->y2;
				x3 = bp->x3;
				y3 = bp->y3;
			}
			if (gv->curveto3) {
				lflags = ((bp[1].code != ART_LINETO) && (bp[1].code != ART_CURVETO)) ? NR_PATH_LAST : 0;
				flags = cflags | fflags | lflags;
				if (!gv->curveto3 (x0, y0, x1, y1, x2, y2, x3, y3, flags, data)) return FALSE;
				fflags = FALSE;
			}
			x0 = x3;
			y0 = y3;
			break;
		default:
			fprintf (stderr, "Invalid ArtBpath code '%d'\n", bp->code);
			return FALSE;
			break;
		}
	}
	if (cflags && ((x0 != sx) || (y0 != sy))) {
		/* Have to close previous subpath */
		if (gv->lineto) {
			flags = NR_PATH_CLOSED | NR_PATH_LAST;
			if (!gv->lineto (x0, y0, sx, sy, flags, data)) return FALSE;
		}
	}
	if (gv->endpath) {
		flags = cflags | fflags | NR_PATH_LAST;
		if (!gv->endpath (x0, y0, sx, sy, flags, data)) return FALSE;
	}
	return TRUE;
}

unsigned int
nr_path_forall_art_flat (const ArtBpath *path, NRMatrixF *transform, float tolerance,
			 const NRPathGVector *gv, void *data)
{
	struct _NRPathFlattenData fdata;
	fdata.pgv = gv;
	fdata.data = data;
	fdata.tolerance2 = tolerance * tolerance;

	return nr_path_forall_art (path, transform, &fpgv, &fdata);
}

unsigned int
nr_path_forall_art_vpath (const ArtVpath *vpath, NRMatrixF *transform, const NRPathGVector *gv, void *data)
{
	const ArtVpath *vp;
	float x0, y0, x1, y1, sx, sy;
	unsigned int cflags, fflags, lflags, flags;

	x0 = y0 = x1 = y1 = 0.0;
	sx = sy = 0.0;
	cflags = 0;
	fflags = NR_PATH_FIRST;
	lflags = 0;

	for (vp = vpath; vp->code != ART_END; vp++) {
		switch (vp->code) {
		case ART_MOVETO:
		case ART_MOVETO_OPEN:
			if (cflags && ((x0 != sx) || (y0 != sy))) {
				/* Have to close previous subpath */
				if (gv->lineto) {
					flags = NR_PATH_CLOSED | NR_PATH_LAST;
					if (!gv->lineto (x0, y0, sx, sy, flags, data)) return FALSE;
				}
			}
			if (gv->endpath) {
				flags = cflags | fflags | NR_PATH_LAST;
				if (!gv->endpath (x0, y0, sx, sy, flags, data)) return FALSE;
			}
			if (transform) {
				sx = x0 = NR_MATRIX_DF_TRANSFORM_X (transform, vp->x, vp->y);
				sy = y0 = NR_MATRIX_DF_TRANSFORM_Y (transform, vp->x, vp->y);
			} else {
				sx = x0 = vp->x;
				sy = y0 = vp->y;
			}
			if (gv->moveto) {
				cflags = (vp->code == ART_MOVETO) ? NR_PATH_CLOSED : 0;
				fflags = NR_PATH_FIRST;
				lflags = 0;
				flags = cflags | fflags | lflags;
				if (!gv->moveto (x0, y0, flags, data)) return FALSE;
			}
			break;
		case ART_LINETO:
			if (transform) {
				x1 = NR_MATRIX_DF_TRANSFORM_X (transform, vp->x, vp->y);
				y1 = NR_MATRIX_DF_TRANSFORM_Y (transform, vp->x, vp->y);
			} else {
				x1 = vp->x;
				y1 = vp->y;
			}
			if (gv->lineto) {
				lflags = (vp[1].code != ART_LINETO) ? NR_PATH_LAST : 0;
				flags = cflags | fflags | lflags;
				if (!gv->lineto (x0, y0, x1, y1, flags, data)) return FALSE;
				fflags = FALSE;
			}
			x0 = x1;
			y0 = y1;
			break;
		default:
			fprintf (stderr, "Invalid ArtVpath code '%d'\n", vp->code);
			return FALSE;
			break;
		}
	}
	if (cflags && ((x0 != sx) || (y0 != sy))) {
		/* Have to close previous subpath */
		if (gv->lineto) {
			flags = NR_PATH_CLOSED | NR_PATH_LAST;
			if (!gv->lineto (x0, y0, sx, sy, flags, data)) return FALSE;
		}
	}
	if (gv->endpath) {
		flags = cflags | fflags | NR_PATH_LAST;
		if (!gv->endpath (x0, y0, sx, sy, flags, data)) return FALSE;
	}
	return TRUE;
}

NRDynamicPath *
nr_dynamic_path_new (unsigned int nelements)
{
	NRDynamicPath *dpath;
	nelements = MIN (nelements, 4);
	dpath = (NRDynamicPath *) malloc (sizeof (NRDynamicPath));
	memset (dpath, 0x0, sizeof (NRDynamicPath));
	dpath->path = (NRPath *) malloc (sizeof (NRPath) + nelements * sizeof (NRPathElement) - sizeof (NRPathElement));
	dpath->path->nelements = 0;
	dpath->path->offset = 0;
	dpath->path->nsegments = 0;
	dpath->refcount = 1;
	dpath->size = nelements;
	dpath->hascpt = 0;
	dpath->isstatic = 0;
	return dpath;
}

NRDynamicPath *
nr_dynamic_path_ref (NRDynamicPath *dpath)
{
	if (!dpath || (dpath->refcount < 1)) return NULL;
	dpath->refcount += 1;
	return dpath;
}

NRDynamicPath *
nr_dynamic_path_unref (NRDynamicPath *dpath)
{
	if (!dpath || (dpath->refcount < 1)) return NULL;
	dpath->refcount -= 1;
	if (dpath->refcount < 1) {
		free (dpath->path);
		free (dpath);
	}
	return NULL;
}

/* Return TRUE on success */

unsigned int
nr_dynamic_path_moveto (NRDynamicPath *dpath, float x0, float y0)
{
	/* Finish segment and code */
	dpath->segstart = dpath->path->offset + dpath->path->nelements;
	dpath->codepos = dpath->segstart;
	/* Set currentpoint */
	dpath->hascpt = 1;
	dpath->cpx = x0;
	dpath->cpy = y0;
	return TRUE;
}

static void
nr_dynamic_path_ensure_space (NRDynamicPath *dpath, unsigned int req)
{
	if ((dpath->size - (dpath->path->offset + dpath->path->nelements)) < req) {
		req = (dpath->path->offset + dpath->path->nelements) + req - dpath->size;
		req = MIN (req, 32);
		dpath->size += req;
		dpath->path = realloc (dpath->path, sizeof (NRPath) + (dpath->size - 1) * sizeof (NRPathElement));
	}
}

unsigned int
nr_dynamic_path_lineto (NRDynamicPath *dpath, float x1, float y1)
{
	NRPathElement *el, *sel;
	if (!dpath->hascpt) return FALSE;
	if (dpath->segstart == (dpath->path->offset + dpath->path->nelements)) {
		/* Segment is not started yet */
		/* Need len, x0, y0, LINETO, x1, y1 */
		nr_dynamic_path_ensure_space (dpath, 6);
		el = &dpath->path->elements[dpath->path->offset + dpath->path->nelements];
		NR_PATH_ELEMENT_SET_LENGTH (el, 6);
		NR_PATH_ELEMENT_SET_CLOSED (el, 0);
		NR_PATH_ELEMENT_SET_VALUE (el + 1, dpath->cpx);
		NR_PATH_ELEMENT_SET_VALUE (el + 2, dpath->cpy);
		NR_PATH_ELEMENT_SET_LENGTH (el + 3, 1);
		NR_PATH_ELEMENT_SET_CODE (el + 3, NR_PATH_LINETO);
		NR_PATH_ELEMENT_SET_VALUE (el + 4, x1);
		NR_PATH_ELEMENT_SET_VALUE (el + 5, y1);
		dpath->codepos = (dpath->path->offset + dpath->path->nelements) + 3;
		dpath->path->nelements += 6;
		dpath->path->nsegments += 1;
	} else if (NR_PATH_ELEMENT_CODE (&dpath->path->elements[dpath->codepos]) != NR_PATH_LINETO) {
		/* Have to start new lineto */
		/* Need LINETO, x1, y1 */
		nr_dynamic_path_ensure_space (dpath, 3);
		el = &dpath->path->elements[dpath->path->offset + dpath->path->nelements];
		NR_PATH_ELEMENT_SET_LENGTH (el, 1);
		NR_PATH_ELEMENT_SET_CODE (el, NR_PATH_LINETO);
		NR_PATH_ELEMENT_SET_VALUE (el + 1, x1);
		NR_PATH_ELEMENT_SET_VALUE (el + 2, y1);
		dpath->codepos = (dpath->path->offset + dpath->path->nelements);
		dpath->path->nelements += 3;
	} else {
		/* Continue existing lineto */
		/* Need x1, y1 */
		nr_dynamic_path_ensure_space (dpath, 2);
		el = &dpath->path->elements[dpath->path->offset + dpath->path->nelements];
		NR_PATH_ELEMENT_SET_VALUE (el, x1);
		NR_PATH_ELEMENT_SET_VALUE (el + 1, y1);
		el = &dpath->path->elements[dpath->codepos];
		NR_PATH_ELEMENT_SET_LENGTH (el, NR_PATH_ELEMENT_LENGTH (el) + 1);
		dpath->path->nelements += 2;
	}
	sel = &dpath->path->elements[dpath->segstart];
	NR_PATH_ELEMENT_SET_LENGTH (sel, dpath->path->nelements - dpath->segstart);
	return TRUE;
}

unsigned int
nr_dynamic_path_curveto2 (NRDynamicPath *dpath, float x1, float y1, float x2, float y2)
{
	NRPathElement *el, *sel;
	if (!dpath->hascpt) return FALSE;
	if (dpath->segstart == (dpath->path->offset + dpath->path->nelements)) {
		/* Segment is not started yet */
		/* Need len, x0, y0, CURVETO2, x1, y1 x2 y2 */
		nr_dynamic_path_ensure_space (dpath, 8);
		el = &dpath->path->elements[dpath->path->offset + dpath->path->nelements];
		NR_PATH_ELEMENT_SET_LENGTH (el, 8);
		NR_PATH_ELEMENT_SET_CLOSED (el, 0);
		NR_PATH_ELEMENT_SET_VALUE (el + 1, dpath->cpx);
		NR_PATH_ELEMENT_SET_VALUE (el + 2, dpath->cpy);
		NR_PATH_ELEMENT_SET_LENGTH (el + 3, 1);
		NR_PATH_ELEMENT_SET_CODE (el + 3, NR_PATH_CURVETO2);
		NR_PATH_ELEMENT_SET_VALUE (el + 4, x1);
		NR_PATH_ELEMENT_SET_VALUE (el + 5, y1);
		NR_PATH_ELEMENT_SET_VALUE (el + 6, x2);
		NR_PATH_ELEMENT_SET_VALUE (el + 7, y2);
		dpath->codepos = (dpath->path->offset + dpath->path->nelements) + 3;
		dpath->path->nelements += 8;
		dpath->path->nsegments += 1;
	} else if (NR_PATH_ELEMENT_CODE (&dpath->path->elements[dpath->codepos]) != NR_PATH_CURVETO2) {
		/* Have to start new lineto */
		/* Need CURVETO2, x1, y1 x2 y2 */
		nr_dynamic_path_ensure_space (dpath, 5);
		el = &dpath->path->elements[dpath->path->offset + dpath->path->nelements];
		NR_PATH_ELEMENT_SET_LENGTH (el, 1);
		NR_PATH_ELEMENT_SET_CODE (el, NR_PATH_CURVETO2);
		NR_PATH_ELEMENT_SET_VALUE (el + 1, x1);
		NR_PATH_ELEMENT_SET_VALUE (el + 2, y1);
		NR_PATH_ELEMENT_SET_VALUE (el + 3, x2);
		NR_PATH_ELEMENT_SET_VALUE (el + 4, y2);
		dpath->codepos = (dpath->path->offset + dpath->path->nelements);
		dpath->path->nelements += 5;
	} else {
		/* Continue existing lineto */
		/* Need x1, y1 x2 y2 */
		nr_dynamic_path_ensure_space (dpath, 4);
		el = &dpath->path->elements[dpath->path->offset + dpath->path->nelements];
		NR_PATH_ELEMENT_SET_VALUE (el, x1);
		NR_PATH_ELEMENT_SET_VALUE (el + 1, y1);
		NR_PATH_ELEMENT_SET_VALUE (el + 2, x2);
		NR_PATH_ELEMENT_SET_VALUE (el + 3, y2);
		el = &dpath->path->elements[dpath->codepos];
		NR_PATH_ELEMENT_SET_LENGTH (el, NR_PATH_ELEMENT_LENGTH (el) + 1);
		dpath->path->nelements += 4;
	}
	sel = &dpath->path->elements[dpath->segstart];
	NR_PATH_ELEMENT_SET_LENGTH (sel, dpath->path->nelements - dpath->segstart);
	return TRUE;
}

unsigned int
nr_dynamic_path_curveto3 (NRDynamicPath *dpath, float x1, float y1, float x2, float y2, float x3, float y3)
{
	NRPathElement *el, *sel;
	if (!dpath->hascpt) return FALSE;
	if (dpath->segstart == (dpath->path->offset + dpath->path->nelements)) {
		/* Segment is not started yet */
		/* Need len, x0, y0, CURVETO3, x1, y1 x2 y2 x3 y3 */
		nr_dynamic_path_ensure_space (dpath, 10);
		el = &dpath->path->elements[dpath->path->offset + dpath->path->nelements];
		NR_PATH_ELEMENT_SET_LENGTH (el, 10);
		NR_PATH_ELEMENT_SET_CLOSED (el, 0);
		NR_PATH_ELEMENT_SET_VALUE (el + 1, dpath->cpx);
		NR_PATH_ELEMENT_SET_VALUE (el + 2, dpath->cpy);
		NR_PATH_ELEMENT_SET_LENGTH (el + 3, 1);
		NR_PATH_ELEMENT_SET_CODE (el + 3, NR_PATH_CURVETO3);
		NR_PATH_ELEMENT_SET_VALUE (el + 4, x1);
		NR_PATH_ELEMENT_SET_VALUE (el + 5, y1);
		NR_PATH_ELEMENT_SET_VALUE (el + 6, x2);
		NR_PATH_ELEMENT_SET_VALUE (el + 7, y2);
		NR_PATH_ELEMENT_SET_VALUE (el + 8, x3);
		NR_PATH_ELEMENT_SET_VALUE (el + 9, y3);
		dpath->codepos = (dpath->path->offset + dpath->path->nelements) + 3;
		dpath->path->nelements += 10;
		dpath->path->nsegments += 1;
	} else if (NR_PATH_ELEMENT_CODE (&dpath->path->elements[dpath->codepos]) != NR_PATH_CURVETO3) {
		/* Have to start new lineto */
		/* Need LINETO, x1, y1 x2 y2 x3 y3 */
		nr_dynamic_path_ensure_space (dpath, 7);
		el = &dpath->path->elements[dpath->path->offset + dpath->path->nelements];
		NR_PATH_ELEMENT_SET_LENGTH (el, 1);
		NR_PATH_ELEMENT_SET_CODE (el, NR_PATH_CURVETO3);
		NR_PATH_ELEMENT_SET_VALUE (el + 1, x1);
		NR_PATH_ELEMENT_SET_VALUE (el + 2, y1);
		NR_PATH_ELEMENT_SET_VALUE (el + 3, x2);
		NR_PATH_ELEMENT_SET_VALUE (el + 4, y2);
		NR_PATH_ELEMENT_SET_VALUE (el + 5, x3);
		NR_PATH_ELEMENT_SET_VALUE (el + 6, y3);
		dpath->codepos = (dpath->path->offset + dpath->path->nelements);
		dpath->path->nelements += 7;
	} else {
		/* Continue existing lineto */
		/* Need x1, y1 x2 y2 x3 y3 */
		nr_dynamic_path_ensure_space (dpath, 6);
		el = &dpath->path->elements[dpath->path->offset + dpath->path->nelements];
		NR_PATH_ELEMENT_SET_VALUE (el, x1);
		NR_PATH_ELEMENT_SET_VALUE (el + 1, y1);
		NR_PATH_ELEMENT_SET_VALUE (el + 2, x2);
		NR_PATH_ELEMENT_SET_VALUE (el + 3, y2);
		NR_PATH_ELEMENT_SET_VALUE (el + 4, x3);
		NR_PATH_ELEMENT_SET_VALUE (el + 5, y3);
		el = &dpath->path->elements[dpath->codepos];
		NR_PATH_ELEMENT_SET_LENGTH (el, NR_PATH_ELEMENT_LENGTH (el) + 1);
		dpath->path->nelements += 6;
	}
	sel = &dpath->path->elements[dpath->segstart];
	NR_PATH_ELEMENT_SET_LENGTH (sel, dpath->path->nelements - dpath->segstart);
	return TRUE;
}

unsigned int
nr_dynamic_path_closepath (NRDynamicPath *dpath)
{
	NRPathElement *sel, *el;
	if (!dpath->hascpt) return FALSE;
	if (dpath->segstart == dpath->path->nelements) return FALSE;
	sel = &dpath->path->elements[dpath->segstart];
	el = &dpath->path->elements[dpath->path->offset + dpath->path->nelements - 2];
#if 0
	if ((NR_PATH_ELEMENT_VALUE (sel) != NR_PATH_ELEMENT_VALUE (el)) ||
	    (NR_PATH_ELEMENT_VALUE (sel + 1) != NR_PATH_ELEMENT_VALUE (el + 1))) {
		nr_dynamic_path_lineto (dpath, NR_PATH_ELEMENT_VALUE (sel), NR_PATH_ELEMENT_VALUE (sel + 1));
	}
#endif
	NR_PATH_ELEMENT_SET_CLOSED (sel, 1);
	dpath->hascpt = FALSE;
	return TRUE;
}

/* ---------- older stuff ---------- */

static void nr_curve_bbox (double x000, double y000, double x001, double y001, double x011, double y011, double x111, double y111, NRRectF *bbox);

NRBPath *
nr_path_duplicate_transform (NRBPath *d, NRBPath *s, NRMatrixF *transform)
{
	int i;

	if (!s->path) {
		d->path = NULL;
		return d;
	}

	i = 0;
	while (s->path[i].code != ART_END) i += 1;

	d->path = art_new (ArtBpath, i + 1);

	i = 0;
	while (s->path[i].code != ART_END) {
		d->path[i].code = s->path[i].code;
		if (s->path[i].code == ART_CURVETO) {
			d->path[i].x1 = NR_MATRIX_DF_TRANSFORM_X (transform, s->path[i].x1, s->path[i].y1);
			d->path[i].y1 = NR_MATRIX_DF_TRANSFORM_Y (transform, s->path[i].x1, s->path[i].y1);
			d->path[i].x2 = NR_MATRIX_DF_TRANSFORM_X (transform, s->path[i].x2, s->path[i].y2);
			d->path[i].y2 = NR_MATRIX_DF_TRANSFORM_Y (transform, s->path[i].x2, s->path[i].y2);
		}
		d->path[i].x3 = NR_MATRIX_DF_TRANSFORM_X (transform, s->path[i].x3, s->path[i].y3);
		d->path[i].y3 = NR_MATRIX_DF_TRANSFORM_Y (transform, s->path[i].x3, s->path[i].y3);
		i += 1;
	}
	d->path[i].code = ART_END;

	return d;
}

void
nr_line_wind_distance (double Ax, double Ay, double Bx, double By, double Px, double Py, int *wind, float *best)
{
	double Dx, Dy, s;
	double dist2;

	/* Find distance */
	Dx = Bx - Ax;
	Dy = By - Ay;

	if (best) {
		s = ((Px - Ax) * Dx + (Py - Ay) * Dy) / (Dx * Dx + Dy * Dy);
		if (s <= 0.0) {
			dist2 = (Px - Ax) * (Px - Ax) + (Py - Ay) * (Py - Ay);
		} else if (s >= 1.0) {
			dist2 = (Px - Bx) * (Px - Bx) + (Py - By) * (Py - By);
		} else {
			double Qx, Qy;
			Qx = Ax + s * Dx;
			Qy = Ay + s * Dy;
			dist2 = (Px - Qx) * (Px - Qx) + (Py - Qy) * (Py - Qy);
		}
		if (dist2 < (*best * *best)) *best = (float) sqrt (dist2);
	}

	if (wind) {
		/* Find wind */
		if ((Ax >= Px) && (Bx >= Px)) return;
		if ((Ay >= Py) && (By >= Py)) return;
		if ((Ay < Py) && (By < Py)) return;
		if (Ay == By) return;
		/* Ctach upper y bound */
		if (Ay == Py) {
			if (Ax < Px) *wind -= 1;
			return;
		} else if (By == Py) {
			if (Bx < Px) *wind += 1;
			return;
		} else {
			double Qx;
			/* Have to calculate intersection */
			Qx = Ax + Dx * (Py - Ay) / Dy;
			if (Qx < Px) {
				*wind += (Dy > 0.0) ? 1 : -1;
			}
		}
	}
}

static void
nr_curve_bbox_wind_distance (double x000, double y000,
			     double x001, double y001,
			     double x011, double y011,
			     double x111, double y111,
			     NRPointF *pt,
			     NRRectF *bbox, int *wind, float *best,
			     float tolerance)
{
	double x0, y0, x1, y1, len2;
	double Px, Py;
	int needdist, needwind, needline;

	Px = pt->x;
	Py = pt->y;

	needdist = 0;
	needwind = 0;
	needline = 0;

	if (bbox) nr_curve_bbox (x000, y000, x001, y001, x011, y011, x111, y111, bbox);

	x0 = MIN (x000, x001);
	x0 = MIN (x0, x011);
	x0 = MIN (x0, x111);
	y0 = MIN (y000, y001);
	y0 = MIN (y0, y011);
	y0 = MIN (y0, y111);
	x1 = MAX (x000, x001);
	x1 = MAX (x1, x011);
	x1 = MAX (x1, x111);
	y1 = MAX (y000, y001);
	y1 = MAX (y1, y011);
	y1 = MAX (y1, y111);

	if (best) {
		/* Quicly adjust to endpoints */
		len2 = (x000 - Px) * (x000 - Px) + (y000 - Py) * (y000 - Py);
		if (len2 < (*best * *best)) *best = (float) sqrt (len2);
		len2 = (x111 - Px) * (x111 - Px) + (y111 - Py) * (y111 - Py);
		if (len2 < (*best * *best)) *best = (float) sqrt (len2);

		if (((x0 - Px) < *best) && ((y0 - Py) < *best) && ((Px - x1) < *best) && ((Py - y1) < *best)) {
			/* Point is inside sloppy bbox */
			/* Now we have to decide, whether subdivide */
			/* fixme: (Lauris) */
			if (((y1 - y0) > 5.0) || ((x1 - x0) > 5.0)) {
				needdist = 1;
			} else {
				needline = 1;
			}
		}
	}
	if (!needdist && wind) {
		if ((y1 >= Py) && (y0 < Py) && (x0 < Px)) {
			/* Possible intersection at the left */
			/* Now we have to decide, whether subdivide */
			/* fixme: (Lauris) */
			if (((y1 - y0) > 5.0) || ((x1 - x0) > 5.0)) {
				needwind = 1;
			} else {
				needline = 1;
			}
		}
	}

	if (needdist || needwind) {
		double x00t, x0tt, xttt, x1tt, x11t, x01t;
		double y00t, y0tt, yttt, y1tt, y11t, y01t;
		double s, t;

		t = 0.5;
		s = 1 - t;

		x00t = s * x000 + t * x001;
		x01t = s * x001 + t * x011;
		x11t = s * x011 + t * x111;
		x0tt = s * x00t + t * x01t;
		x1tt = s * x01t + t * x11t;
		xttt = s * x0tt + t * x1tt;

		y00t = s * y000 + t * y001;
		y01t = s * y001 + t * y011;
		y11t = s * y011 + t * y111;
		y0tt = s * y00t + t * y01t;
		y1tt = s * y01t + t * y11t;
		yttt = s * y0tt + t * y1tt;

		nr_curve_bbox_wind_distance (x000, y000, x00t, y00t, x0tt, y0tt, xttt, yttt, pt, NULL, wind, best, tolerance);
		nr_curve_bbox_wind_distance (xttt, yttt, x1tt, y1tt, x11t, y11t, x111, y111, pt, NULL, wind, best, tolerance);
	} else if (1 || needline) {
		nr_line_wind_distance (x000, y000, x111, y111, pt->x, pt->y, wind, best);
	}
}

void
nr_path_matrix_f_point_f_bbox_wind_distance (NRBPath *bpath, NRMatrixF *m, NRPointF *pt,
					     NRRectF *bbox, int *wind, float *dist,
					     float tolerance)
{
	double x0, y0, x3, y3;
	const ArtBpath *p;

	if (!bpath->path) {
		if (wind) *wind = 0;
		if (dist) *dist = NR_HUGE_F;
		return;
	}

	if (!m) m = &NR_MATRIX_F_IDENTITY;

	x0 = y0 = 0.0;
	x3 = y3 = 0.0;

	for (p = bpath->path; p->code != ART_END; p+= 1) {
		switch (p->code) {
		case ART_MOVETO_OPEN:
		case ART_MOVETO:
			x0 = m->c[0] * p->x3 + m->c[2] * p->y3 + m->c[4];
			y0 = m->c[1] * p->x3 + m->c[3] * p->y3 + m->c[5];
			if (bbox) {
				bbox->x0 = (float) MIN (bbox->x0, x0);
				bbox->y0 = (float) MIN (bbox->y0, y0);
				bbox->x1 = (float) MAX (bbox->x1, x0);
				bbox->y1 = (float) MAX (bbox->y1, y0);
			}
			break;
		case ART_LINETO:
			x3 = m->c[0] * p->x3 + m->c[2] * p->y3 + m->c[4];
			y3 = m->c[1] * p->x3 + m->c[3] * p->y3 + m->c[5];
			if (bbox) {
				bbox->x0 = (float) MIN (bbox->x0, x3);
				bbox->y0 = (float) MIN (bbox->y0, y3);
				bbox->x1 = (float) MAX (bbox->x1, x3);
				bbox->y1 = (float) MAX (bbox->y1, y3);
			}
			if (dist || wind) {
				nr_line_wind_distance (x0, y0, x3, y3, pt->x, pt->y, wind, dist);
			}
			x0 = x3;
			y0 = y3;
			break;
		case ART_CURVETO:
			x3 = m->c[0] * p->x3 + m->c[2] * p->y3 + m->c[4];
			y3 = m->c[1] * p->x3 + m->c[3] * p->y3 + m->c[5];
			nr_curve_bbox_wind_distance (x0, y0,
						     m->c[0] * p->x1 + m->c[2] * p->y1 + m->c[4],
						     m->c[1] * p->x1 + m->c[3] * p->y1 + m->c[5],
						     m->c[0] * p->x2 + m->c[2] * p->y2 + m->c[4],
						     m->c[1] * p->x2 + m->c[3] * p->y2 + m->c[5],
						     x3, y3,
						     pt,
						     bbox, wind, dist, tolerance);
			x0 = x3;
			y0 = y3;
			break;
		default:
			break;
		}
	}
}

/* Fast bbox calculation */
/* Thanks to Nathan Hurst for suggesting it */

static void
nr_curve_bbox (double x000, double y000, double x001, double y001, double x011, double y011, double x111, double y111, NRRectF *bbox)
{
	double a, b, c, D;

	bbox->x0 = (float) MIN (bbox->x0, x111);
	bbox->y0 = (float) MIN (bbox->y0, y111);
	bbox->x1 = (float) MAX (bbox->x1, x111);
	bbox->y1 = (float) MAX (bbox->y1, y111);

	/*
	 * xttt = s * (s * (s * x000 + t * x001) + t * (s * x001 + t * x011)) + t * (s * (s * x001 + t * x011) + t * (s * x011 + t * x111))
	 * xttt = s * (s2 * x000 + s * t * x001 + t * s * x001 + t2 * x011) + t * (s2 * x001 + s * t * x011 + t * s * x011 + t2 * x111)
	 * xttt = s * (s2 * x000 + 2 * st * x001 + t2 * x011) + t * (s2 * x001 + 2 * st * x011 + t2 * x111)
	 * xttt = s3 * x000 + 2 * s2t * x001 + st2 * x011 + s2t * x001 + 2st2 * x011 + t3 * x111
	 * xttt = s3 * x000 + 3s2t * x001 + 3st2 * x011 + t3 * x111
	 * xttt = s3 * x000 + (1 - s) 3s2 * x001 + (1 - s) * (1 - s) * 3s * x011 + (1 - s) * (1 - s) * (1 - s) * x111
	 * xttt = s3 * x000 + (3s2 - 3s3) * x001 + (3s - 6s2 + 3s3) * x011 + (1 - 2s + s2 - s + 2s2 - s3) * x111
	 * xttt = (x000 - 3 * x001 + 3 * x011 -     x111) * s3 +
	 *        (       3 * x001 - 6 * x011 + 3 * x111) * s2 +
	 *        (                  3 * x011 - 3 * x111) * s  +
	 *        (                                 x111)
	 * xttt' = (3 * x000 - 9 * x001 +  9 * x011 - 3 * x111) * s2 +
	 *         (           6 * x001 - 12 * x011 + 6 * x111) * s  +
	 *         (                       3 * x011 - 3 * x111)
	 */

	/* a * s2 + b * s + c == 0 */

	a = 3 * x000 - 9 * x001 + 9 * x011 - 3 * x111;
	b = 6 * x001 - 12 * x011 + 6 * x111;
	c = 3 * x011 - 3 * x111;

	if (fabs (a) < NR_EPSILON_F) {
		/* s = -c / b */
		if (fabs (b) > NR_EPSILON_F) {
			double s, t, xttt;
			s = -c / b;
			if ((s > 0.0) && (s < 1.0)) {
				t = 1.0 - s;
				xttt = s * s * s * x000 + 3 * s * s * t * x001 + 3 * s * t * t * x011 + t * t * t * x111;
				bbox->x0 = (float) MIN (bbox->x0, xttt);
				bbox->x1 = (float) MAX (bbox->x1, xttt);
			}
		}
	} else {
		/* s = (-b +/- sqrt (b * b - 4 * a * c)) / 2 * a; */
		D = b * b - 4 * a * c;
		if (D >= 0.0) {
			double d, s, t, xttt;
			/* Have solution */
			d = sqrt (D);
			s = (-b + d) / (2 * a);
			if ((s > 0.0) && (s < 1.0)) {
				t = 1.0 - s;
				xttt = s * s * s * x000 + 3 * s * s * t * x001 + 3 * s * t * t * x011 + t * t * t * x111;
				bbox->x0 = (float) MIN (bbox->x0, xttt);
				bbox->x1 = (float) MAX (bbox->x1, xttt);
			}
			s = (-b - d) / (2 * a);
			if ((s > 0.0) && (s < 1.0)) {
				t = 1.0 - s;
				xttt = s * s * s * x000 + 3 * s * s * t * x001 + 3 * s * t * t * x011 + t * t * t * x111;
				bbox->x0 = (float) MIN (bbox->x0, xttt);
				bbox->x1 = (float) MAX (bbox->x1, xttt);
			}
		}
	}

	a = 3 * y000 - 9 * y001 + 9 * y011 - 3 * y111;
	b = 6 * y001 - 12 * y011 + 6 * y111;
	c = 3 * y011 - 3 * y111;

	if (fabs (a) < NR_EPSILON_F) {
		/* s = -c / b */
		if (fabs (b) > NR_EPSILON_F) {
			double s, t, yttt;
			s = -c / b;
			if ((s > 0.0) && (s < 1.0)) {
				t = 1.0 - s;
				yttt = s * s * s * y000 + 3 * s * s * t * y001 + 3 * s * t * t * y011 + t * t * t * y111;
				bbox->y0 = (float) MIN (bbox->y0, yttt);
				bbox->y1 = (float) MAX (bbox->y1, yttt);
			}
		}
	} else {
		/* s = (-b +/- sqrt (b * b - 4 * a * c)) / 2 * a; */
		D = b * b - 4 * a * c;
		if (D >= 0.0) {
			double d, s, t, yttt;
			/* Have solution */
			d = sqrt (D);
			s = (-b + d) / (2 * a);
			if ((s > 0.0) && (s < 1.0)) {
				t = 1.0 - s;
				yttt = s * s * s * y000 + 3 * s * s * t * y001 + 3 * s * t * t * y011 + t * t * t * y111;
				bbox->y0 = (float) MIN (bbox->y0, yttt);
				bbox->y1 = (float) MAX (bbox->y1, yttt);
			}
			s = (-b - d) / (2 * a);
			if ((s > 0.0) && (s < 1.0)) {
				t = 1.0 - s;
				yttt = s * s * s * y000 + 3 * s * s * t * y001 + 3 * s * t * t * y011 + t * t * t * y111;
				bbox->y0 = (float) MIN (bbox->y0, yttt);
				bbox->y1 = (float) MAX (bbox->y1, yttt);
			}
		}
	}
}

void
nr_path_matrix_f_bbox_f_union (NRBPath *bpath, NRMatrixF *m,
			       NRRectF *bbox,
			       float tolerance)
{
	double x0, y0, x3, y3;
	const ArtBpath *p;

	if (!bpath->path) return;

	if (!m) m = &NR_MATRIX_F_IDENTITY;

	x0 = y0 = 0.0;
	x3 = y3 = 0.0;

	for (p = bpath->path; p->code != ART_END; p+= 1) {
		switch (p->code) {
		case ART_MOVETO_OPEN:
		case ART_MOVETO:
			x0 = m->c[0] * p->x3 + m->c[2] * p->y3 + m->c[4];
			y0 = m->c[1] * p->x3 + m->c[3] * p->y3 + m->c[5];
			bbox->x0 = (float) MIN (bbox->x0, x0);
			bbox->y0 = (float) MIN (bbox->y0, y0);
			bbox->x1 = (float) MAX (bbox->x1, x0);
			bbox->y1 = (float) MAX (bbox->y1, y0);
			break;
		case ART_LINETO:
			x3 = m->c[0] * p->x3 + m->c[2] * p->y3 + m->c[4];
			y3 = m->c[1] * p->x3 + m->c[3] * p->y3 + m->c[5];
			bbox->x0 = (float) MIN (bbox->x0, x3);
			bbox->y0 = (float) MIN (bbox->y0, y3);
			bbox->x1 = (float) MAX (bbox->x1, x3);
			bbox->y1 = (float) MAX (bbox->y1, y3);
			x0 = x3;
			y0 = y3;
			break;
		case ART_CURVETO:
			x3 = m->c[0] * p->x3 + m->c[2] * p->y3 + m->c[4];
			y3 = m->c[1] * p->x3 + m->c[3] * p->y3 + m->c[5];
			nr_curve_bbox (x0, y0,
				       m->c[0] * p->x1 + m->c[2] * p->y1 + m->c[4],
				       m->c[1] * p->x1 + m->c[3] * p->y1 + m->c[5],
				       m->c[0] * p->x2 + m->c[2] * p->y2 + m->c[4],
				       m->c[1] * p->x2 + m->c[3] * p->y2 + m->c[5],
				       x3, y3,
				       bbox);
			x0 = x3;
			y0 = y3;
			break;
		default:
			break;
		}
	}
}


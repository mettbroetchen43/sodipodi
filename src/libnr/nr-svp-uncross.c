#define __NR_SVP_UNCROSS_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#define noNR_EXTRA_CHECK

#define NR_QUANT_Y 16.0
#define NR_COORD_SNAP(v) (floor (NR_QUANT_Y * (v) + 0.5) / NR_QUANT_Y)
#define NR_COORD_SNAP_UP(v) (ceil (NR_QUANT_Y * (v)) / NR_QUANT_Y)
#define NR_COORD_SNAP_DOWN(v) (floor (NR_QUANT_Y * (v)) / NR_QUANT_Y)
#define NR_COORD_TOLERANCE 0.01

#include <math.h>
#include <assert.h>
#include <stdio.h>

#include "nr-macros.h"
#include "nr-values.h"
#include "nr-svp-uncross.h"

typedef struct _NRSVPSlice NRSVPSlice;

struct _NRSVPSlice {
	NRSVPSlice *next;
	NRSVP *svp;
	NRVertex *vertex;
	NRCoord x;
	NRCoord y;
};

static NRSVP *nr_svp_slice_break (NRSVPSlice *s, double x, double y, NRSVP *svp);
static NRSVP *nr_svp_slice_break_y_and_continue_x (NRSVPSlice *s, double y, double x, NRSVP *svp, double ytest, NRFlat **flats);

static NRSVPSlice *nr_svp_slice_new (NRSVP *svp, NRCoord y);
static void nr_svp_slice_free_one (NRSVPSlice *slice);
static int nr_svp_slice_compare (NRSVPSlice *l, NRSVPSlice *r);
static NRSVPSlice *nr_svp_slice_insert_sorted (NRSVPSlice *start, NRSVPSlice *slice);

static NRSVPSlice *nr_svp_slice_stretch_list (NRSVPSlice *slices, NRCoord y);

#ifdef NR_EXTRA_CHECK
static void
nr_svp_test_slices (NRSVPSlice *slices, double yslice, const unsigned char *prefix, int colinear, int test, int close)
{
	NRSVPSlice *s;
	int wind;
	wind = 0;
	for (s = slices; s && s->next; s = s->next) {
		NRSVPSlice *cs, *ns;
		double cx0, cy0, cx1, cy1, cx;
		double nx0, ny0, nx1, ny1, nx;
		cs = s;
		ns = s->next;
		cx0 = cs->vertex->x;
		cy0 = cs->vertex->y;
		cx1 = cs->vertex->next->x;
		cy1 = cs->vertex->next->y;
		if (yslice == cy0) {
			cx = cx0;
		} else {
			cx = cx0 + (cx1 - cx0) * (yslice - cy0) / (cy1 - cy0);
		}
		nx0 = ns->vertex->x;
		ny0 = ns->vertex->y;
		nx1 = ns->vertex->next->x;
		ny1 = ns->vertex->next->y;
		if (yslice == ny0) {
			nx = nx0;
		} else {
			nx = nx0 + (nx1 - nx0) * (yslice - ny0) / (ny1 - ny0);
		}
		if (cx > nx) {
			printf ("%s: Slice x %g > %g\n", prefix, cx, nx);
		} else if (cx == nx) {
			double ldx, ldy, rdx, rdy;
			double d;
			ldx = cx1 - cx;
			ldy = cy1 - yslice;
			rdx = nx1 - nx;
			rdy = ny1 - yslice;
			d = ldx * rdy - ldy * rdx;
			if (d == 0.0) {
				if (colinear) printf ("%s: Slice x %g COLINEAR %g\n", prefix, cx, nx);
			} else if (d > 0.0) {
				if (test) printf ("%s: Slice x %g > TEST %g\n", prefix, cx, nx);
			}
		} else if (cx > (nx - NR_EPSILON_F)) {
			if (close) printf ("%s: Slice x %g < EPSILON %g\n", prefix, cx, nx);
		}
		wind += s->svp->wind;
	}
	if (s) wind += s->svp->wind;
	if (wind & 1) printf ("%s: Weird wind %d\n", prefix, wind);
}
#define CHECK_SLICES(s,y,p,c,t,n) nr_svp_test_slices (s,y,p,c,t,n)
#else
#define CHECK_SLICES(s,y,p,c,t,n)
#endif

NRSVP *
nr_svp_uncross_full (NRSVP *svp, NRFlat *flats)
{
	NRSVP *lsvp, *csvp, *nsvp;
	NRFlat *nflat, *fl, *f;
	NRSVPSlice *slices, *s;
	NRCoord yslice, ynew;

	if (!svp) return NULL;
	assert (svp->vertex);

	slices = NULL;

	/* First slicing position */
	yslice = svp->vertex->y;
	nflat = flats;
	/* Drop all flats below initial slice */
	/* Equal can be dropped too in given case */
	fl = NULL;
	for (f = nflat; f && (f->y <= yslice); f = f->next) fl = f;
	if (fl) {
		fl->next = NULL;
		nr_flat_free_list (nflat);
		nflat = f;
	}

	/* fixme: the lsvp stuff is really braindead */
	lsvp = NULL;
	csvp = svp;
	nsvp = svp;

	/* Main iteration */
	while ((slices) || (nsvp)) {
		NRSVPSlice * ss, * cs, * ns;
		NRVertex * newvertex;
		NRSVP * newsvp;
		int wind;

		/* Add svps == starting from yslice to slices list */
		assert (!nsvp || (nsvp->vertex->y >= yslice));
		while ((nsvp) && (nsvp->vertex->y == yslice)) {
			NRSVPSlice * newslice;
			newslice = nr_svp_slice_new (nsvp, yslice);
			slices = nr_svp_slice_insert_sorted (slices, newslice);
			nsvp = nsvp->next;
		}
		/* Now everything should be set up */
		CHECK_SLICES (slices, yslice, "PRE", 0, 0, 1);
		/* Process intersections */
		/* This is bitch */
		ss = NULL;
		cs = slices;
		while (cs && cs->next) {
			ns = cs->next;
			if (cs->x > ns->x) {
				/* Something is seriously messed up */
				/* Try to do, what we can */
				/* Break slices */
				csvp = nr_svp_slice_break (cs, cs->x, yslice, csvp);
				csvp = nr_svp_slice_break (ns, ns->x, yslice, csvp);
				/* Set the new starting point */
				f = nr_flat_new_full (yslice, ns->x, cs->x);
				nflat = nr_flat_insert_sorted (nflat, f);
				cs->vertex->x = ns->x;
				cs->x = cs->vertex->x;
				/* Reorder slices */
				if (ss) {
					assert (ns->next != ss);
					ss->next = ns->next;
				} else {
					slices = ns->next;
				}
				slices = nr_svp_slice_insert_sorted (slices, cs);
				slices = nr_svp_slice_insert_sorted (slices, ns);
				CHECK_SLICES (slices, yslice, "CHECK", 0, 0, 1);
				/* Start the row from the beginning */
				ss = NULL;
				cs = slices;
			} else if (cs->x == ns->x) {
				/* test continuation direction */
				if (nr_svp_slice_compare (cs, ns) > 0.0) {
					/* Swap slices */
					assert (ns->next != cs);
					cs->next = ns->next;
					assert (cs != ns);
					ns->next = cs;
					if (ss) {
						assert (ns != ss);
						ss->next = ns;
					} else {
						slices = ns;
					}
					cs = ns;
				} else {
					ss = cs;
					cs = ns;
				}
			} else if ((ns->x - cs->x) <= NR_COORD_TOLERANCE) {
				/* Slices are very close at yslice */
				/* Start by breaking slices */
				csvp = nr_svp_slice_break (cs, cs->x, yslice, csvp);
				csvp = nr_svp_slice_break (ns, ns->x, yslice, csvp);
				/* Set the new starting point */
				if (ns->x > cs->x) {
					f = nr_flat_new_full (yslice, cs->x, ns->x);
					nflat = nr_flat_insert_sorted (nflat, f);
				}
				ns->vertex->x = cs->x;
				ns->x = ns->vertex->x;
				/* Reorder slices */
				if (ss) {
					assert (ns->next != ss);
					ss->next = ns->next;
				} else {
					slices = ns->next;
				}
				slices = nr_svp_slice_insert_sorted (slices, cs);
				slices = nr_svp_slice_insert_sorted (slices, ns);
				CHECK_SLICES (slices, yslice, "CHECK", 0, 0, 1);
				ss = NULL;
				cs = slices;
			} else if ((cs->x > ns->vertex->next->x) || (ns->x < cs->vertex->next->x)) {
				/* (MAX (cs->x, cs->vertex->next->x) > MIN (ns->x, ns->vertex->next->x)) */
				/* Potential intersection */
				double xba, yba, xdc, ydc;
				double d;

				/* Bitch 'o' bitches */
				xba = cs->vertex->next->x - cs->x;
				yba = cs->vertex->next->y - cs->y;
				xdc = ns->vertex->next->x - ns->x;
				ydc = ns->vertex->next->y - ns->y;
				d = xba * ydc - yba * xdc;

				if (fabs (d) > NR_EPSILON_F) {
					double xac, yac, numr, nums;
					double r, s, x, y;

					/* Not parallel */
					xac = cs->vertex->x - ns->vertex->x;
					yac = cs->vertex->y - ns->vertex->y;
					numr = yac * xdc - xac * ydc;
					nums = yac * xba - xac * yba;
					r = numr / d;
					s = nums / d;
					x = cs->vertex->x + r * xba;
					y = cs->vertex->y + r * yba;
					y = NR_COORD_SNAP_DOWN (y);

					if (y == yslice) {
						/* Slices are very close at yslice */
						/* Start by breaking slices */
						csvp = nr_svp_slice_break (cs, cs->x, yslice, csvp);
						csvp = nr_svp_slice_break (ns, ns->x, yslice, csvp);
						if (cs->x != x) {
							double x0, x1;
							x0 = MIN (x, cs->x);
							x1 = MAX (x, cs->x);
							f = nr_flat_new_full (y, x0, x1);
							nflat = nr_flat_insert_sorted (nflat, f);
						}
						if (ns->x != x) {
							double x0, x1;
							x0 = MIN (x, ns->x);
							x1 = MAX (x, ns->x);
							f = nr_flat_new_full (y, x0, x1);
							nflat = nr_flat_insert_sorted (nflat, f);
						}
						/* Set the new starting point */
						cs->vertex->x = x;
						cs->x = cs->vertex->x;
						ns->vertex->x = x;
						ns->x = ns->vertex->x;
						/* Reorder slices */
						if (ss) {
							assert (ns->next != ss);
							ss->next = ns->next;
						} else {
							slices = ns->next;
						}
						slices = nr_svp_slice_insert_sorted (slices, cs);
						slices = nr_svp_slice_insert_sorted (slices, ns);
						CHECK_SLICES (slices, yslice, "CHECK", 0, 0, 1);
						/* Start the row from the beginning */
						ss = NULL;
						cs = slices;
					} else if (y > yslice) {
						if ((y <= cs->vertex->next->y) && (y <= ns->vertex->next->y)) {
							/* Postpone by breaking svp */
							csvp = nr_svp_slice_break_y_and_continue_x (cs, y, x, csvp, yslice, &nflat);
							csvp = nr_svp_slice_break_y_and_continue_x (ns, y, x, csvp, yslice, &nflat);
						}
						/* fixme: Slight disturbance is possible so we should repeat */
						ss = cs;
						cs = ns;
					} else {
						ss = cs;
						cs = ns;
					}
				} else {
					ss = cs;
					cs = ns;
				}
			} else {
				ss = cs;
				cs = ns;
			}
		}
		/* Process flats (NB! we advance nflat to first > y) */
		assert (!nflat || (nflat->y >= yslice));

		fl = NULL;
		for (f = nflat; f && (f->y == yslice); f = f->next) {
			for (s = slices; s != NULL; s = s->next) {
				assert (s->vertex->y <= yslice);
				assert (s->vertex->next->y > yslice);
				/* fixme: We can safely use EPSILON here */
				if ((s->x >= f->x0) && (s->x <= f->x1)) {
					if (s->vertex->y < yslice) {
						/* Mid-segment intersection */
						/* Create continuation svp */
						newvertex = nr_vertex_new_xy (s->x, s->y);
						newvertex->next = s->vertex->next;
						newsvp = nr_svp_new_vertex_wind (newvertex, s->svp->wind);
						/* Trim starting svp */
						newvertex = nr_vertex_new_xy (s->x, s->y);
						s->vertex->next = newvertex;
						nr_svp_calculate_bbox (s->svp);
						/* Insert new SVP into main list */
						/* new svp slice is included by definition */
						csvp = nr_svp_insert_sorted (csvp, newsvp);
						/* fixme: We should maintain pointer to ssvp */
						/* New svp is inserted before nsvp by definition, so we can ignore management */
						/* Old svp will be excluded by definition, so we can shortcut */
						s->svp = newsvp;
						s->vertex = newsvp->vertex;
						/* s->x and s->y are correct by definition */
					} else if (s->vertex != s->svp->vertex) {
						assert (s->vertex->y == yslice);
						/* Inter-segment intersection */
						/* Winding may change here */
						/* Create continuation svp */
						newvertex = nr_vertex_new_xy (s->x, s->y);
						newvertex->next = s->vertex->next;
						newsvp = nr_svp_new_vertex_wind (newvertex, s->svp->wind);
						/* Trim starting svp */
						s->vertex->next = NULL;
						nr_svp_calculate_bbox (s->svp);
						/* Insert new SVP into main list */
						/* new svp slice is included by definition */
						csvp = nr_svp_insert_sorted (csvp, newsvp);
						/* fixme: We should maintain pointer to ssvp */
						/* New svp is inserted before nsvp by definition, so we can ignore management */
						/* Old svp will be excluded by definition, so we can shortcut */
						s->svp = newsvp;
						s->vertex = newsvp->vertex;
						/* s->x and s->y are correct by definition */
					}
				}
			}
			fl = f;
		}
		if (fl) {
			fl->next = NULL;
			nr_flat_free_list (nflat);
			nflat = f;
		}
		CHECK_SLICES (slices, yslice, "POST", 0, 1, 1);
		/* Calculate winds */
		wind = 0;
		for (s = slices; s != NULL; s = s->next) {
			wind += s->svp->wind;
			if (s->y == s->svp->vertex->y) {
				/* Starting SVP */
				/* fixme: winding rules */
				s->svp->wind = (wind & 0x1) ? 1 : -1;
			}
			/* printf ("Wind: %g %s %d %d\n", s->y, s->y == s->svp->vertex->y ? "+" : " ", s->svp->wind, wind); */
		}
		if (wind & 1) printf ("Weird final wind: %d\n", wind);
		/* Calculate next yslice */
		ynew = 1e18;
		for (s = slices; s != NULL; s = s->next) {
			assert (s->vertex->next);
			assert (s->vertex->next->y > yslice);
			if (s->vertex->next->y < ynew) ynew = s->vertex->next->y;
		}
		/* fixme: Keep svp pointers */
		if ((nflat) && (nflat->y < ynew)) ynew = nflat->y;
		nsvp = csvp;
		while ((nsvp) && (nsvp->vertex->y == yslice)) {
			nsvp = nsvp->next;
		}
		if ((nsvp) && (nsvp->vertex->y < ynew)) ynew = nsvp->vertex->y;
		assert (ynew > yslice);
		yslice = ynew;
		/* Stretch existing slices to new position */
		slices = nr_svp_slice_stretch_list (slices, yslice);
		CHECK_SLICES (slices, yslice, "STRETCH", 0, 1, 1);
		/* Advance svp counters */
		if (lsvp) {
			lsvp->next = csvp;
		} else {
			svp = csvp;
		}
		while (csvp && csvp != nsvp) {
			lsvp = csvp;
			csvp = csvp->next;
		}
	}
	if (nflat) nr_flat_free_list (nflat);

	return svp;
}

static NRSVP *
nr_svp_slice_break (NRSVPSlice *s, double x, double y, NRSVP *svp)
{
	NRVertex *newvx;
	NRSVP *newsvp;

	if (s->vertex->y < y) {
		/* Mid-segment intersection */
		/* Create continuation svp */
		newvx = nr_vertex_new_xy (x, y);
		newvx->next = s->vertex->next;
		newsvp = nr_svp_new_vertex_wind (newvx, s->svp->wind);
		assert (newsvp->vertex->y < newsvp->vertex->next->y);
		/* Trim starting svp */
		newvx = nr_vertex_new_xy (x, y);
		s->vertex->next = newvx;
		nr_svp_calculate_bbox (s->svp);
		assert (s->svp->vertex->y < s->svp->vertex->next->y);
		/* Insert new SVP into main list */
		/* new svp slice is included by definition */
		svp = nr_svp_insert_sorted (svp, newsvp);
		/* fixme: We should maintain pointer to ssvp */
		/* New svp is inserted before nsvp by definition, so we can ignore management */
		/* Old svp will be excluded by definition, so we can shortcut */
		s->svp = newsvp;
		s->vertex = newsvp->vertex;
		/* s->x and s->y are correct by definition */
	} else if (s->vertex != s->svp->vertex) {
		assert (s->vertex->y == y);
		/* Inter-segment intersection */
		/* Winding may change here */
		/* Create continuation svp */
		newvx = nr_vertex_new_xy (x, y);
		newvx->next = s->vertex->next;
		newsvp = nr_svp_new_vertex_wind (newvx, s->svp->wind);
		assert (newsvp->vertex->y < newsvp->vertex->next->y);
		/* Trim starting svp */
		s->vertex->next = NULL;
		nr_svp_calculate_bbox (s->svp);
		assert (s->svp->vertex->y < s->svp->vertex->next->y);
		/* Insert new SVP into main list */
		/* new svp slice is included by definition */
		svp = nr_svp_insert_sorted (svp, newsvp);
		/* fixme: We should maintain pointer to ssvp */
		/* New svp is inserted before nsvp by definition, so we can ignore management */
		/* Old svp will be excluded by definition, so we can shortcut */
		s->svp = newsvp;
		s->vertex = newsvp->vertex;
		/* s->x and s->y are correct by definition */
	}
	return svp;
}

static NRSVP *
nr_svp_slice_break_y_and_continue_x (NRSVPSlice *s, double y, double x, NRSVP *svp, double ytest, NRFlat **flats)
{
	NRVertex *newvx;
	NRSVP *newsvp;

	assert (y > s->y);
	assert (y > s->vertex->y);
	assert (y <= s->vertex->next->y);

	if (y < s->vertex->next->y) {
		double dx, dy;
		/* Mid-segment intersection */
		/* Create continuation svp */
		newvx = nr_vertex_new_xy (x, y);
		newvx->next = s->vertex->next;
		newsvp = nr_svp_new_vertex_wind (newvx, s->svp->wind);
		assert (newsvp->vertex->y < newsvp->vertex->next->y);
		assert (newsvp->vertex->y > s->y);
		assert (newsvp->vertex->y > ytest);
		/* Trim starting svp */
		dx = s->vertex->next->x - s->vertex->x;
		dy = s->vertex->next->y - s->vertex->y;
		newvx = nr_vertex_new_xy (s->vertex->x + (y - s->vertex->y) * dx / dy, y);

		/* Set the new starting point */
		if (newvx->x != x) {
			NRFlat *f;
			double x0, x1;
			x0 = MIN (x, newvx->x);
			x1 = MAX (x, newvx->x);
			f = nr_flat_new_full (y, x0, x1);
			*flats = nr_flat_insert_sorted (*flats, f);
		}

		s->vertex->next = newvx;
		nr_svp_calculate_bbox (s->svp);
		assert (s->svp->vertex->y < s->svp->vertex->next->y);
		/* Insert new SVP into list */
		svp = nr_svp_insert_sorted (svp, newsvp);
		assert (svp);
		assert (s->y >= s->vertex->y);
	} else if (s->vertex->next->next) {

		/* Set the new starting point */
		if (s->vertex->next->x != x) {
			NRFlat *f;
			double x0, x1;
			x0 = MIN (x, s->vertex->next->x);
			x1 = MAX (x, s->vertex->next->x);
			f = nr_flat_new_full (y, x0, x1);
			*flats = nr_flat_insert_sorted (*flats, f);
		}

		/* Create continuation svp */
		newvx = nr_vertex_new_xy (x, y);
		newvx->next = s->vertex->next->next;
		newsvp = nr_svp_new_vertex_wind (newvx, s->svp->wind);
		assert (newsvp->vertex->y < newsvp->vertex->next->y);
		assert (newsvp->vertex->y > s->y);
		assert (newsvp->vertex->y > ytest);
		/* Trim starting svp */
		s->vertex->next->next = NULL;
		nr_svp_calculate_bbox (s->svp);
		assert (s->svp->vertex->y < s->svp->vertex->next->y);
		/* Insert new SVP into list */
		svp = nr_svp_insert_sorted (svp, newsvp);
		assert (svp);
		assert (s->y >= s->vertex->y);
	} else {
		/* Still have to place flat */
		if (s->vertex->next->x != x) {
			NRFlat *f;
			double x0, x1;
			x0 = MIN (x, s->vertex->next->x);
			x1 = MAX (x, s->vertex->next->x);
			f = nr_flat_new_full (y, x0, x1);
			*flats = nr_flat_insert_sorted (*flats, f);
		}
	}
	return svp;
}

#if 0
static void
nr_svp_slice_ensure_vertex_at (NRSVPSlice *s, NRCoord x, NRCoord y)
{
	/* Invariant 1 */
	assert (y >= s->y);
	/* Invariant 2 */
	assert (y >= s->vertex->y);
	/* Invariant 3 */
	assert (y <= s->vertex->next->y);

	if (y == s->y) {
		s->x = x;
	}
	if (y == s->vertex->y) {
		s->vertex->x = x;
	} else if (y < s->vertex->next->y) {
		NRVertex *nvx;
		nvx = nr_vertex_new_xy (x, y);
		nvx->next = s->vertex->next;
		s->vertex->next = nvx;
	} else {
		s->vertex->next->x = x;
	}
	/* We expect bbox to be correct (x0 <= x <= x1) */
}
#endif

/*
 * Memory management stuff follows (remember goals?)
 */

/* Slices */

#define NR_SLICE_ALLOC_SIZE 32
static NRSVPSlice * ffslice = NULL;

NRSVPSlice *
nr_svp_slice_new (NRSVP * svp, NRCoord y)
{
	NRSVPSlice * s;
	NRVertex * v;

	assert (svp);
	assert (svp->vertex);
	/* fixme: We try snapped slices - not sure, whether correct */
	assert (y == NR_COORD_SNAP (y));
	/* Slices startpoints are included, endpoints excluded */
	/* g_return_val_if_fail (y >= svp->bbox.y0, NULL); */
	/* g_return_val_if_fail (y < svp->bbox.y1, NULL); */

	s = ffslice;

	if (s == NULL) {
		int i;
		s = nr_new (NRSVPSlice, NR_SLICE_ALLOC_SIZE);
		for (i = 1; i < (NR_SLICE_ALLOC_SIZE - 1); i++) s[i].next = &s[i + 1];
		s[NR_SLICE_ALLOC_SIZE - 1].next = NULL;
		ffslice = s + 1;
	} else {
		ffslice = s->next;
	}

#if 0
	s->prev = NULL;
#endif
	s->next = NULL;
	s->svp = svp;

	v = svp->vertex;
	while ((v->next) && (v->next->y <= y)) v = v->next;
	assert (v->next);

	s->vertex = v;
	if (v->y == y) {
		s->x = v->x;
	} else {
		s->x = NR_COORD_SNAP (v->x + (v->next->x - v->x) * (y - v->y) / (v->next->y - v->y));
	}
	s->y = y;

	return s;
}

void
nr_svp_slice_free_one (NRSVPSlice * slice)
{
	slice->next = ffslice;
#if 0
	if (ffslice) ffslice->prev = slice;
#endif
	ffslice = slice;
#if 0
	slice->prev = NULL;
#endif
}

#if 0
void
nr_svp_slice_free_list (NRSVPSlice * slice)
{
	NRSVPSlice * l;

	if (!slice) return;

	for (l = slice; l->next != NULL; l = l->next);

	l->next = ffslice;
#if 0
	if (ffslice) ffslice->prev = l;
#endif
	ffslice = slice;
#if 0
	slice->prev = NULL;
#endif
}
#endif

NRSVPSlice *
nr_svp_slice_insert_sorted (NRSVPSlice * start, NRSVPSlice * slice)
{
	NRSVPSlice * s, * l;

	assert (start != slice);

	if (!start) {
		slice->next = NULL;
		return slice;
	}
	if (!slice) return start;

	if (nr_svp_slice_compare (slice, start) <= 0) {
		slice->next = start;
		return slice;
	}

	s = start;
	for (l = start->next; l != NULL; l = l->next) {
		if (nr_svp_slice_compare (slice, l) <= 0) {
			assert (l != slice);
			slice->next = l;
			assert (slice != s);
			s->next = slice;
			return start;
		}
		s = l;
	}

	slice->next = NULL;
	assert (slice != s);
	s->next = slice;

	return start;
}

NRSVPSlice *
nr_svp_slice_stretch_list (NRSVPSlice * slices, NRCoord y)
{
	NRSVPSlice * p, * s;

	/* fixme: We try snapped slices - not sure, whether correct */
	assert (y == NR_COORD_SNAP (y));

	p = NULL;
	s = slices;

	while (s) {
		if (s->svp->bbox.y1 <= y) {
			/* Remove exhausted slice */
			if (p) {
				assert (s->next != p);
				p->next = s->next;
				nr_svp_slice_free_one (s);
				s = p->next;
			} else {
				slices = s->next;
				nr_svp_slice_free_one (s);
				s = slices;
			}
		} else {
			NRVertex * v;
			/* Stretch slice */
			v = s->vertex;
			while ((v->next) && (v->next->y <= y)) v = v->next;
			assert (v->next);

			s->vertex = v;
			if (v->y == y) {
				s->x = v->x;
			} else {
#if 0
				s->x = NR_COORD_SNAP (v->x + (v->next->x - v->x) * (y - v->y) / (v->next->y - v->y));
#else
				s->x = v->x + (double) (v->next->x - v->x) * (y - v->y) / (v->next->y - v->y);
#endif
			}
			s->y = y;
			p = s;
			s = s->next;
		}
	}

	return slices;
}

static int
nr_svp_slice_compare (NRSVPSlice *l, NRSVPSlice *r)
{
	double ldx, ldy, rdx, rdy;
	double d;

	assert (l->y == r->y);
	assert (l->vertex->next->y > l->y);
	assert (r->vertex->next->y > r->y);

	if (l->x < r->x) return -1;
	if (l->x > r->x) return 1;

	/* Y is same, X is same, checking for line orders */
	ldx = l->vertex->next->x - l->vertex->x;
	ldy = l->vertex->next->y - l->y;
	rdx = r->vertex->next->x - r->vertex->x;
	rdy = r->vertex->next->y - r->y;

	d = ldx * rdy - ldy * rdx;
	/* fixme: test almost zero cases */
	if (d < 0) return -1;
	if (d > 0) return 1;
	return 0;
}

/*
 * Test, whether vertex can be considered to be lying on line
 */

#if 0
static unsigned int
nr_test_point_line (NRVertex *a, NRVertex *b, NRCoord cx, NRCoord cy)
{
	float xba, yba, xac, yac;
	float n;

	/*
	 * L = sqrt (xba * xba + yba * yba)
	 * n = yac * xba - xac * yba
	 * s = n / (L * L)
	 * d = s * L
	 *
	 * We test for d < TOLERANCE
	 * d * d < TOLERANCE * TOLERANCE
	 * s * s * L * L < TOLERANCE * TOLERANCE
	 * n * n / (L * L) < TOLERANCE * TOLERANCE
	 * n * n < TOLERANCE * TOLERANCE * L * L
	 *
	 */

	xba = b->x - a->x;
	yba = b->y - a->y;
	xac = a->x - cx;
	yac = a->y - cy;

	n = yac * xba - xac * yba;

	return (n * n) < (2 * NR_COORD_TOLERANCE * NR_COORD_TOLERANCE * (xba * xba + yba * yba));
}
#endif

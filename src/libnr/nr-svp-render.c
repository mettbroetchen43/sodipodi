#define __NR_SVP_RENDER_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include "nr-macros.h"
#include "nr-pixops.h"
#include "nr-svp-render.h"

typedef struct _NRSlice NRSlice;

struct _NRSlice {
	NRSlice *next;
	NRSVP *svp;
	NRVertex *vertex;
	NRCoord x;
	NRCoord y;
	NRCoord stepx;
};

typedef struct _NRRun NRRun;

struct _NRRun {
	NRRun *next;
	NRCoord x0, y0, x1, y1;
	float step;
	float final;
	float x;
	float value;
};

static NRSlice *nr_slice_new (NRSVP *svp, NRCoord y);
static void nr_slice_free_one (NRSlice *s);
static void nr_slice_free_list (NRSlice *s);
static NRSlice *nr_slice_insert_sorted (NRSlice *start, NRSlice *slice);
static int nr_slice_compare (NRSlice *l, NRSlice *r);

static NRRun *nr_run_new (NRCoord x0, NRCoord y0, NRCoord x1, NRCoord y1, int wind);
static void nr_run_free_one (NRRun *run);
static void nr_run_free_list (NRRun *run);
static NRRun *nr_run_insert_sorted (NRRun *start, NRRun *run);

void
nr_pixblock_render_svp_rgba (NRPixBlock *dpb, NRSVP *svp, NRULong rgba)
{
	NRSVP *nsvp;
	NRSlice *slices;
	int x0, y0, x1, y1, ystart;
	unsigned long fg_r, fg_g, fg_b, fg_a;
	unsigned char *px, *rowbuffer;
	int y;

	if (!svp) return;

	if (dpb->mode != NR_PIXBLOCK_MODE_R8G8B8A8P) return;

	x0 = dpb->area.x0;
	y0 = dpb->area.y0;
	x1 = dpb->area.x1;
	y1 = dpb->area.y1;
	px = NR_PIXBLOCK_PX (dpb);

	/* Find starting pixel row */
	/* g_assert (svp->bbox.y0 == svp->vertex->y); */
	ystart = (int) svp->bbox.y0;
	if (ystart >= y1) return;
	if (ystart > y0) {
		px += (ystart - y0) * dpb->rs;
		y0 = ystart;
	}

	/* Construct initial slice list */
	slices = NULL;
	nsvp = svp;
	while ((nsvp) && (nsvp->bbox.y0 <= y0)) {
		/* g_assert (nsvp->bbox.y0 == nsvp->vertex->y); */
		if (nsvp->bbox.y1 > y0) {
			NRSlice *newslice;
			newslice = nr_slice_new (nsvp, y0);
			slices = nr_slice_insert_sorted (slices, newslice);
		}
		nsvp = nsvp->next;
	}
	if ((!nsvp) && (!slices)) return;

	/* Get colors */
	fg_r = (rgba >> 24) & 0xff;
	fg_g = (rgba >> 16) & 0xff;
	fg_b = (rgba >> 8) & 0xff;
	fg_a = rgba & 0xff;

	/* Row buffer */
	/* fixme: not needed */
	rowbuffer = px;

	/* Main iteration */
	for (y = y0; y < y1; y++) {
		NRSlice * ss, * cs;
		NRRun * runs;
		int xstart;
		float globalval;
		unsigned char *d;
		int x;

		/* Add possible new svps to slice list */
		while ((nsvp) && (nsvp->bbox.y0 < (y + 1))) {
			NRSlice * newslice;
			/* fixme: we should use safely nsvp->vertex->y here */
			newslice = nr_slice_new (nsvp, MAX (y, nsvp->bbox.y0));
			slices = nr_slice_insert_sorted (slices, newslice);
			nsvp = nsvp->next;
		}
		/* Construct runs, stretching slices */
		runs = NULL;
		ss = NULL;
		cs = slices;
		while (cs) {
			/* g_assert (cs->y >= y0); */
			/* g_assert (cs->y < (y + 1)); */
			while ((cs->y < (y + 1)) && (cs->vertex->next)) {
				NRCoord x0, y0, x1, y1;
				NRRun * newrun;
				x0 = cs->x;
				y0 = cs->y;
				if (cs->vertex->next->y > (y + 1)) {
					/* The same slice continues */
					x1 = x0 + ((y + 1) - y0) * cs->stepx;
					y1 = y + 1;
					cs->x = x1;
					cs->y = y1;
				} else {
					cs->vertex = cs->vertex->next;
					x1 = cs->vertex->x;
					y1 = cs->vertex->y;
					cs->x = x1;
					cs->y = y1;
					if (cs->vertex->next ) {
						cs->stepx = (cs->vertex->next->x - x1) / (cs->vertex->next->y - y1);
					}
				}
				newrun = nr_run_new (x0, y0, x1, y1, cs->svp->wind);
				/* fixme: we should use walking forward/backward instead */
				runs = nr_run_insert_sorted (runs, newrun);
			}
			if (cs->vertex->next) {
				ss = cs;
				cs = cs->next;
			} else {
				/* Slice is exhausted */
				if (ss) {
					ss->next = cs->next;
					nr_slice_free_one (cs);
					cs = ss->next;
				} else {
					slices = cs->next;
					nr_slice_free_one (cs);
					cs = slices;
				}
			}
		}
		/* Slices are expanded to next scanline */
		/* Run list is generated */
		/* find initial value */
		globalval = 0.0;
		if ((runs) && (x0 < runs->x0)) {
			/* First run starts right from x0 */
			xstart = (int) runs->x0;
		} else {
			NRRun * sr, * cr;
			/* First run starts left from x0 */
			xstart = x0;
			sr = NULL;
			cr = runs;
			while ((cr) && (cr->x0 < x0)) {
				if (cr->x1 <= x0) {
					globalval += cr->final;
					if (sr) {
						sr->next = cr->next;
						nr_run_free_one (cr);
						cr = sr->next;
					} else {
						runs = cr->next;
						nr_run_free_one (cr);
						cr = runs;
					}
				} else {
					cr->x = x0;
					cr->value = (x0 - cr->x0) * cr->step;
					sr = cr;
					cr = cr->next;
				}
			}
		}

		/* Running buffer */
		d = rowbuffer + 4 * (xstart - x0);

		for (x = xstart; (runs) && (x < x1); x++) {
			NRRun *sr, *cr;
			float xnext;
			float localval;
			int coverage;
			unsigned int ca;

			xnext = x + 1.0;
			/* process runs */
			/* fixme: generate fills */
			localval = globalval;
			sr = NULL;
			cr = runs;
			while ((cr) && (cr->x0 < xnext)) {
				if (cr->x1 <= xnext) {
					/* Run ends here */
					globalval += cr->final;
					localval += (cr->x1 - cr->x) * (cr->value + cr->final) / 2.0;
					localval += (xnext - cr->x1) * cr->final;
#ifdef NR_VERBOSE
					if ((localval < -NR_EPSILON) || (localval > 1.0 + NR_EPSILON)) {
						g_print ("A: localval += (%f - %f) * (%f + %f) / 2\n", cr->x1, cr->x, cr->value, cr->final);
						g_print ("A: localval += (%f - %f) * %f\n", xnext, cr->x1, cr->final);
						g_print ("A Y: %d X: %d Globalval: %f Localval: %f\n", y, x, globalval, localval);
					}
#endif
					if (sr) {
						sr->next = cr->next;
						nr_run_free_one (cr);
						cr = sr->next;
					} else {
						runs = cr->next;
						nr_run_free_one (cr);
						cr = runs;
					}
				} else {
					/* Run continues through xnext */
					localval += (xnext - cr->x) * (cr->value + (xnext - cr->x) * cr->step / 2.0);
#ifdef NR_VERBOSE
					if ((localval < -NR_EPSILON) || (localval > 1.0 + NR_EPSILON)) {
						g_print ("B: Run is %f %f %f %f step %f final %f x %f value %f\n",
							 cr->x0, cr->y0, cr->x1, cr->y1, cr->step, cr->final, cr->x, cr->value);
						g_print ("B: localval += (%f - %f) * (%f + %f / 2)\n", xnext, cr->x, cr->value, cr->step);
						g_print ("B Y: %d X: %d Globalval: %f Localval: %f\n", y, x, globalval, localval);
					}
#endif
					cr->x = xnext;
					cr->value = (xnext - cr->x0) * cr->step;
					sr = cr;
					cr = cr->next;
				}
			}
			/* Draw */
			coverage = (int) (localval * 255.9999);
			coverage = CLAMP (coverage, 0, 255);
			ca = NR_PREMUL (coverage, fg_a);
#if 1
			if (ca == 0) {
				/* Transparent FG, NOP */
			} else if ((ca == 255) || (d[3] == 0)) {
				/* Full coverage, COPY */
				d[0] = NR_PREMUL (fg_r, ca);
				d[1] = NR_PREMUL (fg_g, ca);
				d[2] = NR_PREMUL (fg_b, ca);
				d[3] = ca;
			} else {
				/* Full composition */
				d[0] = NR_COMPOSENPP (fg_r, ca, d[0], d[3]);
				d[1] = NR_COMPOSENPP (fg_g, ca, d[1], d[3]);
				d[2] = NR_COMPOSENPP (fg_b, ca, d[2], d[3]);
				d[3] = (65025 - (255 - ca) * (255 - d[3]) + 127) / 255;
			}
#else
			d[0] = fg_r;
			d[1] = fg_g;
			d[2] = fg_b;
			d[3] = fg_a;
#endif
			d += 4;
		}
		nr_run_free_list (runs);
		rowbuffer += dpb->rs;
	}
	if (slices) nr_slice_free_list (slices);
}

/*
 * Memory management stuff follows (remember goals?)
 */

/* Slices */

#define NR_SLICE_ALLOC_SIZE 32
static NRSlice *ffslice = NULL;

NRSlice *
nr_slice_new (NRSVP * svp, NRCoord y)
{
	NRSlice *s;
	NRVertex *v;

	/* g_assert (svp); */
	/* g_assert (svp->vertex); */
	/* fixme: not sure, whether correct */
	/* g_assert (y == NR_COORD_SNAP (y)); */
	/* Slices startpoints are included, endpoints excluded */
	/* g_return_val_if_fail (y >= svp->bbox.y0, NULL); */
	/* g_return_val_if_fail (y < svp->bbox.y1, NULL); */

	s = ffslice;

	if (s == NULL) {
		int i;
		s = nr_new (NRSlice, NR_SLICE_ALLOC_SIZE);
		for (i = 1; i < (NR_SLICE_ALLOC_SIZE - 1); i++) s[i].next = &s[i + 1];
		s[NR_SLICE_ALLOC_SIZE - 1].next = NULL;
		ffslice = s + 1;
	} else {
		ffslice = s->next;
	}

	s->next = NULL;
	s->svp = svp;

	v = svp->vertex;
	while ((v->next) && (v->next->y <= y)) v = v->next;
	/* g_assert (v->next); */

	s->vertex = v;
	if (v->y == y) {
		s->x = v->x;
	} else {
		s->x = v->x + (v->next->x - v->x) * (y - v->y) / (v->next->y - v->y);
	}
	s->y = y;
	s->stepx = (s->vertex->next->x - s->vertex->x) / (s->vertex->next->y - s->vertex->y);

	return s;
}

void
nr_slice_free_one (NRSlice * slice)
{
	slice->next = ffslice;
	ffslice = slice;
}

void
nr_slice_free_list (NRSlice * slice)
{
	NRSlice * l;

	if (!slice) return;

	for (l = slice; l->next != NULL; l = l->next);

	l->next = ffslice;
	ffslice = slice;
}

NRSlice *
nr_slice_insert_sorted (NRSlice * start, NRSlice * slice)
{
	NRSlice * s, * l;

	if (!start) return slice;
	if (!slice) return start;

	if (nr_slice_compare (slice, start) <= 0) {
		slice->next = start;
		return slice;
	}

	s = start;
	for (l = start->next; l != NULL; l = l->next) {
		if (nr_slice_compare (slice, l) <= 0) {
			slice->next = l;
			s->next = slice;
			return start;
		}
		s = l;
	}

	slice->next = NULL;
	s->next = slice;

	return start;
}

static int
nr_slice_compare (NRSlice * l, NRSlice * r)
{
	if (l->y == r->y) {
		if (l->x < r->x) return -1;
		if (l->x > r->x) return 1;
		if (l->stepx < r->stepx) return -1;
		if (l->stepx > r->stepx) return 1;
		return 0;
	} else if (l->y > r->y) {
		NRVertex * v;
		NRCoord x, stepx;
		/* This is bitch - we have to determine r values at l->y */
		v = r->vertex;
		while ((v->next) && (v->next->y <= l->y)) v = v->next;
		/* If v is last vertex, r ends before l starts */
		if (!v->next) return 1;
		if (v->y == l->y) {
			x = v->x;
		} else {
			x = v->x + (v->next->x - v->x) * (l->y - v->y) / (v->next->y - v->y);
		}
		if (l->x < x) return -1;
		if (l->x > x) return 1;
		stepx = (v->next->x - v->x) / (v->next->y - v->y);
		if (l->stepx < stepx) return -1;
		if (l->stepx > stepx) return 1;
		return 0;
	} else {
		NRVertex * v;
		NRCoord x, stepx;
		/* This is bitch - we have to determine l value at r->y */
		v = l->vertex;
		while ((v->next) && (v->next->y <= r->y)) v = v->next;
		/* If v is last vertex, l ends before r starts */
		if (!v->next) return -1;
		if (v->y == r->y) {
			x = v->x;
		} else {
			x = v->x + (v->next->x - v->x) * (r->y - v->y) / (v->next->y - v->y);
		}
		if (x < r->x) return -1;
		if (x > r->x) return 1;
		stepx = (v->next->x - v->x) / (v->next->y - v->y);
		if (stepx < r->stepx) return -1;
		if (stepx > r->stepx) return 1;
		return 0;
	}
}

#define NR_RUN_ALLOC_SIZE 32
static NRRun *ffrun = NULL;

static NRRun *
nr_run_new (NRCoord x0, NRCoord y0, NRCoord x1, NRCoord y1, int wind)
{
	NRRun * r;

	r = ffrun;

	if (r == NULL) {
		int i;
		r = nr_new (NRRun, NR_RUN_ALLOC_SIZE);
		for (i = 1; i < (NR_RUN_ALLOC_SIZE - 1); i++) (r + i)->next = (r + i + 1);
		(r + NR_RUN_ALLOC_SIZE - 1)->next = NULL;
		ffrun = r + 1;
	} else {
		ffrun = r->next;
	}

	r->next = NULL;

	if (x0 <= x1) {
		r->x0 = x0;
		r->x1 = x1;
		r->y0 = y0;
		r->y1 = y1;
		r->step = (x0 == x1) ? 0.0 : wind * (y1 - y0) / (x1 - x0);
	} else {
		r->x0 = x1;
		r->x1 = x0;
		r->y0 = y1;
		r->y1 = y0;
		r->step = wind * (y1 - y0) / (x0 - x1);
	}

	r->final = wind * (y1 - y0);

	r->value = 0.0;
	r->x = r->x0;

	return r;
}

static void
nr_run_free_one (NRRun * run)
{
	run->next = ffrun;
	ffrun = run;
}

static void
nr_run_free_list (NRRun * run)
{
	NRRun * l;

	if (!run) return;

	for (l = run; l->next != NULL; l = l->next);
	l->next = ffrun;
	ffrun = run;
}

static NRRun *
nr_run_insert_sorted (NRRun * start, NRRun * run)
{
	NRRun * s, * l;

	if (!start) return run;
	if (!run) return start;

	if (run->x0 < start->x0) {
		run->next = start;
		return run;
	}

	s = start;
	for (l = start->next; l != NULL; l = l->next) {
		if (run->x0 < l->x0) {
			run->next = l;
			s->next = run;
			return start;
		}
		s = l;
	}

	s->next = run;

	return start;
}

#if 0
/*
 * Determinate the exact order of 2 noncrossing lines
 */

static gint
nr_x_order (NRLine * l0, NRLine * l1)
{
	float xx0, yy0, x1x0, y1y0;
	float ds, de;

	/* Determine l0 start relative to l1 */
	x1x0 = l1->e.x - l1->s.x;
	y1y0 = l1->e.y - l1->s.y;
	xx0 = l0->s.x - l1->s.x;
	yy0 = l0->s.y - l1->s.y;

	if ((xx0 == 0.0) && (yy0 == 0.0)) {
		/* l0 start lies on l1 */
		xx0 = l0->e.x - l1->s.x;
		yy0 = l0->e.y - l1->s.y;
		de = xx0 * y1y0 - yy0 * x1x0;
		if (de < 0.0) return -1;
		if (de > 0.0) return 1;
		return 0;
	}

	ds = xx0 * y1y0 - yy0 * x1x0;

	/* l0 end relative to l1 */
	xx0 = l0->s.x - l1->s.x;
	yy0 = l0->s.y - l1->s.y;

	if ((xx0 == 0.0) && (yy0 == 0.0)) {
		/* l0 end lies on l1 */
		if (ds < 0.0) return -1;
		if (ds > 0.0) return 1;
		return 0;
	}

	de = xx0 * y1y0 - yy0 * x1x0;

	if (ds == 0.0) {
		if (de < 0.0) return -1;
		if (de > 0.0) return 1;
		return 0;
	}
	if (de == 0.0) {
		if (ds < 0.0) return -1;
		if (ds > 0.0) return 1;
		return 0;
	}

	if ((ds < 0.0) && (de < 0.0)) return -1;
	if ((de > 0.0) && (de > 0.0)) return 1;

	/* l0 start and end are on opposite sides of l1 */
	/* Determine l1 start relative to l0 */
	x1x0 = l0->e.x - l0->s.x;
	y1y0 = l0->e.y - l0->s.y;
	xx0 = l1->s.x - l0->s.x;
	yy0 = l1->s.y - l0->s.y;

	if ((xx0 == 0.0) && (yy0 == 0.0)) {
		/* l1 start lies on l0 */
		xx0 = l1->e.x - l0->s.x;
		yy0 = l1->e.y - l0->s.y;
		de = xx0 * y1y0 - yy0 * x1x0;
		if (de < 0.0) return 1;
		if (de > 0.0) return -1;
		return 0;
	}

	ds = xx0 * y1y0 - yy0 * x1x0;

	if (ds < 0.0) return 1;
	if (ds > 0.0) return -1;

	/* l1 end relative to l0 */
	xx0 = l1->s.x - l0->s.x;
	yy0 = l1->s.y - l0->s.y;

	if ((xx0 == 0.0) && (yy0 == 0.0)) {
		/* l1 end lies on l0 */
		if (ds < 0.0) return 1;
		if (ds > 0.0) return -1;
		return 0;
	}

	de = xx0 * y1y0 - yy0 * x1x0;

	if (de < 0.0) return 1;
	if (de > 0.0) return -1;
	return 0;
}
#endif


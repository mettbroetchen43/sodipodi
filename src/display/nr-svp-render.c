#define _NR_SVP_RENDER_C_

#include <math.h>
#include "nr-svp-render.h"

typedef struct _NRRSpan NRRSpan;

struct _NRRSpan {
	NRRSpan * next;
	NRLine * line;
	NRPoint s;
	NRPoint e;
	float stepx;
};

typedef struct _NRRun NRRun;

struct _NRRun {
	NRRun * next;
	NRPoint s;
	NRPoint e;
	float step;
	float value;
	float final;
	float x;
};

static NRRSpan * nr_rspan_new (NRLine * line);
static void nr_rspan_free (NRRSpan * rs);
static void nr_rspan_free_list (NRRSpan * rs);
static NRRSpan * nr_rspan_insert_sorted (NRRSpan * start, NRRSpan * span);

static NRRun * nr_run_new (NRRSpan * span);
static void nr_run_free (NRRun * run);
static void nr_run_free_list (NRRun * run);
static NRRun * nr_run_insert_sorted (NRRun * start, NRRun * run);

static gint nr_x_order (NRLine * l0, NRLine * l1);

void
nr_svp_render_rgb_rgba (NRSVP * svp, guchar * buffer, gint x0, gint y0, gint width, gint height, gint rowstride, guint32 rgba)
{
	NRRSpan * spans;
	NRRun * runs;
	NRLine * line;
	gint32 xstart, ystart;
	gint32 x1, y1;
	guchar * bs, * br;
	guint fg_r, fg_g, fg_b, fg_a;
	gint y;

	if (!svp) return;

	x1 = x0 + width;
	y1 = y0 + height;
	spans = NULL;

	/* Construct initial spans from lines crossing y0 */
	for (line = svp->lines; (line) && (line->s.y < y0); line = line->next) {
		if (line->e.y > y0) {
			NRRSpan * new;
			new = nr_rspan_new (line);
			new->s.x = line->s.x + (line->e.x - line->s.x) * (y0 - line->s.y) / (line->e.y - line->s.y);
			new->s.y = y0;
			if (!spans) {
				spans = new;
			} else {
				spans = nr_rspan_insert_sorted (spans, new);
			}
		}
	}

	if (spans) {
		ystart = y0;
	} else {
		if (!line) return;
		ystart = (gint32) floor (line->s.y);
	}

	/* Get colors */
	fg_r = (rgba >> 24) & 0xff;
	fg_g = (rgba >> 16) & 0xff;
	fg_b = (rgba >> 8) & 0xff;
	fg_a = rgba & 0xff;

	/* Initial buffer */
	bs = buffer + (ystart - y0) * rowstride;

	/* Main iteration */
	for (y = ystart; y < y1; y ++) {
		NRRSpan * s, * s0;
		NRRun * r, * r0;
		float globalval;
		gint x;
		/* Add new lines starting between y and y+1 to spans */
		while ((line) && (line->s.y < (y + 1.0))) {
			NRRSpan * new;
			new = nr_rspan_new (line);
			new->s.x = line->s.x;
			new->s.y = line->s.y;
			if (!spans) {
				spans = new;
			} else {
				spans = nr_rspan_insert_sorted (spans, new);
			}
			line = line->next;
		}
		/* Now we have sorted list of spans with CORRECT STARTPOINTS */
		/* Generate endpoints */
		for (s = spans; s != NULL; s = s->next) {
			if (s->line->e.y < y + 1.0) {
				s->e.x = s->line->e.x;
				s->e.y = s->line->e.y;
			} else {
				if (s->s.y == y) {
					s->e.x = s->s.x + s->stepx;
				} else {
					s->e.x = s->s.x + s->stepx * (y + 1.0 - s->s.y);
				}
				s->e.y = y + 1.0;
			}
		}

		/* Generate runs */
		runs = NULL;
		for (s = spans; s != NULL; s = s->next) {
			runs = nr_run_insert_sorted (runs, nr_run_new (s));
		}
		/* fixme: killpoints */
		/* Find initial value */
		globalval = 0.0;
		if ((runs) && (x0 < runs->s.x)) {
			xstart = (gint32) floor (runs->s.x);
		} else {
			xstart = x0;
			r0 = NULL;
			r = runs;
			while ((r) && (r->s.x < x0)) {
				if (r->e.x <= x0) {
					globalval += r->final;
					if (r0) {
						r0->next = r->next;
						nr_run_free (r);
						r = r0->next;
					} else {
						runs = r->next;
						nr_run_free (r);
						r = runs;
					}
				} else {
					r->x = x0;
					r->value = (x0 - r->s.x) * r->step;
					r0 = r;
					r = r->next;
				}
			}
		}

		/* Running buffer */
		br = bs + 3 * (xstart - x0);

		for (x = xstart; (runs) && (x < x1); x++) {
			float xnext;
			float localval;
			guint coverage;

			xnext = x + 1.0;
			/* process runs */
			/* fixme: generate fills */
			localval = globalval;
			r0 = NULL;
			r = runs;
			while ((r) && (r->s.x < xnext)) {
				if (r->e.x <= xnext) {
					globalval += r->final;
					localval +=  (r->e.x - r->x) * (r->value + r->final) / 2.0;
					localval += (xnext - r->e.x) * r->final;
					if ((localval < 0.0) || (localval > 1.0)) {
						g_print ("A: localval += (%f - %f) * (%f + %f) / 2\n", r->e.x, r->x, r->value, r->final);
						g_print ("A: localval += (%f - %f) * %f\n", xnext, r->e.x, r->final);
						g_print ("A Y: %d X: %d Globalval: %f Localval: %f\n", y, x, globalval, localval);
					}
					if (r0) {
						r0->next = r->next;
						nr_run_free (r);
						r = r0->next;
					} else {
						runs = r->next;
						nr_run_free (r);
						r = runs;
					}
				} else {
#if 0
					g_print ("C: localval += %f\n", r->value + (xnext - r->x) * r->step / 2.0);
#endif
					localval += (xnext - r->x) * (r->value + r->step / 2.0);
					if ((localval < 0.0) || (localval > 1.0)) {
						g_print ("B: localval += (%f - %f) * (%f + %f / 2)\n", xnext, r->x, r->value, r->step);
						g_print ("B Y: %d X: %d Globalval: %f Localval: %f\n", y, x, globalval, localval);
					}
					r->x = xnext;
					r->value = (xnext - r->s.x) * r->step;
					r0 = r;
					r = r->next;
				}
			}
			/* Draw */
			coverage = (guint) (floor (localval * 255.99));
			coverage = (coverage * fg_a + 0x80) >> 8;
			*br++ = *br + (((fg_r - *br) * coverage + 0x80) >> 8);
			*br++ = *br + (((fg_g - *br) * coverage + 0x80) >> 8);
			*br++ = *br + (((fg_b - *br) * coverage + 0x80) >> 8);
		}
		nr_run_free_list (runs);

		/* Remove exhausted spans and update crossings */
		s0 = NULL;
		s = spans;
		while (s) {
			if (s->line->e.y <= y) {
				if (s0) {
					s0->next = s->next;
					nr_rspan_free (s);
					s = s0->next;
				} else {
					spans = s->next;
					nr_rspan_free (s);
					s = spans;
				}
			} else {
				s->s.x = s->e.x;
				s->s.y = s->e.y;
				s0 = s;
				s = s->next;
			}
		}
		bs += rowstride;
	}
	nr_rspan_free_list (spans);
}

/*
 * Memory management stuff follows (remember goals?)
 */

#define NR_RS_ALLOC_SIZE 32
static NRRSpan * firstfreers = NULL;

static NRRSpan *
nr_rspan_new (NRLine * line)
{
	NRRSpan * rs;

	rs = firstfreers;

	if (rs == NULL) {
		gint i;
		rs = g_new (NRRSpan, NR_RS_ALLOC_SIZE);
		for (i = 1; i < (NR_RS_ALLOC_SIZE - 1); i++) (rs + i)->next = (rs + i + 1);
		(rs + NR_RS_ALLOC_SIZE - 1)->next = NULL;
		firstfreers = rs + 1;
	} else {
		firstfreers = rs->next;
	}

	rs->next = NULL;
	rs->line = line;
	rs->stepx = (line->e.x - line->s.x) / (line->e.y - line->s.y);

	return rs;
}

static NRRSpan *
nr_rspan_insert_sorted (NRRSpan * start, NRRSpan * span)
{
	NRRSpan * s, * l;

	if (!start) return span;
	if (!span) return start;

	if (nr_x_order (span->line, start->line) < 0) {
		span->next = start;
		return span;
	}

	s = start;
	for (l = start->next; l != NULL; l = l->next) {
		if (nr_x_order (span->line, l->line) < 0) {
			span->next = l;
			s->next = span;
			return start;
		}
		s = l;
	}

	s->next = span;

	return start;
}

static void
nr_rspan_free (NRRSpan * rs)
{
	rs->next = firstfreers;
	firstfreers = rs;
}

static void
nr_rspan_free_list (NRRSpan * rs)
{
	NRRSpan * l;

	if (!rs) return;

	for (l = rs; l->next != NULL; l = l->next);
	l->next = firstfreers;
	firstfreers = rs;
}

#define NR_RUN_ALLOC_SIZE 32
static NRRun * firstfreerun = NULL;

static NRRun *
nr_run_new (NRRSpan * span)
{
	NRRun * r;

	r = firstfreerun;

	if (r == NULL) {
		gint i;
		r = g_new (NRRun, NR_RUN_ALLOC_SIZE);
		for (i = 1; i < (NR_RUN_ALLOC_SIZE - 1); i++) (r + i)->next = (r + i + 1);
		(r + NR_RUN_ALLOC_SIZE - 1)->next = NULL;
		firstfreerun = r + 1;
	} else {
		firstfreerun = r->next;
	}

	r->next = NULL;

	if (span->s.x == span->e.x) {
		r->s.x = r->e.x = span->s.x;
		r->s.y = span->s.y;
		r->e.y = span->e.y;
		r->step = 0.0;
		r->final = r->e.y - r->s.y;
	} else if (span->s.x < span->e.x) {
		r->s = span->s;
		r->e = span->e;
		r->step = (r->e.y - r->s.y) / (r->e.x - r->s.x);
		r->final = r->e.y - r->s.y;
	} else {
		r->s = span->e;
		r->e = span->s;
		r->step = (r->s.y - r->e.y) / (r->e.x - r->s.x);
		r->final = r->s.y - r->e.y;
	}

	r->step *= -span->line->direction;
	r->final *= -span->line->direction;

	r->value = 0.0;
	r->x = r->s.x;

	return r;
}

static void
nr_run_free (NRRun * run)
{
	run->next = firstfreerun;
	firstfreerun = run;
}

static void
nr_run_free_list (NRRun * run)
{
	NRRun * l;

	if (!run) return;

	for (l = run; l->next != NULL; l = l->next);
	l->next = firstfreerun;
	firstfreerun = run;
}

static NRRun *
nr_run_insert_sorted (NRRun * start, NRRun * run)
{
	NRRun * s, * l;

	if (!start) return run;
	if (!run) return start;

	if (run->s.x < start->s.x) {
		run->next = start;
		return run;
	}

	s = start;
	for (l = start->next; l != NULL; l = l->next) {
		if (run->s.x < l->s.x) {
			run->next = l;
			s->next = run;
			return start;
		}
		s = l;
	}

	s->next = run;

	return start;
}

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


#define _NR_SVP_RENDER_C_

#include <math.h>
#include "nr-svp-render.h"

typedef struct _NRRSpan NRRSpan;

struct _NRRSpan {
	NRRSpan * next;
	NRLine * line;
	NRPoint s;
	NRPoint e;
	float cvstep;
	float endcv;
};

static NRRSpan * nr_rspan_new (NRLine * line);
static void nr_rspan_free (NRRSpan * rs);
static void nr_rspan_free_list (NRRSpan * rs);
static NRRSpan * nr_rspan_insert_sorted (NRRSpan * start, NRRSpan * span);
static gint nr_x_order (NRLine * l0, NRLine * l1);

void
nr_svp_render_rgb_rgba (NRSVP * svp, guchar * buffer, gint x0, gint y0, gint width, gint height, gint rowstride, guint32 rgba)
{
	NRRSpan * spans;
	NRLine * line;
	gint32 x1, y1;
	gint y;

	x0 = x0 << 4;
	y0 = y0 << 4;
	x1 = (x0 + width) << 4;
	y1 = (y0 + height) << 4;
	spans = NULL;

	/* Construct initial spans */
	for (line = svp->lines; (line) && (line->s.y < y0); line = line->next) {
		if (line->e.y > y0) {
			NRRSpan * new;
			new = nr_rspan_new (line);
			if (!spans) {
				spans = new;
			} else {
				spans = nr_rspan_insert_sorted (spans, new);
			}
		}
	}

	if (!spans) {
		if (!line) return;
		y0 = line->s.y;
	}

	/* Main iteration */
	for (y = y0; y < y1; y += 16) {
		NRRSpan * s;
		/* Add new lines to spans */
		while ((line) && (line->s.y < (y + 16))) {
			NRRSpan * new;
			new = nr_rspan_new (line);
			if (!spans) {
				spans = new;
			} else {
				spans = nr_rspan_insert_sorted (spans, new);
			}
			line = line->next;
		}
		/* Now we have sorted list of spans */
		/* Fill run data */
		for (s = spans; s != NULL; s = s->next) {
			g_assert (s->line->s.y < y + 16);
			g_assert (s->line->e.y > y);
			if (s->line->s.y >= y) {
				/* We start at current line */
				s->s.x = s->line->s.x;
				s->s.y = s->line->s.y;
			} else {
				/* Find intersection with y */
			}
			if (s->line->e.y < y + 16) {
				/* We end before next line */
				s->e.x = s->line->e.x;
				s->e.y = s->line->e.y;
			} else {
				/* Find intersection with y + 16 */
			}
			if (s->s.x > s->e.x) {
				/* Swap points */
				NRPoint t;
				t = s->s;
				s->s = s->e;
				s->e = t;
			}
			s->endcv = fabs (s->e.y - s->s.y);
			s->cvstep = fabs (s->endcv * 16.0 / (s->e.x - s->s.x));

			/* Sillyrender */

			/* Start walking through spans, filling pixels and generating runs, as needed */
			/* All right-infinite runs will be added to single global run */
		}
	}
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


#define _NR_SVP_UNCROSS_C_

#include <math.h>
#include "nr-svp-uncross.h"

typedef struct _NRWindSpan NRWindSpan;

struct _NRWindSpan {
	NRWindSpan * next;
	NRLine * line;
};

static NRLine * nr_svp_uncross_line (NRLine * line);
static gint nr_x_order_simple (NRLine * l0, NRLine * l1);
static NRWindSpan * nr_wind_span_new (NRLine * line);
static void nr_wind_span_free (NRWindSpan * ws);
static void nr_wind_span_free_list (NRWindSpan * ws);

void
nr_svp_uncross (NRSVP * svp)
{
	NRLine * line;

	if (!svp) return;

	line = svp->lines;

	while ((line) && (line->next)) {
		line = nr_svp_uncross_line (line);
	}
}

static NRLine *
nr_svp_uncross_line (NRLine * line)
{
	NRLine * l;
	gint32 xmin, xmax;

	xmin = MIN (line->s.x, line->e.x);
	xmax = MAX (line->s.x, line->e.x);

	for (l = line->next; (l) && (l->s.y < line->e.y); l = l->next) {
		if (((l->s.x <= l->e.x) && (l->s.x <= xmax) && (l->e.x >= xmin)) ||
		    ((l->s.x >= l->e.x) && (l->e.x <= xmax) && (l->s.x >= xmin))) {
			float xba, xdc, yba, ydc;
			float d;
			/* Binding boxes intersect */
			xba = line->e.x - line->s.x;
			xdc = l->e.x - l->s.x;
			yba = line->e.y - line->s.y;
			ydc = l->e.y - l->s.y;
			d = xba * ydc - yba * xdc;
			if (d != 0.0) {
				float xac, yac;
				float numr, nums;
				float r, s;
				/* Not parallel */
				xac = line->s.x - l->s.x;
				yac = line->s.y - l->s.y;
				numr = yac * xdc - xac * ydc;
				nums = yac * xba - xac * yba;
				r = numr / d;
				s = nums / d;
				if ((r > 0) && (r < 1.0) && (s > 0) && (s < 1.0)) {
					gint32 x, y;
					y = line->s.y + (gint32) floor (r * yba + 0.5);
					if ((y > line->s.y) && (y < line->e.y) && (y > l->s.y) && (y < l->e.y)) {
						NRLine * new;
						x = line->s.x + (gint32) floor (r * xba + 0.5);
						new = nr_line_new_xyxyd (x, y, line->e.x, line->e.y, line->direction);
						line->e.x = x;
						line->e.y = y;
						line->next = nr_lines_insert_sorted (line->next, new);
						new = nr_line_new_xyxyd (x, y, l->e.x, l->e.y, l->direction);
						l->e.x = x;
						l->e.y = y;
						l->next = nr_lines_insert_sorted (l->next, new);
						return line;
					}
				}
			} else {
				/* fixme: Test colinear case */
			}
		}
	}

	return line->next;
}

NRSVP *
nr_svp_rewind (NRSVP * svp)
{
	NRSVP * new;
	NRLine * newlines, * l, * nl;
	NRWindSpan * spans, * freespans;
	NRWindSpan * ps, * s, * ns;
	gint wind;

	if (!svp) return NULL;

	newlines = NULL;
	spans = freespans = s = NULL;

	for (l = svp->lines; l != NULL; l = l->next) {
		/* Determine left-side wind */
		/* fixme: optimize */
		wind = 0;
		ps = NULL;
		s = spans;
		while (s) {
			if (s->line->e.y <= l->s.y) {
				NRWindSpan * t;
				/* Span is exhausted */
				if (ps) {
					ps->next = s->next;
				} else {
					spans = s->next;
				}
				t = s;
				s = s->next;
				t->next = freespans;
				freespans = t;
			} else {
				/* Still working span */
				/* Determine span x order */
				if (nr_x_order_simple (l, s->line) >= 0) {
					/* We are at left */
					wind += s->line->direction;
				} else {
					/* We are at right */
					break;
				}
				ps = s;
				s = s->next;
			}
		}
		wind += l->direction;
		/* fixme: winding rules */
		nl = nr_line_new_xyxyd (l->s.x, l->s.y, l->e.x, l->e.y, (wind & 0x1) ? -1 : 1);
		nl->next = newlines;
		newlines = nl;
		/* Insert into span list */
		ns = nr_wind_span_new (l);
		ns->next = s;
		if (ps) {
			ps->next = ns;
		} else {
			spans = ns;
		}
	}

	nr_wind_span_free_list (spans);

	new = nr_svp_new ();
	new->lines = nr_lines_reverse_list (newlines);

	return new;
}

static gint
nr_x_order_simple (NRLine * l0, NRLine * l1)
{
	float xx0, yy0, x1x0, y1y0;
	float d;

	xx0 = l0->s.x - l1->s.x;
	yy0 = l0->s.y - l1->s.y;
	if ((xx0 == 0.0) && (yy0 == 0.0)) {
		xx0 = l0->e.x - l1->s.x;
		yy0 = l0->e.y - l1->s.y;
	}
	x1x0 = l1->e.x - l1->s.x;
	y1y0 = l1->e.y - l1->s.y;

	d = xx0 * y1y0 - yy0 * x1x0;

	/* fixme: test almost zero cases */
	if (d < 0.0) return -1;
	if (d > 0.0) return 1;
	return 0;
}

/*
 * Memory management stuff follows (remember goals?)
 */

#define NR_WS_ALLOC_SIZE 32
static NRWindSpan * firstfreews = NULL;

static NRWindSpan *
nr_wind_span_new (NRLine * line)
{
	NRWindSpan * ws;

	ws = firstfreews;

	if (ws == NULL) {
		gint i;
		ws = g_new (NRWindSpan, NR_WS_ALLOC_SIZE);
		for (i = 1; i < (NR_WS_ALLOC_SIZE - 1); i++) (ws + i)->next = (ws + i + 1);
		(ws + NR_WS_ALLOC_SIZE - 1)->next = NULL;
		firstfreews = ws + 1;
	} else {
		firstfreews = ws->next;
	}

	ws->next = NULL;
	ws->line = line;

	return ws;
}

static void
nr_wind_span_free (NRWindSpan * ws)
{
	ws->next = firstfreews;
	firstfreews = ws;
}

static void
nr_wind_span_free_list (NRWindSpan * ws)
{
	NRWindSpan * l;

	if (!ws) return;

	for (l = ws; l->next != NULL; l = l->next);
	l->next = firstfreews;
	firstfreews = ws;
}


#define _NR_SVP_C_

#include <math.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath_stroke.h>
#include "nr-svp-uncross.h"
#include "nr-svp.h"

#define NR_NEXT(v,l) if (++(v) >= (l)) (v) = 0
#define NR_PREV(v,l) if (--(v) < 0) (v) = l

static NRLine * nr_svp_merge_segments (GSList * segments);
static void nr_svp_split_segments_flat (GSList * segments, NRLine * flat);
static void nr_svp_split_flat (NRLine * l, NRLine * flat);
static NRLine * nr_lines_sort (NRLine * l);
static NRLine * nr_lines_sort_merge (NRLine * l1, NRLine * l2);

NRSVP *
nr_svp_from_art_vpath (ArtVpath * vpath)
{
	NRSVP * svp;
	ArtVpath * s;
	NRLine * line, * start, * flat;
	NRCoord sx, sy, x, y;
	gint dir, newdir;
	GSList * segments;

	start = flat = NULL;
	dir = 0;
	segments = NULL;
	sx = sy = 0;

	for (s = vpath; s->code != ART_END; s++) {
		switch (s->code) {
		case ART_MOVETO:
		case ART_MOVETO_OPEN:
			sx = NR_COORD_FROM_ART (s->x);
			sy = NR_COORD_FROM_ART (s->y);
			if (start) {
				if (dir > 0) start = nr_lines_reverse_list (start);
				segments = g_slist_prepend (segments, start);
			}
			start = NULL;
			dir = 0;
			break;
		case ART_LINETO:
			x = NR_COORD_FROM_ART (s->x);
			y = NR_COORD_FROM_ART (s->y);
			if (y != sy) {
				newdir = (y > sy) ? 1 : -1;
				if (newdir != dir) {
					if (start) {
						if (dir > 0) start = nr_lines_reverse_list (start);
						segments = g_slist_prepend (segments, start);
					}
					start = NULL;
					dir = newdir;
				}
				/* Add line to lines list */
				line = nr_line_new_xyxy (sx, sy, x, y);
				line->next = start;
				start = line;
				sx = x;
				sy = y;
			} else if (x != sx) {
				/* Add horizontal lines to flat list */
				line = nr_line_new ();
				line->next = flat;
				line->direction = 0;
				line->s.x = MIN (sx, x);
				line->s.y = y;
				line->e.x = MAX (sx, x);
				line->e.y = y;
				flat = line;
				sx = x;
			}
			break;
		default:
			/* fixme: free lists */
			return NULL;
			break;
		}
	}
	if (start) {
		if (dir > 0) start = nr_lines_reverse_list (start);
		segments = g_slist_prepend (segments, start);
	}

	if (!segments) return NULL;

	/* Now split all lines crossing flats */

	if (flat != NULL) {
		flat = nr_lines_sort (flat);
		nr_svp_split_segments_flat (segments, flat);
		nr_line_free_list (flat);
	}

	/* Merge segments */

	svp = nr_svp_new ();
	svp->lines = nr_svp_merge_segments (segments);
	g_slist_free (segments);

	nr_svp_uncross (svp);

	return svp;
}

static NRLine *
nr_svp_merge_segments (GSList * segments)
{
	NRLine ** lines, * start, * ln;
	GSList * l;
	gint len, c, i;

	len = 0;
	for (l = segments; l != NULL; l = l->next) len++;
	lines = alloca (len * sizeof (NRLine *));
	i = 0;
	for (l = segments; l != NULL; l = l->next) lines[i++] = (NRLine *) l->data;

	c = 0;
	for (i = 1; i < len; i++) {
		if (nr_lines_compare (lines[i], lines[c]) < 0) c = i;
	}

	start = ln = lines[c];
	lines[c] = lines[c]->next;
	if (lines[c] == NULL) lines[c] = lines[--len];

	while (len > 0) {
		c = 0;
		for (i = 1; i < len; i++) if (nr_lines_compare (lines[i], lines[c]) < 0) c = i;
		ln->next = lines[c];
		ln = ln->next;
		lines[c] = lines[c]->next;
		if (lines[c] == NULL) lines[c] = lines[--len];
	}

	return start;
}

static void
nr_svp_split_segments_flat (GSList * segments, NRLine * flat)
{
	GSList * l;

	for (l = segments; l != NULL; l = l->next) {
		nr_svp_split_flat ((NRLine *) l->data, flat);
	}
}

static void
nr_svp_split_flat (NRLine * l, NRLine * flat)
{
	NRLine * fstart, * f, * new;

	fstart = flat;

	while (l != NULL) {
		NRCoord xmin, xmax;
		/* Find first flat > ystart */
		while ((fstart) && (fstart->s.y <= l->s.y)) fstart = fstart->next;
		if (!fstart) return;
		xmin = MIN (l->s.x, l->e.x);
		xmax = MAX (l->s.x, l->e.x);
		f = fstart;
		while ((f) && (f->s.y < l->e.y)) {
			/* Y overlap */
			if ((f->s.x <= xmax) && (f->e.x >= xmin)) {
				NRCoord x;
				/* Bounding boxes intersect */
				/* x = x0 + (x1 - x0)(y - y0)/(y1 - y0) */
				x = NR_COORD_SNAP (l->s.x + (l->e.x - l->s.x) * (f->s.y - l->s.y) / (l->e.y - l->s.y));
				if ((x >= f->s.x) && (x <= f->e.x)) {
					/* We intersect */
					new = nr_line_new_xyxyd (x, f->s.y, l->e.x, l->e.y, l->direction);
					l->e.x = x;
					l->e.y = f->s.y;
					new->next = l->next;
					l->next = new;
					/* We can be sure that this line does not intersect more, so break */
					break;
				}
			}
			/* We do not intersect, so try next flat */
			f = f->next;
		}
		l = l->next;
	}
}

ArtSVP *
nr_art_svp_from_svp (NRSVP * svp)
{
	ArtSVP * asvp;
	NRLine * l;
	gint size;
	gint sn;

	if (!svp) {
		asvp = art_alloc (sizeof (ArtSVP));
		asvp->n_segs = 0;
		return asvp;
	}

	size = 0;
	for (l = svp->lines; l != NULL; l = l->next) size++;

	asvp = art_alloc (sizeof (ArtSVP) + (size - 1) * sizeof (ArtSVPSeg));
	asvp->n_segs = size;

	sn = 0;
	for (l = svp->lines; l != NULL; l = l->next) {
		ArtSVPSeg * aseg;

		aseg = &asvp->segs[sn];

		aseg->n_points = 2;
		aseg->dir = (l->direction == -1);
		aseg->points = art_new (ArtPoint, 2);
		aseg->points[0].x = NR_COORD_TO_ART (l->s.x);
		aseg->points[0].y = NR_COORD_TO_ART (l->s.y);
		aseg->points[1].x = NR_COORD_TO_ART (l->e.x);
		aseg->points[1].y = NR_COORD_TO_ART (l->e.y);
		aseg->bbox.x0 = MIN (aseg->points[0].x, aseg->points[1].x);
		aseg->bbox.y0 = aseg->points[0].y;
		aseg->bbox.x1 = MAX (aseg->points[0].x, aseg->points[1].x);
		aseg->bbox.y1 = aseg->points[1].y;

		sn++;
	}

	return asvp;
}

static NRLine *
nr_lines_sort (NRLine * l)
{
	NRLine * l1, * l2;

	if (!l) return NULL;
	if (!l->next) return l;

	l1 = l;
	l2 = l->next;

	while ((l2 = l2->next) != NULL) {
		if ((l2 = l2->next) == NULL) break;
		l1 = l1->next;
	}

	l2 = l1->next;
	l1->next = NULL;

	return nr_lines_sort_merge (nr_lines_sort (l), nr_lines_sort (l2));
}

static NRLine *
nr_lines_sort_merge (NRLine * l1, NRLine * l2)
{
	NRLine line, * l;

	l = &line;

	while (l1 && l2) {
		if (nr_lines_compare (l1, l2) < 0) {
			l = l->next = l1;
			l1 = l1->next;
		} else {
			l = l->next = l2;
			l2 = l2->next;
		}
	}

	l->next = l1 ? l1 : l2;

	return line.next;
}

gint
nr_lines_compare (NRLine * l0, NRLine * l1)
{
	float xx0, yy0, x1x0, y1y0;
	float d;

	if (l0->s.y < l1->s.y) return -1;
	if (l0->s.y > l1->s.y) return 1;
	if (l0->s.x < l1->s.x) return -1;
	if (l0->s.x > l1->s.x) return 1;

	/* Y is same, X is same, checking for line orders */

	xx0 = l0->e.x - l1->s.x;
	yy0 = l0->e.y - l1->s.y;
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

#define NR_LINE_ALLOC_SIZE 32
static NRLine * firstfreeline = NULL;

NRLine *
nr_line_new (void)
{
	NRLine * l;

	l = firstfreeline;

	if (l == NULL) {
		gint i;
		l = g_new (NRLine, NR_LINE_ALLOC_SIZE);
		for (i = 1; i < (NR_LINE_ALLOC_SIZE - 1); i++) (l + i)->next = (l + i + 1);
		(l + NR_LINE_ALLOC_SIZE - 1)->next = NULL;
		firstfreeline = l + 1;
	} else {
		firstfreeline = l->next;
	}

	l->next = NULL;

	return l;
}

NRLine *
nr_line_new_xyxy (NRCoord x0, NRCoord y0, NRCoord x1, NRCoord y1)
{
	NRLine * l;

	g_assert (y0 != y1);

	l = nr_line_new ();

	if (y1 > y0) {
		l->direction = 1;
		l->s.x = x0;
		l->s.y = y0;
		l->e.x = x1;
		l->e.y = y1;
	} else {
		l->direction = -1;
		l->s.x = x1;
		l->s.y = y1;
		l->e.x = x0;
		l->e.y = y0;
	}

	return l;
}

NRLine *
nr_line_new_xyxyd (NRCoord x0, NRCoord y0, NRCoord x1, NRCoord y1, gint direction)
{
	NRLine * l;

	g_assert (y0 < y1);

	l = nr_line_new ();

	l->direction = direction;
	l->s.x = x0;
	l->s.y = y0;
	l->e.x = x1;
	l->e.y = y1;

	return l;
}

void
nr_line_free (NRLine * line)
{
	line->next = firstfreeline;
	firstfreeline = line;
}

void
nr_line_free_list (NRLine * line)
{
	NRLine * l;

	for (l = line; l->next != NULL; l = l->next);
	l->next = firstfreeline;
	firstfreeline = line;
}

NRLine *
nr_lines_reverse_list (NRLine * line)
{
	NRLine * prev;

	prev = NULL;

	while (line) {
		NRLine * next;
		next = line->next;
		line->next = prev;
		prev = line;
		line = next;
	}

	return prev;
}

NRLine *
nr_lines_insert_sorted (NRLine * start, NRLine * line)
{
	NRLine * s, * l;

	if (!start) return line;
	if (!line) return start;

	if (nr_lines_compare (line, start) < 0) {
		line->next = start;
		return line;
	}

	s = start;
	for (l = start->next; l != NULL; l = l->next) {
		if (nr_lines_compare (line, l) < 0) {
			line->next = l;
			s->next = line;
			return start;
		}
		s = l;
	}

	s->next = line;

	return start;
}

NRSVP *
nr_svp_new (void)
{
	NRSVP * svp;

	svp = g_new (NRSVP, 1);
	svp->lines = NULL;

	return svp;
}

void
nr_svp_free (NRSVP * svp)
{
	g_return_if_fail (svp != NULL);

	if (svp->lines) nr_line_free_list (svp->lines);
	g_free (svp);
}



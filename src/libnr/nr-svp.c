#define __NR_SVP_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include "nr-macros.h"
#include "nr-svp.h"

NRSVP *
nr_svp_from_art_svp (ArtSVP *asvp)
{
	NRSVP *svp;
	int i, j;
	svp = NULL;
	for (i = asvp->n_segs - 1; i >= 0; i--) {
		ArtSVPSeg *seg;
		NRSVP *psvp;
		seg = &asvp->segs[i];
		psvp = nr_svp_new ();
		psvp->next = svp;
		svp = psvp;
		svp->vertex = NULL;
		for (j = seg->n_points - 1; j >= 0; j--) {
			NRVertex *vx;
			vx = nr_vertex_new_xy (seg->points[j].x, seg->points[j].y);
			vx->next = svp->vertex;
			svp->vertex = vx;
		}
		svp->wind = seg->dir ? 1 : -1;
		svp->bbox.x0 = seg->bbox.x0;
		svp->bbox.y0 = seg->bbox.y0;
		svp->bbox.x1 = seg->bbox.x1;
		svp->bbox.y1 = seg->bbox.y1;
	}
	return svp;
}

/* NRVertex */

#define NR_VERTEX_ALLOC_SIZE 256
static NRVertex *ffvertex = NULL;

NRVertex *
nr_vertex_new (void)
{
	NRVertex * v;

	v = ffvertex;

	if (v == NULL) {
		int i;
		v = nr_new (NRVertex, NR_VERTEX_ALLOC_SIZE);
		for (i = 1; i < (NR_VERTEX_ALLOC_SIZE - 1); i++) v[i].next = &v[i + 1];
		v[NR_VERTEX_ALLOC_SIZE - 1].next = NULL;
		ffvertex = v + 1;
	} else {
		ffvertex = v->next;
	}

	return v;
}

NRVertex *
nr_vertex_new_xy (NRCoord x, NRCoord y)
{
	NRVertex * v;

	v = nr_vertex_new ();

	v->x = x;
	v->y = y;

	return v;
}

void
nr_vertex_free_one (NRVertex * v)
{
	v->next = ffvertex;
	ffvertex = v;
}

void
nr_vertex_free_list (NRVertex * v)
{
	NRVertex * l;

	for (l = v; l->next != NULL; l = l->next);
	l->next = ffvertex;
	ffvertex = v;
}

NRVertex *
nr_vertex_reverse_list (NRVertex * v)
{
	NRVertex * p;

	p = NULL;

	while (v) {
		NRVertex * n;
		n = v->next;
		v->next = p;
		p = v;
		v = n;
	}

	return p;
}

/* NRSVP */

#define NR_SVP_ALLOC_SIZE 32
static NRSVP *ffsvp = NULL;

NRSVP *
nr_svp_new (void)
{
	NRSVP *svp;

	svp = ffsvp;

	if (svp == NULL) {
		int i;
		svp = nr_new (NRSVP, NR_SVP_ALLOC_SIZE);
		for (i = 1; i < (NR_SVP_ALLOC_SIZE - 1); i++) svp[i].next = &svp[i + 1];
		svp[NR_SVP_ALLOC_SIZE - 1].next = NULL;
		ffsvp = svp + 1;
	} else {
		ffsvp = svp->next;
	}

	return svp;
}

NRSVP *
nr_svp_new_full (NRVertex *vertex, NRRectF *bbox, int wind)
{
	NRSVP *svp;

	svp = nr_svp_new ();

	svp->vertex = vertex;
	svp->bbox = *bbox;
	svp->wind = wind;

	return svp;
}

NRSVP *
nr_svp_new_vertex_wind (NRVertex *vertex, int wind)
{
	NRSVP * svp;

	svp = nr_svp_new ();

	svp->vertex = vertex;
	svp->wind = wind;
	nr_svp_calculate_bbox (svp);

	return svp;
}

void
nr_svp_free_one (NRSVP *svp)
{
	svp->next = ffsvp;
	ffsvp = svp;
}

void
nr_svp_free_list (NRSVP *svp)
{
	NRSVP *l;

	if (svp) {
		for (l = svp; l->next != NULL; l = l->next);
		l->next = ffsvp;
		ffsvp = svp;
	}
}


NRSVP *
nr_svp_remove (NRSVP *start, NRSVP *svp)
{
	NRSVP * s, * l;

	s = NULL;
	l = start;
	while (l != svp) {
		s = l;
		l = l->next;
	}

	if (s) {
		s->next = l->next;
		return start;
	}

	return svp->next;
}

NRSVP *
nr_svp_insert_sorted (NRSVP *start, NRSVP *svp)
{
	NRSVP * s, * l;

	if (!start) return svp;
	if (!svp) return start;

	if (nr_svp_compare (svp, start) <= 0) {
		svp->next = start;
		return svp;
	}

	s = start;
	for (l = start->next; l != NULL; l = l->next) {
		if (nr_svp_compare (svp, l) <= 0) {
			svp->next = l;
			s->next = svp;
			return start;
		}
		s = l;
	}

	svp->next = NULL;
	s->next = svp;

	return start;
}

NRSVP *
nr_svp_move_sorted (NRSVP *start, NRSVP *svp)
{
	NRSVP *s, *l;

	s = 0;
	l = start;
	while (l != svp) {
		s = l;
		l = l->next;
	}

	if (s) {
		s->next = nr_svp_insert_sorted (svp->next, svp);
		return start;
	}

	return nr_svp_insert_sorted (start, svp);
}

int
nr_svp_compare (NRSVP *l, NRSVP *r)
{
	float xx0, yy0, x1x0, y1y0;
	float d;

	if (l->vertex->y < r->vertex->y) return -1;
	if (l->vertex->y > r->vertex->y) return 1;
	if (l->vertex->x < r->vertex->x) return -1;
	if (l->vertex->x > r->vertex->x) return 1;

	/* Y is same, X is same, checking for line orders */

	xx0 = l->vertex->next->x - r->vertex->x;
	yy0 = l->vertex->next->y - r->vertex->y;
	x1x0 = r->vertex->next->x - r->vertex->x;
	y1y0 = r->vertex->next->y - r->vertex->y;

	d = xx0 * y1y0 - yy0 * x1x0;

	/* fixme: test almost zero cases */
	if (d < 0.0) return -1;
	if (d > 0.0) return 1;

	return 0;
}

void
nr_svp_calculate_bbox (NRSVP *svp)
{
	NRVertex * v;

	svp->bbox.x0 = svp->bbox.y0 = 1e18;
	svp->bbox.x1 = svp->bbox.y1 = -1e18;

	for (v = svp->vertex; v != NULL; v = v->next) {
		svp->bbox.x0 = MIN (svp->bbox.x0, v->x);
		svp->bbox.y0 = MIN (svp->bbox.y0, v->y);
		svp->bbox.x1 = MAX (svp->bbox.x1, v->x);
		svp->bbox.y1 = MAX (svp->bbox.y1, v->y);
	}
}

/* NRFlat */

#define NR_FLAT_ALLOC_SIZE 32
static NRFlat *ffflat = NULL;

NRFlat *
nr_flat_new_full (NRCoord y, NRCoord x0, NRCoord x1)
{
	NRFlat *flat;

	flat = ffflat;

	if (!flat) {
		int i;
		flat = nr_new (NRFlat, NR_FLAT_ALLOC_SIZE);
		for (i = 1; i < (NR_FLAT_ALLOC_SIZE - 1); i++) flat[i].next = &flat[i + 1];
		flat[NR_FLAT_ALLOC_SIZE - 1].next = NULL;
		ffflat = flat + 1;
	} else {
		ffflat = flat->next;
	}

	flat->y = y;
	flat->x0 = x0;
	flat->x1 = x1;

	return flat;
}

void
nr_flat_free_one (NRFlat *flat)
{
	flat->next = ffflat;
	ffflat = flat;
}

void
nr_flat_free_list (NRFlat *flat)
{
	NRFlat *l;

	if (flat) {
		for (l = flat; l->next != NULL; l = l->next);
		l->next = ffflat;
		ffflat = flat;
	}
}

NRFlat *
nr_flat_insert_sorted (NRFlat *start, NRFlat *flat)
{
	NRFlat *s, *l;

	if (!start) return flat;
	if (!flat) return start;

	if ((flat->y < start->y) || ((flat->y == start->y) && (flat->x0 <= start->x0))) {
		flat->next = start;
		return flat;
	}

	s = start;
	for (l = start->next; l != NULL; l = l->next) {
		if ((flat->y < l->y) || ((flat->y == l->y) && (flat->x0 <= l->x0))) {
			flat->next = l;
			s->next = flat;
			return start;
		}
		s = l;
	}

	flat->next = NULL;
	s->next = flat;

	return start;
}

/*
 * Old stuff
 */

#if 0

static NRLine * nr_lines_sort (NRLine * l);
static NRLine * nr_lines_sort_merge (NRLine * l1, NRLine * l2);

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

static void nr_svp_segment_split_flat_list (NRVertex * vertex, NRFlat * flat);

static void
nr_svp_segment_split_flat_list (NRVertex * vertex, NRFlat * flat)
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

#endif


#if 0
NRSVP *
nr_svp_from_art_vpath (ArtVpath * vpath)
{
	NRSVP * svp;
	NRVertex * start, * vertex;
	NRFlat * flats, * flat;
	NRDRect bbox;
	gint dir, newdir;
	NRCoord sx, sy, x, y;
	ArtVpath * s;

	svp = NULL;
	start = NULL;
	flats = NULL;
	dir = 0;
	/* Kill warning */
	bbox.x0 = bbox.y0 = bbox.x1 = bbox.y1 = 0.0;
	sx = sy = 0;

	for (s = vpath; s->code != ART_END; s++) {
		switch (s->code) {
		case ART_MOVETO:
		case ART_MOVETO_OPEN:
			sx = NR_COORD_FROM_ART (s->x);
			sy = NR_COORD_FROM_ART (s->y);
			if (start) {
				NRSVP * new;
				/* We have running segment */
				if (dir > 0) {
					/* We are upwards, prepended, so reverse */
					start = nr_vertex_reverse_list (start);
				}
				new = nr_svp_new_full (start, &bbox, dir);
				svp = nr_svp_insert_sorted (svp, new);
			}
			start = NULL;
			dir = 0;
			break;
		case ART_LINETO:
			x = NR_COORD_FROM_ART (s->x);
			y = NR_COORD_FROM_ART (s->y);
			if (y != sy) {
				/* We have valid line */
				newdir = (y > sy) ? 1 : -1;
				if (newdir != dir) {
					/* We have either start or turn */
					if (start) {
						NRSVP * new;
						/* We have running segment */
						if (dir > 0) {
							/* We are upwards, prepended, so reverse */
							start = nr_vertex_reverse_list (start);
						}
						new = nr_svp_new_full (start, &bbox, dir);
						svp = nr_svp_insert_sorted (svp, new);
					}
					start = NULL;
					dir = newdir;
				}
				if (!start) {
					start = nr_vertex_new_xy (sx, sy);
					bbox.x0 = bbox.x1 = sx;
					bbox.y0 = bbox.y1 = sy;
				}
				/* Add vertex to list */
				vertex = nr_vertex_new_xy (x, y);
				vertex->next = start;
				start = vertex;
				nr_drect_stretch_xy (&bbox, x, y);
				sx = x;
				sy = y;
			} else if (x != sx) {
				/* Horizontal line ends running segment */
				if (start) {
					NRSVP * new;
					/* We have running segment */
					if (dir > 0) {
						/* We are upwards, prepended, so reverse */
						start = nr_vertex_reverse_list (start);
					}
					new = nr_svp_new_full (start, &bbox, dir);
					svp = nr_svp_insert_sorted (svp, new);
				}
				start = NULL;
				dir = 0;
				/* Add horizontal lines to flat list */
				flat = nr_flat_new_full (y, MIN (sx, x), MAX (sx, x));
				flats = nr_flat_insert_sorted (flats, flat);
				sx = x;
				/* sy = y ;-) */
			}
			break;
		default:
			/* fixme: free lists */
			return NULL;
			break;
		}
	}
	if (start) {
		NRSVP * new;
		/* We have running segment */
		if (dir > 0) {
			/* We are upwards, prepended, so reverse */
			start = nr_vertex_reverse_list (start);
		}
		new = nr_svp_new_full (start, &bbox, dir);
		svp = nr_svp_insert_sorted (svp, new);
	}

	/* Test for empty SVP */
	if (!svp) {
		nr_flat_free_list (flats);
		return NULL;
	}

#if 0
	/* Now split all lines crossing flats */
	if (flats) {
		nr_svp_split_flat_list (svp, flats);
		nr_flat_free_list (flats);
	}

#if 0
	nr_svp_uncross (svp);
#endif
#else
	svp = nr_svp_uncross_full (svp, flats);
	if (flats) nr_flat_free_list (flats);
#endif

	return svp;
}

ArtSVP *
nr_art_svp_from_svp (NRSVP * svp)
{
	ArtSVP * asvp;
	NRSVP * s;
	gint n_segs, sn;

	if (!svp) {
		asvp = art_alloc (sizeof (ArtSVP));
		asvp->n_segs = 0;
		return asvp;
	}

	n_segs = 0;
	for (s = svp; s != NULL; s = s->next) n_segs++;

	asvp = art_alloc (sizeof (ArtSVP) + (n_segs - 1) * sizeof (ArtSVPSeg));
	asvp->n_segs = n_segs;

	sn = 0;
	for (s = svp; s != NULL; s = s->next) {
		ArtSVPSeg * aseg;
		NRVertex * v;
		gint n_points, pn;

		aseg = &asvp->segs[sn];

		n_points = 0;
		for (v = s->vertex; v != NULL; v = v->next) n_points++;
		aseg->n_points = n_points;

		aseg->dir = (s->wind == -1);

		aseg->points = art_new (ArtPoint, n_points);

		pn = 0;
		for (v = s->vertex; v != NULL; v = v->next) {
			aseg->points[pn].x = NR_COORD_TO_ART (v->x);
			aseg->points[pn].y = NR_COORD_TO_ART (v->y);
			pn++;
		}

		aseg->bbox.x0 = NR_COORD_TO_ART (s->bbox.x0);
		aseg->bbox.y0 = NR_COORD_TO_ART (s->bbox.y0);
		aseg->bbox.x1 = NR_COORD_TO_ART (s->bbox.x1);
		aseg->bbox.y1 = NR_COORD_TO_ART (s->bbox.y1);

		sn++;
	}

	return asvp;
}

#endif


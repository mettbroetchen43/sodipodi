#define __NR_SVP_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#define NR_COORD_X_FROM_ART(v) (v)
#define NR_COORD_Y_FROM_ART(v) (floor (64.0 * v + 0.5) / 64.0)
#define NR_COORD_TO_ART(v) (v)

#include <libart_lgpl/art_misc.h>

#include "nr-macros.h"
#include "nr-svp-uncross.h"
#include "nr-svp.h"

NRSVP *
nr_svp_from_art_vpath (ArtVpath *vpath)
{
	NRSVP * svp;
	NRVertex * start, * vertex;
	NRFlat * flats, * flat;
	NRRectF bbox;
	int dir, newdir;
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
			sx = NR_COORD_X_FROM_ART (s->x);
			sy = NR_COORD_Y_FROM_ART (s->y);
			if (start) {
				NRSVP *new;
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
			x = NR_COORD_X_FROM_ART (s->x);
			y = NR_COORD_Y_FROM_ART (s->y);
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
				/* Stretch bbox */
				bbox.x0 = MIN (bbox.x0, x);
				bbox.y0 = MIN (bbox.y0, y);
				bbox.x1 = MAX (bbox.x1, x);
				bbox.y1 = MAX (bbox.y1, y);
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

	if (svp) {
		svp = nr_svp_uncross_full (svp, flats);
		}
	nr_flat_free_list (flats);

	return svp;
}

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

ArtSVP *
nr_art_svp_from_svp (NRSVP * svp)
{
	ArtSVP * asvp;
	NRSVP * s;
	int n_segs, sn;

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
		int n_points, pn;

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

	v->next = NULL;

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

	svp->next = NULL;

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

	flat->next = NULL;
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


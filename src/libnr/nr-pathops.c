#define __NR_PATHOPS_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

/* Points are 24 bit (range 2e20 with precision 2e-4) */

#define QUANT 16.0
#define IQUANT (1.0 / QUANT)
#define IQUANT2 (0.5 / QUANT)
#define IQUANT4 (0.25 / QUANT)
#define QROUND(v) (floor (QUANT * (v) + 0.5) / QUANT)

#include <math.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#include "nr-pathops.h"

static struct _NRNode *nr_node_alloc (void);

/* Build flat list for node */
/* Next must be defined */

static struct _NRFlatNode *
nr_node_flatten_curve (double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3,
		       double s0, double s3, double tolerance2,
		       struct _NRFlatNode *next)
{
	struct _NRFlatNode *node;
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
	f2 = tolerance2;
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
	/* Add initial node if needed */
	x0 = QROUND (x0);
	y0 = QROUND (y0);
	if ((x0 != next->x) || (y0 != next->y)) {
		node = nr_flat_node_new (x0, y0, s0);
		node->next = next;
		next->prev = node;
		node->prev = NULL;
		return node;
	} else {
		return next;
	}

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

	node = nr_node_flatten_curve (xttt, yttt, x1tt, y1tt, x11t, y11t, x3, y3,
				      0.5 * (s0 + s3), s3, tolerance2,
				      next);
	node = nr_node_flatten_curve (x0, y0, x00t, y00t, x0tt, y0tt, xttt, yttt,
				      s0, 0.5 * (s0 + s3), tolerance2,
				      node);
	return node;
}

static struct _NRFlatNode *
nr_node_flat_list_build (const struct _NRNode *node)
{
	struct _NRFlatNode *flat;
	flat = nr_flat_node_new (node->next->x3, node->next->y3, 1.0);
	flat->next = NULL;
	flat->prev = NULL;
	return nr_node_flatten_curve (node->x3, node->y3,
				      node->next->x1, node->next->y1,
				      node->next->x2, node->next->y2,
				      node->next->x3, node->next->y3,
				      0.0, 1.0, 0.25,
				      flat);
}

struct _NRNodePathBuildData {
	struct _NRNodePath *npath;
	unsigned int curseg;
	/* struct _NRNode *curnode; */
	float x0, y0;
	unsigned int val;
};

static unsigned int
nr_node_path_build_moveto (float x0, float y0, unsigned int flags, void *data)
{
	struct _NRNodePathBuildData *ndata;
	struct _NRNodeSeg *nseg;
	ndata = (struct _NRNodePathBuildData *) data;
	/* Current segment */
	nseg = ndata->npath->segs + ndata->curseg;
	nseg->nodes = NULL;
	nseg->last = NULL;
	nseg->closed = ((flags & NR_PATH_CLOSED) != 0);
	nseg->value = ndata->val;
	/* Round and save coordinates */
	ndata->x0 = QROUND (x0);
	ndata->y0 = QROUND (y0);
	/* ndata->curnode = NULL; */
	return 1;
}

static unsigned int
nr_node_path_build_lineto (float x0, float y0, float x1, float y1, unsigned int flags, void *data)
{
	struct _NRNodePathBuildData *ndata;
	struct _NRNodeSeg *nseg;
	ndata = (struct _NRNodePathBuildData *) data;
	/* Current segment */
	nseg = ndata->npath->segs + ndata->curseg;
	x1 = QROUND (x1);
	y1 = QROUND (y1);
	if ((x1 != ndata->x0) || (y1 != ndata->y0)) {
		struct _NRNode *node;
		if (!nseg->nodes) {
			/* Prepend initial segment */
			node = nr_node_new_line (ndata->x0, ndata->y0);
			nseg->nodes = node;
			nseg->last = node;
			/* ndata->curnode = node; */
		}
		node = nr_node_new_line (x1, y1);
		node->prev = nseg->last;
		nseg->last->next = node;
		nseg->last = node;
		/* Save currentpoint */
		ndata->x0 = x1;
		ndata->y0 = y1;
	}
	return 1;
}

static unsigned int
nr_node_path_build_curveto3 (float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3,
			     unsigned int flags, void *data)
{
	struct _NRNodePathBuildData *ndata;
	struct _NRNodeSeg *nseg;
	ndata = (struct _NRNodePathBuildData *) data;
	/* Current segment */
	nseg = ndata->npath->segs + ndata->curseg;
	x0 = QROUND (x0);
	y0 = QROUND (y0);
	x1 = QROUND (x1);
	y1 = QROUND (y1);
	x2 = QROUND (x2);
	y2 = QROUND (y2);
	x3 = QROUND (x3);
	y3 = QROUND (y3);
	if ((x3 != x0) || (y3 != y0) || (x1 != x0) || (y3 != y0) || (x2 != x0) || (y2 != y0)) {
		struct _NRNode *node;
		if (!nseg->nodes) {
			/* Prepend initial segment */
			node = nr_node_new_line (ndata->x0, ndata->y0);
			nseg->nodes = node;
			nseg->last = node;
			/* ndata->curnode = node; */
		}
		node = nr_node_new_curve (x1, y1, x2, y2, x3, y3);
		node->prev = nseg->last;
		nseg->last->next = node;
		nseg->last = node;
		/* Save currentpoint */
		ndata->x0 = x3;
		ndata->y0 = y3;
	}
	return 1;
}

static unsigned int
nr_node_path_build_curveto2 (float x0, float y0, float x1, float y1, float x2, float y2,
			     unsigned int flags, void *data)
{
	return nr_node_path_build_curveto3 (x0, y0,
					    x1 + (x0 - x1) / 3.0, y1 + (y0 - y1) / 3.0,
					    x1 + (x2 - x1) / 3.0, y1 + (y2 - y1) / 3.0, x2, y2, flags, data);
}

static unsigned int
nr_node_path_build_endpath (float ex, float ey, float sx, float sy, unsigned int flags, void *data)
{
	struct _NRNodePathBuildData *ndata;
	struct _NRNodeSeg *nseg;
	ndata = (struct _NRNodePathBuildData *) data;
	/* Current segment */
	nseg = ndata->npath->segs + ndata->curseg;
	sx = QROUND (sx);
	sy = QROUND (sy);
	ex = QROUND (ex);
	ey = QROUND (ey);
	if ((flags & NR_PATH_CLOSED) && ((sx != ex) || (sy != ey))) {
		nr_node_path_build_lineto (ex, ey, sx, sy, flags, data);
	}
	if (nseg->nodes) {
		ndata->curseg += 1;
	}
	return 1;
}

static NRPathGVector npgv = {
	nr_node_path_build_moveto,
	nr_node_path_build_lineto,
	nr_node_path_build_curveto2,
	nr_node_path_build_curveto3,
	nr_node_path_build_endpath
};

struct _NRNodePath *
nr_node_path_new_from_path (const NRPath *path, unsigned int value)
{
	struct _NRNodePathBuildData ndata;
	unsigned int size, i;
	size = sizeof (struct _NRNodePath) + (path->nsegments - 1) * sizeof (struct _NRNodeSeg);
	ndata.npath = (struct _NRNodePath *) malloc (size);
	ndata.npath->nsegs = path->nsegments;
	for (i = 0; i < ndata.npath->nsegs; i++) {
		ndata.npath->segs[i].nodes = NULL;
	}
	ndata.curseg = 0;
	ndata.val = value;
	if (!nr_path_forall (path, NULL, &npgv, &ndata)) {
		nr_node_path_free (ndata.npath);
		return NULL;
	}
	return ndata.npath;
}

void
nr_node_path_free (struct _NRNodePath *npath)
{
	unsigned int i;
	for (i = 0; i < npath->nsegs; i++) {
		struct _NRNodeSeg *seg;
		struct _NRNode *node;
		seg = npath->segs + i;
		node = seg->nodes;
		while (node) {
			struct _NRNode *next;
			next = node->next;
			nr_node_free (node);
			node = next;
		}
	}
	free (npath);
}

static struct _NRNode *
nr_node_list_copy (const struct _NRNode *src)
{
	struct _NRNode *first, *ref, *dnode;
	const struct _NRNode *snode;
	first = NULL;
	ref = NULL;
	for (snode = src; snode; snode = snode->next) {
		dnode = nr_node_alloc ();
		dnode->prev = ref;
		if (ref) ref->next = dnode;
		dnode->x1 = snode->x1;
		dnode->y1 = snode->y1;
		dnode->x2 = snode->x2;
		dnode->y2 = snode->y2;
		dnode->x3 = snode->x3;
		dnode->y3 = snode->y3;
		dnode->isline = snode->isline;
		dnode->flats = NULL;
		if (!first) first = dnode;
		ref = dnode;
	}
	dnode->next = NULL;
	return first;
}

struct _NRNodePath *
nr_node_path_concat (struct _NRNodePath *paths[], unsigned int npaths)
{
	struct _NRNodePath *npath;
	unsigned int nsegs, segpos, i, j;
	nsegs = 0;
	for (i = 0; i < npaths; i++) {
		nsegs += paths[i]->nsegs;
	}
	npath = (struct _NRNodePath *) malloc (sizeof (struct _NRNodePath) + (nsegs - 1) * sizeof (struct _NRNodeSeg));
	npath->nsegs = nsegs;
	segpos = 0;
	for (i = 0; i < npaths; i++) {
		for (j = 0; j < paths[i]->nsegs; j++) {
			const struct _NRNodeSeg *sseg;
			struct _NRNodeSeg *dseg;
			sseg = paths[i]->segs + j;
			dseg = npath->segs + segpos + j;
			dseg->closed = sseg->closed;
			dseg->value = sseg->value;
			dseg->nodes = nr_node_list_copy (sseg->nodes);
			dseg->last = dseg->nodes;
			while (dseg->last->next) dseg->last = dseg->last->next;
		}
		segpos += paths[i]->nsegs;
	}
	return npath;
}

static unsigned int
nr_node_list_insert_line (struct _NRNode *node, const NRPointF *p)
{
	struct _NRNode *nnode;
	if ((p->x == node->x3) && (p->y == node->y3)) return 0;
	if ((p->x == node->next->x3) && (p->y == node->next->y3)) return 0;
	nnode = nr_node_new_line (p->x, p->y);
	nnode->prev = node;
	nnode->next = node->next;
	node->next = nnode;
	nnode->next->prev = nnode;
	return 1;
}

static unsigned int
nr_node_list_insert_curve (struct _NRNode *node, struct _NRFlatNode *flat, const NRPointF *p)
{
	struct _NRNode *nnode;
	struct _NRFlatNode *nflat, *f;
	double x000, y000, x001, y001, x011, y011, x111, y111;
	double x00t, y00t, x01t, y01t, x0tt, y0tt, x1tt, y1tt, x11t, y11t, xttt, yttt;
	double dlen, slen, s, t;

	/* Stop if rounded to the next node */
	if ((p->x == flat->next->x) && (p->y == flat->next->y) && !flat->next->next) return 0;

	/* These kill self-interection - they are probably not needed as well (Lauris) */
	/* if ((p->x == node->x3) && (p->y == node->y3)) return 0; */
	/* if ((p->x == node->next->x3) && (p->y == node->next->y3)) return 0; */
	dlen = hypot (p->x - flat->x, p->y - flat->y);
	slen = hypot (flat->next->x - flat->x, flat->next->y - flat->y);
	s = flat->s + (flat->next->s - flat->s) * dlen / slen;
	t = 1.0 - s;

	x000 = node->x3;
	y000 = node->y3;
	x001 = node->next->x1;
	y001 = node->next->y1;
	x011 = node->next->x2;
	y011 = node->next->y2;
	x111 = node->next->x3;
	y111 = node->next->y3;

	x00t = t * x000 + s * x001;
	x01t = t * x001 + s * x011;
	x11t = t * x011 + s * x111;
	x0tt = t * x00t + s * x01t;
	x1tt = t * x01t + s * x11t;
	xttt = t * x0tt + s * x1tt;

	y00t = t * y000 + s * y001;
	y01t = t * y001 + s * y011;
	y11t = t * y011 + s * y111;
	y0tt = t * y00t + s * y01t;
	y1tt = t * y01t + s * y11t;
	yttt = t * y0tt + s * y1tt;

	nnode = nr_node_new_curve (QROUND (x00t), QROUND (y00t), QROUND (x0tt), QROUND (y0tt), p->x, p->y);
	nnode->prev = node;
	nnode->next = node->next;
	node->next = nnode;
	nnode->next->prev = nnode;
	nnode->next->x1 = QROUND (x1tt);
	nnode->next->y1 = QROUND (y1tt);
	nnode->next->x2 = QROUND (x11t);
	nnode->next->y2 = QROUND (y11t);

	/* Insert new flat at the beginning of new seg if needed */
	if ((p->x != flat->next->x) || (p->y != flat->next->y)) {
		nflat = nr_flat_node_new (p->x, p->y, s);
		nflat->next = flat->next;
		nflat->next->prev = nflat;
		nflat->prev = NULL;
		nnode->flats = nflat;
	} else {
		nnode->flats = flat->next;
		nnode->flats->prev = NULL;
	}
	/* Append flat to old segment is needed */
	if ((p->x != flat->x) || (p->y != flat->y)) {
		nflat = nr_flat_node_new (p->x, p->y, s);
		nflat->next = NULL;
		nflat->prev = flat;
		flat->next = nflat;
	} else {
		flat->next = NULL;
	}

	/* Recalculate s */
	for (f = node->flats; f; f = f->next) f->s = f->s / s;
	for (f = nnode->flats; f; f = f->next) f->s = (f->s - s) / (1.0 - s);

	return 1;
}

static unsigned int
nr_node_uncross (struct _NRNode *n0_0, struct _NRNode *n1_0)
{
	struct _NRNode *n0_1, *n1_1;
	NRPointF a0, a1, b0, b1;
	NRPointD ca[2], cb[2];
	NRPointF cp;
	unsigned int nda, ndb;
	n0_1 = n0_0->next;
	n1_1 = n1_0->next;
	/* Test for equality */
	if (n0_1->isline && n1_1->isline) {
		if ((n0_0->x3 == n1_0->x3) && (n0_0->y3 == n1_0->y3) &&
		    (n0_1->x3 == n1_1->x3) && (n0_1->y3 == n1_1->y3)) return 0;
		if ((n0_0->x3 == n1_1->x3) && (n0_0->y3 == n1_1->y3) &&
		    (n0_1->x3 == n1_0->x3) && (n0_1->y3 == n1_0->y3)) return 0;
	} else if ((n0_0 != n1_0) && !n0_1->isline && !n1_1->isline) {
		if ((n0_0->x3 == n1_0->x3) && (n0_0->y3 == n1_0->y3) &&
		    (n0_1->x1 == n1_1->x1) && (n0_1->y1 == n1_1->y1) &&
		    (n0_1->x2 == n1_1->x2) && (n0_1->y2 == n1_1->y2) &&
		    (n0_1->x3 == n1_1->x3) && (n0_1->y3 == n1_1->y3)) return 0;
		if ((n0_0->x3 == n1_1->x3) && (n0_0->y3 == n1_1->y3) &&
		    (n0_1->x1 == n1_1->x2) && (n0_1->y1 == n1_1->y2) &&
		    (n0_1->x2 == n1_1->x1) && (n0_1->y2 == n1_1->y1) &&
		    (n0_1->x3 == n1_0->x3) && (n0_1->y3 == n1_0->y3)) return 0;
	}
	if (n0_1->isline) {
		a0.x = n0_0->x3;
		a0.y = n0_0->y3;
		a1.x = n0_1->x3;
		a1.y = n0_1->y3;
		if (n1_1->isline) {
			b0.x = n1_0->x3;
			b0.y = n1_0->y3;
			b1.x = n1_1->x3;
			b1.y = n1_1->y3;
			if (nr_segment_find_intersections (a0, a1, b0, b1, &nda, ca, &ndb, cb)) {
				/* Insert new nodes at ca and cb */
				while (nda > 0) {
					cp.x = QROUND (ca[nda - 1].x);
					cp.y = QROUND (ca[nda - 1].y);
					nr_node_list_insert_line (n0_0, &cp);
					nda -= 1;
				}
				while (ndb > 0) {
					cp.x = QROUND (cb[ndb - 1].x);
					cp.y = QROUND (cb[ndb - 1].y);
					nr_node_list_insert_line (n1_0, &cp);
					ndb -= 1;
				}
				return 1;
			}
		} else {
			struct _NRFlatNode *f0;
			if (!n1_0->flats) n1_0->flats = nr_node_flat_list_build (n1_0);
			/* Iterate over flats */
			for (f0 = n1_0->flats; f0 && f0->next; f0 = f0->next) {
				if ((f0->prev) &&
				    (((f0->x == a0.x) && (f0->y == a0.y)) || ((f0->x == a1.x) && (f0->y == a1.y)))) {
					/* f0 starts at other line */
					cp.x = f0->x;
					cp.y = f0->y;
					nr_node_list_insert_curve (n1_0, f0, &cp);
				} else {
					b0.x = f0->x;
					b0.y = f0->y;
					b1.x = f0->next->x;
					b1.y = f0->next->y;
					if (nr_segment_find_intersections (a0, a1, b0, b1, &nda, ca, &ndb, cb)) {
						/* Insert new nodes at ca and cb */
						while (nda > 0) {
							cp.x = QROUND (ca[nda - 1].x);
							cp.y = QROUND (ca[nda - 1].y);
							nr_node_list_insert_line (n0_0, &cp);
							nda -= 1;
						}
						while (ndb > 0) {
							cp.x = QROUND (cb[ndb - 1].x);
							cp.y = QROUND (cb[ndb - 1].y);
							nr_node_list_insert_curve (n1_0, f0, &cp);
							ndb -= 1;
						}
					}
				}
			}
		}
	} else {
		if (!n0_0->flats) n0_0->flats = nr_node_flat_list_build (n0_0);
		if (n1_1->isline) {
			struct _NRFlatNode *f0;
			b0.x = n1_0->x3;
			b0.y = n1_0->y3;
			b1.x = n1_1->x3;
			b1.y = n1_1->y3;
			/* Iterate over flats */
			for (f0 = n0_0->flats; f0 && f0->next; f0 = f0->next) {
				if ((f0->prev) &&
				    (((f0->x == b0.x) && (f0->y == b0.y)) || ((f0->x == b1.x) && (f0->y == b1.y)))) {
					/* f0 starts at other line */
					cp.x = f0->x;
					cp.y = f0->y;
					nr_node_list_insert_curve (n0_0, f0, &cp);
				} else {
					a0.x = f0->x;
					a0.y = f0->y;
					a1.x = f0->next->x;
					a1.y = f0->next->y;
					if (nr_segment_find_intersections (a0, a1, b0, b1, &nda, ca, &ndb, cb)) {
						/* Insert new nodes at ca and cb */
						while (nda > 0) {
							cp.x = QROUND (ca[nda - 1].x);
							cp.y = QROUND (ca[nda - 1].y);
							nr_node_list_insert_curve (n0_0, f0, &cp);
							nda -= 1;
						}
						while (ndb > 0) {
							cp.x = QROUND (cb[ndb - 1].x);
							cp.y = QROUND (cb[ndb - 1].y);
							nr_node_list_insert_line (n1_0, &cp);
							ndb -= 1;
						}
					}
				}
			}
		} else {
			struct _NRFlatNode *f0, *f1;
			if (!n1_0->flats) n1_0->flats = nr_node_flat_list_build (n1_0);
			/* Iterate over flats */
			for (f0 = n0_0->flats; f0 && f0->next; f0 = f0->next) {
				for (f1 = n1_0->flats; f0 && f0->next && f1 && f1->next; f1 = f1->next) {
					/* If processing self-intersect, skip identical flats */
					if ((n0_0 == n1_0) && (f0 == f1)) continue;
					if ((f0->prev) && (f0->x == f1->x) && (f0->y == f1->y)) {
						/* f0 starts at other line */
						cp.x = f0->x;
						cp.y = f0->y;
						nr_node_list_insert_curve (n0_0, f0, &cp);
					}
					if ((f1->prev) && (f1->x == f0->x) && (f1->y == f0->y)) {
						/* f1 starts at other line */
						cp.x = f1->x;
						cp.y = f1->y;
						nr_node_list_insert_curve (n1_0, f1, &cp);
					}
					/* Previous check may have clipped flat list */
					if (!f0->next || !f1->next) continue;
					a0.x = f0->x;
					a0.y = f0->y;
					a1.x = f0->next->x;
					a1.y = f0->next->y;
					b0.x = f1->x;
					b0.y = f1->y;
					b1.x = f1->next->x;
					b1.y = f1->next->y;
					if (nr_segment_find_intersections (a0, a1, b0, b1, &nda, ca, &ndb, cb)) {
						/* Insert new nodes at ca and cb */
						/* B has to be first for self-interect to work */
						while (ndb > 0) {
							cp.x = QROUND (cb[ndb - 1].x);
							cp.y = QROUND (cb[ndb - 1].y);
							nr_node_list_insert_curve (n1_0, f1, &cp);
							ndb -= 1;
						}
						while (nda > 0) {
							cp.x = QROUND (ca[nda - 1].x);
							cp.y = QROUND (ca[nda - 1].y);
							nr_node_list_insert_curve (n0_0, f0, &cp);
							nda -= 1;
						}
					}
				}
			}
		}
	}
	return 0;
}

struct _NRNodePath *
nr_node_path_uncross (struct _NRNodePath *path)
{
	struct _NRNodePath *npath;
	unsigned int i0, i1, i;
	struct _NRNodeSeg *segs;
	unsigned int sizsegs, numsegs;
	/* Step 1 - add nodes to all intersections */
	for (i0 = 0; i0 < path->nsegs; i0++) {
		struct _NRNodeSeg *seg0, *seg1;
		seg0 = path->segs + i0;
		for (i1 = i0; i1 < path->nsegs; i1++) {
			struct _NRNode *n0_0, *n1_0;
			seg1 = path->segs + i1;
			/* fixme: Self-intersect */
			n0_0 = seg0->nodes;
			while (n0_0 && n0_0->next) {
				/* fixme: Self-intersect - does it work now? (Lauris) */
				n1_0 = (seg0 != seg1) ? seg1->nodes : n0_0;
				while (n1_0 && n1_0->next) {
					nr_node_uncross (n0_0, n1_0);
					n1_0 = n1_0->next;
				}
				n0_0 = n0_0->next;
			}
		}
	}
	sizsegs = path->nsegs;
	numsegs = path->nsegs;
	segs = (struct _NRNodeSeg *) malloc (sizsegs * sizeof (struct _NRNodeSeg));
	for (i = 0; i < numsegs; i++) {
		const struct _NRNodeSeg *sseg;
		struct _NRNodeSeg *dseg;
		sseg = path->segs + i;
		dseg = segs + i;
		dseg->closed = sseg->closed;
		dseg->value = sseg->value;
		dseg->nodes = nr_node_list_copy (sseg->nodes);
		dseg->last = dseg->nodes;
		while (dseg->last->next) dseg->last = dseg->last->next;
	}
	/* Step 2 - break at coincident nodes */
	for (i0 = 0; i0 < numsegs; i0++) {
		struct _NRNodeSeg *seg0, *seg1;
		seg0 = segs + i0;
		for (i1 = i0; i1 < numsegs; i1++) {
			struct _NRNode *n0, *n1;
			seg1 = segs + i1;
			/* fixme: Self-intersect */
			n0 = seg0->nodes->next;
			while (n0 && n0->next) {
				/* fixme: Self-intersect */
				n1 = (seg0 != seg1) ? seg1->nodes : n0->next;
				/* Test again because n0 may be clipped */
				while (n0 && n0->next && n1) {
					if ((n0->x3 == n1->x3) && (n0->y3 == n1->y3)) {
						struct _NRNodeSeg *seg;
						/* Ensure space */
						if ((sizsegs - numsegs) < 2) {
							sizsegs = sizsegs << 2;
							segs = realloc (segs, sizsegs * sizeof (struct _NRNodeSeg));
							seg0 = segs + i0;
							seg1 = segs + i1;
						}
						/* break n0 */
						seg = segs + numsegs;
						seg->nodes = nr_node_new_line (n0->x3, n0->y3);
						seg->nodes->next = n0->next;
						seg->nodes->next->prev = seg->nodes;
						seg->last = seg0->last;
						seg->closed = seg0->closed;
						seg->value = seg0->value;
						numsegs += 1;
						n0->next = NULL;
						seg0->last = n0;
						/* break n1 if not endpoint */
						if (n1->prev && n1->next) {
							seg = segs + numsegs;
							seg->nodes = nr_node_new_line (n1->x3, n1->y3);
							seg->nodes->next = n1->next;
							seg->nodes->next->prev = seg->nodes;
							seg->last = seg1->last;
							seg->closed = seg1->closed;
							seg->value = seg1->value;
							numsegs += 1;
							n1->next = NULL;
							seg1->last = n1;
						}
					}
					n1 = n1->next;
				}
				n0 = n0->next;
			}
		}
	}

	/* Dummy copy */
	npath = (struct _NRNodePath *) malloc (sizeof (struct _NRNodePath) + (numsegs - 1) * sizeof (struct _NRNodeSeg));
	npath->nsegs = numsegs;
	for (i = 0; i < numsegs; i++) {
		const struct _NRNodeSeg *sseg;
		struct _NRNodeSeg *dseg;
		sseg = segs + i;
		dseg = npath->segs + i;
		*dseg = *sseg;
	}
	free (segs);
	return npath;
}

unsigned int
nr_segment_find_intersections (NRPointF a0, NRPointF a1, NRPointF b0, NRPointF b1,
			       unsigned int *nda, NRPointD *ca, unsigned int *ndb, NRPointD *cb)
{
	double adx, ady, bdx, bdy;
	double d;

	/* We need nonzero distances */
	assert ((a0.x != a1.x) || (a0.y != a1.y));
	assert ((b0.x != b1.x) || (b0.y != b1.y));

	/* No precision loss here */
	adx = a1.x - a0.x;
	ady = a1.y - a0.y;
	bdx = b1.x - b0.x;
	bdy = b1.y - b0.y;
	/* Maximum difference 48 bits */
	/* No precision loss here */
	/* Vektorkorrutis */
	d = adx * bdy - ady * bdx;
	/* Test if parallel */
	if (d == 0.0) {
		double abdx, abdy;
		/* Conincidence test */
		/* adx/ady == abdx/abdy */
		abdx = b1.x - a0.x;
		abdy = b1.y - a0.y;
		if (adx * abdy == abdx * ady) {
			double adm, bdm;
			double b0am, b1am, a0bm, a1bm;
			/* Coincident */
			/* Find bigger scaler */
			adm = (fabs (adx) > fabs (ady)) ? adx : ady;
			/* b0 position on a multiplied by adm */
			b0am = (fabs (adx) > fabs (ady)) ? (b0.x - a0.x) : (b0.y - a0.y);
			/* b1 position on a multiplied by adm */
			b1am = (fabs (adx) > fabs (ady)) ? (b1.x - a0.x) : (b1.y - a0.y);
			/* Remove sign */
			if (adm < 0.0) {
				adm = -adm;
				b0am = -b0am;
				b1am = -b1am;
			}
			/* Test if any b lies on a */
			if ((b0am <= 0.0) && (b1am <= 0.0)) return 0;
			if ((b0am >= adm) && (b1am >= adm)) return 0;
			/* Write ordered points */
			*nda = 0;
			if (b0am < b1am) {
				if (b0am > 0.0) {
					ca[*nda].x = b0.x;
					ca[*nda].y = b0.y;
					*nda += 1;
				}
				if (b1am < adm) {
					ca[*nda].x = b1.x;
					ca[*nda].y = b1.y;
					*nda += 1;
				}
			}
			/* Find bigger scaler */
			bdm = (fabs (bdx) > fabs (bdy)) ? bdx : bdy;
			/* b0 position on a multiplied by adm */
			a0bm = (fabs (bdx) > fabs (bdy)) ? (a0.x - b0.x) : (a0.y - b0.y);
			/* b1 position on a multiplied by adm */
			a1bm = (fabs (bdx) > fabs (bdy)) ? (a1.x - b0.x) : (a1.y - b0.y);
			/* Remove sign */
			if (bdm < 0.0) {
				bdm = -bdm;
				a0bm = -a0bm;
				a1bm = -a1bm;
			}
			/* Test if any b lies on a */
			if ((a0bm <= 0.0) && (a1bm <= 0.0)) return 0;
			if ((a0bm >= bdm) && (a1bm >= bdm)) return 0;
			/* Write ordered points */
			*ndb = 0;
			if (a0bm < a1bm) {
				if (a0bm > 0.0) {
					cb[*ndb].x = a0.x;
					cb[*ndb].y = a0.y;
					*ndb += 1;
				}
				if (a1bm < bdm) {
					cb[*ndb].x = a1.x;
					cb[*ndb].y = a1.y;
					*ndb += 1;
				}
			}
			return (*nda > 0) || (*ndb > 0);
		} else {
			/* Do not touch each other */
			return 0;
		}
	} else {
		double badx, bady;
		double sd, td;
		/* Not parallel */
		/* Find vector b0a0 */
		badx = a0.x - b0.x;
		bady = a0.y - b0.y;
		/* Intersection scaled with d */
		sd = bdx * bady - bdy * badx;
		td = adx * bady - ady * badx;
		/* Set intersection points */
		*nda = 0;
		*ndb = 0;
		if (d > 0.0) {
			if ((sd >= 0.0) && (sd <= d) && (td >= 0.0) && (td <= d)) {
				/* fixme: Potential loss of precision (Lauris) */
				/* fixme: Maybe not (think) (Lauris) */
				if ((sd > 0.0) && (sd < d)) {
					ca[*nda].x = a0.x + adx * (sd / d);
					ca[*nda].y = a0.y + ady * (sd / d);
					*nda += 1;
				}
				if ((td > 0.0) && (td < d)) {
					cb[*ndb].x = b0.x + bdx * (td / d);
					cb[*ndb].y = b0.y + bdy * (td / d);
					*ndb += 1;
				}
			}
		} else {
			if ((sd <= 0.0) && (sd >= d) && (td <= 0.0) && (td >= d)) {
				if ((sd < 0.0) && (sd > d)) {
					ca[*nda].x = a0.x + adx * (sd / d);
					ca[*nda].y = a0.y + ady * (sd / d);
					*nda += 1;
				}
				if ((td < 0.0) && (td > d)) {
					cb[*ndb].x = b0.x + bdx * (td / d);
					cb[*ndb].y = b0.y + bdy * (td / d);
					*ndb += 1;
				}
			}
		}
		return (*nda > 0) || (*ndb > 0);
	}
}

/* Memory management */

#define BLOCKSIZE_NODE 32
static struct _NRNode *nodes = NULL;

static struct _NRNode *
nr_node_alloc (void)
{
	struct _NRNode *node;
	if (!nodes) {
		unsigned int i;
		nodes = (struct _NRNode *) malloc (BLOCKSIZE_NODE * sizeof (struct _NRNode));
		for (i = 0; i < (BLOCKSIZE_NODE - 1); i++) nodes[i].next = nodes + i + 1;
		nodes[BLOCKSIZE_NODE - 1].next = NULL;
	}
	node = nodes;
	nodes = node->next;
	memset (node, 0x0, sizeof (struct _NRNode));
	return node;
}

struct _NRNode *
nr_node_new_line (float x3, float y3)
{
	struct _NRNode *node;
	node = nr_node_alloc ();
	node->x3 = x3;
	node->y3 = y3;
	node->isline = 1;
	return node;
}

struct _NRNode *
nr_node_new_curve (float x1, float y1, float x2, float y2, float x3, float y3)
{
	struct _NRNode *node;
	node = nr_node_alloc ();
	node->x1 = x1;
	node->y1 = y1;
	node->x2 = x2;
	node->y2 = y2;
	node->x3 = x3;
	node->y3 = y3;
	return node;
}

void
nr_node_free (struct _NRNode *node)
{
	while (node->flats) {
		struct _NRFlatNode *flat;
		flat = node->flats;
		node->flats = flat->next;
		nr_flat_node_free (flat);
	}
	node->next = nodes;
	nodes = node;
}

#define BLOCKSIZE_FLAT 64
static struct _NRFlatNode *flatnodes = NULL;

struct _NRFlatNode *
nr_flat_node_new (float x, float y, double s)
{
	struct _NRFlatNode *node;
	if (!flatnodes) {
		unsigned int i;
		flatnodes = (struct _NRFlatNode *) malloc (BLOCKSIZE_FLAT * sizeof (struct _NRFlatNode));
		for (i = 0; i < (BLOCKSIZE_FLAT - 1); i++) flatnodes[i].next = flatnodes + i + 1;
		flatnodes[BLOCKSIZE_FLAT - 1].next = NULL;
	}
	node = flatnodes;
	flatnodes = node->next;
	memset (node, 0x0, sizeof (struct _NRFlatNode));
	node->x = x;
	node->y = y;
	node->s = s;
	return node;
}

void
nr_flat_node_free (struct _NRFlatNode *node)
{
	node->next = flatnodes;
	flatnodes = node;
}


#ifndef __NR_PATHOPS_H__
#define __NR_PATHOPS_H__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <libnr/nr-types.h>
#include <libnr/nr-path.h>

struct _NRFlatNode {
	struct _NRFlatNode *next;
	struct _NRFlatNode *prev;
	double s;
	float x, y;
};

struct _NRNode {
	struct _NRNode *next;
	struct _NRNode *prev;
	float x1, y1, x2, y2, x3, y3;
	unsigned int isline : 1;
	struct _NRFlatNode *flats;
};

struct _NRNodeSeg {
	struct _NRNode *nodes;
	struct _NRNode *last;
	unsigned int closed : 1;
	unsigned int value : 8;
};

struct _NRNodePath {
	unsigned int nsegs;
	struct _NRNodeSeg segs[1];
};

struct _NRNodePath *nr_node_path_new_from_path (const NRPath *path, unsigned int value);
void nr_node_path_free (struct _NRNodePath *npath);

struct _NRNodePath *nr_node_path_concat (struct _NRNodePath *paths[], unsigned int npaths);

struct _NRNodePath *nr_node_path_uncross (struct _NRNodePath *path);

struct _NRNodePath *nr_node_path_rewind (struct _NRNodePath *path, int ngroups, int *and, int *or, int *self);

/*
 * Returns TRUE if segments intersect
 * nda and ndb are number of intersection points on segment a and b
 * (0-1 if intersect, 0-2 if coincident)
 * ca and cb are ordered arrays of intersection points
 */

unsigned int nr_segment_find_intersections (NRPointF a0, NRPointF a1, NRPointF b0, NRPointF b1,
					    unsigned int *nda, NRPointD *ca, unsigned int *ndb, NRPointD *cb);

/*
 * Returns left winding count of point and segment
 *
 * + is forward, - is reverse
 * upper and lower are endpoint winding counts
 * exact determines whether exact match counts
 */

int nr_segment_find_wind (NRPointD a, NRPointD b, NRPointD p, int *upper, int *lower, unsigned int exact);

/* Memory management */

struct _NRNode *nr_node_new_line (float x3, float y3);
struct _NRNode *nr_node_new_curve (float x1, float y1, float x2, float y2, float x3, float y3);
void nr_node_free (struct _NRNode *node);

struct _NRFlatNode *nr_flat_node_new (float x, float y, double s);
void nr_flat_node_free (struct _NRFlatNode *node);

#endif

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

#include <libnr/nr-path.h>

typedef struct _NRNodePathGroup NRNodePathGroup;
typedef struct _NRNodePath NRNodePath;
typedef struct _NRNode NRNode;
typedef struct _NRConnection NRConnection;

struct _NRConnection {
	NRNode *node;
	unsigned int isline : 1;
	double x, y;
};

struct _NRNode {
	/* Linking is only used to temporarily join coincident nodes */
	NRNode *next;
	NRNode *prev;
	unsigned int value : 24;
	unsigned int isfirst : 1;
	unsigned int islast : 1;
	double x, y;
	unsigned int nconnections;
	NRConnection connections[2];
};

struct _NRNodePath {
	NRNode *nodes;
	unsigned int value : 24;
	unsigned int closed : 1;
};

struct _NRNodePathGroup {
	unsigned int npaths;
	NRNodePath paths[1];
};

NRNodePathGroup *nr_node_path_group_from_path (NRPath *path);
NRNodePathGroup *nr_node_path_group_free (NRNodePathGroup *npg);

void nr_node_path_group_join_coincident (NRNodePathGroup *npg);

NRPath *nr_path_new_from_node_path_group (NRNodePathGroup *npg);

/* Lala */

int nr_path_intersect_self (double path[], double pos[], double tolerance);

typedef struct _NRPathSeg NRPathSeg;

struct _NRPathSeg {
	NRPathSeg *next;
	unsigned int value : 24;
	unsigned int iscurve : 1;
	unsigned int isfirst : 1;
	unsigned int islast : 1;
	double x0, y0, x1, y1, x2, y2;
};

NRPathSeg *nr_path_seg_new_curve (NRPathSeg *next, unsigned int value, unsigned int isfirst, unsigned int islast,
				  double x0, double y0, double x1, double y1, double x2, double y2);
NRPathSeg *nr_path_seg_new_line (NRPathSeg *next, unsigned int value, unsigned int isfirst, unsigned int islast,
				 double x0, double y0);
void nr_path_seg_free (NRPathSeg *seg);

#endif

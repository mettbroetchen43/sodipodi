#ifndef __NR_SVP_H__
#define __NR_SVP_H__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <libnr/nr-types.h>

/* fixme: Move/remove this (Lauris) */
typedef float NRCoord;

/* Sorted vector paths */

typedef struct _NRSVPSegment NRSVPSegment;
typedef struct _NRSVPFlat NRSVPFlat;
typedef struct _NRSVP NRSVP;

struct _NRSVPSegment {
	NRShort wind;
	NRUShort length;
	NRULong start;
	float x0, x1;
};

struct _NRSVPFlat {
	NRShort wind;
	NRUShort length;
	float y;
	float x0, x1;
};

struct _NRSVP {
	unsigned int length;
	NRPointF *points;
	NRSVPSegment segments[1];
};

#define NR_SVPSEG_X0(svp,sidx) ((svp)->segments[sidx].x0)
#define NR_SVPSEG_Y0(svp,sidx) ((svp)->points[(svp)->segments[sidx].start].y)
#define NR_SVPSEG_X1(svp,sidx) ((svp)->segments[sidx].x1)
#define NR_SVPSEG_Y1(svp,sidx) ((svp)->points[(svp)->segments[sidx].start + (svp)->segments[sidx].length - 1].y)

void nr_svp_free (NRSVP *svp);

int nr_svp_point_wind (NRSVP *svp, float x, float y);
double nr_svp_point_distance (NRSVP *svp, float x, float y);

/* Sorted vertex lists */

typedef struct _NRVertex NRVertex;
typedef struct _NRSVL NRSVL;
typedef struct _NRFlat NRFlat;

struct _NRVertex {
	NRVertex *next;
	NRCoord x, y;
};

struct _NRSVL {
	NRSVL *next;
	NRVertex *vertex;
	NRRectF bbox;
	NRShort dir;
	NRShort wind;
};

struct _NRFlat {
	NRFlat *next;
	NRCoord y, x0, x1;
};

NRSVP *nr_svp_from_svl (NRSVL *svl, NRFlat *flat);

/* fixme: Remove these if ready (Lauris) */
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp.h>

NRSVL *nr_svl_from_art_vpath (ArtVpath *vpath);
NRSVL *nr_svl_from_art_svp (ArtSVP *asvp);
ArtSVP *nr_art_svp_from_svl (NRSVL *svl);

int nr_svl_point_wind (NRSVL *svl, float x, float y);

/* NRVertex */

NRVertex *nr_vertex_new (void);
NRVertex *nr_vertex_new_xy (NRCoord x, NRCoord y);
void nr_vertex_free_one (NRVertex *v);
void nr_vertex_free_list (NRVertex *v);

NRVertex *nr_vertex_reverse_list (NRVertex *v);

/* NRSVP */

NRSVL *nr_svl_new (void);
NRSVL *nr_svl_new_full (NRVertex *vertex, NRRectF *bbox, int wind);
NRSVL *nr_svl_new_vertex_wind (NRVertex *vertex, int wind);
void nr_svl_free_one (NRSVL *svl);
void nr_svl_free_list (NRSVL *svl);

NRSVL *nr_svl_remove (NRSVL *start, NRSVL *svp);
NRSVL *nr_svl_insert_sorted (NRSVL *start, NRSVL *svp);
NRSVL *nr_svl_move_sorted (NRSVL *start, NRSVL *svp);
int nr_svl_compare (NRSVL *l, NRSVL *r);

void nr_svl_calculate_bbox (NRSVL *svl);

/* NRFlat */

NRFlat *nr_flat_new_full (NRCoord y, NRCoord x0, NRCoord x1);
void nr_flat_free_one (NRFlat *flat);
void nr_flat_free_list (NRFlat *flat);

NRFlat *nr_flat_insert_sorted (NRFlat *start, NRFlat *flat);

#endif

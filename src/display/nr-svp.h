#ifndef _NR_SVP_H_
#define _NR_SVP_H_

/*
 * Goals:
 *
 * 1. Minimize the number of allocs
 *
 */

#include <glib.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp.h>
#include "nr-primitives.h"

typedef struct _NRFlat NRFlat;
typedef struct _NRVertex NRVertex;
typedef struct _NRSVP NRSVP;

struct _NRVertex {
	NRVertex * next;
	NRCoord x, y;
};

struct _NRSVP {
	NRSVP * next;
	NRVertex * vertex;
	NRDRect bbox;
	int wind;
};

NRSVP * nr_svp_from_art_vpath (ArtVpath * vpath);
ArtSVP * nr_art_svp_from_svp (NRSVP * svp);

/* Memory management stuff */

NRVertex * nr_vertex_new (void);
NRVertex * nr_vertex_new_xy (NRCoord x, NRCoord y);
void nr_vertex_free_one (NRVertex * v);
void nr_vertex_free_list (NRVertex * v);

NRVertex * nr_vertex_reverse_list (NRVertex * v);

NRSVP * nr_svp_new (void);
NRSVP * nr_svp_new_full (NRVertex * vertex, NRDRect * bbox, gint wind);
NRSVP * nr_svp_new_vertex_wind (NRVertex * vertex, gint wind);
void nr_svp_free_one (NRSVP * svp);
void nr_svp_free_list (NRSVP * svp);

NRSVP * nr_svp_remove (NRSVP * start, NRSVP * svp);
NRSVP * nr_svp_insert_sorted (NRSVP * start, NRSVP * svp);
NRSVP * nr_svp_move_sorted (NRSVP * start, NRSVP * svp);
gint nr_svp_compare (NRSVP * l, NRSVP * r);

void nr_svp_calculate_bbox (NRSVP * svp);

/* Internal stuff */

struct _NRFlat {
	NRFlat * next;
	NRCoord y, x0, x1;
};

NRFlat * nr_flat_new_full (NRCoord y, NRCoord x0, NRCoord x1);
void nr_flat_free_one (NRFlat * flat);
void nr_flat_free_list (NRFlat * flat);

NRFlat * nr_flat_insert_sorted (NRFlat * start, NRFlat * flat);

#endif

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

typedef float NRCoord;

typedef struct _NRPoint NRPoint;

struct _NRPoint {
	float x;
	float y;
};

typedef struct _NRLine NRLine;

struct _NRLine {
	NRLine * next;
	gint direction;
	NRPoint s;
	NRPoint e;
};

typedef struct _NRSVP NRSVP;

struct _NRSVP {
	NRLine * lines;
};

#define NR_COORD_FROM_ART(v) (rint ((v) * 16.0) / 16.0)
#define NR_COORD_SNAP(v) (rint ((v) * 16.0) / 16.0)
#define NR_COORD_TO_ART(v) (v)

NRLine * nr_line_new (void);
NRLine * nr_line_new_xyxy (NRCoord x0, NRCoord y0, NRCoord x1, NRCoord y1);
NRLine * nr_line_new_xyxyd (NRCoord x0, NRCoord y0, NRCoord x1, NRCoord y1, gint direction);
void nr_line_free (NRLine * line);
void nr_line_free_list (NRLine * line);
NRLine * nr_lines_reverse_list (NRLine * line);
NRLine * nr_lines_insert_sorted (NRLine * start, NRLine * line);
gint nr_lines_compare (NRLine * l1, NRLine * l2);

NRSVP * nr_svp_new (void);
void nr_svp_free (NRSVP * svp);

NRSVP * nr_svp_from_art_vpath (ArtVpath * vpath);
ArtSVP * nr_art_svp_from_svp (NRSVP * svp);

#endif

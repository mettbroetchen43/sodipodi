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

typedef struct _NRPoint NRPoint;

struct _NRPoint {
	gint32 x;
	gint32 y;
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

#define NR_COORD_FROM_ART(v) ((int) floor ((v) * 16.0 + 0.5))
#define NR_COORD_TO_ART(v) (((double) (v)) / 16.0)
#define NR_COORD_MUL(a,b) ((((a) >> 4) * (b)) + ((((a) & 0xf) * (b)) >> 4))
#define NR_COORD_MUL_DIV(a,b,c) ((a) * (b) / (c))

NRLine * nr_line_new (void);
NRLine * nr_line_new_xyxy (gint32 x0, gint32 y0, gint32 x1, gint32 y1);
NRLine * nr_line_new_xyxyd (gint32 x0, gint32 y0, gint32 x1, gint32 y1, gint direction);
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

#ifndef _NR_SVP_UNCROSS_H_
#define _NR_SVP_UNCROSS_H_

#include "nr-svp.h"

typedef struct _NRSVPSlice NRSVPSlice;

struct _NRSVPSlice {
#if 0
	NRSVPSlice * prev;
#endif
	NRSVPSlice * next;
	NRSVP * svp;
	NRVertex * vertex;
	NRCoord x;
	NRCoord y;
};

NRSVP * nr_svp_uncross_full (NRSVP * svp, NRFlat * flats);

NRSVP * nr_svp_uncross (NRSVP * svp);
NRSVP * nr_svp_rewind (NRSVP * svp);
void nr_svp_split_flat_list (NRSVP * svp, NRFlat * flat);

NRSVPSlice * nr_svp_slice_new (NRSVP * svp, NRCoord y);
void nr_svp_slice_free_one (NRSVPSlice * slice);
void nr_svp_slice_free_list (NRSVPSlice * slice);
NRSVPSlice * nr_svp_slice_insert_sorted (NRSVPSlice * start, NRSVPSlice * slice);

NRSVPSlice * nr_svp_slice_stretch_list (NRSVPSlice * slices, NRCoord y);

#endif

#define _NR_SVP_UNCROSS_C_

#include <math.h>
#include "nr-svp-uncross.h"

static gint nr_svp_slice_compare (NRSVPSlice * l, NRSVPSlice * r);

NRSVP *
nr_svp_uncross_full (NRSVP * svp, NRFlat * flats)
{
	NRSVP * nsvp;
	NRFlat * nflat;
	NRSVPSlice * slices, * s;
	NRCoord yslice, ynew;

	if (!svp) return NULL;
	g_assert (svp->vertex);

	slices = NULL;
	yslice = svp->vertex->y;
	nflat = flats;
	nsvp = svp;

	/* Main iteration */
	while ((slices) || (nsvp)) {
		/* Stretch existing slices */
		/* fixme: This should probably go to end of loop */
		slices = nr_svp_slice_stretch_list (slices, yslice);
		/* Advance flats < yslice */
		/* fixme: This is really needed only for initial step */
		while ((nflat) && (nflat->y < yslice)) nflat = nflat->next;
		/* Add svps == starting from yslice to slices list */
		g_assert ((!nsvp) || (nsvp->vertex->y >= yslice));
		while ((nsvp) && (nsvp->vertex->y == yslice)) {
			NRSVPSlice * newslice;
			newslice = nr_svp_slice_new (nsvp, yslice);
			slices = nr_svp_slice_insert_sorted (slices, newslice);
			nsvp = nsvp->next;
		}
		/* Now everything should be set up */
		/* Process intersections */
		/* Process flats (NB! we advance nflat to first > y) */
		while ((nflat) && (nflat->y == yslice)) {
			for (s = slices; s != NULL; s = s->next) {
				g_assert (s->vertex->y <= yslice);
				g_assert (s->vertex->next->y > yslice);
				/* fixme: We can safely use EPSILON here */
				if ((s->x >= nflat->x0) && (s->x <= nflat->x1)) {
					if (s->vertex->y < yslice) {
						NRVertex * newvertex;
						NRSVP * newsvp;
						/* Mid-segment intersection */
						/* Create continuation svp */
						newvertex = nr_vertex_new_xy (s->x, s->y);
						newvertex->next = s->vertex->next;
						newsvp = nr_svp_new_vertex_wind (newvertex, s->svp->wind);
						/* Trim starting svp */
						newvertex = nr_vertex_new_xy (s->x, s->y);
						s->vertex->next = newvertex;
						nr_svp_calculate_bbox (s->svp);
						/* Insert new SVP into main list */
						/* new svp slice is included by definition */
						svp = nr_svp_insert_sorted (svp, newsvp);
						/* fixme: We should maintain pointer to ssvp */
						/* New svp is inserted before nsvp by definition, so we can ignore management */
						/* Old svp will be excluded by definition, so we can shortcut */
						s->svp = newsvp;
						s->vertex = newsvp->vertex;
						/* s->x and s->y are correct by definition */
					} else if (s->vertex != s->svp->vertex) {
						NRVertex * newvertex;
						NRSVP * newsvp;
						g_assert (s->vertex->y == yslice);
						/* Inter-segment intersection */
						/* Create continuation svp */
						newvertex = nr_vertex_new_xy (s->x, s->y);
						newvertex->next = s->vertex->next;
						newsvp = nr_svp_new_vertex_wind (newvertex, s->svp->wind);
						/* Trim starting svp */
						s->vertex->next = NULL;
						nr_svp_calculate_bbox (s->svp);
						/* Insert new SVP into main list */
						/* new svp slice is included by definition */
						svp = nr_svp_insert_sorted (svp, newsvp);
						/* fixme: We should maintain pointer to ssvp */
						/* New svp is inserted before nsvp by definition, so we can ignore management */
						/* Old svp will be excluded by definition, so we can shortcut */
						s->svp = newsvp;
						s->vertex = newsvp->vertex;
						/* s->x and s->y are correct by definition */
					}
				}
			}
			nflat = nflat->next;
		}
		/* calculate winds */
		/* Calculate next yslice */
		ynew = 1e18;
		for (s = slices; s != NULL; s = s->next) {
			g_assert (s->vertex->next);
			g_assert (s->vertex->next->y > yslice);
			if (s->vertex->next->y < ynew) ynew = s->vertex->next->y;
		}
		if ((nflat) && (nflat->y < ynew)) ynew = nflat->y;
		if ((nsvp) && (nsvp->vertex->y < ynew)) ynew = nsvp->vertex->y;
		yslice = ynew;
	}

	return svp;
}

#if 0
#define NR_EPSILON 1e-9

static NRSVP * nr_svp_uncross_one (NRSVP * ssvp, NRSVP * svp, NRFlat ** flats);
static gboolean nr_svp_uncross_colinear (NRSVP * ssvp, NRVertex * ssv, NRVertex * scv, NRSVP * svp, NRVertex * sv, NRVertex * cv, NRFlat ** flats);
static gboolean nr_svp_uncross_crossing (NRSVP * ssvp, NRVertex * ssv, NRVertex * scv, NRSVP * svp, NRVertex * sv, NRVertex * cv, NRFlat ** flats);
static gint nr_x_order_simple (NRVertex * v0, NRVertex * v1);

NRSVP *
nr_svp_uncross (NRSVP * svp)
{
	NRSVP * base, * current;
	NRFlat * flats;

	if (!svp) return NULL;

	flats = NULL;
	base = svp;
	current = base->next;

	while ((current) && (current->next)) {
		current = nr_svp_uncross_one (current, current->next, &flats);
	}

	if (flats) {
		if (svp) nr_svp_split_flat_list (svp, flats);
		nr_flat_free_list (flats);
	}

	return svp;
}

/* fixme: optimize sequential processing */

static NRSVP *
nr_svp_uncross_one (NRSVP * ssvp, NRSVP * svp, NRFlat ** flats)
{
	/* Find first with colliding bbox */
	while (svp) {
		if (nr_drect_do_intersect (&ssvp->bbox, &svp->bbox)) {
			NRVertex * ssv, * scv;
			/* Find first (possibly) colliding vertex/svp pair */
			ssv = NULL;
			scv = ssvp->vertex;
			while (scv->next) {
				if ((scv->next->y > svp->bbox.y0) &&
				    (scv->y <= svp->bbox.y1) &&
				    (MAX (scv->x, scv->next->x) >= svp->bbox.x0) &&
				    (MIN (scv->x, scv->next->x) <= svp->bbox.x1)) {
					NRVertex * sv, * cv;
					NRCoord sxmin, sxmax;
					/* Find first (possibly) colliding vertex/vertex pair */
					sxmin = MIN (scv->x, scv->next->x);
					sxmax = MAX (scv->x, scv->next->x);
					sv = NULL;
					cv = svp->vertex;
					while (cv->next) {
						if ((cv->next->y > scv->y) &&
						    (cv->y <= scv->next->y) &&
						    (MAX (cv->x, cv->next->x) >= sxmin) &&
						    (MIN (cv->y, cv->next->x) <= sxmax)) {
							/* Potential intersection */
							/* fixme: move computation here */
							if (nr_x_order_simple (scv, cv) == 0) {
								/* Possibly coincident */
								if (nr_svp_uncross_colinear (ssvp, ssv, scv, svp, sv, cv, flats)) {
									return ssvp;
								}
							} else {
								/* Possibly intersecting */
								if (nr_svp_uncross_crossing (ssvp, ssv, scv, svp, sv, cv, flats)) {
									return ssvp;
								}
							}
						}
						/* cv didn't intersect with scv */
						sv = cv;
						cv = cv->next;
					}
				}
				/* scv didn't intersect with any svp vertex */
				ssv = scv;
				scv = scv->next;
			}
		}
		svp = svp->next;
	}

	/* We didn't intersect with any following SVP */
	return ssvp->next;
}

/* Returns TRUE if intersect */
/* fixme: optimize */
/* fixme: strict numerical analysis */

static gboolean
nr_svp_uncross_colinear (NRSVP * ssvp, NRVertex * ssv, NRVertex * scv, NRSVP * svp, NRVertex * sv, NRVertex * cv, NRFlat ** flats)
{
	NRCoord x0, y0, x1, y1;
	NRVertex * s0, * s1, * c0, * c1, * common;
	gint wind0, wind1;

	/* Speedup */
	if ((scv->x == cv->next->x) && (scv->y == cv->next->y)) return FALSE;
	if ((scv->next->x == cv->x) && (scv->next->y == cv->y)) return FALSE;

	/* Invariant */
	g_assert (((scv->next->x >= scv->x) && (cv->next->x >= cv->x)) || ((scv->next->x <= scv->x) && (cv->next->x <= cv->x)));

	s0 = s1 = c0 = c1 = common = NULL;
	wind0 = ssvp->wind;
	wind1 = svp->wind;

	/* Find coordinates of common segment */
	if (scv->y < cv->y) {
		x0 = cv->x;
		y0 = cv->y;
	} else if (scv->y > cv->y) {
		x0 = scv->x;
		y0 = scv->y;
	} else {
		y0 = scv->y;
		if ((scv->next->x >= scv->x) && (cv->x >= scv->x)) {
			x0 = cv->x;
		} else {
			x0 = scv->x;
		}
	}
	if (scv->next->y > cv->next->y) {
		x1 = cv->next->x;
		y1 = cv->next->y;
	} else if (scv->next->y < cv->next->y) {
		x1 = scv->next->x;
		y1 = scv->next->y;
	} else {
		y1 = scv->next->y;
		if ((scv->next->x >= scv->x) && (cv->next->x <= scv->next->x)) {
			x1 = cv->next->x;
		} else {
			x1 = scv->next->x;
		}
	}

	/*
	 * Now we have to do:
	 *
	 * scv -> (x0, y0)
	 * (x1, y1) -> scv->next
	 * cv -> (x0, y0)
	 * (x1, y1) -> scv->next;
	 * (x0, y0) -> (x1, y1)
	 */

	/* Split ssvp */
	/* Continuation segment */
	if (scv->next->y > y1) {
		/* easy case - add new segment */
		s1 = nr_vertex_new_xy (x1, y1);
		s1->next = scv->next;
		scv->next = NULL;
	} else {
		g_assert (scv->next->y == y1);
		if (scv->next->x != x0) {
			NRFlat * flat;
			/* We create flat */
			flat = nr_flat_new_full (y1, MIN (x1, scv->next->x), MAX (x1, scv->next->x));
			*flats = nr_flat_insert_sorted (*flats, flat);
		}
		if (scv->next->next) {
			/* ssvp continues */
			s1 = scv->next;
			scv->next = NULL;
		} else {
			/* ssvp stops here */
			nr_vertex_free_one (scv->next);
			scv->next = NULL;
		}
	}
	/* invarionat: scv->next == NULL */
	/* Initial segment */
	if (scv->y < y0) {
		/* easy case - add new segment */
		s0 = nr_vertex_new_xy (x0, y0);
		scv->next = s0;
		s0 = ssvp->vertex;
		ssvp->vertex = NULL;
	} else {
		g_assert (scv->y == y0);
		/* Bitch */
		if (scv->x != x0) {
			NRFlat * flat;
			/* We create flat */
			flat = nr_flat_new_full (y0, MIN (x0, scv->x), MAX (x0, scv->x));
			*flats = nr_flat_insert_sorted (*flats, flat);
		}
		if (ssv) {
			/* Initial exists */
			s0 = ssvp->vertex;
			ssvp->vertex = NULL;
		} else {
			/* No initial */
			nr_vertex_free_one (scv);
			ssvp->vertex = NULL;
		}
	}

	/* Split svp */
	/* Continuation segment */
	if (cv->next->y > y1) {
		/* easy case - add new segment */
		c1 = nr_vertex_new_xy (x1, y1);
		c1->next = cv->next;
		cv->next = NULL;
	} else {
		g_assert (cv->next->y == y1);
		if (cv->next->x != x1) {
			NRFlat * flat;
			/* We create flat */
			flat = nr_flat_new_full (y1, MIN (x1, cv->next->x), MAX (x1, cv->next->x));
			*flats = nr_flat_insert_sorted (*flats, flat);
		}
		if (cv->next->next) {
			/* svp continues */
			c1 = cv->next;
			cv->next = NULL;
		} else {
			/* svp stops here */
			nr_vertex_free_one (cv->next);
			cv->next = NULL;
		}
	}
	/* cv->next == NULL */
	/* Initial segment */
	if (cv->y < y0) {
		/* easy case - add new segment */
		c0 = nr_vertex_new_xy (x0, y0);
		cv->next = c0;
		c0 = svp->vertex;
		svp->vertex = NULL;
	} else {
		g_assert (cv->y == y0);
		/* Bitch */
		if (cv->x != x0) {
			NRFlat * flat;
			/* We create flat */
			flat = nr_flat_new_full (y0, MIN (x0, cv->x), MAX (x0, cv->x));
			*flats = nr_flat_insert_sorted (*flats, flat);
		}
		if (sv) {
			/* Initial exists */
			c0 = svp->vertex;
			svp->vertex = NULL;
		} else {
			/* No initial */
			nr_vertex_free_one (cv);
			svp->vertex = NULL;
		}
	}

	/* Create common segment (or flat) */
	if (y0 < y1) {
		/* Good - we can create segment */
		common = nr_vertex_new_xy (x0, y0);
		common->next = nr_vertex_new_xy (x1, y1);
	} else {
		if (x0 != x1) {
			NRFlat * flat;
			/* We create flat */
			flat = nr_flat_new_full (y0, MIN (x0, x1), MAX (x0, x1));
			*flats = nr_flat_insert_sorted (*flats, flat);
		}
	}

	/* Process */
	if (c1) {
		NRSVP * n;
		n = nr_svp_new_vertex_wind (c1, wind1);
		svp->next = nr_svp_insert_sorted (svp->next, n);
	}
	if (c0) {
		svp->vertex = c0;
		nr_svp_calculate_bbox (svp);
	} else {
		ssvp->next = nr_svp_remove (ssvp->next, svp);
		nr_svp_free_one (svp);
	}

	if (s0) {
		ssvp->vertex = s0;
		nr_svp_calculate_bbox (ssvp);
	} else if (common) {
		ssvp->vertex = common;
		ssvp->wind = wind0 + wind1;
		nr_svp_calculate_bbox (ssvp);
		common = NULL;
	} else {
		g_assert (s1 != NULL);
		/* fixme: this is probably unsorted */
		ssvp->vertex = s1;
		nr_svp_calculate_bbox (ssvp);
		s1 = NULL;
	}

	if (common) {
		NRSVP * n;
		n = nr_svp_new_vertex_wind (common, wind0 + wind1);
		ssvp->next = nr_svp_insert_sorted (ssvp->next, n);
	}
	if (s1) {
		NRSVP * n;
		n = nr_svp_new_vertex_wind (s1, wind0);
		ssvp->next = nr_svp_insert_sorted (ssvp->next, n);
	}

	return TRUE;
}

/* fixme: optimize */
/* fixme: strict numerical analysis */

static gboolean
nr_svp_uncross_crossing (NRSVP * ssvp, NRVertex * ssv, NRVertex * scv, NRSVP * svp, NRVertex * sv, NRVertex * cv, NRFlat ** flats)
{
	NRSVP * n;
	NRVertex * s0, * s1, * c0, * c1;
	NRCoord x, y;
	float xba, xdc, yba, ydc;
	float xac, yac, numr, nums;
	float r, s;
	float d;

	xba = scv->next->x - scv->x;
	xdc = cv->next->x - cv->x;
	yba = scv->next->y - scv->y;
	ydc = cv->next->y - cv->y;
	d = xba * ydc - yba * xdc;

	if ((d > -NR_EPSILON) || (d < NR_EPSILON)) {
		/* Parallel */
		return FALSE;
	}

	s0 = s1 = c0 = c1 = NULL;

	/* Not parallel */
	xac = scv->x - cv->x;
	yac = scv->y - cv->y;
	numr = yac * xdc - xac * ydc;
	nums = yac * xba - xac * yba;
	r = numr / d;
	s = nums / d;

	/* fixme: this is suboptimal, as we potentially split every line twice */

	if ((r < -NR_EPSILON) || (r > 1.0 + NR_EPSILON) || (s < -NR_EPSILON) || (s > 1.0 + NR_EPSILON)) return FALSE;

	y = NR_COORD_SNAP (scv->y + r * yba);
	x = NR_COORD_SNAP (scv->x + r * xba);

	/* Draw scv -> (x, y) */

	if (y < scv->next->y) {
		/* We have real line */
		s1 = nr_vertex_new_xy (x, y);
		s1->next = scv->next;
		scv->next = NULL;
	} else {
	}

	return TRUE;
}

#if 0
	g_assert (cv);

	/* Here we are */

	NRLine * l;
	NRCoord xmin, xmax;

	xmin = MIN (line->s.x, line->e.x);
	xmax = MAX (line->s.x, line->e.x);

	for (l = line->next; (l) && (l->s.y < line->e.y); l = l->next) {
		if (((l->s.x <= l->e.x) && (l->s.x <= xmax) && (l->e.x >= xmin)) ||
		    ((l->s.x >= l->e.x) && (l->e.x <= xmax) && (l->s.x >= xmin))) {
			float xba, xdc, yba, ydc;
			float d;
			/* Binding boxes intersect */
			xba = line->e.x - line->s.x;
			xdc = l->e.x - l->s.x;
			yba = line->e.y - line->s.y;
			ydc = l->e.y - l->s.y;
			d = xba * ydc - yba * xdc;
			if (d != 0.0) {
				float xac, yac;
				float numr, nums;
				float r, s;
				/* Not parallel */
				xac = line->s.x - l->s.x;
				yac = line->s.y - l->s.y;
				numr = yac * xdc - xac * ydc;
				nums = yac * xba - xac * yba;
				r = numr / d;
				s = nums / d;
				if ((r > 0.0) && (r < 1.0) && (s > 0.0) && (s < 1.0)) {
					gint32 x, y;
					y = NR_COORD_SNAP (line->s.y + r * yba);
					if ((y > line->s.y) && (y < line->e.y) && (y > l->s.y) && (y < l->e.y)) {
						NRLine * new;
						x = NR_COORD_SNAP (line->s.x + r * xba);
						new = nr_line_new_xyxyd (x, y, line->e.x, line->e.y, line->direction);
						line->e.x = x;
						line->e.y = y;
						line->next = nr_lines_insert_sorted (line->next, new);
						new = nr_line_new_xyxyd (x, y, l->e.x, l->e.y, l->direction);
						l->e.x = x;
						l->e.y = y;
						l->next = nr_lines_insert_sorted (l->next, new);
						return line;
					}
				}
			} else {
				/* fixme: Test colinear case */
			}
		}
	}

	return line->next;
}
#endif

NRSVP *
nr_svp_rewind (NRSVP * svp)
{
	NRSVP * new;
	NRLine * newlines, * l, * nl;
	NRWindSpan * spans, * freespans;
	NRWindSpan * ps, * s, * ns;
	gint wind;

	if (!svp) return NULL;

	newlines = NULL;
	spans = freespans = s = NULL;

	for (l = svp->lines; l != NULL; l = l->next) {
		/* Determine left-side wind */
		/* fixme: optimize */
		wind = 0;
		ps = NULL;
		s = spans;
		while (s) {
			if (s->line->e.y <= l->s.y) {
				NRWindSpan * t;
				/* Span is exhausted */
				if (ps) {
					ps->next = s->next;
				} else {
					spans = s->next;
				}
				t = s;
				s = s->next;
				t->next = freespans;
				freespans = t;
			} else {
				/* Still working span */
				/* Determine span x order */
				if (nr_x_order_simple (l, s->line) >= 0) {
					/* We are at left */
					wind += s->line->direction;
				} else {
					/* We are at right */
					break;
				}
				ps = s;
				s = s->next;
			}
		}
		wind += l->direction;
		/* fixme: winding rules */
		nl = nr_line_new_xyxyd (l->s.x, l->s.y, l->e.x, l->e.y, (wind & 0x1) ? -1 : 1);
		nl->next = newlines;
		newlines = nl;
		/* Insert into span list */
		ns = nr_wind_span_new (l);
		ns->next = s;
		if (ps) {
			ps->next = ns;
		} else {
			spans = ns;
		}
	}

	nr_wind_span_free_list (spans);

	new = nr_svp_new ();
	new->lines = nr_lines_reverse_list (newlines);

	return new;
}

/*
 * Determinate whether segments are coincident
 */

static gint
nr_x_order_simple (NRVertex * v0, NRVertex * v1)
{
	float xx0, yy0, x1x0, y1y0;
	float d;

	xx0 = v0->x - v1->x;
	yy0 = v0->y - v1->y;
	if ((xx0 == 0.0) && (yy0 == 0.0)) {
		xx0 = v0->next->x - v1->x;
		yy0 = v0->next->y - v1->y;
		/* Simple case - starting points are coincident */
		x1x0 = v1->next->x - v1->x;
		y1y0 = v1->next->y - v1->y;
		d = xx0 * y1y0 - yy0 * x1x0;

		if (d < -NR_EPSILON) return -1;
		if (d > NR_EPSILON) return 1;
		return 0;
	}
	x1x0 = v1->next->x - v1->x;
	y1y0 = v1->next->y - v1->y;
	d = xx0 * y1y0 - yy0 * x1x0;

	if (d < -NR_EPSILON) return -1;
	if (d > NR_EPSILON) return 1;

	/* v0 start lies on v1 */
	/* test v0 end */
	xx0 = v0->next->x - v1->x;
	yy0 = v0->next->y - v1->y;
	x1x0 = v1->next->x - v1->x;
	y1y0 = v1->next->y - v1->y;
	d = xx0 * y1y0 - yy0 * x1x0;

	if (d < -NR_EPSILON) return -1;
	if (d > NR_EPSILON) return 1;
	return 0;
}
#endif

/*
 * Flats
 */

void
nr_svp_split_flat_list (NRSVP * svp, NRFlat * flat)
{
	NRFlat * cflat;
	NRSVPSlice * slices;
	NRSVP * ssvp, * csvp;

	g_assert (svp);
	g_assert (flat);

	/* Construct initial slice list */
	slices = NULL;
	ssvp = NULL;
	csvp = svp;
	cflat = flat;
	while ((cflat) && (!slices)) {
		while ((csvp) && (csvp->bbox.y0 <= flat->y)) {
			if (csvp->bbox.y1 > flat->y) {
				NRSVPSlice * new;
				/* fixme: Insert sorted for beautification */
				new = nr_svp_slice_new (csvp, flat->y);
				new->next = slices;
#if 0
				if (slices) slices->prev = new;
#endif
				slices = new;
			}
			ssvp = csvp;
			csvp = csvp->next;
		}
		if (!slices) cflat = cflat->next;
	}

	if (!slices) return;
	g_assert (ssvp);

	while (cflat) {
		NRSVPSlice * s;
		/* Test intersection */
		for (s = slices; s != NULL; s = s->next) {
			g_assert (s->vertex->y <= cflat->y);
			g_assert (s->vertex->next->y > cflat->y);
			/* fixme: We should differentiate between asc & desc lines */
			if ((s->x >= cflat->x0) && (s->x <= cflat->x1) && (s->vertex->y < cflat->y)) {
				NRVertex * newvertex;
				NRSVP * newsvp;
				/* Do intersection */
				/* Create new SVP segment */
				newvertex = nr_vertex_new_xy (s->x, s->y);
				newvertex->next = s->vertex->next;
				newsvp = nr_svp_new_vertex_wind (newvertex, s->svp->wind);
				/* Trim existing svp */
				newvertex = nr_vertex_new_xy (s->x, s->y);
				s->vertex->next = newvertex;
				nr_svp_calculate_bbox (s->svp);
				/* Insert new SVP into main list */
				/* new svp slice is included by definition */
				svp = nr_svp_insert_sorted (svp, newsvp);
				/* If we are inserted directly after ssvp, advance counter */
				if (ssvp->next == svp) {
					ssvp = svp;
					csvp = svp->next;
				}
				/* Old SVP will be excluded by definition, so we can shortcut */
				s->svp = newsvp;
				s->vertex = newsvp->vertex;
				/* s->x and s->y are correct by definition */
			}
		}

		/* Here we are - advance flat */
		cflat = cflat->next;
		/* Test, whether we are at the same level */
		if ((cflat) && ((!slices) || (cflat->y != slices->y))) {
			g_assert ((!slices) || (cflat->y > slices->y));
			/* stretch slices */
			slices = nr_svp_slice_stretch_list (slices, cflat->y);
			g_assert ((!slices) || (cflat->y == slices->y));
			/* Add new segments to slices */
			while ((csvp) && (csvp->bbox.y0 <= cflat->y)) {
				if (csvp->bbox.y1 > cflat->y) {
					NRSVPSlice * new;
					/* fixme: Insert sorted for beautification */
					new = nr_svp_slice_new (csvp, cflat->y);
					new->next = slices;
#if 0
					if (slices) slices->prev = new;
#endif
					slices = new;
				}
				ssvp = csvp;
				csvp = csvp->next;
			}
		}
	}

	if (slices) nr_svp_slice_free_list (slices);
}

/*
 * Memory management stuff follows (remember goals?)
 */

/* Slices */

#define NR_SLICE_ALLOC_SIZE 32
static NRSVPSlice * ffslice = NULL;

NRSVPSlice *
nr_svp_slice_new (NRSVP * svp, NRCoord y)
{
	NRSVPSlice * s;
	NRVertex * v;

	g_assert (svp);
	g_assert (svp->vertex);
	/* fixme: We try snapped slices - not sure, whether correct */
	g_assert (y == NR_COORD_SNAP (y));
	/* Slices startpoints are included, endpoints excluded */
	g_return_val_if_fail (y >= svp->bbox.y0, NULL);
	g_return_val_if_fail (y < svp->bbox.y1, NULL);

	s = ffslice;

	if (s == NULL) {
		gint i;
		s = g_new (NRSVPSlice, NR_SLICE_ALLOC_SIZE);
		for (i = 1; i < (NR_SLICE_ALLOC_SIZE - 1); i++) s[i].next = &s[i + 1];
		s[NR_SLICE_ALLOC_SIZE - 1].next = NULL;
		ffslice = s + 1;
	} else {
		ffslice = s->next;
	}

#if 0
	s->prev = NULL;
#endif
	s->next = NULL;
	s->svp = svp;

	v = svp->vertex;
	while ((v->next) && (v->next->y <= y)) v = v->next;
	g_assert (v->next);

	s->vertex = v;
	if (v->y == y) {
		s->x = v->x;
	} else {
		s->x = NR_COORD_SNAP (v->x + (v->next->x - v->x) * (y - v->y) / (v->next->y - v->y));
	}
	s->y = y;

	return s;
}

void
nr_svp_slice_free_one (NRSVPSlice * slice)
{
	slice->next = ffslice;
#if 0
	if (ffslice) ffslice->prev = slice;
#endif
	ffslice = slice;
#if 0
	slice->prev = NULL;
#endif
}

void
nr_svp_slice_free_list (NRSVPSlice * slice)
{
	NRSVPSlice * l;

	if (!slice) return;

	for (l = slice; l->next != NULL; l = l->next);

	l->next = ffslice;
#if 0
	if (ffslice) ffslice->prev = l;
#endif
	ffslice = slice;
#if 0
	slice->prev = NULL;
#endif
}

NRSVPSlice *
nr_svp_slice_insert_sorted (NRSVPSlice * start, NRSVPSlice * slice)
{
	NRSVPSlice * s, * l;

	if (!start) return slice;
	if (!slice) return start;

	if (nr_svp_slice_compare (slice, start) <= 0) {
		slice->next = start;
		return slice;
	}

	s = start;
	for (l = start->next; l != NULL; l = l->next) {
		if (nr_svp_slice_compare (slice, l) <= 0) {
			slice->next = l;
			s->next = slice;
			return start;
		}
		s = l;
	}

	slice->next = NULL;
	s->next = slice;

	return start;
}

NRSVPSlice *
nr_svp_slice_stretch_list (NRSVPSlice * slices, NRCoord y)
{
	NRSVPSlice * p, * s;

	/* fixme: We try snapped slices - not sure, whether correct */
	g_assert (y == NR_COORD_SNAP (y));

	p = NULL;
	s = slices;

	while (s) {
		if (s->svp->bbox.y1 <= y) {
			/* Remove exhausted slice */
			if (p) {
				p->next = s->next;
				nr_svp_slice_free_one (s);
				s = p->next;
			} else {
				slices = s->next;
				nr_svp_slice_free_one (s);
				s = slices;
			}
		} else {
			NRVertex * v;
			/* Stretch slice */
			v = s->vertex;
			while ((v->next) && (v->next->y <= y)) v = v->next;
			g_assert (v->next);

			s->vertex = v;
			if (v->y == y) {
				s->x = v->x;
			} else {
				s->x = NR_COORD_SNAP (v->x + (v->next->x - v->x) * (y - v->y) / (v->next->y - v->y));
			}
			s->y = y;
			s = s->next;
		}
	}

	return slices;
}

static gint
nr_svp_slice_compare (NRSVPSlice * l, NRSVPSlice * r)
{
	float xx0, yy0, x1x0, y1y0;
	float d;

	g_assert (l->y == r->y);
	g_assert (l->vertex->next->y > l->y);
	g_assert (r->vertex->next->y > r->y);

	if (l->x < r->x) return -1;
	if (l->x > r->x) return 1;

	/* Y is same, X is same, checking for line orders */

	xx0 = l->vertex->next->x - r->x;
	yy0 = l->vertex->next->y - r->y;
	x1x0 = r->vertex->next->x - r->x;
	y1y0 = r->vertex->next->y - r->y;

	d = xx0 * y1y0 - yy0 * x1x0;

	/* fixme: test almost zero cases */
	if (d < 0.0) return -1;
	if (d > 0.0) return 1;

	return 0;
}

/*
 * Old stuff
 */

#if 0

typedef struct _NRWindSpan NRWindSpan;

struct _NRWindSpan {
	NRWindSpan * next;
	NRLine * line;
};

static NRWindSpan * nr_wind_span_new (NRLine * line);
static void nr_wind_span_free (NRWindSpan * ws);
static void nr_wind_span_free_list (NRWindSpan * ws);

#define NR_WS_ALLOC_SIZE 32
static NRWindSpan * firstfreews = NULL;

static NRWindSpan *
nr_wind_span_new (NRLine * line)
{
	NRWindSpan * ws;

	ws = firstfreews;

	if (ws == NULL) {
		gint i;
		ws = g_new (NRWindSpan, NR_WS_ALLOC_SIZE);
		for (i = 1; i < (NR_WS_ALLOC_SIZE - 1); i++) (ws + i)->next = (ws + i + 1);
		(ws + NR_WS_ALLOC_SIZE - 1)->next = NULL;
		firstfreews = ws + 1;
	} else {
		firstfreews = ws->next;
	}

	ws->next = NULL;
	ws->line = line;

	return ws;
}

static void
nr_wind_span_free (NRWindSpan * ws)
{
	ws->next = firstfreews;
	firstfreews = ws;
}

static void
nr_wind_span_free_list (NRWindSpan * ws)
{
	NRWindSpan * l;

	if (!ws) return;

	for (l = ws; l->next != NULL; l = l->next);
	l->next = firstfreews;
	firstfreews = ws;
}

#endif


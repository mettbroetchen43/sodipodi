#define __NR_SVP_RENDER_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include "nr-macros.h"
#include "nr-pixops.h"
#include "nr-svp-render.h"

static void nr_svp_render (NRSVP *svp, unsigned char *px, unsigned int bpp, unsigned int rs, int x0, int y0, int x1, int y1,
			   void (* run) (unsigned char *px, int len, int c0_24, int s0_24, void *data), void *data);

static void nr_svl_render (NRSVL *svl, unsigned char *px, unsigned int bpp, unsigned int rs, int x0, int y0, int x1, int y1,
			   void (* run) (unsigned char *px, int len, int c0_24, int s0_24, void *data), void *data);

static void nr_svp_run_A8_OR (unsigned char *d, int len, int c0_24, int s0_24, void *data);
static void nr_svp_run_R8G8B8 (unsigned char *d, int len, int c0_24, int s0_24, void *data);
static void nr_svp_run_R8G8B8A8P_EMPTY (unsigned char *d, int len, int c0_24, int s0_24, void *data);
static void nr_svp_run_R8G8B8A8P_R8G8B8A8P (unsigned char *d, int len, int c0_24, int s0_24, void *data);
static void nr_svp_run_R8G8B8A8N_R8G8B8A8N (unsigned char *d, int len, int c0_24, int s0_24, void *data);

/* Renders graymask of svl into buffer */

void
nr_pixblock_render_svp_mask_or (NRPixBlock *d, NRSVP *svp)
{
	nr_svp_render (svp, NR_PIXBLOCK_PX (d), 1, d->rs,
		       d->area.x0, d->area.y0, d->area.x1, d->area.y1,
		       nr_svp_run_A8_OR, NULL);
}

/* Renders graymask of svl into buffer */

void
nr_pixblock_render_svl_mask_or (NRPixBlock *d, NRSVL *svl)
{
	nr_svl_render (svl, NR_PIXBLOCK_PX (d), 1, d->rs,
		       d->area.x0, d->area.y0, d->area.x1, d->area.y1,
		       nr_svp_run_A8_OR, NULL);
}

/* Renders colored SVL into buffer (has to be RGB/RGBA) */

void
nr_pixblock_render_svl_rgba (NRPixBlock *dpb, NRSVL *svl, NRULong rgba)
{
	unsigned char c[4];

	c[0] = NR_RGBA32_R (rgba);
	c[1] = NR_RGBA32_G (rgba);
	c[2] = NR_RGBA32_B (rgba);
	c[3] = NR_RGBA32_A (rgba);

	switch (dpb->mode) {
	case NR_PIXBLOCK_MODE_R8G8B8:
		nr_svl_render (svl, NR_PIXBLOCK_PX (dpb), 3, dpb->rs,
			       dpb->area.x0, dpb->area.y0, dpb->area.x1, dpb->area.y1,
			       nr_svp_run_R8G8B8, c);
		break;
	case NR_PIXBLOCK_MODE_R8G8B8A8P:
		if (dpb->empty) {
			nr_svl_render (svl, NR_PIXBLOCK_PX (dpb), 4, dpb->rs,
				       dpb->area.x0, dpb->area.y0, dpb->area.x1, dpb->area.y1,
				       nr_svp_run_R8G8B8A8P_EMPTY, c);
		} else {
			nr_svl_render (svl, NR_PIXBLOCK_PX (dpb), 4, dpb->rs,
				       dpb->area.x0, dpb->area.y0, dpb->area.x1, dpb->area.y1,
				       nr_svp_run_R8G8B8A8P_R8G8B8A8P, c);
		}
		break;
	case NR_PIXBLOCK_MODE_R8G8B8A8N:
		nr_svl_render (svl, NR_PIXBLOCK_PX (dpb), 4, dpb->rs,
			       dpb->area.x0, dpb->area.y0, dpb->area.x1, dpb->area.y1,
			       nr_svp_run_R8G8B8A8N_R8G8B8A8N, c);
		break;
	default:
		break;
	}
}

static void
nr_svp_run_A8_OR (unsigned char *d, int len, int c0_24, int s0_24, void *data)
{
	if ((c0_24 >= 0xff0000) && (s0_24 == 0x0)) {
		/* Simple copy */
		while (len > 0) {
			d[0] = 255;
			d += 1;
			len -= 1;
		}
	} else {
		while (len > 0) {
			unsigned int ca, da;
			/* Draw */
			ca = c0_24 >> 16;
			da = 65025 - (255 - ca) * (255 - d[0]);
			d[0] = (da + 127) / 255;
			d += 1;
			c0_24 += s0_24;
			c0_24 = CLAMP (c0_24, 0, 16777216);
			len -= 1;
		}
	}
}

static void
nr_svp_run_R8G8B8 (unsigned char *d, int len, int c0_24, int s0_24, void *data)
{
	unsigned char *c;

	c = (unsigned char *) data;

	if ((c0_24 >= 0xff0000) && (c[3] == 0xff) && (s0_24 == 0x0)) {
		/* Simple copy */
		while (len > 0) {
			d[0] = c[0];
			d[1] = c[1];
			d[2] = c[2];
			d += 3;
			len -= 1;
		}
	} else {
		while (len > 0) {
			unsigned int ca;
			/* Draw */
			ca = c0_24 >> 16;
			ca = NR_PREMUL (ca, c[3]);
			if (ca == 0) {
				/* Transparent FG, NOP */
			} else if (ca == 255) {
				/* Full coverage, COPY */
				d[0] = c[0];
				d[1] = c[1];
				d[2] = c[2];
			} else {
				d[0] = NR_COMPOSEN11 (c[0], ca, d[0]);
				d[1] = NR_COMPOSEN11 (c[1], ca, d[1]);
				d[2] = NR_COMPOSEN11 (c[2], ca, d[2]);
			}
			d += 3;
			c0_24 += s0_24;
			/* c24 = CLAMP (c24, 0, 16777216); */
			len -= 1;
		}
	}
}

static void
nr_svp_run_R8G8B8A8P_EMPTY (unsigned char *d, int len, int c0_24, int s0_24, void *data)
{
	unsigned char *c;

	c = (unsigned char *) data;

	if ((c0_24 >= 0xff0000) && (s0_24 == 0x0)) {
		unsigned char r, g, b;
		/* Simple copy */
		r = NR_PREMUL (c[0], c[3]);
		g = NR_PREMUL (c[1], c[3]);
		b = NR_PREMUL (c[2], c[3]);
		while (len > 0) {
			d[0] = r;
			d[1] = g;
			d[2] = b;
			d[3] = c[3];
			d += 4;
			len -= 1;
		}
	} else {
		while (len > 0) {
			unsigned int ca;
			/* Draw */
			ca = c0_24 >> 16;
			ca = NR_PREMUL (ca, c[3]);
			if (ca == 0) {
				/* Transparent FG, NOP */
			} else {
				/* Full coverage, COPY */
				d[0] = NR_PREMUL (c[0], ca);
				d[1] = NR_PREMUL (c[1], ca);
				d[2] = NR_PREMUL (c[2], ca);
				d[3] = ca;
			}
			d += 4;
			len -= 1;
		}
	}
}

static void
nr_svp_run_R8G8B8A8P_R8G8B8A8P (unsigned char *d, int len, int c0_24, int s0_24, void *data)
{
	unsigned char *c;

	c = (unsigned char *) data;

	if ((c0_24 >= 0xff0000) && (c[3] == 0xff) && (s0_24 == 0x0)) {
		/* Simple copy */
		while (len > 0) {
			d[0] = c[0];
			d[1] = c[1];
			d[2] = c[2];
			d[3] = c[3];
			d += 4;
			len -= 1;
		}
	} else {
		while (len > 0) {
			unsigned int ca;
			/* Draw */
			ca = c0_24 >> 16;
			ca = NR_PREMUL (ca, c[3]);
			if (ca == 0) {
				/* Transparent FG, NOP */
			} else if ((ca == 255) || (d[3] == 0)) {
				/* Full coverage, COPY */
				d[0] = NR_PREMUL (c[0], ca);
				d[1] = NR_PREMUL (c[1], ca);
				d[2] = NR_PREMUL (c[2], ca);
				d[3] = ca;
			} else {
				/* Full composition */
				d[0] = NR_COMPOSENPP (c[0], ca, d[0], d[3]);
				d[1] = NR_COMPOSENPP (c[1], ca, d[1], d[3]);
				d[2] = NR_COMPOSENPP (c[2], ca, d[2], d[3]);
				d[3] = (65025 - (255 - ca) * (255 - d[3]) + 127) / 255;
			}
			d += 4;
			c0_24 += s0_24;
			/* c24 = CLAMP (c24, 0, 16777216); */
			len -= 1;
		}
	}
}

static void
nr_svp_run_R8G8B8A8N_R8G8B8A8N (unsigned char *d, int len, int c0_24, int s0_24, void *data)
{
	unsigned char *c;

	c = (unsigned char *) data;

	if ((c0_24 >= 0xff0000) && (s0_24 == 0x0)) {
		/* Simple copy */
		while (len > 0) {
			d[0] = c[0];
			d[1] = c[1];
			d[2] = c[2];
			d[3] = c[3];
			d += 4;
			len -= 1;
		}
	} else {
		while (len > 0) {
			unsigned int ca;
			/* Draw */
			ca = c0_24 >> 16;
			ca = NR_PREMUL (ca, c[3]);
			if (ca == 0) {
				/* Transparent FG, NOP */
			} else if ((ca == 255) || (d[3] == 0)) {
				/* Full coverage, COPY */
				d[0] = c[0];
				d[1] = c[1];
				d[2] = c[2];
				d[3] = ca;
			} else {
				unsigned int da;
				/* Full composition */
				da = 65025 - (255 - ca) * (255 - d[3]);
				d[0] = NR_COMPOSENNN_A7 (c[0], ca, d[0], d[3], da);
				d[1] = NR_COMPOSENNN_A7 (c[1], ca, d[1], d[3], da);
				d[2] = NR_COMPOSENNN_A7 (c[2], ca, d[2], d[3], da);
				d[3] = (da + 127) / 255;
			}
			d += 4;
			c0_24 += s0_24;
			/* c24 = CLAMP (c24, 0, 16777216); */
			len -= 1;
		}
	}
}

typedef struct _NRRun NRRun;

struct _NRRun {
	NRRun *next;
	double x0, y0, x1, y1;
	double step;
	double final;
	double x;
	double value;
};

static NRRun *nr_run_new (NRCoord x0, NRCoord y0, NRCoord x1, NRCoord y1, int wind);
static NRRun *nr_run_free_one (NRRun *run);
static void nr_run_free_list (NRRun *run);
static NRRun *nr_run_insert_sorted (NRRun *start, NRRun *run);

typedef struct _NRSlice NRSlice;

struct _NRSlice {
	NRSlice *next;
	int wind;
	NRPointF *points;
	unsigned int current;
	unsigned int last;
	double x;
	double y;
	double stepx;
};

static NRSlice *nr_slice_new (int wind, NRPointF *points, unsigned int length, NRCoord y);
static NRSlice *nr_slice_free_one (NRSlice *s);
static void nr_slice_free_list (NRSlice *s);
static NRSlice *nr_slice_insert_sorted (NRSlice *start, NRSlice *slice);
static int nr_slice_compare (NRSlice *l, NRSlice *r);

static void
nr_svp_render (NRSVP *svp, unsigned char *px, unsigned int bpp, unsigned int rs, int x0, int y0, int x1, int y1,
	       void (* run) (unsigned char *px, int len, int c0_24, int s0_24, void *data), void *data)
{
	NRSlice *slices;
	int sidx;
	int ystart;
	unsigned char *rowbuffer;
	int y;

	if (!svp || !svp->length) return;

	/* Find starting pixel row */
	/* g_assert (svl->bbox.y0 == svl->vertex->y); */
	sidx = 0;
	while (!svp->segments[sidx].length && (sidx < svp->length)) sidx += 1;
	if (sidx >= svp->length) return;
	ystart = (int) NR_SVPSEG_Y0 (svp, sidx);
	if (ystart >= y1) return;
	if (ystart > y0) {
		px += (ystart - y0) * rs;
		y0 = ystart;
	}

	/* Construct initial slice list */
	slices = NULL;
	while ((sidx < svp->length) && (NR_SVPSEG_Y0 (svp, sidx) <= y0)) {
		NRSVPSegment *seg;
		seg = svp->segments + sidx;
		/* g_assert (nsvl->bbox.y0 == nsvl->vertex->y); */
		if (seg->wind && (NR_SVPSEG_Y1 (svp, sidx) > y0)) {
			NRSlice *newslice;
			newslice = nr_slice_new (seg->wind, svp->points + seg->start, seg->length, y0);
			slices = nr_slice_insert_sorted (slices, newslice);
		}
		sidx += 1;
	}
	if (!slices && (sidx >= svp->length)) return;

	/* Row buffer */
	/* fixme: not needed */
	rowbuffer = px;

	/* Main iteration */
	for (y = y0; y < y1; y++) {
		NRSlice *ss, *cs;
		NRRun *runs;
		int xstart;
		float globalval;
		unsigned char *d;
		int x;

		/* Add possible new svls to slice list */
		while ((sidx < svp->length) && (NR_SVPSEG_Y0 (svp, sidx) < (y + 1))) {
			NRSVPSegment *seg;
			seg = svp->segments + sidx;
			if (seg->wind) {
				NRSlice *newslice;
				/* fixme: we should use safely nsvl->vertex->y here */
				newslice = nr_slice_new (seg->wind, svp->points + seg->start, seg->length, MAX (y, NR_SVPSEG_Y0 (svp, sidx)));
				slices = nr_slice_insert_sorted (slices, newslice);
			}
			sidx += 1;
		}
		/* Construct runs, stretching slices */
		/* fixme: This step can be optimized by continuing long runs and adding only new ones (Lauris) */
		runs = NULL;
		ss = NULL;
		cs = slices;
		while (cs) {
			/* g_assert (cs->y >= y0); */
			/* g_assert (cs->y < (y + 1)); */
			while ((cs->y < (y + 1)) && (cs->current < cs->last)) {
				NRCoord x0, y0, x1, y1;
				NRRun * newrun;
				x0 = cs->x;
				y0 = cs->y;
				if (cs->points[cs->current + 1].y > (y + 1)) {
					/* The same slice continues */
					x1 = x0 + ((y + 1) - y0) * cs->stepx;
					y1 = y + 1;
					cs->x = x1;
					cs->y = y1;
				} else {
					cs->current += 1;
					x1 = cs->points[cs->current].x;
					y1 = cs->points[cs->current].y;
					cs->x = x1;
					cs->y = y1;
					if (cs->current < cs->last) {
						cs->stepx = (cs->points[cs->current + 1].x - x1) / (cs->points[cs->current + 1].y - y1);
					}
				}
				newrun = nr_run_new (x0, y0, x1, y1, cs->wind);
				/* fixme: we should use walking forward/backward instead */
				runs = nr_run_insert_sorted (runs, newrun);
			}
			if (cs->current < cs->last) {
				ss = cs;
				cs = cs->next;
			} else {
				/* Slice is exhausted */
				cs = nr_slice_free_one (cs);
				if (ss) {
					ss->next = cs;
				} else {
					slices = cs;
				}
			}
		}
		/* Slices are expanded to next scanline */
		/* Run list is generated */
		/* find initial value */
		globalval = 0.0;
		if ((runs) && (x0 < runs->x0)) {
			/* First run starts right from x0 */
			xstart = (int) runs->x0;
		} else {
			NRRun *sr, *cr;
			/* First run starts left from x0 */
			xstart = x0;
			sr = NULL;
			cr = runs;
			while ((cr) && (cr->x0 < x0)) {
				if (cr->x1 <= x0) {
					globalval += cr->final;
					/* Remove exhausted current run */
					cr = nr_run_free_one (cr);
					if (sr) {
						sr->next = cr;
					} else {
						runs = cr;
					}
				} else {
					cr->x = x0;
					cr->value = (x0 - cr->x0) * cr->step;
					sr = cr;
					cr = cr->next;
				}
			}
		}

		/* Running buffer */
		d = rowbuffer + bpp * (xstart - x0);

		for (x = xstart; (runs) && (x < x1); x++) {
			NRRun *sr, *cr;
			float xnext;
			float localval;
			unsigned int fill;
			float fillstep;
			int xstop;
			int c24;

			xnext = x + 1.0;
			/* process runs */
			/* fixme: generate fills */
			localval = globalval;
			sr = NULL;
			cr = runs;
			fill = TRUE;
			fillstep = 0.0;
			xstop = x1;
			while ((cr) && (cr->x0 < xnext)) {
				if (cr->x1 <= xnext) {
					/* Run ends here */
					/* No fill */
					fill = FALSE;
					/* Continue with final value */
					globalval += cr->final;
					/* Add initial trapezoid */
					localval += (cr->x1 - cr->x) * (cr->value + cr->final) / 2.0;
					/* Add final rectangle */
					localval += (xnext - cr->x1) * cr->final;
					/* Remove exhausted run */
					cr = nr_run_free_one (cr);
					if (sr) {
						sr->next = cr;
					} else {
						runs = cr;
					}
				} else {
					/* Run continues through xnext */
					if (fill) {
						if (cr->x0 > x) {
							fill = FALSE;
						} else {
							xstop = MIN (xstop, (int) floor (cr->x1));
							fillstep += cr->step;
						}
					}
					localval += (xnext - cr->x) * (cr->value + (xnext - cr->x) * cr->step / 2.0);
					cr->x = xnext;
					cr->value = (xnext - cr->x0) * cr->step;
					sr = cr;
					cr = cr->next;
				}
			}
			if (fill) {
				if (cr) xstop = MIN (xstop, (int) floor (cr->x0));
			}
			localval = CLAMP (localval, 0.0, 1.0);
			c24 = (int) (16777215 * localval + 0.5);
			if (fill && (xstop > xnext)) {
				int s24;
				s24 = (int) (16777215 * fillstep + 0.5);
				if ((s24 != 0) || (c24 > 65535)) {
					run (d, xstop - x, c24, s24, data);
				}
				d += bpp * (xstop - x);
				x = xstop - 1;
			} else {
				run (d, 1, c24, 0, data);
				d += bpp;
			}
		}
		nr_run_free_list (runs);
		rowbuffer += rs;
	}
	if (slices) nr_slice_free_list (slices);
}

/* Slices */

#define NR_SLICE_ALLOC_SIZE 32
static NRSlice *ffslice = NULL;

static NRSlice *
nr_slice_new (int wind, NRPointF *points, unsigned int length, NRCoord y)
{
	NRSlice *s;
	NRPointF *p;

	/* g_assert (svl); */
	/* g_assert (svl->vertex); */
	/* fixme: not sure, whether correct */
	/* g_assert (y == NR_COORD_SNAP (y)); */
	/* Slices startpoints are included, endpoints excluded */
	/* g_return_val_if_fail (y >= svl->bbox.y0, NULL); */
	/* g_return_val_if_fail (y < svl->bbox.y1, NULL); */

	s = ffslice;

	if (s == NULL) {
		int i;
		s = nr_new (NRSlice, NR_SLICE_ALLOC_SIZE);
		for (i = 1; i < (NR_SLICE_ALLOC_SIZE - 1); i++) s[i].next = &s[i + 1];
		s[NR_SLICE_ALLOC_SIZE - 1].next = NULL;
		ffslice = s + 1;
	} else {
		ffslice = s->next;
	}

	s->next = NULL;
	s->wind = wind;
	s->points = points;
	s->current = 0;
	s->last = length - 1;

	while ((s->current < s->last) && (s->points[s->current + 1].y <= y)) s->current += 1;
	p = s->points + s->current;

	if (s->points[s->current].y == y) {
		s->x = p[0].x;
	} else {
		s->x = p[0].x + (p[1].x - p[0].x) * (y - p[0].y) / (p[1].y - p[0].y);
	}
	s->y = y;
	s->stepx = (p[1].x - p[0].x) / (p[1].y - p[0].y);

	return s;
}

static NRSlice *
nr_slice_free_one (NRSlice *slice)
{
	NRSlice *next;
	next = slice->next;
	slice->next = ffslice;
	ffslice = slice;
	return next;
}

static void
nr_slice_free_list (NRSlice *slice)
{
	NRSlice *l;

	if (!slice) return;

	for (l = slice; l->next != NULL; l = l->next);

	l->next = ffslice;
	ffslice = slice;
}

static NRSlice *
nr_slice_insert_sorted (NRSlice * start, NRSlice * slice)
{
	NRSlice * s, * l;

	if (!start) return slice;
	if (!slice) return start;

	if (nr_slice_compare (slice, start) <= 0) {
		slice->next = start;
		return slice;
	}

	s = start;
	for (l = start->next; l != NULL; l = l->next) {
		if (nr_slice_compare (slice, l) <= 0) {
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

static int
nr_slice_compare (NRSlice *l, NRSlice *r)
{
	if (l->y == r->y) {
		if (l->x < r->x) return -1;
		if (l->x > r->x) return 1;
		if (l->stepx < r->stepx) return -1;
		if (l->stepx > r->stepx) return 1;
	} else if (l->y > r->y) {
		int pidx;
		NRPointF *p;
		double x, ldx, rdx;
		/* This is bitch - we have to determine r values at l->y */
		pidx = 0;
		while ((pidx < r->last) && (r->points[pidx + 1].y <= l->y)) pidx += 1;
		/* If v is last vertex, r ends before l starts */
		if (pidx >= r->last) return 1;
		p = r->points + pidx;
		if (p[0].y == l->y) {
			x = p[0].x;
		} else {
			x = p[0].x + (p[1].x - p[0].x) * (l->y - p[0].y) / (p[1].y - p[0].y);
		}
		if (l->x < x) return -1;
		if (l->x > x) return 1;
		ldx = l->stepx * (p[1].y - p[0].y);
		rdx = p[1].x - p[0].x;
		if (ldx < rdx) return -1;
		if (ldx > rdx) return 1;
	} else {
		int pidx;
		NRPointF *p;
		double x, ldx, rdx;
		/* This is bitch - we have to determine l value at r->y */
		pidx = 0;
		while ((pidx < l->last) && (l->points[pidx + 1].y <= r->y)) pidx += 1;
		/* If v is last vertex, l ends before r starts */
		if (pidx >= l->last) return 1;
		p = l->points + pidx;
		if (p[0].y == r->y) {
			x = p[0].x;
		} else {
			x = p[0].x + (p[1].x - p[0].x) * (r->y - p[0].y) / (p[1].y - p[0].y);
		}
		if (x < r->x) return -1;
		if (x > r->x) return 1;
		ldx = l->stepx * (p[1].y - p[0].y);
		rdx = p[1].x - p[0].x;
		if (ldx < rdx) return -1;
		if (ldx > rdx) return 1;
	}
	return 0;
}

typedef struct _NRSliceL NRSliceL;

struct _NRSliceL {
	NRSliceL *next;
	NRSVL *svl;
	NRVertex *vertex;
	double x;
	double y;
	double stepx;
};

static NRSliceL *nr_slice_new_l (NRSVL *svl, NRCoord y);
static NRSliceL *nr_slice_free_one_l (NRSliceL *s);
static void nr_slice_free_list_l (NRSliceL *s);
static NRSliceL *nr_slice_insert_sorted_l (NRSliceL *start, NRSliceL *slice);
static int nr_slice_compare_l (NRSliceL *l, NRSliceL *r);

static void
nr_svl_render (NRSVL *svl, unsigned char *px, unsigned int bpp, unsigned int rs, int x0, int y0, int x1, int y1,
	       void (* run) (unsigned char *px, int len, int c0_24, int s0_24, void *data), void *data)
{
	NRSVL *nsvl;
	NRSliceL *slices;
	int ystart;
	unsigned char *rowbuffer;
	int y;

	if (!svl) return;

	/* Find starting pixel row */
	/* g_assert (svl->bbox.y0 == svl->vertex->y); */
	ystart = (int) svl->bbox.y0;
	if (ystart >= y1) return;
	if (ystart > y0) {
		px += (ystart - y0) * rs;
		y0 = ystart;
	}

	/* Construct initial slice list */
	slices = NULL;
	nsvl = svl;
	while ((nsvl) && (nsvl->bbox.y0 <= y0)) {
		/* g_assert (nsvl->bbox.y0 == nsvl->vertex->y); */
		if (nsvl->bbox.y1 > y0) {
			NRSliceL *newslice;
			newslice = nr_slice_new_l (nsvl, y0);
			slices = nr_slice_insert_sorted_l (slices, newslice);
		}
		nsvl = nsvl->next;
	}
	if ((!nsvl) && (!slices)) return;

	/* Row buffer */
	/* fixme: not needed */
	rowbuffer = px;

	/* Main iteration */
	for (y = y0; y < y1; y++) {
		NRSliceL * ss, * cs;
		NRRun * runs;
		int xstart;
		float globalval;
		unsigned char *d;
		int x;

		/* Add possible new svls to slice list */
		while ((nsvl) && (nsvl->bbox.y0 < (y + 1))) {
			NRSliceL * newslice;
			/* fixme: we should use safely nsvl->vertex->y here */
			newslice = nr_slice_new_l (nsvl, MAX (y, nsvl->bbox.y0));
			slices = nr_slice_insert_sorted_l (slices, newslice);
			nsvl = nsvl->next;
		}
		/* Construct runs, stretching slices */
		/* fixme: This step can be optimized by continuing long runs and adding only new ones (Lauris) */
		runs = NULL;
		ss = NULL;
		cs = slices;
		while (cs) {
			/* g_assert (cs->y >= y0); */
			/* g_assert (cs->y < (y + 1)); */
			while ((cs->y < (y + 1)) && (cs->vertex->next)) {
				NRCoord x0, y0, x1, y1;
				NRRun * newrun;
				x0 = cs->x;
				y0 = cs->y;
				if (cs->vertex->next->y > (y + 1)) {
					/* The same slice continues */
					x1 = x0 + ((y + 1) - y0) * cs->stepx;
					y1 = y + 1;
					cs->x = x1;
					cs->y = y1;
				} else {
					cs->vertex = cs->vertex->next;
					x1 = cs->vertex->x;
					y1 = cs->vertex->y;
					cs->x = x1;
					cs->y = y1;
					if (cs->vertex->next ) {
						cs->stepx = (cs->vertex->next->x - x1) / (cs->vertex->next->y - y1);
					}
				}
				newrun = nr_run_new (x0, y0, x1, y1, cs->svl->wind);
				/* fixme: we should use walking forward/backward instead */
				runs = nr_run_insert_sorted (runs, newrun);
			}
			if (cs->vertex->next) {
				ss = cs;
				cs = cs->next;
			} else {
				/* Slice is exhausted */
				cs = nr_slice_free_one_l (cs);
				if (ss) {
					ss->next = cs;
				} else {
					slices = cs;
				}
			}
		}
		/* Slices are expanded to next scanline */
		/* Run list is generated */
		/* find initial value */
		globalval = 0.0;
		if ((runs) && (x0 < runs->x0)) {
			/* First run starts right from x0 */
			xstart = (int) runs->x0;
		} else {
			NRRun * sr, * cr;
			/* First run starts left from x0 */
			xstart = x0;
			sr = NULL;
			cr = runs;
			while ((cr) && (cr->x0 < x0)) {
				if (cr->x1 <= x0) {
					globalval += cr->final;
					/* Remove exhausted current run */
					cr = nr_run_free_one (cr);
					if (sr) {
						sr->next = cr;
					} else {
						runs = cr;
					}
				} else {
					cr->x = x0;
					cr->value = (x0 - cr->x0) * cr->step;
					sr = cr;
					cr = cr->next;
				}
			}
		}

		/* Running buffer */
		d = rowbuffer + bpp * (xstart - x0);

		for (x = xstart; (runs) && (x < x1); x++) {
			NRRun *sr, *cr;
			float xnext;
			float localval;
			unsigned int fill;
			float fillstep;
			int xstop;
			int c24;

			xnext = x + 1.0;
			/* process runs */
			/* fixme: generate fills */
			localval = globalval;
			sr = NULL;
			cr = runs;
			fill = TRUE;
			fillstep = 0.0;
			xstop = x1;
			while ((cr) && (cr->x0 < xnext)) {
				if (cr->x1 <= xnext) {
					/* Run ends here */
					/* No fill */
					fill = FALSE;
					/* Continue with final value */
					globalval += cr->final;
					/* Add initial trapezoid */
					localval += (cr->x1 - cr->x) * (cr->value + cr->final) / 2.0;
					/* Add final rectangle */
					localval += (xnext - cr->x1) * cr->final;
					/* Remove exhausted run */
					cr = nr_run_free_one (cr);
					if (sr) {
						sr->next = cr;
					} else {
						runs = cr;
					}
				} else {
					/* Run continues through xnext */
					if (fill) {
						if (cr->x0 > x) {
							fill = FALSE;
						} else {
							xstop = MIN (xstop, (int) floor (cr->x1));
							fillstep += cr->step;
						}
					}
					localval += (xnext - cr->x) * (cr->value + (xnext - cr->x) * cr->step / 2.0);
					cr->x = xnext;
					cr->value = (xnext - cr->x0) * cr->step;
					sr = cr;
					cr = cr->next;
				}
			}
			if (fill) {
				if (cr) xstop = MIN (xstop, (int) floor (cr->x0));
			}
			localval = CLAMP (localval, 0.0, 1.0);
			c24 = (int) (16777215 * localval + 0.5);
			if (fill && (xstop > xnext)) {
				int s24;
				s24 = (int) (16777215 * fillstep + 0.5);
				if ((s24 != 0) || (c24 > 65535)) {
					run (d, xstop - x, c24, s24, data);
				}
				d += bpp * (xstop - x);
				x = xstop - 1;
			} else {
				run (d, 1, c24, 0, data);
				d += bpp;
			}
		}
		nr_run_free_list (runs);
		rowbuffer += rs;
	}
	if (slices) nr_slice_free_list_l (slices);
}

/*
 * Memory management stuff follows (remember goals?)
 */

/* Slices */

#define NR_SLICE_ALLOC_SIZE_L 32
static NRSliceL *ffslice_l = NULL;

static NRSliceL *
nr_slice_new_l (NRSVL * svl, NRCoord y)
{
	NRSliceL *s;
	NRVertex *v;

	/* g_assert (svl); */
	/* g_assert (svl->vertex); */
	/* fixme: not sure, whether correct */
	/* g_assert (y == NR_COORD_SNAP (y)); */
	/* Slices startpoints are included, endpoints excluded */
	/* g_return_val_if_fail (y >= svl->bbox.y0, NULL); */
	/* g_return_val_if_fail (y < svl->bbox.y1, NULL); */

	s = ffslice_l;

	if (s == NULL) {
		int i;
		s = nr_new (NRSliceL, NR_SLICE_ALLOC_SIZE_L);
		for (i = 1; i < (NR_SLICE_ALLOC_SIZE_L - 1); i++) s[i].next = &s[i + 1];
		s[NR_SLICE_ALLOC_SIZE_L - 1].next = NULL;
		ffslice_l = s + 1;
	} else {
		ffslice_l = s->next;
	}

	s->next = NULL;
	s->svl = svl;

	v = svl->vertex;
	while ((v->next) && (v->next->y <= y)) v = v->next;
	/* g_assert (v->next); */

	s->vertex = v;
	if (v->y == y) {
		s->x = v->x;
	} else {
		s->x = v->x + (v->next->x - v->x) * (y - v->y) / (v->next->y - v->y);
	}
	s->y = y;
	s->stepx = (s->vertex->next->x - s->vertex->x) / (s->vertex->next->y - s->vertex->y);

	return s;
}

static NRSliceL *
nr_slice_free_one_l (NRSliceL *slice)
{
	NRSliceL *next;
	next = slice->next;
	slice->next = ffslice_l;
	ffslice_l = slice;
	return next;
}

static void
nr_slice_free_list_l (NRSliceL * slice)
{
	NRSliceL * l;

	if (!slice) return;

	for (l = slice; l->next != NULL; l = l->next);

	l->next = ffslice_l;
	ffslice_l = slice;
}

static NRSliceL *
nr_slice_insert_sorted_l (NRSliceL * start, NRSliceL * slice)
{
	NRSliceL * s, * l;

	if (!start) return slice;
	if (!slice) return start;

	if (nr_slice_compare_l (slice, start) <= 0) {
		slice->next = start;
		return slice;
	}

	s = start;
	for (l = start->next; l != NULL; l = l->next) {
		if (nr_slice_compare_l (slice, l) <= 0) {
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

static int
nr_slice_compare_l (NRSliceL *l, NRSliceL *r)
{
	if (l->y == r->y) {
		if (l->x < r->x) return -1;
		if (l->x > r->x) return 1;
		if (l->stepx < r->stepx) return -1;
		if (l->stepx > r->stepx) return 1;
	} else if (l->y > r->y) {
		NRVertex *v;
		double x, ldx, rdx;
		/* This is bitch - we have to determine r values at l->y */
		v = r->vertex;
		while (v->next && (v->next->y <= l->y)) v = v->next;
		/* If v is last vertex, r ends before l starts */
		if (!v->next) return 1;
		if (v->y == l->y) {
			x = v->x;
		} else {
			x = v->x + (v->next->x - v->x) * (l->y - v->y) / (v->next->y - v->y);
		}
		if (l->x < x) return -1;
		if (l->x > x) return 1;
		ldx = l->stepx * (v->next->y - v->y);
		rdx = v->next->x - v->x;
		if (ldx < rdx) return -1;
		if (ldx > rdx) return 1;
	} else {
		NRVertex * v;
		double x, ldx, rdx;
		/* This is bitch - we have to determine l value at r->y */
		v = l->vertex;
		while (v->next && (v->next->y <= r->y)) v = v->next;
		/* If v is last vertex, l ends before r starts */
		if (!v->next) return -1;
		if (v->y == r->y) {
			x = v->x;
		} else {
			x = v->x + (v->next->x - v->x) * (r->y - v->y) / (v->next->y - v->y);
		}
		if (x < r->x) return -1;
		if (x > r->x) return 1;
		ldx = l->stepx * (v->next->y - v->y);
		rdx = v->next->x - v->x;
		if (ldx < rdx) return -1;
		if (ldx > rdx) return 1;
	}
	return 0;
}

#define NR_RUN_ALLOC_SIZE 32
static NRRun *ffrun = NULL;

static NRRun *
nr_run_new (NRCoord x0, NRCoord y0, NRCoord x1, NRCoord y1, int wind)
{
	NRRun * r;

	r = ffrun;

	if (r == NULL) {
		int i;
		r = nr_new (NRRun, NR_RUN_ALLOC_SIZE);
		for (i = 1; i < (NR_RUN_ALLOC_SIZE - 1); i++) (r + i)->next = (r + i + 1);
		(r + NR_RUN_ALLOC_SIZE - 1)->next = NULL;
		ffrun = r + 1;
	} else {
		ffrun = r->next;
	}

	r->next = NULL;

	if (x0 <= x1) {
		r->x0 = x0;
		r->x1 = x1;
		r->y0 = y0;
		r->y1 = y1;
		r->step = (x0 == x1) ? 0.0 : wind * (y1 - y0) / (x1 - x0);
	} else {
		r->x0 = x1;
		r->x1 = x0;
		r->y0 = y1;
		r->y1 = y0;
		r->step = wind * (y1 - y0) / (x0 - x1);
	}

	r->final = wind * (y1 - y0);

	r->value = 0.0;
	r->x = r->x0;

	return r;
}

static NRRun *
nr_run_free_one (NRRun *run)
{
	NRRun *next;
	next = run->next;
	run->next = ffrun;
	ffrun = run;
	return next;
}

static void
nr_run_free_list (NRRun * run)
{
	NRRun * l;

	if (!run) return;

	for (l = run; l->next != NULL; l = l->next);
	l->next = ffrun;
	ffrun = run;
}

static NRRun *
nr_run_insert_sorted (NRRun * start, NRRun * run)
{
	NRRun * s, * l;

	if (!start) return run;
	if (!run) return start;

	if (run->x0 < start->x0) {
		run->next = start;
		return run;
	}

	s = start;
	for (l = start->next; l != NULL; l = l->next) {
		if (run->x0 < l->x0) {
			run->next = l;
			s->next = run;
			return start;
		}
		s = l;
	}

	s->next = run;

	return start;
}

#if 0
/*
 * Determinate the exact order of 2 noncrossing lines
 */

static gint
nr_x_order (NRLine * l0, NRLine * l1)
{
	float xx0, yy0, x1x0, y1y0;
	float ds, de;

	/* Determine l0 start relative to l1 */
	x1x0 = l1->e.x - l1->s.x;
	y1y0 = l1->e.y - l1->s.y;
	xx0 = l0->s.x - l1->s.x;
	yy0 = l0->s.y - l1->s.y;

	if ((xx0 == 0.0) && (yy0 == 0.0)) {
		/* l0 start lies on l1 */
		xx0 = l0->e.x - l1->s.x;
		yy0 = l0->e.y - l1->s.y;
		de = xx0 * y1y0 - yy0 * x1x0;
		if (de < 0.0) return -1;
		if (de > 0.0) return 1;
		return 0;
	}

	ds = xx0 * y1y0 - yy0 * x1x0;

	/* l0 end relative to l1 */
	xx0 = l0->s.x - l1->s.x;
	yy0 = l0->s.y - l1->s.y;

	if ((xx0 == 0.0) && (yy0 == 0.0)) {
		/* l0 end lies on l1 */
		if (ds < 0.0) return -1;
		if (ds > 0.0) return 1;
		return 0;
	}

	de = xx0 * y1y0 - yy0 * x1x0;

	if (ds == 0.0) {
		if (de < 0.0) return -1;
		if (de > 0.0) return 1;
		return 0;
	}
	if (de == 0.0) {
		if (ds < 0.0) return -1;
		if (ds > 0.0) return 1;
		return 0;
	}

	if ((ds < 0.0) && (de < 0.0)) return -1;
	if ((de > 0.0) && (de > 0.0)) return 1;

	/* l0 start and end are on opposite sides of l1 */
	/* Determine l1 start relative to l0 */
	x1x0 = l0->e.x - l0->s.x;
	y1y0 = l0->e.y - l0->s.y;
	xx0 = l1->s.x - l0->s.x;
	yy0 = l1->s.y - l0->s.y;

	if ((xx0 == 0.0) && (yy0 == 0.0)) {
		/* l1 start lies on l0 */
		xx0 = l1->e.x - l0->s.x;
		yy0 = l1->e.y - l0->s.y;
		de = xx0 * y1y0 - yy0 * x1x0;
		if (de < 0.0) return 1;
		if (de > 0.0) return -1;
		return 0;
	}

	ds = xx0 * y1y0 - yy0 * x1x0;

	if (ds < 0.0) return 1;
	if (ds > 0.0) return -1;

	/* l1 end relative to l0 */
	xx0 = l1->s.x - l0->s.x;
	yy0 = l1->s.y - l0->s.y;

	if ((xx0 == 0.0) && (yy0 == 0.0)) {
		/* l1 end lies on l0 */
		if (ds < 0.0) return 1;
		if (ds > 0.0) return -1;
		return 0;
	}

	de = xx0 * y1y0 - yy0 * x1x0;

	if (de < 0.0) return 1;
	if (de > 0.0) return -1;
	return 0;
}
#endif


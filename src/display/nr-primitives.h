#ifndef _NR_PRIMITIVES_H_
#define _NR_PRIMITIVES_H_

typedef float NRCoord;

typedef struct {
	float x, y;
} NRPoint;

typedef struct {
	float x0, y0, x1, y1;
} NRDRect;

typedef struct {
	int x0, y0, x1, y1;
} NRIRect;

typedef struct {
	float c[6];
} NRAffine;

#define NR_NEXT(v,l) if (++(v) >= (l)) (v) = 0
#define NR_PREV(v,l) if (--(v) < 0) (v) = l

#define NR_COORD_FROM_ART(v) (rint ((v) * 16.0) / 16.0)
#define NR_COORD_SNAP(v) (rint ((v) * 16.0) / 16.0)
#define NR_COORD_TO_ART(v) (v)
#define NR_COORD_TOLERANCE 0.0625

void nr_irect_set_empty (NRIRect * rect);
int nr_irect_is_empty (NRIRect * rect);
int nr_irect_do_intersect (NRIRect * a, NRIRect * b);
int nr_irect_point_is_inside (NRIRect * rect, NRPoint * point);
NRIRect *nr_irect_intersection (NRIRect *dst, NRIRect *r0, NRIRect *r1);
NRIRect *nr_irect_union (NRIRect *dst, NRIRect *r0, NRIRect *r1);

void nr_drect_set_empty (NRDRect * rect);
NRDRect * nr_drect_stretch_xy (NRDRect * rect, NRCoord x, NRCoord y);
int nr_drect_do_intersect (NRDRect * a, NRDRect * b);

NRAffine * nr_affine_set_identity (NRAffine * affine);
NRAffine * nr_affine_multiply (NRAffine * dst, NRAffine * a, NRAffine * b);

/* fixme: Clean this stuff */
int nr_affine_is_identity (const double *affine);

#endif

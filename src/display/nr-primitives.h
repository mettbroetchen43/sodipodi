#ifndef _NR_PRIMITIVES_H_
#define _NR_PRIMITIVES_H_

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

void nr_irect_set_empty (NRIRect * rect);
int nr_irect_is_empty (NRIRect * rect);
int nr_irect_do_intersect (NRIRect * a, NRIRect * b);
int nr_irect_point_is_inside (NRIRect * rect, NRPoint * point);
NRIRect * nr_irect_intersection (NRIRect * dst, NRIRect * r0, NRIRect * r1);

NRAffine * nr_affine_set_identity (NRAffine * affine);
NRAffine * nr_affine_multiply (NRAffine * dst, NRAffine * a, NRAffine * b);

#endif

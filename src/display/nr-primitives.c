#define _NR_PRIMITIVES_C_

#include <glib.h>
#include "nr-primitives.h"

void
nr_irect_set_empty (NRIRect * rect)
{
	g_return_if_fail (rect != NULL);

	rect->x0 = rect->y0 = 0;
	rect->x1 = rect->y1 = 0;
}

int
nr_irect_is_empty (NRIRect * rect)
{
	g_return_val_if_fail (rect != NULL, TRUE);

	return ((rect->x0 < rect->x1) && (rect->y0 < rect->y1));
}

int
nr_irect_do_intersect (NRIRect * a, NRIRect * b)
{
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

	return ((MAX (a->x0, b->x0) < MIN (a->x1, b->x1)) && (MAX (a->y0, b->y0) < MIN (a->y1, b->y1)));
}

int
nr_irect_point_is_inside (NRIRect * rect, NRPoint * point)
{
	g_return_val_if_fail (rect != NULL, FALSE);
	g_return_val_if_fail (point != NULL, FALSE);

	return ((point->x >= rect->x0) && (point->x < rect->x1) && (point->y >= rect->y0) && (point->y < rect->y1));
}

NRIRect *
nr_irect_intersection (NRIRect * dst, NRIRect * r0, NRIRect * r1)
{
	g_return_val_if_fail (dst != NULL, NULL);
	g_return_val_if_fail (r0 != NULL, NULL);
	g_return_val_if_fail (r1 != NULL, NULL);

	if (nr_irect_is_empty (r0)) {
		if (nr_irect_is_empty (r1)) {
			nr_irect_set_empty (dst);
		} else {
			*dst = *r1;
		}
	} else {
		if (nr_irect_is_empty (r1)) {
			*dst = *r0;
		} else {
			dst->x0 = MIN (r0->x0, r1->x0);
			dst->y0 = MIN (r0->y0, r1->y0);
			dst->x1 = MAX (r0->x1, r1->x1);
			dst->y1 = MAX (r0->y1, r1->y1);
		}
	}

	return dst;
}

NRAffine *
nr_affine_set_identity (NRAffine * affine)
{
	static NRAffine identity = {{1.0, 0.0, 0.0, 1.0, 0.0, 0.0}};

	g_return_val_if_fail (affine != NULL, NULL);

	*affine = identity;

	return affine;
}

NRAffine *
nr_affine_multiply (NRAffine * dst, NRAffine * a, NRAffine * b)
{
	NRAffine d;

	g_return_val_if_fail (dst != NULL, NULL);
	g_return_val_if_fail (a != NULL, NULL);
	g_return_val_if_fail (b != NULL, NULL);

	d.c[0] = a->c[0] * b->c[0] + a->c[1] * b->c[2];
	d.c[1] = a->c[0] * b->c[1] + a->c[1] * b->c[3];
	d.c[2] = a->c[2] * b->c[0] + a->c[3] * b->c[2];
	d.c[3] = a->c[2] * b->c[1] + a->c[3] * b->c[3];
	d.c[4] = a->c[4] * b->c[0] + a->c[5] * b->c[2] + b->c[4];
	d.c[5] = a->c[4] * b->c[1] + a->c[5] * b->c[3] + b->c[5];

	*dst = d;

	return dst;
}


#ifndef _NR_PRIMITIVES_H_
#define _NR_PRIMITIVES_H_

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

#endif

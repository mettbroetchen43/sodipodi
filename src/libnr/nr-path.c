#define __NR_PATH_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "nr-path.h"

NRBPath *
nr_path_duplicate_transform (NRBPath *d, NRBPath *s, NRMatrixF *transform)
{
	int i;

	i = 0;
	while (s->path[i].code != ART_END) i += 1;

	d->path = nr_new (ArtBpath, i + 1);

	i = 0;
	while (s->path[i].code != ART_END) {
		d->path[i].code = s->path[i].code;
		if (s->path[i].code == ART_CURVETO) {
			d->path[i].x1 = NR_MATRIX_DF_TRANSFORM_X (transform, s->path[i].x1, s->path[i].y1);
			d->path[i].y1 = NR_MATRIX_DF_TRANSFORM_Y (transform, s->path[i].x1, s->path[i].y1);
			d->path[i].x2 = NR_MATRIX_DF_TRANSFORM_X (transform, s->path[i].x2, s->path[i].y2);
			d->path[i].y2 = NR_MATRIX_DF_TRANSFORM_Y (transform, s->path[i].x2, s->path[i].y2);
		}
		d->path[i].x3 = NR_MATRIX_DF_TRANSFORM_X (transform, s->path[i].x3, s->path[i].y3);
		d->path[i].y3 = NR_MATRIX_DF_TRANSFORM_Y (transform, s->path[i].x3, s->path[i].y3);
		i += 1;
	}
	d->path[i].code = ART_END;

	return d;
}


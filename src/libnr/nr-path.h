#ifndef __NR_PATH_H__
#define __NR_PATH_H__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

typedef struct _NRVPath NRVPath;
typedef struct _NRBPath NRBPath;

#include <libnr/nr-matrix.h>

#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_bpath.h>

struct _NRVPath {
	ArtVpath *path;
};

struct _NRBPath {
	ArtBpath *path;
};

NRBPath *nr_path_duplicate_transform (NRBPath *d, NRBPath *s, NRMatrixF *transform);

void nr_path_matrix_f_point_f_bbox_wind_distance (NRBPath *bpath, NRMatrixF *m, NRPointF *pt,
						  NRRectF *bbox, int *wind, float *dist,
						  float tolerance);

void nr_path_matrix_f_bbox_f_union (NRBPath *bpath, NRMatrixF *m, NRRectF *bbox, float tolerance);

#endif

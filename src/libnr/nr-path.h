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

typedef struct _NRPath NRPath;

enum {
	NR_PATH_LINETO,
	NR_PATH_CURVETO2,
	NR_PATH_CURVETO3
};

enum {
	NR_WIND_RULE_NONZERO,
	NR_WIND_RULE_EVENODD
};

#include <libnr/nr-types.h>

#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_bpath.h>

typedef struct _NRPathCode NRPathCode;
typedef union _NRPathElement NRPathElement;
typedef struct _NRPathGVector NRPathGVector;

struct _NRPathCode {
	unsigned int length : 24;
	unsigned int closed : 1;
	unsigned int code : 7;
};

union _NRPathElement {
	NRPathCode code;
	float value;
};

#define NR_PATH_ELEMENT_LENGTH(e) ((e)->code.length)
#define NR_PATH_ELEMENT_CLOSED(e) ((e)->code.closed)
#define NR_PATH_ELEMENT_CODE(e) ((e)->code.code)
#define NR_PATH_ELEMENT_VALUE(e) ((e)->value)

/* Return value FALSE means error and stops processing */

#define NR_PATH_CLOSED (1 << 0)
#define NR_PATH_FIRST (1 << 1)
#define NR_PATH_LAST (1 << 2)

struct _NRPathGVector {
	unsigned int (* moveto) (float x0, float y0, unsigned int flags, void *data);
	unsigned int (* lineto) (float x0, float y0, float x1, float y1, unsigned int flags, void *data);
	unsigned int (* curveto2) (float x0, float y0, float x1, float y1, float x2, float y2,
				   unsigned int flags, void *data);
	unsigned int (* curveto3) (float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3,
				   unsigned int flags, void *data);
	unsigned int (* closepath) (float ex, float ey, float sx, float sy, unsigned int flags, void *data);
};

/*
 * Path structure
 *   Length (elements) + closed
 *   x0, y0,
 *   code x, y, [x1, y1, x2, y2] [...], code, x, y...
 */

struct _NRPath {
	unsigned int nelements;
	unsigned int nsegments;
	NRPathElement elements[1];
};

NRPath *nr_path_new_from_art_bpath (const ArtBpath *bpath);

unsigned int nr_path_forall (const NRPath *path, NRMatrixF *transform, const NRPathGVector *gv, void *data);
unsigned int nr_path_forall_flat (const NRPath *path, NRMatrixF *transform, float tolerance,
				  const NRPathGVector *gv, void *data);

/* Temporary */
unsigned int nr_path_forall_art (const ArtBpath *path, NRMatrixF *transform, const NRPathGVector *gv, void *data);
unsigned int nr_path_forall_art_flat (const ArtBpath *path, NRMatrixF *transform, float tolerance,
				      const NRPathGVector *gv, void *data);
unsigned int nr_path_forall_art_vpath (const ArtVpath *path, NRMatrixF *transform, const NRPathGVector *gv, void *data);

#if 0
struct _NRDynamicPath {
	unsigned int refcount;
	unsigned int nelements;
	unsigned int hascpt : 1;
	float cpx, cpy;
	NRPath *path;
};

NRDynamicPath *nr_dynamic_path_new (unsigned int nelements);
#endif

/* fixme: Get rid of this (Lauris) */

typedef struct _NRBPath NRBPath;

struct _NRBPath {
	ArtBpath *path;
};

NRBPath *nr_path_duplicate_transform (NRBPath *d, NRBPath *s, NRMatrixF *transform);

void nr_path_matrix_f_point_f_bbox_wind_distance (NRBPath *bpath, NRMatrixF *m, NRPointF *pt,
						  NRRectF *bbox, int *wind, float *dist,
						  float tolerance);

void nr_path_matrix_f_bbox_f_union (NRBPath *bpath, NRMatrixF *m, NRRectF *bbox, float tolerance);

#endif

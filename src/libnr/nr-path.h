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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

typedef union _NRPathElement NRPathElement;
typedef struct _NRPathGVector NRPathGVector;

/* Bits 0-23 value; 24 closed; 25-31 code */

union _NRPathElement {
	unsigned int uval;
	float fval;
};

#define NR_PATH_ELEMENT_LENGTH(e) ((e)->uval & 0x00ffffff)
#define NR_PATH_ELEMENT_CLOSED(e) ((e)->uval & 0x01000000)
#define NR_PATH_ELEMENT_CODE(e) (((e)->uval & 0xfe000000) >> 25)
#define NR_PATH_ELEMENT_VALUE(e) ((e)->fval)

#define NR_PATH_ELEMENT_SET_LENGTH(e,v) ((e)->uval = (((e)->uval & 0xff000000) | ((v) & 0xffffff)))
#define NR_PATH_ELEMENT_SET_CLOSED(e,v) ((e)->uval = (((e)->uval & 0xfeffffff) | ((v) ? 0x1000000 : 0)))
#define NR_PATH_ELEMENT_SET_CODE(e,v) ((e)->uval = (((e)->uval & 0x01ffffff) | ((v) << 25)))
#define NR_PATH_ELEMENT_SET_VALUE(e,v) ((e)->fval = (v))

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
	unsigned int (* endpath) (float ex, float ey, float sx, float sy, unsigned int flags, void *data);
};

/*
 * Path structure
 *   Length (elements) + closed
 *   x0, y0,
 *   code x, y, [x1, y1, x2, y2] [...], code, x, y...
 */

struct _NRPath {
	/* Maximum is 2e30 elements */
	/* Number of elements, including reserved ones */
	unsigned int nelements : 30;
	/* Number of reserved elements */
	unsigned int offset : 2;
	/* Number of path segments (fixme) */
	unsigned int nsegments;
	NRPathElement elements[1];
};

NRPath *nr_path_duplicate_transform (const NRPath *path, const NRMatrixF *transform);

unsigned int nr_path_forall (const NRPath *path, NRMatrixF *transform, const NRPathGVector *gv, void *data);
unsigned int nr_path_forall_flat (const NRPath *path, NRMatrixF *transform, float tolerance,
				  const NRPathGVector *gv, void *data);

void nr_path_matrix_f_bbox_f_union (NRPath *path, NRMatrixF *m, NRRectF *bbox, float tolerance);

#ifdef LIBNR_LIBART
/* Temporary */
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_bpath.h>

NRPath *nr_path_new_from_art_bpath (const ArtBpath *bpath);
unsigned int nr_path_forall_art (const ArtBpath *path, NRMatrixF *transform, const NRPathGVector *gv, void *data);
unsigned int nr_path_forall_art_flat (const ArtBpath *path, NRMatrixF *transform, float tolerance,
				      const NRPathGVector *gv, void *data);
unsigned int nr_path_forall_art_vpath (const ArtVpath *path, NRMatrixF *transform, const NRPathGVector *gv, void *data);
#endif

typedef struct _NRDynamicPath NRDynamicPath;

struct _NRDynamicPath {
	/* Reference count */
	unsigned int refcount;
	/* Number of allocated elements */
	unsigned int size;
	/* Whether data is owned by us */
	unsigned int isstatic : 1;
	/* Whether currentpoint is defined */
	unsigned int hascpt : 1;
	float cpx, cpy;
	/* Start of current segment */
	unsigned int segstart;
	/* Current pathcode position */
	unsigned int codepos;
	/* Our own path structure */
	NRPath *path;
};

NRDynamicPath *nr_dynamic_path_new (unsigned int nelements);
NRDynamicPath *nr_dynamic_path_ref (NRDynamicPath *dpath);
NRDynamicPath *nr_dynamic_path_unref (NRDynamicPath *dpath);

/* Return TRUE on success */
unsigned int nr_dynamic_path_moveto (NRDynamicPath *dpath, float x0, float y0);
unsigned int nr_dynamic_path_lineto (NRDynamicPath *dpath, float x1, float y1);
unsigned int nr_dynamic_path_curveto2 (NRDynamicPath *dpath, float x1, float y1, float x2, float y2);
unsigned int nr_dynamic_path_curveto3 (NRDynamicPath *dpath, float x1, float y1, float x2, float y2, float x3, float y3);
unsigned int nr_dynamic_path_closepath (NRDynamicPath *dpath);

#ifdef LIBNR_LIBART
/* fixme: Get rid of this (Lauris) */

typedef struct _NRBPath NRBPath;

struct _NRBPath {
	ArtBpath *path;
};

NRBPath *nr_bpath_duplicate_transform (NRBPath *d, NRBPath *s, NRMatrixF *transform);

void nr_path_matrix_f_point_f_bbox_wind_distance (NRBPath *bpath, NRMatrixF *m, NRPointF *pt,
						  NRRectF *bbox, int *wind, float *dist,
						  float tolerance);

void nr_bpath_matrix_f_bbox_f_union (NRBPath *bpath, NRMatrixF *m, NRRectF *bbox, float tolerance);
#endif

#endif

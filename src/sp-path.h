#ifndef __SP_PATH_H__
#define __SP_PATH_H__

/*
 * SVG <path> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "helper/curve.h"
#include "sp-shape.h"

#define SP_TYPE_PATH (sp_path_get_type ())
#define SP_PATH(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_PATH, SPPath))
#define SP_PATH_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_PATH, SPPathClass))
#define SP_IS_PATH(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_PATH))
#define SP_IS_PATH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_PATH))

struct _SPPath {
	SPShape shape;
};

struct _SPPathClass {
	SPShapeClass shape_class;
};

unsigned int sp_path_get_type (void);

#if 0
#define SP_PATH_IS_INDEPENDENT(p) (SP_PATH (p)->independent)

/* Utility */

void sp_path_clear (SPPath * path);
void sp_path_add_bpath (SPPath * path, SPCurve * curve, gboolean private, gdouble affine[]);

/* normalizes path - i.e. concatenates all component bpaths. */
/* returns newly allocated bpath - caller has to manage it */
SPCurve * sp_path_normalized_bpath (SPPath * path);

/* creates single component normalized path with private component */
SPCurve * sp_path_normalize (SPPath * path);

/* fixme: it works only for single component (normalized) private paths? */
void sp_path_bpath_modified (SPPath * path, SPCurve * curve);
#endif

#endif

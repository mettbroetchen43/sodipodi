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

#define SP_TYPE_PATH (sp_path_get_type ())
#define SP_PATH(o) (GTK_CHECK_CAST ((o), SP_TYPE_PATH, SPPath))
#define SP_PATH_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_PATH, SPPathClass))
#define SP_IS_PATH(o) (GTK_CHECK_TYPE ((o), SP_TYPE_PATH))
#define SP_IS_PATH_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_PATH))

typedef struct _SPPathComp SPPathComp;

#include "helper/curve.h"
#include "sp-item.h"

struct _SPPathComp {
	SPCurve *curve;
	gboolean private;
	double affine[6];
};

SPPathComp * sp_path_comp_new (SPCurve *curve, gboolean private, double affine[]);
void sp_path_comp_destroy (SPPathComp *comp);

struct _SPPath {
	SPItem item;
	GSList *comp;
	gboolean independent;
};

struct _SPPathClass {
	SPItemClass item_class;
	void (* remove_comp) (SPPath *path, SPPathComp *comp);
	void (* add_comp) (SPPath *path, SPPathComp *comp);
	void (* change_bpath) (SPPath *path, SPPathComp *comp, SPCurve *curve);
};

GtkType sp_path_get_type (void);

#define sp_path_independent(p) (p->independent)

/* methods */

void sp_path_remove_comp (SPPath * path, SPPathComp * comp);
void sp_path_add_comp (SPPath * path, SPPathComp * comp);
void sp_path_change_bpath (SPPath * path, SPPathComp * comp, SPCurve * curve);

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

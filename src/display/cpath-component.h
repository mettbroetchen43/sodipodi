#ifndef SP_CPATH_COMPONENT_H
#define SP_CPATH_COMPONENT_H

/*
 * SPCanvasPath components
 *
 * These are per-object parts of composite shapes
 */

#include <glib.h>
#include <libart_lgpl/art_rect.h>
#include "path-archetype.h"

typedef struct _SPCPathComp SPCPathComp;

struct _SPCPathComp {
	gint refcount;
	/* identifiers */
	SPCurve * curve;
	guint private : 1;
	guint changed : 1;
	guint rule : 2;
	double affine[6];
	double stroke_width;
	ArtPathStrokeJoinType join;
	ArtPathStrokeCapType cap;
	/* state */
	SPPathAT * archetype;
	guint closed : 1;
	gint cx, cy;			/* svp position in canvas_coords */
	ArtDRect bbox;
};

/*
 * Creates new component with given bpath, affine & stroke settings
 * archetype is initially NULL
 */

SPCPathComp * sp_cpath_comp_new (SPCurve * curve,
	gboolean private,
	double affine[],
	double stroke_width,
	ArtPathStrokeJoinType join,
	ArtPathStrokeCapType cap);

void sp_cpath_comp_ref (SPCPathComp * comp);
void sp_cpath_comp_unref (SPCPathComp * comp);

/*
 * Updates component state parameters (archetype, closed, cx, cy, bbox),
 * It is allowed, to change all identifiers by hand, before calling update
 */

void sp_cpath_comp_update (SPCPathComp * comp, double affine[]);

#endif

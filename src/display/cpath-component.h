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
	ArtBpath * bpath;
	gboolean private;
	double affine[6];
	double stroke_width;
	ArtPathStrokeJoinType join;
	ArtPathStrokeCapType cap;
	/* state */
	SPPathAT * archetype;
	gint closed;
	gint cx, cy;			/* svp position in canvas_coords */
	ArtDRect bbox;
};

/*
 * Creates new component with given bpath, affine & stroke settings
 * NB! if component is private, it frees bpath, when destroyed. Otherwise
 *     bpath creator (SPObject ..., font ...) should do it.
 * archetype is initially NULL
 */

SPCPathComp * sp_cpath_comp_new (ArtBpath * bpath,
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

/*
 * Changes current comp parameters
 * This is simple utility functions, which ensures, that private bpath
 * will be freed if necessary
 * Afterwards you have to call update (probably from canvas object update)
 * NB! This frees bpath & forces at bpath to null
 */

void sp_cpath_comp_change (SPCPathComp * comp,
	ArtBpath * bpath,
	gboolean private,
	double affine[],
	double stroke_width,
	ArtPathStrokeJoinType join,
	ArtPathStrokeCapType cap);

#endif

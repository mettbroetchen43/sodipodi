#ifndef SP_PATH_ARCHETYPE_H
#define SP_PATH_ARCHETYPE_H

/*
 * Path Archetypes
 *
 * These are shared components for gnome canvas shapes
 * The main reason of their existence is sharing SVPs for glyphs
 * Maybe we'll include pixmap cache one day also...
 *
 */

#include <glib.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath_stroke.h>
#include "../helper/curve.h"
#ifdef NEW_RENDER
#include "nr-svp.h"
#endif

typedef struct _SPPathAT SPPathAT;

struct _SPPathAT {
	gint refcount;
	/* identifiers */
	SPCurve * curve;
	guint private : 1;	/* Whether archetype can be shared */
	guint rule : 2;
	double affine[4];
	double stroke_width;
	ArtPathStrokeJoinType join;
	ArtPathStrokeCapType cap;
	/* state */
	ArtVpath * vpath;
	ArtSVP * svp;
	ArtSVP * stroke;
	ArtDRect bbox;
#ifdef NEW_RENDER
	NRSVP * nrsvp;
#endif
};

/*
 * Request new or shared archetype with given bpath, affine & stroke
 * Inreases refcount of returned at (use unref, when done with it)
 */

SPPathAT *sp_path_at (SPCurve *curve,
		      gboolean private,
		      double affine[],
		      gint rule,
		      double stroke_width,
		      ArtPathStrokeJoinType join,
		      ArtPathStrokeCapType cap);

void sp_path_at_ref (SPPathAT * at);
void sp_path_at_unref (SPPathAT * at);

#endif

#ifndef SP_PATH_ARCHETYPE_H
#define SP_PATH_ARCHETYPE_H

/*
 * Path Archetypes
 *
 * These are shared components for gnome canvas shapes
 * The main reason of their existence is sharing SVPs for glyphs
 *
 */

#include <glib.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath_stroke.h>

typedef struct _SPPathAT SPPathAT;

struct _SPPathAT {
	gint refcount;
	/* identifiers */
	ArtBpath * bpath;
	gboolean private;
	double affine[6];
	double stroke_width;
	ArtPathStrokeJoinType join;
	ArtPathStrokeCapType cap;
	/* state */
	ArtVpath * vpath;
	ArtSVP * svp;
	ArtSVP * stroke;
	ArtDRect bbox;
};

/*
 * Request new or shared archetype with given bpath, affine & stroke
 * Inreases refcount of returned at (use unref, when done with it)
 */

SPPathAT * sp_path_at (ArtBpath * bpath,
	gboolean private,
	double affine[],
	double stroke_width,
	ArtPathStrokeJoinType join,
	ArtPathStrokeCapType cap);

void sp_path_at_ref (SPPathAT * at);
void sp_path_at_unref (SPPathAT * at);

#endif

#define SP_CPATH_COMPONENT_C

#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_affine.h>
#include <libart_lgpl/art_point.h>
#include "cpath-component.h"

#define DEBUG_PATH_COMP

#ifdef DEBUG_PATH_COMP
	gint num_comp = 0;
#endif

SPCPathComp *
sp_cpath_comp_new (ArtBpath * bpath,
	gboolean private,
	double affine[],
	double stroke_width,
	ArtPathStrokeJoinType join,
	ArtPathStrokeCapType cap)
{
	SPCPathComp * comp;
	gint i;

	g_return_val_if_fail (bpath != NULL, NULL);
	comp = g_new (SPCPathComp, 1);
	g_return_val_if_fail (comp != NULL, NULL);

	comp->refcount = 1;
	comp->bpath = bpath;
	comp->private = private;
	if (affine == NULL) {
		art_affine_identity (comp->affine);
	} else {
		for (i = 0; i < 6; i++) comp->affine[i] = affine[i];
	}
	comp->stroke_width = stroke_width;
	comp->join = join;
	comp->cap = cap;
	comp->archetype = NULL;
	comp->closed = TRUE;
	for (i=0; bpath[i].code != ART_END; i++) {
		if (bpath[i].code == ART_MOVETO_OPEN)
			comp->closed = FALSE;
	}
#ifdef DEBUG_PATH_COMP
	num_comp++;
	g_print ("num_comp = %d\n", num_comp);
#endif
	return comp;
}

void sp_cpath_comp_ref (SPCPathComp * comp)
{
	g_return_if_fail (comp != NULL);

	comp->refcount++;
}

void sp_cpath_comp_unref (SPCPathComp * comp)
{
	g_return_if_fail (comp != NULL);

	comp->refcount--;

	if (comp->refcount < 1) {
		if (comp->archetype) {
			g_print ("comp ref 0, at ref %d\n", comp->archetype->refcount);
			sp_path_at_unref (comp->archetype);
			}
#if 0
		/* We cannot free bpath here - SPPath does it */
		if (comp->private)
			art_free (comp->bpath);
#endif
#ifdef DEBUG_PATH_COMP
		num_comp--;
		g_print ("num_comp = %d\n", num_comp);
#endif
		g_free (comp);
	}
}

void
sp_cpath_comp_update (SPCPathComp * comp, double affine[])
{
	SPPathAT * old_at;
	double a[6];
	ArtPoint p;
	gint i;

	for (i=0; comp->bpath[i].code != ART_END; i++) {
		if (comp->bpath[i].code == ART_MOVETO_OPEN)
			comp->closed = FALSE;
	}

	old_at = comp->archetype;

	art_affine_multiply (a, comp->affine, affine);
	comp->archetype = sp_path_at (comp->bpath, comp->private, a, comp->stroke_width, comp->join, comp->cap);

	if (old_at != NULL) sp_path_at_unref (old_at);

	p.x = comp->affine[4];
	p.y = comp->affine[5];
	art_affine_point (&p, &p, affine);
	comp->cx = p.x;
	comp->cy = p.y;
	comp->bbox.x0 = comp->archetype->bbox.x0 + p.x;
	comp->bbox.y0 = comp->archetype->bbox.y0 + p.y;
	comp->bbox.x1 = comp->archetype->bbox.x1 + p.x;
	comp->bbox.y1 = comp->archetype->bbox.y1 + p.y;
}

void
sp_cpath_comp_change (SPCPathComp * comp,
	ArtBpath * bpath,
	gboolean private,
	double affine[],
	double stroke_width,
	ArtPathStrokeJoinType join,
	ArtPathStrokeCapType cap)
{
	gint i;

#if 0
	/* We cannot free bpath here - SPPath does it */
	if ((comp->bpath != bpath) && (comp->private)) {
		art_free (comp->bpath);
		/* BEWARE! at is not happy with NULL bpath. fortunately
		 * it does not use it, so it does hear of it ;-) */
		if (comp->archetype != NULL) comp->archetype->bpath = NULL;
	}
#endif
	comp->bpath = bpath;
	comp->private = private;
	for (i = 0; i < 6; i++) comp->affine[i] = affine[i];
	comp->stroke_width = stroke_width;
	comp->join = join;
	comp->cap = cap;
}


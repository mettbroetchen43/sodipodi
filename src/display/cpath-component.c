#define SP_CPATH_COMPONENT_C

#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_affine.h>
#include <libart_lgpl/art_point.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_wind.h>
#include "cpath-component.h"

#define noDEBUG_PATH_COMP

#ifdef DEBUG_PATH_COMP
	gint num_comp = 0;
#endif

SPCPathComp *
sp_cpath_comp_new (SPCurve * curve,
	gboolean private,
	double affine[],
	double stroke_width,
	ArtPathStrokeJoinType join,
	ArtPathStrokeCapType cap)
{
	ArtBpath * bpath;
	SPCPathComp * comp;
	gint i;

	g_return_val_if_fail (curve != NULL, NULL);
	comp = g_new (SPCPathComp, 1);
	g_return_val_if_fail (comp != NULL, NULL);

	bpath = curve->bpath;

	comp->refcount = 1;
	comp->curve = curve;
	sp_curve_ref (curve);
	comp->private = private;
	if (affine == NULL) {
		art_affine_identity (comp->affine);
	} else {
		for (i = 0; i < 6; i++) comp->affine[i] = affine[i];
	}
	comp->rule = ART_WIND_RULE_NONZERO;
	comp->stroke_width = stroke_width;
	comp->join = join;
	comp->cap = cap;
	comp->archetype = NULL;
	comp->closed = TRUE;
	for (i=0; bpath[i].code != ART_END; i++) {
		if (bpath[i].code == ART_MOVETO_OPEN)
			comp->closed = FALSE;
	}

	comp->changed = FALSE;

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
		if (comp->archetype) sp_path_at_unref (comp->archetype);
		sp_curve_unref (comp->curve);
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

	art_affine_multiply (a, comp->affine, affine);

	if ((comp->private) && (comp->archetype) && (!comp->changed) &&
	    (comp->rule == comp->archetype->rule) &&
	    (comp->stroke_width == comp->archetype->stroke_width) &&
	    (comp->join == comp->archetype->join) &&
	    (comp->cap == comp->archetype->cap) &&
	    (a[0] == comp->archetype->affine[0]) &&
	    (a[1] == comp->archetype->affine[1]) &&
	    (a[2] == comp->archetype->affine[2]) &&
	    (a[3] == comp->archetype->affine[3])) {
		/* Nothing to do */
	} else {
		old_at = comp->archetype;
		comp->archetype = sp_path_at (comp->curve, comp->private, a, comp->rule, comp->stroke_width, comp->join, comp->cap);
		if (old_at != NULL) sp_path_at_unref (old_at);
	}

	comp->changed = FALSE;

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


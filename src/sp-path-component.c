#define SP_PATH_COMPONENT_C

#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_affine.h>
#include "sp-path-component.h"

SPPathComp *
sp_path_comp_new (SPCurve * curve, gboolean private, double affine[])
{
	SPPathComp * comp;
	gint i;

	g_return_val_if_fail (curve != NULL, NULL);

	comp = g_new (SPPathComp, 1);
	comp->curve = curve;
	sp_curve_ref (curve);
	comp->private = private;
	if (affine != NULL) {
		for (i = 0; i < 6; i++) comp->affine[i] = affine[i];
	} else {
		art_affine_identity (comp->affine);
	}
	return comp;
}

void
sp_path_comp_destroy (SPPathComp * comp)
{
	g_assert (comp != NULL);

	sp_curve_unref (comp->curve);

	g_free (comp);
}

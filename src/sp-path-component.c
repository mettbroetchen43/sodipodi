#define SP_PATH_COMPONENT_C

#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_affine.h>
#include "sp-path-component.h"

SPPathComp *
sp_path_comp_new (ArtBpath * bpath, gboolean private, double affine[])
{
	SPPathComp * comp;
	gint i;

	comp = g_new (SPPathComp, 1);
	comp->bpath = bpath;
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

	if ((comp->private) && (comp->bpath != NULL))
		art_free (comp->bpath);

	g_free (comp);
}

#ifndef SP_PATH_COMPONENT_H
#define SP_PATH_COMPONENT_H

#include <glib.h>
#include <libart_lgpl/art_bpath.h>

typedef struct _SPPathComp SPPathComp;

struct _SPPathComp {
	ArtBpath * bpath;
	gboolean private;
	double affine[6];
};

SPPathComp * sp_path_comp_new (ArtBpath * bpath, gboolean private, double affine[]);
void sp_path_comp_destroy (SPPathComp * comp);

#endif

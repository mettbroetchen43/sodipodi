#ifndef SP_PATH_COMPONENT_H
#define SP_PATH_COMPONENT_H

#include <glib.h>
#include <libart_lgpl/art_bpath.h>
#include "helper/curve.h"

typedef struct _SPPathComp SPPathComp;

struct _SPPathComp {
	SPCurve * curve;
	gboolean private;
	double affine[6];
};

SPPathComp * sp_path_comp_new (SPCurve * curve, gboolean private, double affine[]);
void sp_path_comp_destroy (SPPathComp * comp);

#endif

#define _NR_PRIMITIVES_C_

#include <glib.h>
#include "nr-primitives.h"

void
nr_irect_set_empty (NRIRect * rect)
{
	g_return_if_fail (rect != NULL);

	rect->x0 = rect->y0 = 0;
	rect->x1 = rect->y1 = -1;
}

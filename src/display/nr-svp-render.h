#ifndef _NR_SVP_RENDER_H_
#define _NR_SVP_RENDER_H_

#include "nr-svp.h"

void nr_svp_render_rgb_rgba (NRSVP * svp,
			     guchar * buffer,
			     gint x0, gint y0,
			     gint width, gint height, gint rowstride,
			     guint32 rgba);

#endif

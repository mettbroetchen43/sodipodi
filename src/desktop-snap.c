#define SP_DESKTOP_SNAP_C

#include <math.h>
#include "sp-guide.h"
#include "desktop-snap.h"

gdouble
sp_desktop_free_snap (SPDesktop * desktop, ArtPoint * req)
{
	SPNamedView * nv;
	ArtPoint actual;
	gdouble best, dist;
	gboolean snapped;
	GSList * l;

	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (req != NULL);

	nv = desktop->namedview;
	actual = *req;
	snapped = FALSE;

	if (nv->snaptoguides) {
		/* snap distance in desktop units */
		best = nv->guidetolerance * desktop->w2d[0];
		for (l = nv->hguides; l != NULL; l = l->next) {
			if (fabs (SP_GUIDE (l->data)->position - req->y) < best) {
				best = fabs (SP_GUIDE (l->data)->position - req->y);
				actual.y = SP_GUIDE (l->data)->position;
				snapped = TRUE;
			}
		}
		best = nv->guidetolerance * desktop->w2d[0];
		for (l = nv->vguides; l != NULL; l = l->next) {
			if (fabs (SP_GUIDE (l->data)->position - req->x) < best) {
				best = fabs (SP_GUIDE (l->data)->position - req->x);
				actual.x = SP_GUIDE (l->data)->position;
				snapped = TRUE;
			}
		}
	}

	dist = hypot (actual.x - req->x, actual.y - req->y);

	*req = actual;

	return dist;
}


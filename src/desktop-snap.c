#define SP_DESKTOP_SNAP_C

#include <math.h>
#include "sp-guide.h"
#include "desktop-snap.h"

gdouble
sp_desktop_free_snap (SPDesktop * desktop, ArtPoint * req)
{
	gdouble dh, dv;

	dh = sp_desktop_horizontal_snap (desktop, req);
	dv = sp_desktop_vertical_snap (desktop, req);

	if ((dh < 1e18) && (dv < 1e18)) return hypot (dh, dv);
	if (dh < 1e18) return dh;
	if (dv < 1e18) return dv;
	return 1e18;
}

gdouble
sp_desktop_horizontal_snap (SPDesktop * desktop, ArtPoint * req)
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
		for (l = nv->vguides; l != NULL; l = l->next) {
			if (fabs (SP_GUIDE (l->data)->position - req->x) < best) {
				best = fabs (SP_GUIDE (l->data)->position - req->x);
				actual.x = SP_GUIDE (l->data)->position;
				snapped = TRUE;
			}
		}
	}

	if (nv->snaptogrid) {
		gdouble p;
		best = nv->gridtolerance * desktop->w2d[0];
		p = nv->gridorigin.x + floor ((req->x - nv->gridorigin.x) / nv->gridspacing.x) * nv->gridspacing.x;
		if (fabs (req->x - p) < best) {
			best = fabs (req->x - p);
			actual.x = p;
			snapped = TRUE;
		}
		if (fabs (nv->gridspacing.x - (req->x - p)) < best) {
			best = fabs (nv->gridspacing.x - (req->x - p));
			actual.x = nv->gridspacing.x + p;
			snapped = TRUE;
		}
	}

	dist = (snapped) ? fabs (actual.x - req->x) : 1e18;

	*req = actual;

	return dist;
}

gdouble
sp_desktop_vertical_snap (SPDesktop * desktop, ArtPoint * req)
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
	}

	if (nv->snaptogrid) {
		gdouble p;
		best = nv->gridtolerance * desktop->w2d[0];
		p = nv->gridorigin.y + floor ((req->y - nv->gridorigin.y) / nv->gridspacing.y) * nv->gridspacing.y;
		if (fabs (req->y - p) < best) {
			best = fabs (req->y - p);
			actual.y = p;
			snapped = TRUE;
		}
		if (fabs (nv->gridspacing.y - (req->y - p)) < best) {
			best = fabs (nv->gridspacing.y - (req->y - p));
			actual.y = nv->gridspacing.y + p;
			snapped = TRUE;
		}
	}

	dist = (snapped) ? fabs (actual.y - req->y) : 1e18;

	*req = actual;

	return dist;
}

gdouble
sp_desktop_vector_snap (SPDesktop * desktop, ArtPoint * req, gdouble dx, gdouble dy)
{
	SPNamedView * nv;
	ArtPoint actual;
	gdouble len, best, dist, delta;
	gboolean snapped;
	GSList * l;

	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (req != NULL);

	len = hypot (dx, dy);
	if (len < 1e-18) sp_desktop_free_snap (desktop, req);
	dx /= len;
	dy /= len;

	nv = desktop->namedview;
	snapped = FALSE;
	actual = *req;
	best = nv->guidetolerance * desktop->w2d[0];

	if (fabs (dx) > 1e-18) {
		/* Test horizontal snapping */
		for (l = nv->vguides; l != NULL; l = l->next) {
			delta = SP_GUIDE (l->data)->position - req->x;
			dist = hypot (delta, delta * dy / dx);
			if (dist < best) {
				best = dist;
				actual.x = SP_GUIDE (l->data)->position;
				actual.y = req->y + delta * dy / dx;
				snapped = TRUE;
			}
		}
	}

	if (fabs (dy) > 1e-18) {
		/* Test vertical snapping */
		for (l = nv->hguides; l != NULL; l = l->next) {
			delta = SP_GUIDE (l->data)->position - req->y;
			dist = hypot (delta, delta * dx / dy);
			if (dist < best) {
				best = dist;
				actual.x = req->x + delta * dx / dy;
				actual.y = SP_GUIDE (l->data)->position;
				snapped = TRUE;
			}
		}
	}

	* req = actual;

	if (snapped) return best;

	return 1e18;
}


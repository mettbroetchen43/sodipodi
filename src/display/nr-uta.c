#define _NR_UTA_C_

#include <glib.h>
#include "nr-uta.h"

#define NR_UTILE_X0(u) ((u) >> 24)
#define NR_UTILE_Y0(u) (((u) >> 16) & 0xff)
#define NR_UTILE_X1(u) (((u) >> 8) & 0xff)
#define NR_UTILE_Y1(u) ((u) & 0xff)

#define NR_UTILE_VALUE(x0,y0,x1,y1) (((x0) << 24) | ((y0) << 16) | ((x1) << 8) | (y1))

#define NR_UTA_SHIFT 5
#define NR_UTA_SIZE (1 << NR_UTA_SHIFT)
#define NR_UTA_MASK (NR_UTA_SIZE - 1)

NRUTA *
nr_uta_new (gint x0, gint y0, gint x1, gint y1)
{
	NRUTA * uta;
	NRUTile * utiles;

	g_return_val_if_fail ((x0 < x1) && (y0 < y1), NULL);

	uta = g_new (NRUTA, 1);

	uta->area.x0 = x0;
	uta->area.y0 = y0;
	uta->area.x1 = x1;
	uta->area.y1 = y1;

	uta->width = (((x1 - x0) + NR_UTA_MASK) >> NR_UTA_SHIFT);
	uta->height = (((y1 - y0) + NR_UTA_MASK) >> NR_UTA_SHIFT);

	utiles = g_new0 (NRUTile, uta->width * uta->height);

	uta->utiles = utiles;

	return uta;
}

void
nr_uta_free (NRUTA * uta)
{
	g_return_if_fail (uta != NULL);

	g_free (uta->utiles);
	g_free (uta);
}

/* This is terribly suboptimal */

void
nr_uta_set_rect (NRUTA * uta, NRIRect * rect)
{
	g_return_if_fail (uta != NULL);
	g_return_if_fail (rect != NULL);

	if (!nr_irect_is_empty (rect)) {
		NRIRect clip;
		nr_irect_intersection (&clip, &uta->area, rect);
		if (!nr_irect_is_empty (&clip)) {
			gint x0, y0, x1, y1, x, y, xe, ye;
			NRUTile * u;
			x0 = clip.x0 >> NR_UTA_SHIFT;
			y0 = clip.y0 >> NR_UTA_SHIFT;
			x1 = (clip.x1 + NR_UTA_MASK) >> NR_UTA_SHIFT;
			y1 = (clip.y1 + NR_UTA_MASK) >> NR_UTA_SHIFT;
			for (y = y0; y < y1; y += NR_UTA_SIZE) {
				ye = MIN (y + NR_UTA_MASK, y1);
				for (x = x0; x < x1; x += NR_UTA_SIZE) {
					gint ux0, uy0, ux1, uy1;
					xe = MIN (x + NR_UTA_MASK, x1);
					u = uta->utiles + y * uta->width + x;
					ux0 = MIN (NR_UTILE_X0 (*u), x);
					uy0 = MIN (NR_UTILE_Y0 (*u), y);
					ux1 = MAX (NR_UTILE_X1 (*u), xe);
					uy1 = MAX (NR_UTILE_Y1 (*u), ye);
					*u = NR_UTILE_VALUE (ux0, uy0, ux1, uy1);
				}
			}
		}
	}
}

/* This is even more suboptimal */

void
nr_uta_set_uta (NRUTA * uta, NRUTA * src)
{
	gint x, y;

	g_return_if_fail (uta != NULL);
	g_return_if_fail (src != NULL);

	for (y = src->area.y0; y < src->area.y1; y++) {
		for (x = src->area.x0; x < src->area.x1; x++) {
			NRUTile * u;
			NRIRect c;
			u = src->utiles + y * src->width + x;
			c.x0 = x * NR_UTA_SIZE + NR_UTILE_X0 (*u);
			c.y0 = y * NR_UTA_SIZE + NR_UTILE_Y0 (*u);
			c.x1 = x * NR_UTA_SIZE + NR_UTILE_X1 (*u);
			c.y1 = y * NR_UTA_SIZE + NR_UTILE_Y1 (*u);
			nr_uta_set_rect (uta, &c);
		}
	}
}


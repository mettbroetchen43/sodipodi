#ifndef _NR_UTA_H_
#define _NR_UTA_H_

#include "nr-primitives.h"

typedef struct _NRUTA NRUTA;
/* fixme: We should include glib and use gint32 */
typedef int NRUTile;

struct _NRUTA {
	NRIRect area;
	int width, height;
	NRUTile * utiles;
};

NRUTA * nr_uta_new (int x0, int y0, int x1, int y1);

void nr_uta_free (NRUTA * uta);

void nr_uta_set_rect (NRUTA * uta, NRIRect * rect);
void nr_uta_set_uta (NRUTA * uta, NRUTA * src);

#endif

#ifndef RUBBERBAND_H
#define RUBBERBAND_H

/*
 * rubberband
 *
 * uses desktop coordinates
 */

#include "desktop.h"

/* fixme: do multidocument safe */

void sp_rubberband_start (SPDesktop * desktop, double x, double y);
void sp_rubberband_move (double x, double y);
void sp_rubberband_stop (void);

gboolean sp_rubberband_rect (ArtDRect * rect);

#endif

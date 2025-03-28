#ifndef __RUBBERBAND_H__
#define __RUBBERBAND_H__

/*
 * Rubberbanding selector
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */


#include <libnr/nr-types.h>
#include "forward.h"

/* fixme: do multidocument safe */

void sp_rubberband_start (SPDesktop *desktop, double x, double y);
void sp_rubberband_move (double x, double y);
void sp_rubberband_stop (void);

gboolean sp_rubberband_rect (NRRectD *rect);

#endif

#ifndef __NR_SVP_RENDER_H__
#define __NR_SVP_RENDER_H__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <libnr/nr-pixblock.h>
#include <libnr/nr-svp.h>

void nr_pixblock_render_svp_rgba (NRPixBlock *d, NRSVP *svp, NRULong rgba);

#endif

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

#ifdef __cplusplus
extern "C" {
#endif

/* Renders graymask of svp into buffer */
void nr_pixblock_render_svp_mask_or (NRPixBlock *d, NRSVP *svp);
/* Renders colored SVP into buffer (has to be RGB/RGBA) */
void nr_pixblock_render_svp_rgba (NRPixBlock *d, NRSVP *svp, NRULong rgba);

/* Renders graymask of svp into buffer */
void nr_pixblock_render_svl_mask_or (NRPixBlock *d, NRSVL *svl);
/* Renders colored SVP into buffer (has to be RGB/RGBA) */
void nr_pixblock_render_svl_rgba (NRPixBlock *d, NRSVL *svl, NRULong rgba);

#ifdef __cplusplus
};
#endif

#endif

#ifndef __SP_PRINT_H__
#define __SP_PRINT_H__

/*
 * Frontend to printing
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <libnr/nr-types.h>
#include <libnr/nr-path.h>
#include "forward.h"

unsigned int sp_print_bind (SPPrintContext *ctx, const NRMatrixF *transform, float opacity);
unsigned int sp_print_release (SPPrintContext *ctx);
unsigned int sp_print_fill (SPPrintContext *ctx, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
			    const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox);
unsigned int sp_print_stroke (SPPrintContext *ctx, const NRBPath *bpath, const NRMatrixF *transform, const SPStyle *style,
			      const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox);

unsigned int sp_print_image_R8G8B8A8_N (SPPrintContext *ctx,
					unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
					const NRMatrixF *transform, const SPStyle *style);

#ifndef WITHOUT_PRINT_UI
/* UI frontend */
void sp_print_preview_document (SPDocument *doc);
void sp_print_document (SPDocument *doc, unsigned int direct, const unsigned char *filename);
#endif

#endif

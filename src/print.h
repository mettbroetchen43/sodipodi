#ifndef __SP_PRINT_H__
#define __SP_PRINT_H__

/*
 * Frontend to printing
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libnr/nr-types.h>
#include <libnr/nr-path.h>
#include "forward.h"

unsigned int sp_print_bind (SPPrintContext *ctx, NRMatrixF *transform, float opacity);
unsigned int sp_print_release (SPPrintContext *ctx);

unsigned int sp_print_fill (SPPrintContext *ctx, NRBPath *bpath, NRMatrixF *transform, SPStyle *style);
unsigned int sp_print_stroke (SPPrintContext *ctx, NRBPath *bpath, NRMatrixF *transform, SPStyle *style);
unsigned int sp_print_image_R8G8B8A8_P (SPPrintContext *ctx,
					unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
					NRMatrixF *transform);

#endif

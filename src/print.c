#define __SP_PRINT_C__

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

#include <libgnomeprint/gnome-print.h>

#include "print.h"

struct _SPPrintContext {
	GnomePrintContext *gpc;
};

unsigned int
sp_print_bind (SPPrintContext *ctx, NRMatrixF *transform, float opacity)
{
	gdouble t[6];

	gnome_print_gsave (ctx->gpc);

	t[0] = transform->c[0];
	t[1] = transform->c[1];
	t[2] = transform->c[2];
	t[3] = transform->c[3];
	t[4] = transform->c[4];
	t[5] = transform->c[5];

	gnome_print_concat (ctx->gpc, t);

	/* fixme: Opacity? (lauris) */

	return 1;
}

unsigned int
sp_print_release (SPPrintContext *ctx)
{
	gnome_print_grestore (ctx->gpc);
}

unsigned int
sp_print_fill (SPPrintContext *ctx, NRBPath *bpath, NRMatrixF *transform, SPStyle *style)
{
	return 0;
}

unsigned int
sp_print_stroke (SPPrintContext *ctx, NRBPath *bpath, NRMatrixF *transform, SPStyle *style)
{
	return 0;
}

unsigned int
sp_print_image_R8G8B8A8_P (SPPrintContext *ctx,
			   unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
			   NRMatrixF *transform)
{
	return 0;
}



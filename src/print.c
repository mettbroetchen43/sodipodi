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

#include <config.h>

#include <string.h>
#include <libnr/nr-rect.h>
#include <libnr/nr-pixblock.h>

#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_wind.h>
#include <glib.h>

#ifdef WITH_GNOME_PRINT
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-paper.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprint/gnome-print-master-preview.h>
#include <libgnomeprint/gnome-printer-dialog.h>
#endif

#include "document.h"
#include "sp-item.h"
#include "style.h"
#include "sp-paint-server.h"

#include "print.h"

#ifdef WITH_GNOME_PRINT
struct _SPPrintContext {
	GnomePrintContext *gpc;
};
#else
struct _SPPrintContext {
	gpointer dummy;
};
#endif

unsigned int
sp_print_bind (SPPrintContext *ctx, const NRMatrixF *transform, float opacity)
{
#ifdef WITH_GNOME_PRINT
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
#endif

	return 0;
}

unsigned int
sp_print_release (SPPrintContext *ctx)
{
#ifdef WITH_GNOME_PRINT
	gnome_print_grestore (ctx->gpc);
#endif

	return 0;
}

unsigned int
sp_print_fill (SPPrintContext *ctx, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
	       const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
#ifdef WITH_GNOME_PRINT
	gdouble t[6];

	/* CTM is for information purposes only */
	/* We expect user coordinate system to be set up already */

	t[0] = ctm->c[0];
	t[1] = ctm->c[1];
	t[2] = ctm->c[2];
	t[3] = ctm->c[3];
	t[4] = ctm->c[4];
	t[5] = ctm->c[5];

	if (style->fill.type == SP_PAINT_TYPE_COLOR) {
		float rgb[3], opacity;
		sp_color_get_rgb_floatv (&style->fill.value.color, rgb);
		gnome_print_setrgbcolor (ctx->gpc, rgb[0], rgb[1], rgb[2]);

		/* fixme: */
		opacity = SP_SCALE24_TO_FLOAT (style->fill_opacity.value) * SP_SCALE24_TO_FLOAT (style->opacity.value);
		gnome_print_setopacity (ctx->gpc, opacity);

		gnome_print_bpath (ctx->gpc, bpath->path, FALSE);

		if (style->fill_rule.value == ART_WIND_RULE_ODDEVEN) {
			gnome_print_eofill (ctx->gpc);
		} else {
			gnome_print_fill (ctx->gpc);
		}
	} else if (style->fill.type == SP_PAINT_TYPE_PAINTSERVER) {
		SPPainter *painter;
		gdouble dctm[6];
		ArtDRect dpbox;

		/* fixme: */
		dctm[0] = ctm->c[0];
		dctm[1] = ctm->c[1];
		dctm[2] = ctm->c[2];
		dctm[3] = ctm->c[3];
		dctm[4] = ctm->c[4];
		dctm[5] = ctm->c[5];
		dpbox.x0 = pbox->x0;
		dpbox.y0 = pbox->y0;
		dpbox.x1 = pbox->x1;
		dpbox.y1 = pbox->y1;
		painter = sp_paint_server_painter_new (SP_STYLE_FILL_SERVER (style), dctm, &dpbox);
		if (painter) {
			NRRectF cbox;
			NRRectL ibox;
			NRMatrixF d2i;
			double dd2i[6];
			int x, y;
			unsigned char *rgba;

			nr_rect_f_intersect (&cbox, dbox, bbox);
			ibox.x0 = (long) cbox.x0;
			ibox.y0 = (long) cbox.y0;
			ibox.x1 = (long) (cbox.x1 + 0.9999);
			ibox.y1 = (long) (cbox.y1 + 0.9999);

			nr_matrix_f_invert (&d2i, ctm);

			gnome_print_gsave (ctx->gpc);

			gnome_print_bpath (ctx->gpc, bpath->path, FALSE);

			if (style->fill_rule.value == ART_WIND_RULE_ODDEVEN) {
				gnome_print_eoclip (ctx->gpc);
			} else {
				gnome_print_clip (ctx->gpc);
			}
			dd2i[0] = d2i.c[0];
			dd2i[1] = d2i.c[1];
			dd2i[2] = d2i.c[2];
			dd2i[3] = d2i.c[3];
			dd2i[4] = d2i.c[4];
			dd2i[5] = d2i.c[5];
			gnome_print_concat (ctx->gpc, dd2i);
			/* Now we are in desktop coordinates */
			rgba = nr_pixelstore_16K_new (FALSE, 0x0);
			for (y = ibox.y0; y < ibox.y1; y+= 64) {
				for (x = ibox.x0; x < ibox.x1; x+= 64) {
					memset (rgba, 0x0, 4 * 64 * 64);
					painter->fill (painter, rgba, x, y, 64, 64, 4 * 64);
					gnome_print_gsave (ctx->gpc);
					gnome_print_translate (ctx->gpc, x, y + 64);
					gnome_print_scale (ctx->gpc, 64, -64);
					gnome_print_rgbaimage (ctx->gpc, rgba, 64, 64, 4 * 64);
					gnome_print_grestore (ctx->gpc);
				}
			}
			nr_pixelstore_16K_free (rgba);
			gnome_print_grestore (ctx->gpc);
			sp_painter_free (painter);
		}
	}
#endif

	return 0;
}

unsigned int
sp_print_stroke (SPPrintContext *ctx, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
		 const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
#ifdef WITH_GNOME_PRINT
	gdouble t[6];

	/* CTM is for information purposes only */
	/* We expect user coordinate system to be set up already */

	t[0] = ctm->c[0];
	t[1] = ctm->c[1];
	t[2] = ctm->c[2];
	t[3] = ctm->c[3];
	t[4] = ctm->c[4];
	t[5] = ctm->c[5];

	if (style->stroke.type == SP_PAINT_TYPE_COLOR) {
		float rgb[3], opacity;
		sp_color_get_rgb_floatv (&style->stroke.value.color, rgb);
		gnome_print_setrgbcolor (ctx->gpc, rgb[0], rgb[1], rgb[2]);

		/* fixme: */
		opacity = SP_SCALE24_TO_FLOAT (style->stroke_opacity.value) * SP_SCALE24_TO_FLOAT (style->opacity.value);
		gnome_print_setopacity (ctx->gpc, opacity);

		if (style->stroke_dash.n_dash > 0) {
			gnome_print_setdash (ctx->gpc, style->stroke_dash.n_dash, style->stroke_dash.dash, style->stroke_dash.offset);
		} else {
			gnome_print_setdash (ctx->gpc, 0, NULL, 0.0);
		}

		gnome_print_setlinewidth (ctx->gpc, style->stroke_width.computed);
		gnome_print_setlinejoin (ctx->gpc, style->stroke_linejoin.computed);
		gnome_print_setlinecap (ctx->gpc, style->stroke_linecap.computed);

		gnome_print_bpath (ctx->gpc, bpath->path, FALSE);

		gnome_print_stroke (ctx->gpc);
	}
#endif

	return 0;
}

unsigned int
sp_print_image_R8G8B8A8_N (SPPrintContext *ctx,
			   unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
			   const NRMatrixF *transform, const SPStyle *style)
{
#ifdef WITH_GNOME_PRINT
	gdouble t[6];

	t[0] = transform->c[0];
	t[1] = transform->c[1];
	t[2] = transform->c[2];
	t[3] = transform->c[3];
	t[4] = transform->c[4];
	t[5] = transform->c[5];

	gnome_print_gsave (ctx->gpc);

	gnome_print_concat (ctx->gpc, t);

	if (style->opacity.value != SP_SCALE24_MAX) {
		guchar *dpx, *d, *s;
		gint x, y;
		guint32 alpha;
		alpha = (guint32) floor (SP_SCALE24_TO_FLOAT (style->opacity.value) * 255.9999);
		dpx = g_new (guchar, w * h * 4);
		for (y = 0; y < h; y++) {
			s = px + y * rs;
			d = dpx + y * w * 4;
			memcpy (d, s, w * 4);
			for (x = 0; x < w; x++) {
				d[3] = (s[3] * alpha) / 255;
				s += 4;
				d += 4;
			}
		}
		gnome_print_rgbaimage (ctx->gpc, dpx, w, h, w * 4);
		g_free (dpx);
	} else {
		gnome_print_rgbaimage (ctx->gpc, px, w, h, rs);
	}

	gnome_print_grestore (ctx->gpc);
#endif

	return 0;
}

/* UI */

void
sp_print_preview_document (SPDocument *doc)
{
#ifdef WITH_GNOME_PRINT
	SPPrintContext ctx;
        GnomePrintContext *gpc;
        GnomePrintMaster *gpm;
	GnomePrintMasterPreview *gpmp;
	gchar *title;

	sp_document_ensure_up_to_date (doc);

	gpm = gnome_print_master_new();
	gpc = gnome_print_master_get_context (gpm);
	ctx.gpc = gpc;

	g_return_if_fail (gpm != NULL);
	g_return_if_fail (gpc != NULL);

	/* Print document */
	gnome_print_beginpage (gpc, SP_DOCUMENT_NAME (doc));
	gnome_print_translate (gpc, 0.0, sp_document_height (doc));
	/* From desktop points to document pixels */
	gnome_print_scale (gpc, 0.8, -0.8);
	sp_item_invoke_print (SP_ITEM (sp_document_root (doc)), &ctx);
        gnome_print_showpage (gpc);
        gnome_print_context_close (gpc);

	title = g_strdup_printf (_("Sodipodi (doc name %s..): Print Preview"),"");
	gpmp = gnome_print_master_preview_new (gpm, title);

	gtk_widget_show (GTK_WIDGET(gpmp));

	gnome_print_master_close (gpm);

	g_free (title);
#endif
}

void
sp_print_document (SPDocument *doc)
{
#ifdef WITH_GNOME_PRINT
        GnomePrinter *printer;
	SPPrintContext ctx;
        GnomePrintContext *gpc;

        printer = gnome_printer_dialog_new_modal ();
        if (printer == NULL) return;

	sp_document_ensure_up_to_date (doc);

	gpc = gnome_print_context_new (printer);
	ctx.gpc = gpc;

	g_return_if_fail (gpc != NULL);

	/* Print document */
	gnome_print_beginpage (gpc, SP_DOCUMENT_NAME (doc));
	gnome_print_translate (gpc, 0.0, sp_document_height (doc));
	/* From desktop points to document pixels */
	gnome_print_scale (gpc, 0.8, -0.8);
	sp_item_invoke_print (SP_ITEM (sp_document_root (doc)), &ctx);
        gnome_print_showpage (gpc);
        gnome_print_context_close (gpc);
#endif
}

void
sp_print_document_to_file (SPDocument *doc, const unsigned char *filename)
{
#ifdef WITH_GNOME_PRINT
        GnomePrinter *printer;
	SPPrintContext ctx;
        GnomePrintContext *gpc;

        printer = gnome_printer_new_generic_ps (filename);
        if (printer == NULL) return;

	sp_document_ensure_up_to_date (doc);

	gpc = gnome_print_context_new (printer);
	ctx.gpc = gpc;

	g_return_if_fail (gpc != NULL);

	/* Print document */
	gnome_print_beginpage (gpc, SP_DOCUMENT_NAME (doc));
	gnome_print_translate (gpc, 0.0, sp_document_height (doc));
	/* From desktop points to document pixels */
	gnome_print_scale (gpc, 0.8, -0.8);
	sp_item_invoke_print (SP_ITEM (sp_document_root (doc)), &ctx);
        gnome_print_showpage (gpc);
        gnome_print_context_close (gpc);
#endif
}


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
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprintui/gnome-print-master-preview.h>
#include <libgnomeprintui/gnome-printer-dialog.h>
#endif

#include <gtk/gtkstock.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>

#include "helper/sp-intl.h"
#include "document.h"
#include "sp-item.h"
#include "style.h"
#include "sp-paint-server.h"

#include "print.h"

/* fixme: Sooner or later we want SPPrintContext subclasses (Lauris) */

#ifdef WITH_GNOME_PRINT
struct _SPPrintContext {
	GnomePrintContext *gpc;
};
#else
struct _SPPrintContext {
	FILE *stream;
};
unsigned int sp_print_plain_bind (SPPrintContext *ctx, const NRMatrixF *transform, float opacity);
unsigned int sp_print_plain_release (SPPrintContext *ctx);
unsigned int sp_print_plain_fill (SPPrintContext *ctx, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
				  const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox);
unsigned int sp_print_plain_stroke (SPPrintContext *ctx, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
				    const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox);
unsigned int sp_print_plain_image_R8G8B8A8_N (SPPrintContext *ctx, unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
					      const NRMatrixF *transform, const SPStyle *style);
static void sp_print_bpath (FILE *stream, const ArtBpath *bp);
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

	return 0;
#else
	return sp_print_plain_bind (ctx, transform, opacity);
#endif
}

unsigned int
sp_print_release (SPPrintContext *ctx)
{
#ifdef WITH_GNOME_PRINT
	gnome_print_grestore (ctx->gpc);

	return 0;
#else
	return sp_print_plain_release (ctx);
#endif
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
		NRMatrixD dctm;
		NRRectD dpbox;

		/* fixme: */
		nr_matrix_d_from_f (&dctm, ctm);
		dpbox.x0 = pbox->x0;
		dpbox.y0 = pbox->y0;
		dpbox.x1 = pbox->x1;
		dpbox.y1 = pbox->y1;
		painter = sp_paint_server_painter_new (SP_STYLE_FILL_SERVER (style), NR_MATRIX_D_TO_DOUBLE (&dctm), &dpbox);
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

	return 0;
#else
	return sp_print_plain_fill (ctx, bpath, ctm, style, pbox, dbox, bbox);
#endif
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

	return 0;
#else
	return sp_print_plain_stroke (ctx, bpath, ctm, style, pbox, dbox, bbox);
#endif
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

	return 0;
#else
	return sp_print_image_R8G8B8A8_N (ctx, px, w, h, rs, transform, style);
#endif
}

/* UI */

void
sp_print_preview_document (SPDocument *doc)
{
#ifdef WITH_GNOME_PRINT
	SPPrintContext ctx;
        GnomePrintContext *gpc;
        GnomePrintMaster *gpm;
	GtkWidget *gpmp;
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
        GnomePrintConfig *config;
	GtkWidget *dlg;
	SPPrintContext ctx;
        GnomePrintContext *gpc;
	int btn;

	config = gnome_print_config_default ();
        dlg = gnome_printer_dialog_new (config);
	btn = gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
        if (btn != GTK_RESPONSE_OK) return;

	sp_document_ensure_up_to_date (doc);

	gpc = gnome_print_context_new (config);
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

	gnome_print_config_unref (config);
#else
	GtkWidget *dlg, *vbox, *f, *vb, *l, *e;
	int response;

	dlg = gtk_dialog_new_with_buttons (_("Print destination"), NULL,
					   GTK_DIALOG_MODAL,
					   GTK_STOCK_PRINT,
					   GTK_RESPONSE_OK,
					   GTK_STOCK_CANCEL,
					   GTK_RESPONSE_CANCEL,
					   NULL);

	vbox = GTK_DIALOG (dlg)->vbox;
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);

	f = gtk_frame_new (_("Print destination"));
	gtk_box_pack_start (GTK_BOX (vbox), f, FALSE, FALSE, 4);

	vb = gtk_vbox_new (FALSE, 4);
	gtk_container_add (GTK_CONTAINER (f), vb);
	gtk_container_set_border_width (GTK_CONTAINER (vb), 4);

	l = gtk_label_new (_("Enter destination lpr queue.\n"
			     "Use '> filename' to print to file.\n"
			     "Use '| prog arg...' to pipe to program"));
	gtk_box_pack_start (GTK_BOX (vb), l, FALSE, FALSE, 0);

	e = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (e), "lp");
	gtk_box_pack_start (GTK_BOX (vb), e, FALSE, FALSE, 0);

	gtk_widget_show_all (vbox);
	
	response = gtk_dialog_run (GTK_DIALOG (dlg));

	if (response == GTK_RESPONSE_OK) {
		const unsigned char *fn;
		FILE *osf, *osp;
		SPPrintContext ctx;
		/* Arrgh, have to do something */
		fn = gtk_entry_get_text (GTK_ENTRY (e));
		g_print ("Printing to %s\n", fn);
		osf = NULL;
		osp = NULL;
		if (fn) {
			if (*fn == '|') {
				fn += 1;
				while (isspace (*fn)) fn += 1;
				osp = popen (fn, "w");
				ctx.stream = osp;
			} else if (*fn == '>') {
				fn += 1;
				while (isspace (*fn)) fn += 1;
				osf = fopen (fn, "w+");
				ctx.stream = osf;
			} else {
				unsigned char *qn;
				qn = g_strdup_printf ("lpr %s", fn);
				osp = popen (qn, "w");
				g_free (qn);
				ctx.stream = osp;
			}
		}
		if (ctx.stream) {
			int res;
			sp_document_ensure_up_to_date (doc);
			res = fprintf (ctx.stream, "%g %g translate\n", 0.0, sp_document_height (doc));
			if (res >= 0) res = fprintf (ctx.stream, "0.8 -0.8 scale\n");
			if (res > 0) sp_item_invoke_print (SP_ITEM (sp_document_root (doc)), &ctx);
			if (res > 0) res = fprintf (ctx.stream, "showpage\n");
		}
		if (osf) fclose (osf);
		if (osp) fclose (osp);
	}

	gtk_widget_destroy (dlg);
#endif
}

void
sp_print_document_to_file (SPDocument *doc, const unsigned char *filename)
{
#ifdef WITH_GNOME_PRINT
        GnomePrintConfig *config;
	SPPrintContext ctx;
        GnomePrintContext *gpc;

	config = gnome_print_config_default ();
        if (!gnome_print_config_set (config, "Settings.Engine.Backend.Driver", "gnome-print-ps")) return;
        if (!gnome_print_config_set (config, "Settings.Transport.Backend", "file")) return;
        if (!gnome_print_config_set (config, GNOME_PRINT_KEY_OUTPUT_FILENAME, filename)) return;

	sp_document_ensure_up_to_date (doc);

	gpc = gnome_print_context_new (config);
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
#else
	SPPrintContext ctx;

	ctx.stream = fopen (filename, "w");
	if (ctx.stream) {
		sp_document_ensure_up_to_date (doc);
		fprintf (ctx.stream, "%g %g translate\n", 0.0, sp_document_height (doc));
		fprintf (ctx.stream, "0.8 -0.8 scale\n");
		sp_item_invoke_print (SP_ITEM (sp_document_root (doc)), &ctx);
		fprintf (ctx.stream, "showpage\n");
		fclose (ctx.stream);
	}
#endif
}

#ifndef WITH_GNOME_PRINT

/* Plain PostScript output */

unsigned int
sp_print_plain_bind (SPPrintContext *ctx, const NRMatrixF *transform, float opacity)
{
	if (!ctx->stream) return -1;

	return fprintf (ctx->stream, "gsave [%g %g %g %g %g %g] concat\n",
			transform->c[0],
			transform->c[1],
			transform->c[2],
			transform->c[3],
			transform->c[4],
			transform->c[5]);
}

unsigned int
sp_print_plain_release (SPPrintContext *ctx)
{
	if (!ctx->stream) return -1;

	fprintf (ctx->stream, "grestore\n");

	return 0;
}

unsigned int
sp_print_plain_fill (SPPrintContext *ctx, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
		     const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
	if (!ctx->stream) return -1;

	if (style->fill.type == SP_PAINT_TYPE_COLOR) {
		float rgb[3];

		sp_color_get_rgb_floatv (&style->fill.value.color, rgb);

		fprintf (ctx->stream, "%g %g %g setrgbcolor\n", rgb[0], rgb[1], rgb[2]);

		sp_print_bpath (ctx->stream, bpath->path);

		if (style->fill_rule.value == ART_WIND_RULE_ODDEVEN) {
			fprintf (ctx->stream, "eofill\n");
		} else {
			fprintf (ctx->stream, "fill\n");
		}
	}

	return 0;
}

unsigned int
sp_print_plain_stroke (SPPrintContext *ctx, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
		       const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
	if (!ctx->stream) return -1;

	if (style->stroke.type == SP_PAINT_TYPE_COLOR) {
		float rgb[3];

		sp_color_get_rgb_floatv (&style->stroke.value.color, rgb);

		fprintf (ctx->stream, "%g %g %g setrgbcolor\n", rgb[0], rgb[1], rgb[2]);

		sp_print_bpath (ctx->stream, bpath->path);

		if (style->stroke_dash.n_dash > 0) {
			int i;
			fprintf (ctx->stream, "[");
			for (i = 0; i < style->stroke_dash.n_dash; i++) {
				fprintf (ctx->stream, (i) ? " %g" : "%g", style->stroke_dash.dash[i]);
			}
			fprintf (ctx->stream, "] %g setdash\n", style->stroke_dash.offset);
		} else {
			fprintf (ctx->stream, "[] 0 setdash\n");
		}

		fprintf (ctx->stream, "%g setlinewidth\n", style->stroke_width.computed);
		fprintf (ctx->stream, "%d setlinejoin\n", style->stroke_linejoin.computed);
		fprintf (ctx->stream, "%d setlinecap\n", style->stroke_linecap.computed);

		fprintf (ctx->stream, "stroke\n");
	}

	return 0;
}

unsigned int
sp_print_plain_image_R8G8B8A8_N (SPPrintContext *ctx, unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
				 const NRMatrixF *transform, const SPStyle *style)
{
	int r;

	fprintf (ctx->stream, "gsave\n");
	fprintf (ctx->stream, "/rowdata %d string def\n", 3 * w);
	fprintf (ctx->stream, "[%g %g %g %g %g %g] concat\n",
		 transform->c[0],
		 transform->c[1],
		 transform->c[2],
		 transform->c[3],
		 transform->c[4],
		 transform->c[5]);
	fprintf (ctx->stream, "%d %d 8 [%d 0 0 -%d 0 %d]\n", w, h, w, h, h);
	fprintf (ctx->stream, "{currentfile rowdata readhexstring pop}\n");
	fprintf (ctx->stream, "false 3 colorimage\n");

	for (r = 0; r < h; r++) {
		unsigned char *s;
		int c0, c1, c;
		s = px + r * rs;
		for (c0 = 0; c0 < w; c0 += 24) {
			c1 = MIN (w, c0 + 24);
			for (c = c0; c < c1; c++) {
				static const char xtab[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
				fputc (xtab[s[0] >> 4], ctx->stream);
				fputc (xtab[s[0] & 0xf], ctx->stream);
				fputc (xtab[s[1] >> 4], ctx->stream);
				fputc (xtab[s[1] & 0xf], ctx->stream);
				fputc (xtab[s[2] >> 4], ctx->stream);
				fputc (xtab[s[2] & 0xf], ctx->stream);
				s += 4;
			}
			fputs ("\n", ctx->stream);
		}
	}

	fprintf (ctx->stream, "grestore\n");

	return 0;
}

/* PostScript helpers */

static void
sp_print_bpath (FILE *stream, const ArtBpath *bp)
{
	unsigned int closed;

	fprintf (stream, "newpath\n");
	closed = FALSE;
	while (bp->code != ART_END) {
		switch (bp->code) {
		case ART_MOVETO:
			if (closed) {
				fprintf (stream, "closepath\n");
			}
			closed = TRUE;
			fprintf (stream, "%g %g moveto\n", bp->x3, bp->y3);
			break;
		case ART_MOVETO_OPEN:
			if (closed) {
				fprintf (stream, "closepath\n");
			}
			closed = FALSE;
			fprintf (stream, "%g %g moveto\n", bp->x3, bp->y3);
			break;
		case ART_LINETO:
			fprintf (stream, "%g %g lineto\n", bp->x3, bp->y3);
			break;
		case ART_CURVETO:
			fprintf (stream, "%g %g %g %g %g %g curveto\n", bp->x1, bp->y1, bp->x2, bp->y2, bp->x3, bp->y3);
			break;
		default:
			break;
		}
		bp += 1;
	}
	if (closed) {
		fprintf (stream, "closepath\n");
	}
}
#endif


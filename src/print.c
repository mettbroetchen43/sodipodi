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
#include <ctype.h>

#include <libnr/nr-rect.h>
#include <libnr/nr-pixblock.h>

#include "helper/sp-intl.h"
#include "enums.h"
#include "document.h"
#include "sp-item.h"
#include "style.h"
#include "sp-paint-server.h"
#include "module.h"
#include "print.h"

/* Identity typedef */

struct _SPPrintContext {
	SPModulePrint module;
};

unsigned int
sp_print_bind (SPPrintContext *ctx, const NRMatrixF *transform, float opacity)
{
	if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->bind)
		return ((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->bind (SP_MODULE_PRINT (ctx), transform, opacity);

	return 0;
}

unsigned int
sp_print_release (SPPrintContext *ctx)
{
	if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->release)
		return ((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->release (SP_MODULE_PRINT (ctx));

	return 0;
}

unsigned int
sp_print_fill (SPPrintContext *ctx, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
	       const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
	if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->fill)
		return ((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->fill (SP_MODULE_PRINT (ctx), bpath, ctm, style, pbox, dbox, bbox);

	return 0;
}

unsigned int
sp_print_stroke (SPPrintContext *ctx, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
		 const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
	if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->stroke)
		return ((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->stroke (SP_MODULE_PRINT (ctx), bpath, ctm, style, pbox, dbox, bbox);

	return 0;
}

unsigned int
sp_print_image_R8G8B8A8_N (SPPrintContext *ctx,
			   unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
			   const NRMatrixF *transform, const SPStyle *style)
{
	if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->image)
		return ((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->image (SP_MODULE_PRINT (ctx), px, w, h, rs, transform, style);

	return 0;
}


#ifdef WITH_GNOME_PRINT

/* Gnome Print */

#define SP_TYPE_MODULE_PRINT_GNOME (sp_module_print_gnome_get_type())
#define SP_MODULE_PRINT_GNOME(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_MODULE_PRINT_GNOME, SPModulePrintGnome))
#define SP_IS_MODULE_PRINT_GNOME(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_MODULE_PRINT_GNOME))

typedef struct _SPModulePrintGnome SPModulePrintGnome;
typedef struct _SPModulePrintGnomeClass SPModulePrintGnomeClass;

#include <glib.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkstock.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprintui/gnome-print-master-preview.h>
#include <libgnomeprintui/gnome-printer-dialog.h>

struct _SPModulePrintGnome {
	SPModulePrint module;
	GnomePrintContext *gpc;
};

struct _SPModulePrintGnomeClass {
	SPModulePrintClass module_print_class;
};

unsigned int sp_module_print_gnome_get_type (void);

static void sp_module_print_gnome_class_init (SPModulePrintClass *klass);
static void sp_module_print_gnome_init (SPModulePrintGnome *gpmod);
static void sp_module_print_gnome_finalize (GObject *object);

static unsigned int sp_module_print_gnome_setup (SPModulePrint *mod);
static unsigned int sp_module_print_gnome_begin (SPModulePrint *mod, SPDocument *doc);
static unsigned int sp_module_print_gnome_finish (SPModulePrint *mod);

static unsigned int sp_module_print_gnome_bind (SPModulePrint *mod, const NRMatrixF *transform, float opacity);
static unsigned int sp_module_print_gnome_release (SPModulePrint *mod);
static unsigned int sp_module_print_gnome_fill (SPModulePrint *mod, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
						const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox);
static unsigned int sp_module_print_gnome_stroke (SPModulePrint *mod, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
						  const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox);
static unsigned int sp_module_print_gnome_image (SPModulePrint *mod, unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
						 const NRMatrixF *transform, const SPStyle *style);

static SPModulePrintClass *print_gnome_parent_class;

unsigned int
sp_module_print_gnome_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPModulePrintGnomeClass),
			NULL, NULL,
			(GClassInitFunc) sp_module_print_gnome_class_init,
			NULL, NULL,
			sizeof (SPModulePrintGnome),
			16,
			(GInstanceInitFunc) sp_module_print_gnome_init,
		};
		type = g_type_register_static (SP_TYPE_MODULE_PRINT, "SPModulePrintGnome", &info, 0);
	}
	return type;
}

static void
sp_module_print_gnome_class_init (SPModulePrintClass *klass)
{
	GObjectClass *g_object_class;
	SPModulePrintClass *module_print_class;

	g_object_class = (GObjectClass *)klass;
	module_print_class = (SPModulePrintClass *) klass;

	print_gnome_parent_class = g_type_class_peek_parent (klass);

	g_object_class->finalize = sp_module_print_gnome_finalize;

	module_print_class->setup = sp_module_print_gnome_setup;
	module_print_class->begin = sp_module_print_gnome_begin;
	module_print_class->finish = sp_module_print_gnome_finish;
	module_print_class->bind = sp_module_print_gnome_bind;
	module_print_class->release = sp_module_print_gnome_release;
	module_print_class->fill = sp_module_print_gnome_fill;
	module_print_class->stroke = sp_module_print_gnome_stroke;
	module_print_class->image = sp_module_print_gnome_image;
}

static void
sp_module_print_gnome_init (SPModulePrintGnome *fmod)
{
	/* Nothing here */
}

static void
sp_module_print_gnome_finalize (GObject *object)
{
	SPModulePrintGnome *gpmod;

	gpmod = (SPModulePrintGnome *) object;
	
	G_OBJECT_CLASS (print_gnome_parent_class)->finalize (object);
}

static unsigned int
sp_module_print_gnome_setup (SPModulePrint *mod)
{
	SPModulePrintGnome *gpmod;
        GnomePrintConfig *config;
	GtkWidget *dlg, *vbox, *sel;
	int btn;

	gpmod = (SPModulePrintGnome *) mod;

	config = gnome_print_config_default ();

	dlg = gtk_dialog_new_with_buttons (_("Select printer"), NULL,
					   GTK_DIALOG_MODAL,
					   GTK_STOCK_PRINT,
					   GTK_RESPONSE_OK,
					   GTK_STOCK_CANCEL,
					   GTK_RESPONSE_CANCEL,
					   NULL);

	vbox = GTK_DIALOG (dlg)->vbox;
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);

        sel = gnome_printer_selection_new (config);
	gtk_widget_show (sel);
	gtk_box_pack_start (GTK_BOX (vbox), sel, TRUE, TRUE, 0);

	btn = gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
        if (btn != GTK_RESPONSE_OK) return FALSE;

	gpmod->gpc = gnome_print_context_new (config);

	gnome_print_config_unref (config);

	return TRUE;
}

static unsigned int
sp_module_print_gnome_begin (SPModulePrint *mod, SPDocument *doc)
{
	SPModulePrintGnome *gpmod;

	gpmod = (SPModulePrintGnome *) mod;

	gnome_print_beginpage (gpmod->gpc, SP_DOCUMENT_NAME (doc));
	gnome_print_translate (gpmod->gpc, 0.0, sp_document_height (doc));
	/* From desktop points to document pixels */
	gnome_print_scale (gpmod->gpc, 0.8, -0.8);

	return 0;
}

static unsigned int
sp_module_print_gnome_finish (SPModulePrint *mod)
{
	SPModulePrintGnome *gpmod;

	gpmod = (SPModulePrintGnome *) mod;

	gnome_print_showpage (gpmod->gpc);
	gnome_print_context_close (gpmod->gpc);

	return 0;
}

static unsigned int
sp_module_print_gnome_bind (SPModulePrint *mod, const NRMatrixF *transform, float opacity)
{
	SPModulePrintGnome *gpmod;
	gdouble t[6];

	gpmod = (SPModulePrintGnome *) mod;

	gnome_print_gsave (gpmod->gpc);

	t[0] = transform->c[0];
	t[1] = transform->c[1];
	t[2] = transform->c[2];
	t[3] = transform->c[3];
	t[4] = transform->c[4];
	t[5] = transform->c[5];

	gnome_print_concat (gpmod->gpc, t);

	/* fixme: Opacity? (lauris) */

	return 0;
}

static unsigned int
sp_module_print_gnome_release (SPModulePrint *mod)
{
	SPModulePrintGnome *gpmod;

	gpmod = (SPModulePrintGnome *) mod;

	gnome_print_grestore (gpmod->gpc);

	return 0;
}

static unsigned int
sp_module_print_gnome_fill (SPModulePrint *mod, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
			    const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
	SPModulePrintGnome *gpmod;
	gdouble t[6];

	gpmod = (SPModulePrintGnome *) mod;

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
		gnome_print_setrgbcolor (gpmod->gpc, rgb[0], rgb[1], rgb[2]);

		/* fixme: */
		opacity = SP_SCALE24_TO_FLOAT (style->fill_opacity.value) * SP_SCALE24_TO_FLOAT (style->opacity.value);
		gnome_print_setopacity (gpmod->gpc, opacity);

		gnome_print_bpath (gpmod->gpc, bpath->path, FALSE);

		if (style->fill_rule.value == SP_WIND_RULE_EVENODD) {
			gnome_print_eofill (gpmod->gpc);
		} else {
			gnome_print_fill (gpmod->gpc);
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

			nr_rect_f_intersect (&cbox, dbox, bbox);
			ibox.x0 = (long) cbox.x0;
			ibox.y0 = (long) cbox.y0;
			ibox.x1 = (long) (cbox.x1 + 0.9999);
			ibox.y1 = (long) (cbox.y1 + 0.9999);

			nr_matrix_f_invert (&d2i, ctm);

			gnome_print_gsave (gpmod->gpc);

			gnome_print_bpath (gpmod->gpc, bpath->path, FALSE);

			if (style->fill_rule.value == SP_WIND_RULE_EVENODD) {
				gnome_print_eoclip (gpmod->gpc);
			} else {
				gnome_print_clip (gpmod->gpc);
			}
			dd2i[0] = d2i.c[0];
			dd2i[1] = d2i.c[1];
			dd2i[2] = d2i.c[2];
			dd2i[3] = d2i.c[3];
			dd2i[4] = d2i.c[4];
			dd2i[5] = d2i.c[5];
			gnome_print_concat (gpmod->gpc, dd2i);
			/* Now we are in desktop coordinates */
			for (y = ibox.y0; y < ibox.y1; y+= 64) {
				for (x = ibox.x0; x < ibox.x1; x+= 64) {
					NRPixBlock pb;
					nr_pixblock_setup_fast (&pb, NR_PIXBLOCK_MODE_R8G8B8A8N, x, y, x + 64, y + 64, TRUE);
					painter->fill (painter, &pb);
					gnome_print_gsave (gpmod->gpc);
					gnome_print_translate (gpmod->gpc, x, y + 64);
					gnome_print_scale (gpmod->gpc, 64, -64);
					gnome_print_rgbaimage (gpmod->gpc, NR_PIXBLOCK_PX (&pb), 64, 64, pb.rs);
					gnome_print_grestore (gpmod->gpc);
					nr_pixblock_release (&pb);
				}
			}
			gnome_print_grestore (gpmod->gpc);
			sp_painter_free (painter);
		}
	}

	return 0;
}

static unsigned int
sp_module_print_gnome_stroke (SPModulePrint *mod, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
			      const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
	SPModulePrintGnome *gpmod;
	gdouble t[6];

	gpmod = (SPModulePrintGnome *) mod;

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
		gnome_print_setrgbcolor (gpmod->gpc, rgb[0], rgb[1], rgb[2]);

		/* fixme: */
		opacity = SP_SCALE24_TO_FLOAT (style->stroke_opacity.value) * SP_SCALE24_TO_FLOAT (style->opacity.value);
		gnome_print_setopacity (gpmod->gpc, opacity);

		if (style->stroke_dash.n_dash > 0) {
			gnome_print_setdash (gpmod->gpc, style->stroke_dash.n_dash, style->stroke_dash.dash, style->stroke_dash.offset);
		} else {
			gnome_print_setdash (gpmod->gpc, 0, NULL, 0.0);
		}

		gnome_print_setlinewidth (gpmod->gpc, style->stroke_width.computed);
		gnome_print_setlinejoin (gpmod->gpc, style->stroke_linejoin.computed);
		gnome_print_setlinecap (gpmod->gpc, style->stroke_linecap.computed);

		gnome_print_bpath (gpmod->gpc, bpath->path, FALSE);

		gnome_print_stroke (gpmod->gpc);
	}

	return 0;
}

static unsigned int
sp_module_print_gnome_image (SPModulePrint *mod, unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
			     const NRMatrixF *transform, const SPStyle *style)
{
	SPModulePrintGnome *gpmod;
	gdouble t[6];

	gpmod = (SPModulePrintGnome *) mod;

	t[0] = transform->c[0];
	t[1] = transform->c[1];
	t[2] = transform->c[2];
	t[3] = transform->c[3];
	t[4] = transform->c[4];
	t[5] = transform->c[5];

	gnome_print_gsave (gpmod->gpc);

	gnome_print_concat (gpmod->gpc, t);

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
		gnome_print_rgbaimage (gpmod->gpc, dpx, w, h, w * 4);
		g_free (dpx);
	} else {
		gnome_print_rgbaimage (gpmod->gpc, px, w, h, rs);
	}

	gnome_print_grestore (gpmod->gpc);

	return 0;
}

#else /* WITH_GNOME_PRINT */

/* Plain Print */

#define SP_TYPE_MODULE_PRINT_PLAIN (sp_module_print_plain_get_type())
#define SP_MODULE_PRINT_PLAIN(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_MODULE_PRINT_PLAIN, SPModulePrintPlain))
#define SP_IS_MODULE_PRINT_PLAIN(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_MODULE_PRINT_PLAIN))

typedef struct _SPModulePrintPlain SPModulePrintPlain;
typedef struct _SPModulePrintPlainClass SPModulePrintPlainClass;

#include <glib.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>

struct _SPModulePrintPlain {
	SPModulePrint module;
	FILE *stream;
};

struct _SPModulePrintPlainClass {
	SPModulePrintClass module_print_class;
};

unsigned int sp_module_print_plain_get_type (void);

static void sp_module_print_plain_class_init (SPModulePrintPlainClass *klass);
static void sp_module_print_plain_init (SPModulePrintPlain *fmod);
static void sp_module_print_plain_finalize (GObject *object);

static unsigned int sp_module_print_plain_setup (SPModulePrint *mod);
static unsigned int sp_module_print_plain_begin (SPModulePrint *mod, SPDocument *doc);
static unsigned int sp_module_print_plain_finish (SPModulePrint *mod);
static unsigned int sp_module_print_plain_bind (SPModulePrint *mod, const NRMatrixF *transform, float opacity);
static unsigned int sp_module_print_plain_release (SPModulePrint *mod);
static unsigned int sp_module_print_plain_fill (SPModulePrint *mod, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
						const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox);
static unsigned int sp_module_print_plain_stroke (SPModulePrint *mod, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
						  const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox);
static unsigned int sp_module_print_plain_image (SPModulePrint *mod, unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
						 const NRMatrixF *transform, const SPStyle *style);

static void sp_print_bpath (FILE *stream, const ArtBpath *bp);

static SPModulePrintClass *print_plain_parent_class;

unsigned int
sp_module_print_plain_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPModulePrintPlainClass),
			NULL, NULL,
			(GClassInitFunc) sp_module_print_plain_class_init,
			NULL, NULL,
			sizeof (SPModulePrintPlain),
			16,
			(GInstanceInitFunc) sp_module_print_plain_init,
		};
		type = g_type_register_static (SP_TYPE_MODULE_PRINT, "SPModulePrintPlain", &info, 0);
	}
	return type;
}

static void
sp_module_print_plain_class_init (SPModulePrintPlainClass *klass)
{
	GObjectClass *g_object_class;
	SPModulePrintClass *module_print_class;

	g_object_class = (GObjectClass *)klass;
	module_print_class = (SPModulePrintClass *) klass;

	print_plain_parent_class = g_type_class_peek_parent (klass);

	g_object_class->finalize = sp_module_print_plain_finalize;

	module_print_class->setup = sp_module_print_plain_setup;
	module_print_class->begin = sp_module_print_plain_begin;
	module_print_class->finish = sp_module_print_plain_finish;
	module_print_class->bind = sp_module_print_plain_bind;
	module_print_class->release = sp_module_print_plain_release;
	module_print_class->fill = sp_module_print_plain_fill;
	module_print_class->stroke = sp_module_print_plain_stroke;
	module_print_class->image = sp_module_print_plain_image;
}

static void
sp_module_print_plain_init (SPModulePrintPlain *fmod)
{
	/* Nothing here */
}

static void
sp_module_print_plain_finalize (GObject *object)
{
	SPModulePrintPlain *gpmod;

	gpmod = (SPModulePrintPlain *) object;
	
	if (gpmod->stream) fclose (gpmod->stream);

	G_OBJECT_CLASS (print_plain_parent_class)->finalize (object);
}

static unsigned int
sp_module_print_plain_setup (SPModulePrint *mod)
{
	SPModulePrintPlain *pmod;
	GtkWidget *dlg, *vbox, *f, *vb, *l, *e;
	int response;
	unsigned int ret;

	pmod = (SPModulePrintPlain *) mod;

	ret = FALSE;

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
				pmod->stream = osp;
			} else if (*fn == '>') {
				fn += 1;
				while (isspace (*fn)) fn += 1;
				osf = fopen (fn, "w+");
				pmod->stream = osf;
			} else {
				unsigned char *qn;
				qn = g_strdup_printf ("lpr %s", fn);
				osp = popen (qn, "w");
				g_free (qn);
				pmod->stream = osp;
			}
		}
		if (pmod->stream) ret = TRUE;
	}

	gtk_widget_destroy (dlg);

	return ret;
}

static unsigned int
sp_module_print_plain_begin (SPModulePrint *mod, SPDocument *doc)
{
	SPModulePrintPlain *pmod;
	int res;

	pmod = (SPModulePrintPlain *) mod;

	res = fprintf (pmod->stream, "%g %g translate\n", 0.0, sp_document_height (doc));
	if (res >= 0) res = fprintf (pmod->stream, "0.8 -0.8 scale\n");

	return res;
}

static unsigned int
sp_module_print_plain_finish (SPModulePrint *mod)
{
	int res;

	SPModulePrintPlain *pmod;

	pmod = (SPModulePrintPlain *) mod;

	res = fprintf (pmod->stream, "showpage\n");

	return res;
}

static unsigned int
sp_module_print_plain_bind (SPModulePrint *mod, const NRMatrixF *transform, float opacity)
{
	SPModulePrintPlain *pmod;

	pmod = (SPModulePrintPlain *) mod;

	if (!pmod->stream) return -1;

	return fprintf (pmod->stream, "gsave [%g %g %g %g %g %g] concat\n",
			transform->c[0], transform->c[1],
			transform->c[2], transform->c[3],
			transform->c[4], transform->c[5]);
}

static unsigned int
sp_module_print_plain_release (SPModulePrint *mod)
{
	SPModulePrintPlain *pmod;

	pmod = (SPModulePrintPlain *) mod;

	if (!pmod->stream) return -1;

	return fprintf (pmod->stream, "grestore\n");
}

static unsigned int
sp_module_print_plain_fill (SPModulePrint *mod, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
			    const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
	SPModulePrintPlain *pmod;

	pmod = (SPModulePrintPlain *) mod;

	if (!pmod->stream) return -1;

	if (style->fill.type == SP_PAINT_TYPE_COLOR) {
		float rgb[3];

		sp_color_get_rgb_floatv (&style->fill.value.color, rgb);

		fprintf (pmod->stream, "%g %g %g setrgbcolor\n", rgb[0], rgb[1], rgb[2]);

		sp_print_bpath (pmod->stream, bpath->path);

		if (style->fill_rule.value == SP_WIND_RULE_EVENODD) {
			fprintf (pmod->stream, "eofill\n");
		} else {
			fprintf (pmod->stream, "fill\n");
		}
	}

	return 0;
}

static unsigned int
sp_module_print_plain_stroke (SPModulePrint *mod, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
			      const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
	SPModulePrintPlain *pmod;

	pmod = (SPModulePrintPlain *) mod;

	if (!pmod->stream) return -1;

	if (style->stroke.type == SP_PAINT_TYPE_COLOR) {
		float rgb[3];

		sp_color_get_rgb_floatv (&style->stroke.value.color, rgb);

		fprintf (pmod->stream, "%g %g %g setrgbcolor\n", rgb[0], rgb[1], rgb[2]);

		sp_print_bpath (pmod->stream, bpath->path);

		if (style->stroke_dash.n_dash > 0) {
			int i;
			fprintf (pmod->stream, "[");
			for (i = 0; i < style->stroke_dash.n_dash; i++) {
				fprintf (pmod->stream, (i) ? " %g" : "%g", style->stroke_dash.dash[i]);
			}
			fprintf (pmod->stream, "] %g setdash\n", style->stroke_dash.offset);
		} else {
			fprintf (pmod->stream, "[] 0 setdash\n");
		}

		fprintf (pmod->stream, "%g setlinewidth\n", style->stroke_width.computed);
		fprintf (pmod->stream, "%d setlinejoin\n", style->stroke_linejoin.computed);
		fprintf (pmod->stream, "%d setlinecap\n", style->stroke_linecap.computed);

		fprintf (pmod->stream, "stroke\n");
	}

	return 0;
}

static unsigned int
sp_module_print_plain_image (SPModulePrint *mod, unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
			     const NRMatrixF *transform, const SPStyle *style)
{
	SPModulePrintPlain *pmod;
	int r;

	pmod = (SPModulePrintPlain *) mod;

	fprintf (pmod->stream, "gsave\n");
	fprintf (pmod->stream, "/rowdata %d string def\n", 3 * w);
	fprintf (pmod->stream, "[%g %g %g %g %g %g] concat\n",
		 transform->c[0],
		 transform->c[1],
		 transform->c[2],
		 transform->c[3],
		 transform->c[4],
		 transform->c[5]);
	fprintf (pmod->stream, "%d %d 8 [%d 0 0 -%d 0 %d]\n", w, h, w, h, h);
	fprintf (pmod->stream, "{currentfile rowdata readhexstring pop}\n");
	fprintf (pmod->stream, "false 3 colorimage\n");

	for (r = 0; r < h; r++) {
		unsigned char *s;
		int c0, c1, c;
		s = px + r * rs;
		for (c0 = 0; c0 < w; c0 += 24) {
			c1 = MIN (w, c0 + 24);
			for (c = c0; c < c1; c++) {
				static const char xtab[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
				fputc (xtab[s[0] >> 4], pmod->stream);
				fputc (xtab[s[0] & 0xf], pmod->stream);
				fputc (xtab[s[1] >> 4], pmod->stream);
				fputc (xtab[s[1] & 0xf], pmod->stream);
				fputc (xtab[s[2] >> 4], pmod->stream);
				fputc (xtab[s[2] & 0xf], pmod->stream);
				s += 4;
			}
			fputs ("\n", pmod->stream);
		}
	}

	fprintf (pmod->stream, "grestore\n");

	return 0;
}

#endif /* WITH_GNOME_PRINT */

#ifdef WITH_KDE
#include "helper/kde.h"
#endif

#include "display/nr-arena.h"
#include "display/nr-arena-item.h"

/* fixme: Sooner or later we want SPPrintContext subclasses (Lauris) */

/* UI */

void
sp_print_preview_document (SPDocument *doc)
{
#ifdef lalalaWITH_GNOME_PRINT
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
	SPModulePrint *mod;
	unsigned int ret;

#ifdef WITH_KDE
	mod = sp_kde_get_module_print ();
#else
#ifdef WITH_GNOME_PRINT
	mod = g_object_new (SP_TYPE_MODULE_PRINT_GNOME, NULL);
#else
	mod = g_object_new (SP_TYPE_MODULE_PRINT_PLAIN, NULL);
#endif
#endif

	/* fixme: This has to go into module constructor somehow */
	/* Create new arena */
	mod->base = SP_ITEM (sp_document_root (doc));
	mod->arena = g_object_new (NR_TYPE_ARENA, NULL);
	mod->dkey = sp_item_display_key_new (1);
	mod->root = sp_item_invoke_show (mod->base, mod->arena, mod->dkey);

	if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->setup)
		ret = ((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->setup (mod);

	if (ret) {
		sp_document_ensure_up_to_date (doc);
		/* Print document */
		if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->begin)
			ret = ((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->begin (mod, doc);
		sp_item_invoke_print (mod->base, (SPPrintContext *) mod);
		if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->finish)
			ret = ((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->finish (mod);
	}

	sp_item_invoke_hide (mod->base, mod->dkey);
	mod->base = NULL;
	nr_arena_item_unref (mod->root);
	mod->root = NULL;
	g_object_unref (G_OBJECT (mod->arena));
	mod->arena = NULL;

	g_object_unref (G_OBJECT (mod));
}

void
sp_print_document_to_file (SPDocument *doc, const unsigned char *filename)
{
#ifdef lalala
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
#endif
}

#ifndef WITH_GNOME_PRINT

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





#define __SP_PS_C__

/*
 * PostScript printing
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

/* Plain Print */

#include <config.h>

#include <ctype.h>

#include <glib.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>

#include "helper/sp-intl.h"
#include "enums.h"
#include "document.h"
#include "style.h"

#include "ps.h"

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
#ifndef WIN32
				osp = popen (fn, "w");
#else
				osp = _popen (fn, "w");
#endif
				pmod->stream = osp;
			} else if (*fn == '>') {
				fn += 1;
				while (isspace (*fn)) fn += 1;
				osf = fopen (fn, "w+");
				pmod->stream = osf;
			} else {
				unsigned char *qn;
				qn = g_strdup_printf ("lpr %s", fn);
#ifndef WIN32
				osp = popen (qn, "w");
#else
				osp = _popen (qn, "w");
#endif
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

	res = fprintf (pmod->stream, "%%!\n");
	if (res >= 0) res = fprintf (pmod->stream, "%g %g translate\n", 0.0, sp_document_height (doc));
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

#if 0
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






#define __SP_MODULE_WIN32_C__

/*
 * Windows stuff
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <config.h>

#include <libnr/nr-macros.h>
#include <libnr/nr-matrix.h>

#include "display/nr-arena-item.h"
#include "display/nr-arena.h"
#include "document.h"

#include "win32.h"

static void sp_module_print_win32_class_init (SPModulePrintWin32Class *klass);
static void sp_module_print_win32_init (SPModulePrintWin32 *pmod);
static void sp_module_print_win32_finalize (GObject *object);

static unsigned int sp_module_print_win32_setup (SPModulePrint *mod);
static unsigned int sp_module_print_win32_begin (SPModulePrint *mod, SPDocument *doc);
static unsigned int sp_module_print_win32_finish (SPModulePrint *mod);

static SPModulePrintClass *print_win32_parent_class;

unsigned int
sp_module_print_win32_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPModulePrintWin32Class),
			NULL, NULL,
			(GClassInitFunc) sp_module_print_win32_class_init,
			NULL, NULL,
			sizeof (SPModulePrintWin32),
			16,
			(GInstanceInitFunc) sp_module_print_win32_init,
		};
		type = g_type_register_static (SP_TYPE_MODULE_PRINT, "SPModulePrintWin32", &info, 0);
	}
	return type;
}

static void
sp_module_print_win32_class_init (SPModulePrintWin32Class *klass)
{
	GObjectClass *g_object_class;
	SPModulePrintClass *module_print_class;

	g_object_class = (GObjectClass *)klass;
	module_print_class = (SPModulePrintClass *) klass;

	print_win32_parent_class = g_type_class_peek_parent (klass);

	g_object_class->finalize = sp_module_print_win32_finalize;

	module_print_class->setup = sp_module_print_win32_setup;
	module_print_class->begin = sp_module_print_win32_begin;
	module_print_class->finish = sp_module_print_win32_finish;

#if 0
	module_print_class->bind = sp_module_print_gnome_bind;
	module_print_class->release = sp_module_print_gnome_release;
	module_print_class->fill = sp_module_print_gnome_fill;
	module_print_class->stroke = sp_module_print_gnome_stroke;
	module_print_class->image = sp_module_print_gnome_image;
#endif
}

static void
sp_module_print_win32_init (SPModulePrintWin32 *pmod)
{
	/* Nothing here */
}

static void
sp_module_print_win32_finalize (GObject *object)
{
	SPModulePrintWin32 *w32mod;

	w32mod = (SPModulePrintWin32 *) object;
	
	DeleteDC (w32mod->hDC);

	G_OBJECT_CLASS (print_win32_parent_class)->finalize (object);
}

static unsigned int
sp_module_print_win32_setup (SPModulePrint *mod)
{
	SPModulePrintWin32 *w32mod;
	HRESULT res;
	PRINTDLG pd = {
		sizeof (PRINTDLG),
		NULL, /* hwndOwner */
		NULL, /* hDevMode */
		NULL, /* hDevNames */
		NULL, /* hDC */
		PD_NOPAGENUMS | PD_NOSELECTION | PD_RETURNDC, /* Flags */
		1, 1, 1, 1, /* nFromPage, nToPage, nMinPage, nMaxPage */
		1, /* nCoies */
		NULL, /* hInstance */
		0, /* lCustData */
		NULL, NULL, NULL, NULL, NULL, NULL
	};

	w32mod = (SPModulePrintWin32 *) mod;

	res = PrintDlg (&pd);
	if (!res) return FALSE;

	w32mod->hDC = pd.hDC;

	return TRUE;
}

static unsigned int
sp_module_print_win32_begin (SPModulePrint *mod, SPDocument *doc)
{
	SPModulePrintWin32 *w32mod;
	DOCINFO di = {
		sizeof (DOCINFO),
		NULL, /* lpszDocName */
		NULL, /* lpszOutput */
		NULL, /* lpszDatatype */
		DI_APPBANDING /* fwType */
	};
	int res;

	w32mod = (SPModulePrintWin32 *) mod;

	w32mod->width = sp_document_width (doc);
	w32mod->height = sp_document_height (doc);

	di.lpszDocName = SP_DOCUMENT_NAME (doc);

	res = StartDoc (w32mod->hDC, &di);

	res = StartPage (w32mod->hDC);

	return 0;
}

static unsigned int
sp_module_print_win32_finish (SPModulePrint *mod)
{
	SPModulePrintWin32 *w32mod;
	float dpiX, dpiY;
	float scaleh, scalew;
	int pWidth, pHeight;
	int width, height;
	float x0, y0, x1, y1;
	NRMatrixF affine;
	unsigned char *px;
	int sheight, row;
	BITMAPINFO bmInfo = {
		{
			sizeof (BITMAPINFOHEADER), // bV4Size 
			64, /* biWidth */
			-64, /* biHeight */
			1, /* biPlanes */
			32, /* biBitCount */
			BI_RGB, /* biCompression */
			0, /* biSizeImage */
			2835, /* biXPelsPerMeter */
			2835, /* biYPelsPerMeter */
			0, /* biClrUsed */
			0, /* biClrImportant */
		},
		{0, 0, 0, 0} /* bmiColors */
	};
	int res;

	w32mod = (SPModulePrintWin32 *) mod;

	x0 = 0.0F;
	y0 = 0.0F;
	x1 = w32mod->width;
	y1 = w32mod->height;

	dpiX = (float) GetDeviceCaps (w32mod->hDC, LOGPIXELSX);
	dpiY = (float) GetDeviceCaps (w32mod->hDC, LOGPIXELSY);
	pWidth = GetDeviceCaps (w32mod->hDC, HORZRES); 
	pHeight = GetDeviceCaps (w32mod->hDC, VERTRES); 

	scaleh = dpiX / 72.0;
	scalew = dpiY / 72.0;

	width = w32mod->width * scaleh;
	height = w32mod->height * scalew;
	width = MIN (width, pWidth);
	height = MIN (height, pHeight);

	/*
	 * 1) a[0] * x0 + a[2] * y1 + a[4] = 0.0
	 * 2) a[1] * x0 + a[3] * y1 + a[5] = 0.0
	 * 3) a[0] * x1 + a[2] * y1 + a[4] = width
	 * 4) a[1] * x0 + a[3] * y0 + a[5] = height
	 * 5) a[1] = 0.0;
	 * 6) a[2] = 0.0;
	 *
	 * (1,3) a[0] * x1 - a[0] * x0 = width
	 * a[0] = width / (x1 - x0)
	 * (2,4) a[3] * y0 - a[3] * y1 = height
	 * a[3] = height / (y0 - y1)
	 * (1) a[4] = -a[0] * x0
	 * (2) a[5] = -a[3] * y1
	 */

	affine.c[0] = width / ((x1 - x0) * 1.25);
	affine.c[1] = 0.0;
	affine.c[2] = 0.0;
	affine.c[3] = height / ((y1 - y0) * 1.25);
	affine.c[4] = -affine.c[0] * x0 * 1.25;
	affine.c[5] = -affine.c[3] * y0 * 1.25;

	nr_arena_item_set_transform (mod->root, &affine);

	if ((width < 256) || ((width * height) < 32768)) {
		px = nr_pixelstore_64K_new (FALSE, 0);
		sheight = 65536 / (4 * width);
	} else {
		px = nr_new (unsigned char, 4 * 64 * width);
		sheight = 64;
	}

	/* Printing goes here */
	for (row = 0; row < height; row += sheight) {
		NRPixBlock pb;
		NRRectL bbox;
		NRGC gc;
		int num_rows;

		num_rows = sheight;
		if ((row + num_rows) > height) num_rows = height - row;

		/* Set area of interest */
		bbox.x0 = 0;
		bbox.y0 = row;
		bbox.x1 = width;
		bbox.y1 = row + num_rows;
		/* Update to renderable state */
		nr_matrix_d_set_identity (&gc.transform);
		nr_arena_item_invoke_update (mod->root, &bbox, &gc, NR_ARENA_ITEM_STATE_ALL, NR_ARENA_ITEM_STATE_NONE);

		nr_pixblock_setup_extern (&pb, NR_PIXBLOCK_MODE_R8G8B8A8N, bbox.x0, bbox.y0, bbox.x1, bbox.y1, px, 4 * width, FALSE, FALSE);
		memset (px, 0xff, 4 * num_rows * width);

		/* Render */
		nr_arena_item_invoke_render (mod->root, &bbox, &pb, 0);

		/* Blitter goes here */
		bmInfo.bmiHeader.biWidth = width;
		bmInfo.bmiHeader.biHeight = -num_rows;

		res = SetDIBitsToDevice (w32mod->hDC,
						   0, 0, width, num_rows,
						   0, 0, 0, num_rows, px, &bmInfo, DIB_RGB_COLORS);

		/* Blitter ends here */

		nr_pixblock_release (&pb);
	}

	if ((width < 256) || ((width * height) < 32768)) {
		nr_pixelstore_64K_free (px);
	} else {
		nr_free (px);
	}

	res = EndPage (w32mod->hDC);
	res = EndDoc (w32mod->hDC);

	return 0;
}

#if 0

#if 0
/* Gnome Print */

static unsigned int sp_module_print_gnome_bind (SPModulePrint *mod, const NRMatrixF *transform, float opacity);
static unsigned int sp_module_print_gnome_release (SPModulePrint *mod);
static unsigned int sp_module_print_gnome_fill (SPModulePrint *mod, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
						const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox);
static unsigned int sp_module_print_gnome_stroke (SPModulePrint *mod, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
						  const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox);
static unsigned int sp_module_print_gnome_image (SPModulePrint *mod, unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
						 const NRMatrixF *transform, const SPStyle *style);

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
	mod->arena = (NRArena *) nr_object_new (NR_TYPE_ARENA);
	mod->dkey = sp_item_display_key_new (1);
	mod->root = sp_item_invoke_show (mod->base, mod->arena, mod->dkey, SP_ITEM_SHOW_PRINT);

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
	nr_object_unref ((NRObject *) mod->arena);
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

#endif

#define __SP_PLAIN_C__

/*
 * Plain printing subsystem
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *
 * This code is in public domain
 *
 */

/* Plain Print */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>

#include <libarikkei/arikkei-strlib.h>


#include <libnr/nr-macros.h>
#include <libnr/nr-matrix.h>

#include <glib.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkcombo.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>
#include <gtk/gtktooltips.h>

#include "helper/sp-intl.h"
#include "display/nr-arena-item.h"
#include "enums.h"
#include "document.h"
#include "style.h"

#include "plain.h"

static void sp_module_print_plain_class_init (SPModulePrintPlainClass *klass);
static void sp_module_print_plain_init (SPModulePrintPlain *fmod);
static void sp_module_print_plain_finalize (GObject *object);

static SPRepr *sp_module_print_plain_write (SPModule *module, SPRepr *root);

static unsigned int sp_module_print_plain_setup (SPModulePrint *mod);
static unsigned int private_sp_module_print_plain_setup_output (SPModulePrintPlain *pmod, const unsigned char *filename);
static unsigned int sp_module_print_plain_setup_file (SPModulePrint *mod, const unsigned char *filename);
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

static SPModulePrintClass *print_plain_parent_class;

GType
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
	SPModuleClass *module_class;
	SPModulePrintClass *module_print_class;

	g_object_class = (GObjectClass *) klass;
	module_class = (SPModuleClass *) klass;
	module_print_class = (SPModulePrintClass *) klass;

	print_plain_parent_class = g_type_class_peek_parent (klass);

	g_object_class->finalize = sp_module_print_plain_finalize;

	module_class->write = sp_module_print_plain_write;

	module_print_class->setup = sp_module_print_plain_setup;
	module_print_class->setup_file = sp_module_print_plain_setup_file;
	module_print_class->begin = sp_module_print_plain_begin;
	module_print_class->finish = sp_module_print_plain_finish;
	module_print_class->bind = sp_module_print_plain_bind;
	module_print_class->release = sp_module_print_plain_release;
	module_print_class->fill = sp_module_print_plain_fill;
	module_print_class->stroke = sp_module_print_plain_stroke;
	module_print_class->image = sp_module_print_plain_image;
}

static void
sp_module_print_plain_init (SPModulePrintPlain *pmod)
{
	pmod->dpi = 72;
	pmod->driver_type = SP_PRINT_PLAIN_DRIVER_PS_DEFAULT;
	pmod->driver = NULL;
	pmod->stream = NULL;
}

static void
sp_module_print_plain_finalize (GObject *object)
{
	SPModulePrintPlain *gpmod;

	gpmod = (SPModulePrintPlain *) object;
	
	/* fixme: should really use pclose for popen'd streams */
	if (gpmod->stream) fclose (gpmod->stream);

#ifndef WIN32
	/* restore default signal handling for SIGPIPE */
	(void) signal (SIGPIPE, SIG_DFL);
#endif

	G_OBJECT_CLASS (print_plain_parent_class)->finalize (object);
}

static SPRepr *
sp_module_print_plain_write (SPModule *module, SPRepr *root)
{
	SPRepr *grp, *repr;
	grp = sp_repr_lookup_child (root, "id", "printing");
	if (!grp) {
		grp = sp_repr_new ("group");
		sp_repr_set_attr (grp, "id", "printing");
		sp_repr_set_attr (grp, "name", "Printing");
		sp_repr_append_child (root, grp);
	}
	repr = sp_repr_new ("module");
	sp_repr_set_attr (repr, "id", "plain");
	sp_repr_set_attr (repr, "name", "Plain Printing");
	sp_repr_set_attr (repr, "action", "no");
/* 	sp_repr_set_attr (repr, "driver", "ps"); */
	sp_repr_append_child (grp, repr);
	return repr;
}

struct _SPPlainDriver {
	unsigned int type;
	const unsigned char *name;
	const unsigned char *label;
	const unsigned char *tips;
};

static const struct _SPPlainDriver drivers[] = {
	{ SP_PRINT_PLAIN_DRIVER_PS_DEFAULT,
	  "ps",
	  N_("Print using PostScript operators"),
	  N_("Use PostScript vector operators, resulting image will be (usually) smaller "
		 "and can be arbitrarily scaled, but alpha transparency, "
		 "markers and patterns will be lost")},
	{ SP_PRINT_PLAIN_DRIVER_PS_BITMAP,
	  "ps_bitmap",
	  N_("Print as bitmap"),
	  N_("Print everything as bitmap, resulting image will be (usualy) larger "
		 "and it quality depends on zoom factor, but all graphics "
		 "will be rendered identical to display")},
	{ SP_PRINT_PLAIN_DRIVER_PDF_DEFAULT,
	  "pdf",
	  N_("Print using PDF driver"),
	  N_("Use Portable Document Format for printing"),
	},
/*	{ SP_PRINT_PLAIN_DRIVER_PDF_BITMAP,
	  "pdf_bitmap",
	  N_("Print using rasterized PDF driver"),
	  N_("Use Portable Document Format for printing, but everything as bitmap"),
	},
*/
};
static unsigned int n_drivers = sizeof(drivers)/sizeof(struct _SPPlainDriver);

static unsigned int
sp_module_print_plain_setup (SPModulePrint *mod)
{
	static const guchar *pdr[] = {"72", "75", "100", "144", "150", "200", "300", "360", "600", "1200", "2400", NULL};
	SPModulePrintPlain *pmod;
	GtkWidget *dlg, *vbox, *f, *vb, *rb, *rbs[n_drivers], *hb, *combo, *l, *e;
	GtkTooltips *tt;
	GList *sl;
	int i;
	int response;
	unsigned int ret;
	SPRepr *repr;

	pmod = (SPModulePrintPlain *) mod;
	repr = ((SPModule *) mod)->repr;

	ret = FALSE;

	/* Create dialog */
	tt = gtk_tooltips_new ();
	g_object_ref ((GObject *) tt);
	gtk_object_sink ((GtkObject *) tt);
	dlg = gtk_dialog_new_with_buttons (_("Print destination"), NULL,
					   GTK_DIALOG_MODAL,
					   GTK_STOCK_PRINT,
					   GTK_RESPONSE_OK,
					   GTK_STOCK_CANCEL,
					   GTK_RESPONSE_CANCEL,
					   NULL);

	vbox = GTK_DIALOG (dlg)->vbox;
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
	/* Print properties frame */
	f = gtk_frame_new (_("Print properties"));
	gtk_box_pack_start (GTK_BOX (vbox), f, FALSE, FALSE, 4);
	vb = gtk_vbox_new (FALSE, 4);
	gtk_container_add (GTK_CONTAINER (f), vb);
	gtk_container_set_border_width (GTK_CONTAINER (vb), 4);
	/* Print type */
	if (repr) {
		const unsigned char *driver_name;
		driver_name = sp_repr_get_attr (repr, "driver");
		if (! driver_name) driver_name = "ps";
		for (i = 0; i < n_drivers; i++) {
			if (! strcmp (drivers[i].name, driver_name)) {
				pmod->driver_type = drivers[i].type;
				printf ("plain_setup: set driver_type = %d, driver_name = %s\n", pmod->driver_type, driver_name);
				break;
			}
		}
	}
	rb = NULL;
	for (i = 0; i < n_drivers; i++) {
		GSList *group;
		group = rb ? gtk_radio_button_get_group ((GtkRadioButton *) rb) : NULL;
		rbs[i] = gtk_radio_button_new_with_label (group, drivers[i].label);
		gtk_tooltips_set_tip ((GtkTooltips *) tt, rbs[i], drivers[i].tips, NULL);
		if (pmod->driver_type == drivers[i].type) {
			gtk_toggle_button_set_active ((GtkToggleButton *) rbs[i], TRUE);
		}
		gtk_box_pack_start (GTK_BOX (vb), rbs[i], FALSE, FALSE, 0);
		rb = rbs[i];
	}
	/* Resolution */
	hb = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 0);
	combo = gtk_combo_new ();
	gtk_combo_set_value_in_list (GTK_COMBO (combo), FALSE, FALSE);
	gtk_combo_set_use_arrows (GTK_COMBO (combo), TRUE);
	gtk_combo_set_use_arrows_always (GTK_COMBO (combo), TRUE);
	gtk_widget_set_usize (combo, 64, -1);
	gtk_tooltips_set_tip ((GtkTooltips *) tt, GTK_COMBO (combo)->entry,
						  _("Preferred resolution (dots per inch) of bitmap"), NULL);
	/* Setup strings */
	sl = NULL;
	for (i = 0; pdr[i] != NULL; i++) {
		sl = g_list_prepend (sl, (gpointer) pdr[i]);
	}
	sl = g_list_reverse (sl);
	gtk_combo_set_popdown_strings (GTK_COMBO (combo), sl);
	g_list_free (sl);
	if (repr) {
		const unsigned char *val;
		val = sp_repr_get_attr (repr, "resolution");
		if (val) gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), val);
	}
	gtk_box_pack_end (GTK_BOX (hb), combo, FALSE, FALSE, 0);
	l = gtk_label_new (_("Resolution:"));
	gtk_box_pack_end (GTK_BOX (hb), l, FALSE, FALSE, 0);

	/* Print destination frame */
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
	if (repr && sp_repr_get_attr (repr, "destination")) {
		const unsigned char *val;
		val = sp_repr_get_attr (repr, "destination");
		if (val) gtk_entry_set_text (GTK_ENTRY (e), val);
	} else {
		gtk_entry_set_text (GTK_ENTRY (e), "lp");
	}
	gtk_box_pack_start (GTK_BOX (vb), e, FALSE, FALSE, 0);

	gtk_widget_show_all (vbox);
	
	response = gtk_dialog_run (GTK_DIALOG (dlg));

	g_object_unref ((GObject *) tt);

	if (response == GTK_RESPONSE_OK) {
		const unsigned char *fn;
		const char *sstr;

		for (i = 0; i < n_drivers; i++) {
			if (gtk_toggle_button_get_active ((GtkToggleButton *) rbs[i])) {
				pmod->driver_type = drivers[i].type;
				printf ("plain_setup: set driver_type = %d, driver_name = %s\n", pmod->driver_type, drivers[i].name);
				break;
			}
		}
		sstr = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (combo)->entry));
		pmod->dpi = MAX (atof (sstr), 1);
		/* Arrgh, have to do something */
		fn = gtk_entry_get_text (GTK_ENTRY (e));
		/* g_print ("Printing to %s\n", fn); */
		if (repr) {
			switch (pmod->driver_type) {
			case SP_PRINT_PLAIN_DRIVER_PS_BITMAP:
				printf ("plain_setup: DRIVER_PS_BITMAP\n");
				sp_repr_set_attr (repr, "driver", "ps_bitmap");
				break;
			case SP_PRINT_PLAIN_DRIVER_PDF_DEFAULT:
				printf ("plain_setup: DRIVER_PDF_DEFAULT\n");
				sp_repr_set_attr (repr, "driver", "pdf");
				break;
			case SP_PRINT_PLAIN_DRIVER_PDF_BITMAP:
				printf ("plain_setup: DRIVER_PDF_BITMAP\n");
				sp_repr_set_attr (repr, "driver", "pdf_bitmap");
				break;
			case SP_PRINT_PLAIN_DRIVER_PS_DEFAULT:
				printf ("plain_setup: DRIVER_PS_DEFAULT\n");
				sp_repr_set_attr (repr, "driver", "ps");
				break;
			default:
				g_warning ("sp_module_print_plain_setup: Invalid driver type: %d, fall back to PS\n", pmod->driver_type);
				sp_repr_set_attr (repr, "driver", "ps");
				break;
			}
			
			sp_repr_set_attr (repr, "resolution", sstr);
			sp_repr_set_attr (repr, "destination", fn);
		}
		ret = private_sp_module_print_plain_setup_output (pmod, fn);
		pmod->driver = sp_print_plain_driver_new (pmod->driver_type, pmod);
	}

	gtk_widget_destroy (dlg);

	return ret;
}

static unsigned int
sp_module_print_plain_setup_file (SPModulePrint *mod, const unsigned char *filename)
{
	SPModulePrintPlain *pmod;
	unsigned int ret;
	/* SPRepr *repr; */
	char sstr[16];
	
	ret = FALSE;
	pmod = (SPModulePrintPlain *) mod;
	/* repr = ((SPModule *) mod)->repr; */
	
	pmod->dpi = 300;
	sprintf (sstr, "%d", pmod->dpi);
	
	ret = private_sp_module_print_plain_setup_output (pmod, filename);

	return ret;
}

static unsigned int
private_sp_module_print_plain_setup_output (SPModulePrintPlain *pmod, const unsigned char *filename)
{
	FILE *osf, *osp;
	unsigned int ret;
	
	ret = FALSE;
	osf = NULL;
	osp = NULL;
	if (filename) {
		if (*filename == '|') {
			filename += 1;
			while (isspace (*filename)) filename += 1;
#ifndef WIN32
			osp = popen (filename, "w");
#else
			osp = _popen (filename, "w");
#endif
			pmod->stream = osp;
		} else if (*filename == '>') {
			filename += 1;
			while (isspace (*filename)) filename += 1;
			osf = fopen (filename, "w+");
			pmod->stream = osf;
		} else {
			unsigned char *qn;
			qn = g_strdup_printf ("lpr -P %s", filename);
#ifndef WIN32
			osp = popen (qn, "w");
#else
			osp = _popen (qn, "w");
#endif
			g_free (qn);
			pmod->stream = osp;
		}
	}
	if (pmod->stream) {
#ifndef WIN32
		/* fixme: this is kinda icky */
		(void) signal(SIGPIPE, SIG_IGN);
#endif
		ret = TRUE;
	}
	
	return ret;
}

static unsigned int
sp_module_print_plain_begin (SPModulePrint *mod, SPDocument *doc)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainDriver *driver;
	int res;

	pmod = (SPModulePrintPlain *) mod;
	driver = pmod->driver;

	printf ("plain_begin: driver = %p, driver->begin = %p\n", driver, driver->begin);
	if (driver->begin)
		driver->begin (driver, doc);

	return res;
}

static unsigned int
sp_module_print_plain_finish (SPModulePrint *mod)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainDriver *driver;
	int res;

	pmod = (SPModulePrintPlain *) mod;
	driver = pmod->driver;
	
	printf ("plain_finish: driver->finish = %p\n", driver->finish);
	if (!pmod->stream) return 0;

	if (driver->finish)
		res = driver->finish (driver);

	return res;
}

static unsigned int
sp_module_print_plain_bind (SPModulePrint *mod, const NRMatrixF *transform, float opacity)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainDriver *driver;

	pmod = (SPModulePrintPlain *) mod;
	driver = pmod->driver;

	printf ("plain_bind: driver->bind = %p\n", driver->bind);
	if (!pmod->stream) return -1;

	return driver->bind (driver, transform, opacity);
}

static unsigned int
sp_module_print_plain_release (SPModulePrint *mod)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainDriver *driver;

	pmod = (SPModulePrintPlain *) mod;
	driver = pmod->driver;

	printf ("plain_release\n");
	if (!pmod->stream) return -1;

	return driver->release (driver);
}

static unsigned int
sp_module_print_plain_fill (SPModulePrint *mod, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
			    const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainDriver *driver;

	pmod = (SPModulePrintPlain *) mod;
	driver = pmod->driver;

	if (!pmod->stream) return -1;

	if (driver->fill)
		driver->fill (driver, bpath, ctm, style, pbox, dbox, bbox);

	return 0;
}

static unsigned int
sp_module_print_plain_stroke (SPModulePrint *mod, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
			      const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainDriver *driver;

	pmod = (SPModulePrintPlain *) mod;
	driver = pmod->driver;

	if (!pmod->stream) return -1;

	if (driver->stroke)
		driver->stroke (driver, bpath, ctm, style, pbox, dbox, bbox);

	return 0;
}

static unsigned int
sp_module_print_plain_image (SPModulePrint *mod, unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
			     const NRMatrixF *transform, const SPStyle *style)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainDriver *driver;
	unsigned int res;

	pmod = (SPModulePrintPlain *) mod;
	driver = pmod->driver;

	if (!pmod->stream) return -1;

	if (driver->image)
		res = driver->image (driver, px, w, h, rs, transform, style);

	return res;
}

/* End of GNU GPL code */


#include "modules/plain-ps.h"
#include "modules/plain-pdf.h"

SPPrintPlainDriver *
sp_print_plain_driver_new (SPPrintPlainDriverType type, SPModulePrintPlain *module)
{
	SPPrintPlainDriver *driver;
	
	switch (type) {
	case SP_PRINT_PLAIN_DRIVER_PS_DEFAULT:
		printf ("driver_new: PS_DEFAULT\n");
		driver = sp_plain_ps_driver_new (SP_PLAIN_PS_DEFAULT, module);
		break;
	case SP_PRINT_PLAIN_DRIVER_PS_BITMAP:
		printf ("driver_new: PS_BITMAP\n");
		driver = sp_plain_ps_driver_new (SP_PLAIN_PS_BITMAP, module);
		break;
	case SP_PRINT_PLAIN_DRIVER_PDF_DEFAULT:
		printf ("driver_new: PDF_DEFAULT\n");
		driver = sp_plain_pdf_driver_new (SP_PLAIN_PDF_DEFAULT, module);
		break;
	case SP_PRINT_PLAIN_DRIVER_PDF_BITMAP:
		printf ("driver_new: PDF_BITMAP\n");
		driver = sp_plain_ps_driver_new (SP_PLAIN_PDF_BITMAP, module);
		break;
	default:
		assert (0);
	}

	if (driver->initialize)
		driver->initialize (driver);

	return driver;
}

SPPrintPlainDriver *
sp_print_plain_driver_delete (SPPrintPlainDriver *driver)
{
	printf ("driver_delete\n");

	if (driver) {
		if (driver->finalize)
			driver->finalize (driver);
		free (driver);
	}
	return NULL;
}

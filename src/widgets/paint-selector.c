#define __SP_PAINT_SELECTOR_C__

/*
 * SPPaintSelector
 *
 * Generic paint selector widget
 *
 * Copyright (C) Lauris 2002
 *
 */

#define noSPPS_PREVIEW

#include <config.h>

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <libnr/nr-values.h>
#include <libnr/nr-matrix.h>

#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>

#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-util.h>

#include <libgnomeui/gnome-stock.h>

#include "../sp-item.h"
#include "../sp-gradient.h"

#include "sp-color-selector.h"
/* fixme: Move it from dialogs to here */
#include "gradient-selector.h"

#include "paint-selector.h"

enum {
	MODE_CHANGED,
	GRABBED,
	DRAGGED,
	RELEASED,
	CHANGED,
	LAST_SIGNAL
};

static void sp_paint_selector_class_init (SPPaintSelectorClass *klass);
static void sp_paint_selector_init (SPPaintSelector *slider);
static void sp_paint_selector_destroy (GtkObject *object);

static GtkWidget *sp_paint_selector_style_button_add (SPPaintSelector *psel, const guchar *px, SPPaintSelectorMode mode, GtkRadioButton *last);
static void sp_paint_selector_style_button_toggled (GtkToggleButton *tb, SPPaintSelector *psel);

static void sp_paint_selector_set_mode_empty (SPPaintSelector *psel);
static void sp_paint_selector_set_mode_multiple (SPPaintSelector *psel);
static void sp_paint_selector_set_mode_none (SPPaintSelector *psel);
static void sp_paint_selector_set_mode_color (SPPaintSelector *psel, SPPaintSelectorMode mode);
static void sp_paint_selector_set_mode_gradient (SPPaintSelector *psel, SPPaintSelectorMode mode);
static void sp_paint_selector_set_mode_pattern (SPPaintSelector *psel);
static void sp_paint_selector_set_mode_fractal (SPPaintSelector *psel);

static void sp_paint_selector_set_style_buttons (SPPaintSelector *psel, GtkWidget *active);

static GtkVBoxClass *parent_class;
static guint psel_signals[LAST_SIGNAL] = {0};

GtkType
sp_paint_selector_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPPaintSelector",
			sizeof (SPPaintSelector),
			sizeof (SPPaintSelectorClass),
			(GtkClassInitFunc) sp_paint_selector_class_init,
			(GtkObjectInitFunc) sp_paint_selector_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_VBOX, &info);
	}
	return type;
}

static void
sp_paint_selector_class_init (SPPaintSelectorClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	parent_class = gtk_type_class (GTK_TYPE_VBOX);

	psel_signals[MODE_CHANGED] = gtk_signal_new ("mode_changed",
						 GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						 object_class->type,
						 GTK_SIGNAL_OFFSET (SPPaintSelectorClass, mode_changed),
						 gtk_marshal_NONE__UINT,
						 GTK_TYPE_NONE, 1, GTK_TYPE_UINT);
	psel_signals[GRABBED] =  gtk_signal_new ("grabbed",
						 GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						 object_class->type,
						 GTK_SIGNAL_OFFSET (SPPaintSelectorClass, grabbed),
						 gtk_marshal_NONE__NONE,
						 GTK_TYPE_NONE, 0);
	psel_signals[DRAGGED] =  gtk_signal_new ("dragged",
						 GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						 object_class->type,
						 GTK_SIGNAL_OFFSET (SPPaintSelectorClass, dragged),
						 gtk_marshal_NONE__NONE,
						 GTK_TYPE_NONE, 0);
	psel_signals[RELEASED] = gtk_signal_new ("released",
						 GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						 object_class->type,
						 GTK_SIGNAL_OFFSET (SPPaintSelectorClass, released),
						 gtk_marshal_NONE__NONE,
						 GTK_TYPE_NONE, 0);
	psel_signals[CHANGED] =  gtk_signal_new ("changed",
						 GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						 object_class->type,
						 GTK_SIGNAL_OFFSET (SPPaintSelectorClass, changed),
						 gtk_marshal_NONE__NONE,
						 GTK_TYPE_NONE, 0);
	gtk_object_class_add_signals (object_class, psel_signals, LAST_SIGNAL);

	object_class->destroy = sp_paint_selector_destroy;
}

#define XPAD 4
#define YPAD 1

static void
sp_paint_selector_init (SPPaintSelector *psel)
{
	GtkWidget *w;

	psel->mode = -1;

	/* Paint style button box */
	psel->style = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (psel->style);
	gtk_container_set_border_width (GTK_CONTAINER (psel->style), 4);
	gtk_box_pack_start (GTK_BOX (psel), psel->style, FALSE, FALSE, 0);

	/* Buttons */
	psel->none = sp_paint_selector_style_button_add (psel, "fill_none.xpm",
							 SP_PAINT_SELECTOR_MODE_NONE, NULL);
	psel->solid = sp_paint_selector_style_button_add (psel, "fill_solid.xpm",
							  SP_PAINT_SELECTOR_MODE_COLOR_RGB, GTK_RADIO_BUTTON (psel->none));
	psel->gradient = sp_paint_selector_style_button_add (psel, "fill_gradient.xpm",
							     SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR, GTK_RADIO_BUTTON (psel->solid));
	psel->radial = sp_paint_selector_style_button_add (psel, "fill_radial.xpm",
							   SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL, GTK_RADIO_BUTTON (psel->gradient));
	psel->pattern = sp_paint_selector_style_button_add (psel, "fill_pattern.xpm",
							    SP_PAINT_SELECTOR_MODE_PATTERN, GTK_RADIO_BUTTON (psel->radial));
	gtk_widget_hide (psel->pattern);
	psel->fractal = sp_paint_selector_style_button_add (psel, "fill_fractal.xpm",
							    SP_PAINT_SELECTOR_MODE_FRACTAL, GTK_RADIO_BUTTON (psel->pattern));
	gtk_widget_hide (psel->fractal);

	/* Horizontal separator */
	w = gtk_hseparator_new ();
	gtk_widget_show (w);
	gtk_box_pack_start (GTK_BOX (psel), w, FALSE, FALSE, 0);

	/* Frame */
	psel->frame = gtk_frame_new ("");
	gtk_widget_show (psel->frame);
	gtk_container_set_border_width (GTK_CONTAINER (psel->frame), 4);
	gtk_box_pack_start (GTK_BOX (psel), psel->frame, TRUE, TRUE, 0);

	/* Last used color */
	psel->rgba = 0x000000ff;
}

static void
sp_paint_selector_destroy (GtkObject *object)
{
	SPPaintSelector *psel;

	psel = SP_PAINT_SELECTOR (object);

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static GtkWidget *
sp_paint_selector_style_button_add (SPPaintSelector *psel, const guchar *pixmap, SPPaintSelectorMode mode, GtkRadioButton *last)
{
	GtkWidget *b, *w;
	gchar *path;

	b = gtk_radio_button_new ((last) ? gtk_radio_button_group (last) : NULL);
	gtk_widget_show (b);
	gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (b), FALSE);
	gtk_object_set_data (GTK_OBJECT (b), "mode", GUINT_TO_POINTER (mode));
	path = g_concat_dir_and_file (SODIPODI_GLADEDIR, pixmap);
	w = gnome_stock_pixmap_widget (GTK_WIDGET (psel), path);
	g_free (path);
	gtk_widget_show (w);
	gtk_container_add (GTK_CONTAINER (b), w);
	gtk_box_pack_start (GTK_BOX (psel->style), b, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (b), "toggled", GTK_SIGNAL_FUNC (sp_paint_selector_style_button_toggled), psel);

	return b;
}

static void
sp_paint_selector_style_button_toggled (GtkToggleButton *tb, SPPaintSelector *psel)
{
	if (!psel->update && gtk_toggle_button_get_active (tb)) {
		sp_paint_selector_set_mode (psel, GPOINTER_TO_UINT (gtk_object_get_data (GTK_OBJECT (tb), "mode")));
	}
}

GtkWidget *
sp_paint_selector_new (void)
{
	SPPaintSelector *psel;

	psel = gtk_type_new (SP_TYPE_PAINT_SELECTOR);

	sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_MULTIPLE);

	return GTK_WIDGET (psel);
}

void
sp_paint_selector_set_mode (SPPaintSelector *psel, SPPaintSelectorMode mode)
{
	if (psel->mode != mode) {
		psel->update = TRUE;
		g_print ("Mode change %d -> %d\n", psel->mode, mode);
		switch (mode) {
		case SP_PAINT_SELECTOR_MODE_EMPTY:
			sp_paint_selector_set_mode_empty (psel);
			break;
		case SP_PAINT_SELECTOR_MODE_MULTIPLE:
			sp_paint_selector_set_mode_multiple (psel);
			break;
		case SP_PAINT_SELECTOR_MODE_NONE:
			sp_paint_selector_set_mode_none (psel);
			break;
		case SP_PAINT_SELECTOR_MODE_COLOR_RGB:
		case SP_PAINT_SELECTOR_MODE_COLOR_CMYK:
			sp_paint_selector_set_mode_color (psel, mode);
			break;
		case SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR:
		case SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL:
			sp_paint_selector_set_mode_gradient (psel, mode);
			break;
		case SP_PAINT_SELECTOR_MODE_PATTERN:
			sp_paint_selector_set_mode_pattern (psel);
			break;
		case SP_PAINT_SELECTOR_MODE_FRACTAL:
			sp_paint_selector_set_mode_fractal (psel);
			break;
		default:
			g_warning ("file %s: line %d: Unknown paint mode %d", __FILE__, __LINE__, mode);
			break;
		}
		psel->mode = mode;
		gtk_signal_emit (GTK_OBJECT (psel), psel_signals[MODE_CHANGED], psel->mode);
		psel->update = FALSE;
	}
}

void
sp_paint_selector_set_color_rgba_floatv (SPPaintSelector *psel, gfloat *rgba)
{
	SPColorSelector *csel;

	g_print ("PaintSelector set RGBA\n");

	sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_COLOR_RGB);

	csel = gtk_object_get_data (GTK_OBJECT (psel->selector), "color-selector");

	sp_color_selector_set_any_rgba_float (csel, rgba[0], rgba[1], rgba[2], rgba[3]);
}

void
sp_paint_selector_set_color_cmyka_floatv (SPPaintSelector *psel, gfloat *cmyka)
{
	SPColorSelector *csel;

	g_print ("PaintSelector set CMYKA\n");

	sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_COLOR_CMYK);

	csel = gtk_object_get_data (GTK_OBJECT (psel->selector), "color-selector");

	sp_color_selector_set_any_cmyka_float (csel, cmyka[0], cmyka[1], cmyka[2], cmyka[3], cmyka[4]);
}

void
sp_paint_selector_set_gradient_linear (SPPaintSelector *psel, SPGradient *vector)
{
	SPGradientSelector *gsel;

	g_print ("PaintSelector set GRADIENT LINEAR\n");

	sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR);

	gsel = gtk_object_get_data (GTK_OBJECT (psel->selector), "gradient-selector");

	sp_gradient_selector_set_mode (gsel, SP_GRADIENT_SELECTOR_MODE_LINEAR);
	sp_gradient_selector_set_vector (gsel, (vector) ? SP_OBJECT_DOCUMENT (vector) : NULL, vector);
}

void
sp_paint_selector_set_lgradient_position (SPPaintSelector *psel, gdouble x0, gdouble y0, gdouble x1, gdouble y1)
{
	SPGradientSelector *gsel;

	g_return_if_fail (psel != NULL);
	g_return_if_fail (SP_IS_PAINT_SELECTOR (psel));
	g_return_if_fail (psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR);

	gsel = gtk_object_get_data (GTK_OBJECT (psel->selector), "gradient-selector");

	sp_gradient_selector_set_lgradient_position (gsel, x0, y0, x1, y1);
}

void
sp_paint_selector_set_gradient_radial (SPPaintSelector *psel, SPGradient *vector)
{
	SPGradientSelector *gsel;

	g_print ("PaintSelector set GRADIENT RADIAL\n");

	sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL);

	gsel = gtk_object_get_data (GTK_OBJECT (psel->selector), "gradient-selector");

	sp_gradient_selector_set_mode (gsel, SP_GRADIENT_SELECTOR_MODE_RADIAL);
	sp_gradient_selector_set_vector (gsel, (vector) ? SP_OBJECT_DOCUMENT (vector) : NULL, vector);
}

void
sp_paint_selector_set_rgradient_position (SPPaintSelector *psel, gdouble cx, gdouble cy, gdouble fx, gdouble fy, gdouble r)
{
	SPGradientSelector *gsel;

	g_return_if_fail (psel != NULL);
	g_return_if_fail (SP_IS_PAINT_SELECTOR (psel));
	g_return_if_fail (psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL);

	gsel = gtk_object_get_data (GTK_OBJECT (psel->selector), "gradient-selector");

	sp_gradient_selector_set_rgradient_position (gsel, cx, cy, fx, fy, r);
}

void
sp_paint_selector_set_gradient_bbox (SPPaintSelector *psel, gdouble x0, gdouble y0, gdouble x1, gdouble y1)
{
	SPGradientSelector *gsel;

	g_return_if_fail (psel != NULL);
	g_return_if_fail (SP_IS_PAINT_SELECTOR (psel));
	g_return_if_fail ((psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR) ||
			  (psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL));

	gsel = gtk_object_get_data (GTK_OBJECT (psel->selector), "gradient-selector");

	sp_gradient_selector_set_bbox (gsel, x0, y0, x1, y1);
}

void
sp_paint_selector_set_gradient_gs2d_matrix_f (SPPaintSelector *psel, NRMatrixF *gs2d)
{
	SPGradientSelector *gsel;

	g_return_if_fail (psel != NULL);
	g_return_if_fail (SP_IS_PAINT_SELECTOR (psel));
	g_return_if_fail ((psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR) ||
			  (psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL));

	gsel = gtk_object_get_data (GTK_OBJECT (psel->selector), "gradient-selector");

	sp_gradient_selector_set_gs2d_matrix_f (gsel, gs2d);
}

void
sp_paint_selector_get_gradient_gs2d_matrix_f (SPPaintSelector *psel, NRMatrixF *gs2d)
{
	SPGradientSelector *gsel;

	g_return_if_fail (psel != NULL);
	g_return_if_fail (SP_IS_PAINT_SELECTOR (psel));
	g_return_if_fail ((psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR) ||
			  (psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL));

	gsel = gtk_object_get_data (GTK_OBJECT (psel->selector), "gradient-selector");

	sp_gradient_selector_get_gs2d_matrix_f (gsel, gs2d);
}

void
sp_paint_selector_get_rgba_floatv (SPPaintSelector *psel, gfloat *rgba)
{
	SPColorSelector *csel;

	g_return_if_fail (psel->mode == SP_PAINT_SELECTOR_MODE_COLOR_RGB);

	csel = gtk_object_get_data (GTK_OBJECT (psel->selector), "color-selector");

	return sp_color_selector_get_rgba_floatv (csel, rgba);
}

void
sp_paint_selector_get_cmyka_floatv (SPPaintSelector *psel, gfloat *cmyka)
{
	SPColorSelector *csel;

	g_return_if_fail (psel->mode == SP_PAINT_SELECTOR_MODE_COLOR_CMYK);

	csel = gtk_object_get_data (GTK_OBJECT (psel->selector), "color-selector");

	return sp_color_selector_get_cmyka_floatv (csel, cmyka);
}

SPGradient *
sp_paint_selector_get_gradient_vector (SPPaintSelector *psel)
{
	SPGradientSelector *gsel;

	g_return_val_if_fail ((psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR) ||
			      (psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL), NULL);

	gsel = gtk_object_get_data (GTK_OBJECT (psel->selector), "gradient-selector");

	return sp_gradient_selector_get_vector (gsel);
}

void
sp_paint_selector_get_gradient_position_floatv (SPPaintSelector *psel, gfloat *pos)
{
	SPGradientSelector *gsel;

	g_return_if_fail ((psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR) ||
			  (psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL));

	gsel = gtk_object_get_data (GTK_OBJECT (psel->selector), "gradient-selector");

	if (psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR) {
		return sp_gradient_selector_get_lgradient_position_floatv (gsel, pos);
	} else {
		return sp_gradient_selector_get_rgradient_position_floatv (gsel, pos);
	}
}

void
sp_paint_selector_write_lineargradient (SPPaintSelector *psel, SPLinearGradient *lg, SPItem *item)
{
	gfloat p[4];
	ArtDRect bbox;
	gdouble ctm[6];
	NRMatrixF fctm, gs2d, g2d, d2g, gs2g;
	NRRectF fbb;
	double e;

	g_return_if_fail (psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR);

	sp_item_invoke_bbox (item, &bbox, (gdouble *) &NR_MATRIX_D_IDENTITY);
	sp_item_i2doc_affine (item, ctm);
	fctm.c[0] = ctm[0];
	fctm.c[1] = ctm[1];
	fctm.c[2] = ctm[2];
	fctm.c[3] = ctm[3];
	fctm.c[4] = ctm[4];
	fctm.c[5] = ctm[5];
	fbb.x0 = bbox.x0;
	fbb.y0 = bbox.y0;
	fbb.x1 = bbox.x1;
	fbb.y1 = bbox.y1;
	sp_paint_selector_get_gradient_gs2d_matrix_f (psel, &gs2d);
	sp_gradient_get_g2d_matrix_f (SP_GRADIENT (lg), &fctm, &fbb, &g2d);
	nr_matrix_f_invert (&d2g, &g2d);
	nr_matrix_multiply_fff (&gs2g, &gs2d, &d2g);
	e = NR_MATRIX_DF_EXPANSION (&gs2g);
	if (e < 0.001) e = 0.001;
	gs2g.c[0] /= e;
	gs2g.c[1] /= e;
	gs2g.c[2] /= e;
	gs2g.c[3] /= e;
	nr_matrix_multiply_fff (&gs2d, &gs2g, &g2d);
	sp_gradient_set_gs2d_matrix_f (SP_GRADIENT (lg), &fctm, &fbb, &gs2d);
	sp_paint_selector_get_gradient_position_floatv (psel, p);
	p[0] *= e;
	p[1] *= e;
	p[2] *= e;
	p[3] *= e;
	sp_lineargradient_set_position (lg, p[0], p[1], p[2], p[3]);
}

void
sp_paint_selector_write_radialgradient (SPPaintSelector *psel, SPRadialGradient *rg, SPItem *item)
{
	gfloat p[5];
	ArtDRect bbox;
	gdouble ctm[6];
	NRMatrixF fctm, gs2d, g2d, d2g, gs2g;
	NRRectF fbb;
	double e;

	g_return_if_fail (psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL);

	sp_item_invoke_bbox (item, &bbox, (gdouble *) &NR_MATRIX_D_IDENTITY);
	sp_item_i2doc_affine (item, ctm);
	fctm.c[0] = ctm[0];
	fctm.c[1] = ctm[1];
	fctm.c[2] = ctm[2];
	fctm.c[3] = ctm[3];
	fctm.c[4] = ctm[4];
	fctm.c[5] = ctm[5];
	fbb.x0 = bbox.x0;
	fbb.y0 = bbox.y0;
	fbb.x1 = bbox.x1;
	fbb.y1 = bbox.y1;
	sp_paint_selector_get_gradient_gs2d_matrix_f (psel, &gs2d);
	sp_gradient_get_g2d_matrix_f (SP_GRADIENT (rg), &fctm, &fbb, &g2d);
	nr_matrix_f_invert (&d2g, &g2d);
	nr_matrix_multiply_fff (&gs2g, &gs2d, &d2g);
	e = NR_MATRIX_DF_EXPANSION (&gs2g);
	if (e < 0.001) e = 0.001;
	gs2g.c[0] /= e;
	gs2g.c[1] /= e;
	gs2g.c[2] /= e;
	gs2g.c[3] /= e;
	nr_matrix_multiply_fff (&gs2d, &gs2g, &g2d);
	sp_gradient_set_gs2d_matrix_f (SP_GRADIENT (rg), &fctm, &fbb, &gs2d);
	sp_paint_selector_get_gradient_position_floatv (psel, p);
	p[0] *= e;
	p[1] *= e;
	p[2] *= e;
	p[3] *= e;
	p[4] *= e;
	sp_radialgradient_set_position (rg, p[0], p[1], p[2], p[3], p[4]);
}

static void
sp_paint_selector_set_mode_empty (SPPaintSelector *psel)
{
	sp_paint_selector_set_style_buttons (psel, NULL);
	gtk_widget_set_sensitive (psel->style, FALSE);

	if (psel->selector) {
		gtk_widget_destroy (psel->selector);
		psel->selector = NULL;
	}

	gtk_frame_set_label (GTK_FRAME (psel->frame), _("No objects"));
}

static void
sp_paint_selector_set_mode_multiple (SPPaintSelector *psel)
{
	sp_paint_selector_set_style_buttons (psel, NULL);
	gtk_widget_set_sensitive (psel->style, TRUE);

	if (psel->selector) {
		gtk_widget_destroy (psel->selector);
		psel->selector = NULL;
	}

	gtk_frame_set_label (GTK_FRAME (psel->frame), _("Multiple styles"));
}

static void
sp_paint_selector_set_mode_none (SPPaintSelector *psel)
{
	sp_paint_selector_set_style_buttons (psel, psel->none);
	gtk_widget_set_sensitive (psel->style, TRUE);

	if (psel->selector) {
		gtk_widget_destroy (psel->selector);
		psel->selector = NULL;
	}

	gtk_frame_set_label (GTK_FRAME (psel->frame), _("No paint"));
}

/* Color paint */

static SPColorSelectorMode default_rgb_mode = SP_COLOR_SELECTOR_MODE_RGB;

static void
sp_paint_selector_color_mode_activate (GtkWidget *widget, SPPaintSelector *psel)
{
	SPColorSelector *csel;
	SPColorSelectorMode cselmode;
	SPPaintSelectorMode pselmode;

	csel = gtk_object_get_data (GTK_OBJECT (psel->selector), "color-selector");
	cselmode = GPOINTER_TO_UINT (gtk_object_get_data (GTK_OBJECT (widget), "mode"));
	/* We have to set it manually here, because RGB/HSV is ignored by psel */
	sp_color_selector_set_mode (csel, cselmode);

	switch (cselmode) {
	case SP_COLOR_SELECTOR_MODE_RGB:
	case SP_COLOR_SELECTOR_MODE_HSV:
		default_rgb_mode = cselmode;
		pselmode = SP_PAINT_SELECTOR_MODE_COLOR_RGB;
		break;
	case SP_COLOR_SELECTOR_MODE_CMYK:
		pselmode = SP_PAINT_SELECTOR_MODE_COLOR_CMYK;
		break;
	default:
		g_warning ("file %s: line %d: Illegal mode %d", __FILE__, __LINE__, cselmode);
		pselmode = SP_PAINT_SELECTOR_MODE_COLOR_RGB;
		break;
	}

	/* It is completely safe, as nops are silently ignored */
	sp_paint_selector_set_mode (psel, pselmode);
}

static void
sp_paint_selector_color_grabbed (SPColorSelector *csel, SPPaintSelector *psel)
{
	gtk_signal_emit (GTK_OBJECT (psel), psel_signals[GRABBED]);
}

static void
sp_paint_selector_color_dragged (SPColorSelector *csel, SPPaintSelector *psel)
{
	gtk_signal_emit (GTK_OBJECT (psel), psel_signals[DRAGGED]);
}

static void
sp_paint_selector_color_released (SPColorSelector *csel, SPPaintSelector *psel)
{
	gtk_signal_emit (GTK_OBJECT (psel), psel_signals[RELEASED]);
}

static void
sp_paint_selector_color_changed (SPColorSelector *csel, SPPaintSelector *psel)
{
	psel->rgba = sp_color_selector_get_rgba32 (csel);

	gtk_signal_emit (GTK_OBJECT (psel), psel_signals[CHANGED]);
}

static void
sp_paint_selector_set_mode_color (SPPaintSelector *psel, SPPaintSelectorMode mode)
{
	GtkWidget *csel, *cselmode;

	sp_paint_selector_set_style_buttons (psel, psel->solid);
	gtk_widget_set_sensitive (psel->style, TRUE);

	if ((psel->mode == SP_PAINT_SELECTOR_MODE_COLOR_RGB) || (psel->mode == SP_PAINT_SELECTOR_MODE_COLOR_CMYK)) {
		/* Already have color selector */
		csel = gtk_object_get_data (GTK_OBJECT (psel->selector), "color-selector");
		cselmode = gtk_object_get_data (GTK_OBJECT (psel->selector), "mode-menu");
	} else {
		GtkWidget *vb, *hb, *l, *m, *i;
		if (psel->selector) {
			gtk_widget_destroy (psel->selector);
			psel->selector = NULL;
		}
		/* Create new color selector */
		/* Create vbox */
		vb = gtk_vbox_new (FALSE, 4);
		gtk_widget_show (vb);
		/* Create hbox */
		hb = gtk_hbox_new (FALSE, 4);
		gtk_widget_show (hb);
		gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 4);
		/* Label */
		l = gtk_label_new (_("Mode:"));
		gtk_widget_show (l);
		gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
		gtk_box_pack_start (GTK_BOX (hb), l, TRUE, TRUE, 4);
		/* Create option menu */
		cselmode = gtk_option_menu_new ();
		gtk_widget_show (cselmode);
		/* Create menu */
		m = gtk_menu_new ();
		gtk_widget_show (m);
		i = gtk_menu_item_new_with_label (_("RGB"));
		gtk_widget_show (i);
		gtk_object_set_data (GTK_OBJECT (i), "mode", GUINT_TO_POINTER (SP_COLOR_SELECTOR_MODE_RGB));
		gtk_signal_connect (GTK_OBJECT (i), "activate",
				    GTK_SIGNAL_FUNC (sp_paint_selector_color_mode_activate), psel);
		gtk_menu_append (GTK_MENU (m), i);
		i = gtk_menu_item_new_with_label (_("HSV"));
		gtk_widget_show (i);
		gtk_object_set_data (GTK_OBJECT (i), "mode", GUINT_TO_POINTER (SP_COLOR_SELECTOR_MODE_HSV));
		gtk_signal_connect (GTK_OBJECT (i), "activate",
				    GTK_SIGNAL_FUNC (sp_paint_selector_color_mode_activate), psel);
		gtk_menu_append (GTK_MENU (m), i);
		i = gtk_menu_item_new_with_label (_("CMYK"));
		gtk_widget_show (i);
		gtk_object_set_data (GTK_OBJECT (i), "mode", GUINT_TO_POINTER (SP_COLOR_SELECTOR_MODE_CMYK));
		gtk_signal_connect (GTK_OBJECT (i), "activate",
				    GTK_SIGNAL_FUNC (sp_paint_selector_color_mode_activate), psel);
		gtk_menu_append (GTK_MENU (m), i);
		gtk_option_menu_set_menu (GTK_OPTION_MENU (cselmode), m);
		gtk_object_set_data (GTK_OBJECT (vb), "mode-menu", cselmode);
		gtk_box_pack_start (GTK_BOX (hb), cselmode, FALSE, FALSE, 0);
		/* Color selector */
		csel = sp_color_selector_new ();
		gtk_widget_show (csel);
		gtk_object_set_data (GTK_OBJECT (vb), "color-selector", csel);
		gtk_box_pack_start (GTK_BOX (vb), csel, FALSE, FALSE, 0);
		gtk_signal_connect (GTK_OBJECT (csel), "grabbed", GTK_SIGNAL_FUNC (sp_paint_selector_color_grabbed), psel);
		gtk_signal_connect (GTK_OBJECT (csel), "dragged", GTK_SIGNAL_FUNC (sp_paint_selector_color_dragged), psel);
		gtk_signal_connect (GTK_OBJECT (csel), "released", GTK_SIGNAL_FUNC (sp_paint_selector_color_released), psel);
		gtk_signal_connect (GTK_OBJECT (csel), "changed", GTK_SIGNAL_FUNC (sp_paint_selector_color_changed), psel);
		/* Pack everything to frame */
		gtk_container_add (GTK_CONTAINER (psel->frame), vb);
		psel->selector = vb;

		/* Set color */
		sp_color_selector_set_rgba32 (SP_COLOR_SELECTOR (csel), psel->rgba);
	}

	if (mode == SP_PAINT_SELECTOR_MODE_COLOR_RGB) {
		gtk_option_menu_set_history (GTK_OPTION_MENU (cselmode), (default_rgb_mode == SP_COLOR_SELECTOR_MODE_RGB) ? 0 : 1);
		sp_color_selector_set_mode (SP_COLOR_SELECTOR (csel), default_rgb_mode);
	} else {
		gtk_option_menu_set_history (GTK_OPTION_MENU (cselmode), 2);
		sp_color_selector_set_mode (SP_COLOR_SELECTOR (csel), SP_COLOR_SELECTOR_MODE_CMYK);
	}

	gtk_frame_set_label (GTK_FRAME (psel->frame), _("Color paint"));

	g_print ("Color req\n");
}

/* Gradient */

static void
sp_paint_selector_gradient_grabbed (SPColorSelector *csel, SPPaintSelector *psel)
{
	gtk_signal_emit (GTK_OBJECT (psel), psel_signals[GRABBED]);
}

static void
sp_paint_selector_gradient_dragged (SPColorSelector *csel, SPPaintSelector *psel)
{
	gtk_signal_emit (GTK_OBJECT (psel), psel_signals[DRAGGED]);
}

static void
sp_paint_selector_gradient_released (SPColorSelector *csel, SPPaintSelector *psel)
{
	gtk_signal_emit (GTK_OBJECT (psel), psel_signals[RELEASED]);
}

static void
sp_paint_selector_gradient_changed (SPColorSelector *csel, SPPaintSelector *psel)
{
	gtk_signal_emit (GTK_OBJECT (psel), psel_signals[CHANGED]);
}

static void
sp_paint_selector_set_mode_gradient (SPPaintSelector *psel, SPPaintSelectorMode mode)
{
	GtkWidget *gsel;

	/* fixme: We do not need function-wide gsel at all */

	if (mode == SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR) {
		sp_paint_selector_set_style_buttons (psel, psel->gradient);
	} else {
		sp_paint_selector_set_style_buttons (psel, psel->radial);
	}
	gtk_widget_set_sensitive (psel->style, TRUE);

	if ((psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR) || (psel->mode == SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL)) {
		/* Already have gradient selector */
		gsel = gtk_object_get_data (GTK_OBJECT (psel->selector), "gradient-selector");
	} else {
		if (psel->selector) {
			gtk_widget_destroy (psel->selector);
			psel->selector = NULL;
		}
		/* Create new gradient selector */
		gsel = sp_gradient_selector_new ();
		gtk_widget_show (gsel);
		gtk_signal_connect (GTK_OBJECT (gsel), "grabbed", GTK_SIGNAL_FUNC (sp_paint_selector_gradient_grabbed), psel);
		gtk_signal_connect (GTK_OBJECT (gsel), "dragged", GTK_SIGNAL_FUNC (sp_paint_selector_gradient_dragged), psel);
		gtk_signal_connect (GTK_OBJECT (gsel), "released", GTK_SIGNAL_FUNC (sp_paint_selector_gradient_released), psel);
		gtk_signal_connect (GTK_OBJECT (gsel), "changed", GTK_SIGNAL_FUNC (sp_paint_selector_gradient_changed), psel);
		/* Pack everything to frame */
		gtk_container_add (GTK_CONTAINER (psel->frame), gsel);
		psel->selector = gsel;
		gtk_object_set_data (GTK_OBJECT (psel->selector), "gradient-selector", gsel);
	}

	/* Actually we have to set optiomenu history here */
	if (mode == SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR) {
		sp_gradient_selector_set_mode (SP_GRADIENT_SELECTOR (gsel), SP_GRADIENT_SELECTOR_MODE_LINEAR);
		gtk_frame_set_label (GTK_FRAME (psel->frame), _("Linear gradient"));
	} else {
		sp_gradient_selector_set_mode (SP_GRADIENT_SELECTOR (gsel), SP_GRADIENT_SELECTOR_MODE_RADIAL);
		gtk_frame_set_label (GTK_FRAME (psel->frame), _("Radial gradient"));
	}

	g_print ("Gradient req\n");
}

static void
sp_paint_selector_set_mode_pattern (SPPaintSelector *psel)
{
	g_print ("Pattern req\n");
}

static void
sp_paint_selector_set_mode_fractal (SPPaintSelector *psel)
{
	g_print ("Fractal req\n");
}

static void
sp_paint_selector_set_style_buttons (SPPaintSelector *psel, GtkWidget *active)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psel->none), (active == psel->none));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psel->solid), (active == psel->solid));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psel->gradient), (active == psel->gradient));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psel->radial), (active == psel->radial));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psel->pattern), (active == psel->pattern));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psel->fractal), (active == psel->fractal));
}


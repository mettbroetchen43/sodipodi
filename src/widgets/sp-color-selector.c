#define __SP_COLOR_SELECTOR_C__

/*
 * SPColorSelector
 *
 * A block of 3 color sliders plus spinbuttons
 *
 * Copyright (C) Lauris Kaplinski <lauris@ximian.com> 2001
 *
 */

#include <math.h>
#include <gtk/gtksignal.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkspinbutton.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "sp-color-selector.h"

enum {
	GRABBED,
	DRAGGED,
	RELEASED,
	CHANGED,
	LAST_SIGNAL
};

#define CSEL_CHANNEL_R (1 << 0)
#define CSEL_CHANNEL_G (1 << 1)
#define CSEL_CHANNEL_B (1 << 2)
#define CSEL_CHANNEL_A (1 << 3)
#define CSEL_CHANNEL_H (1 << 0)
#define CSEL_CHANNEL_S (1 << 1)
#define CSEL_CHANNEL_V (1 << 2)
#define CSEL_CHANNEL_C (1 << 0)
#define CSEL_CHANNEL_M (1 << 1)
#define CSEL_CHANNEL_Y (1 << 2)
#define CSEL_CHANNEL_K (1 << 3)
#define CSEL_CHANNEL_CMYKA (1 << 4)

#define CSEL_CHANNELS_ALL 0

#define RGBA_FROM_FLOAT(r,g,b,a) \
	((guint) floor ((r) * 255.9999) << 24) | \
	((guint) floor ((g) * 255.9999) << 16) | \
	((guint) floor ((b) * 255.9999) << 8) | \
	((guint) floor ((a) * 255.9999))

static void sp_color_selector_class_init (SPColorSelectorClass *klass);
static void sp_color_selector_init (SPColorSelector *slider);
static void sp_color_selector_destroy (GtkObject *object);

static void sp_color_selector_adjustment_any_changed (GtkAdjustment *adjustment, SPColorSelector *csel);
static void sp_color_selector_slider_any_grabbed (SPColorSlider *slider, SPColorSelector *csel);
static void sp_color_selector_slider_any_released (SPColorSlider *slider, SPColorSelector *csel);
static void sp_color_selector_slider_any_changed (SPColorSlider *slider, SPColorSelector *csel);

static void sp_color_selector_adjustment_changed (SPColorSelector *csel, guint channel);

static void sp_color_selector_update_sliders (SPColorSelector *csel, guint channels);

static const guchar *sp_color_selector_hue_map (void);

static void sp_color_selector_rgb_from_hsv (gdouble *rgb, gdouble h, gdouble s, gdouble v);
static void sp_color_selector_rgb_from_cmyk (gdouble *rgb, gdouble c, gdouble m, gdouble y, gdouble k);
static void sp_color_selector_hsv_from_rgb (gdouble *hsv, gdouble r, gdouble g, gdouble b);
static void sp_color_selector_cmyk_from_rgb (gdouble *cmyk, gdouble r, gdouble g, gdouble b);

static GtkTableClass *parent_class;
static guint csel_signals[LAST_SIGNAL] = {0};

GtkType
sp_color_selector_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPColorSelector",
			sizeof (SPColorSelector),
			sizeof (SPColorSelectorClass),
			(GtkClassInitFunc) sp_color_selector_class_init,
			(GtkObjectInitFunc) sp_color_selector_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_TABLE, &info);
	}
	return type;
}

static void
sp_color_selector_class_init (SPColorSelectorClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	parent_class = gtk_type_class (GTK_TYPE_TABLE);

	csel_signals[GRABBED] =  gtk_signal_new ("grabbed",
						 GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						 object_class->type,
						 GTK_SIGNAL_OFFSET (SPColorSelectorClass, grabbed),
						 gtk_marshal_NONE__NONE,
						 GTK_TYPE_NONE, 0);
	csel_signals[DRAGGED] =  gtk_signal_new ("dragged",
						 GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						 object_class->type,
						 GTK_SIGNAL_OFFSET (SPColorSelectorClass, dragged),
						 gtk_marshal_NONE__NONE,
						 GTK_TYPE_NONE, 0);
	csel_signals[RELEASED] = gtk_signal_new ("released",
						 GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						 object_class->type,
						 GTK_SIGNAL_OFFSET (SPColorSelectorClass, released),
						 gtk_marshal_NONE__NONE,
						 GTK_TYPE_NONE, 0);
	csel_signals[CHANGED] =  gtk_signal_new ("changed",
						 GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						 object_class->type,
						 GTK_SIGNAL_OFFSET (SPColorSelectorClass, changed),
						 gtk_marshal_NONE__NONE,
						 GTK_TYPE_NONE, 0);
	gtk_object_class_add_signals (object_class, csel_signals, LAST_SIGNAL);

	object_class->destroy = sp_color_selector_destroy;
}

#define PAD 4

static void
sp_color_selector_init (SPColorSelector *csel)
{
	gint i;

	csel->updating = FALSE;
	csel->dragging = FALSE;

	gtk_table_set_homogeneous (GTK_TABLE (csel), FALSE);

	/* Create components */
	for (i = 0; i < 5; i++) {
		/* Label */
		csel->l[i] = gtk_label_new ("");
		gtk_misc_set_alignment (GTK_MISC (csel->l[i]), 1.0, 0.5);
		gtk_widget_show (csel->l[i]);
		gtk_table_attach (GTK_TABLE (csel), csel->l[i], 0, 1, i, i + 1, GTK_FILL, GTK_FILL, PAD, PAD);
		/* Adjustment */
		csel->a[i] = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.1, 0.1);
		/* Slider */
		csel->s[i] = sp_color_slider_new (csel->a[i]);
		gtk_widget_show (csel->s[i]);
		gtk_table_attach (GTK_TABLE (csel), csel->s[i], 2, 3, i, i + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, PAD, PAD);
		/* Spinbutton */
		csel->b[i] = gtk_spin_button_new (GTK_ADJUSTMENT (csel->a[i]), 0.01, 2);
		gtk_widget_show (csel->b[i]);
		gtk_table_attach (GTK_TABLE (csel), csel->b[i], 3, 4, i, i + 1, 0, 0, PAD, PAD);
		/* Attach channel value to adjustment */
		gtk_object_set_data (GTK_OBJECT (csel->a[i]), "channel", GINT_TO_POINTER (i));
		/* Signals */
		gtk_signal_connect (GTK_OBJECT (csel->a[i]), "value_changed",
				    GTK_SIGNAL_FUNC (sp_color_selector_adjustment_any_changed), csel);
		gtk_signal_connect (GTK_OBJECT (csel->s[i]), "grabbed",
				    GTK_SIGNAL_FUNC (sp_color_selector_slider_any_grabbed), csel);
		gtk_signal_connect (GTK_OBJECT (csel->s[i]), "released",
				    GTK_SIGNAL_FUNC (sp_color_selector_slider_any_released), csel);
		gtk_signal_connect (GTK_OBJECT (csel->s[i]), "changed",
				    GTK_SIGNAL_FUNC (sp_color_selector_slider_any_changed), csel);
	}

	/* Initial mode is none, so it works */
	sp_color_selector_set_mode (csel, SP_COLOR_SELECTOR_MODE_RGB);
}

static void
sp_color_selector_destroy (GtkObject *object)
{
	SPColorSelector *csel;
	gint i;

	csel = SP_COLOR_SELECTOR (object);

	for (i = 0; i < 5; i++) {
		csel->l[i] = NULL;
		csel->a[i] = NULL;
		csel->s[i] = NULL;
		csel->b[i] = NULL;
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

GtkWidget *
sp_color_selector_new (void)
{
	SPColorSelector *csel;

	csel = gtk_type_new (SP_TYPE_COLOR_SELECTOR);

	sp_color_selector_set_rgba_float (csel, 1.0, 1.0, 1.0, 1.0);

	return GTK_WIDGET (csel);
}

#define CLOSE_ENOUGH(a,b) (fabs ((a) - (b)) < 1e-9)

void
sp_color_selector_set_rgba_float (SPColorSelector *csel, gfloat r, gfloat g, gfloat b, gfloat a)
{
	gdouble c[4];

	g_return_if_fail (csel != NULL);
	g_return_if_fail (SP_IS_COLOR_SELECTOR (csel));

	sp_color_selector_get_rgba_double (csel, c);
	if (CLOSE_ENOUGH (r, c[0]) && CLOSE_ENOUGH (g, c[1]) && CLOSE_ENOUGH (b, c[2]) && CLOSE_ENOUGH (a, c[3])) return;

	switch (csel->mode) {
	case SP_COLOR_SELECTOR_MODE_RGB:
		csel->updating = TRUE;
		gtk_adjustment_set_value (csel->a[0], r);
		gtk_adjustment_set_value (csel->a[1], g);
		gtk_adjustment_set_value (csel->a[2], b);
		gtk_adjustment_set_value (csel->a[3], a);
		csel->updating = FALSE;
		sp_color_selector_update_sliders (csel, CSEL_CHANNELS_ALL);
		gtk_signal_emit (GTK_OBJECT (csel), csel_signals[CHANGED]);
		break;
	case SP_COLOR_SELECTOR_MODE_HSV:
		c[0] = csel->a[0]->value;
		sp_color_selector_hsv_from_rgb (c, r, g, b);
		csel->updating = TRUE;
		gtk_adjustment_set_value (csel->a[0], c[0]);
		gtk_adjustment_set_value (csel->a[1], c[1]);
		gtk_adjustment_set_value (csel->a[2], c[2]);
		gtk_adjustment_set_value (csel->a[3], a);
		csel->updating = FALSE;
		sp_color_selector_update_sliders (csel, CSEL_CHANNELS_ALL);
		gtk_signal_emit (GTK_OBJECT (csel), csel_signals[CHANGED]);
		break;
	case SP_COLOR_SELECTOR_MODE_CMYK:
		sp_color_selector_cmyk_from_rgb (c, r, g, b);
		csel->updating = TRUE;
		gtk_adjustment_set_value (csel->a[0], c[0]);
		gtk_adjustment_set_value (csel->a[1], c[1]);
		gtk_adjustment_set_value (csel->a[2], c[2]);
		gtk_adjustment_set_value (csel->a[3], c[3]);
		gtk_adjustment_set_value (csel->a[4], a);
		csel->updating = FALSE;
		sp_color_selector_update_sliders (csel, CSEL_CHANNELS_ALL);
		gtk_signal_emit (GTK_OBJECT (csel), csel_signals[CHANGED]);
		break;
	default:
		g_warning ("file %s: line %d: Illegal color selector mode", __FILE__, __LINE__);
		break;
	}
}

void
sp_color_selector_set_cmyka_float (SPColorSelector *csel, gfloat c, gfloat m, gfloat y, gfloat k, gfloat a)
{
	gdouble rgb[4], hsv[4];

	g_return_if_fail (csel != NULL);
	g_return_if_fail (SP_IS_COLOR_SELECTOR (csel));

	/* fixme: Test close enough */

	switch (csel->mode) {
	case SP_COLOR_SELECTOR_MODE_RGB:
		sp_color_selector_rgb_from_cmyk (rgb, c, m, y, k);
		csel->updating = TRUE;
		gtk_adjustment_set_value (csel->a[0], rgb[0]);
		gtk_adjustment_set_value (csel->a[1], rgb[1]);
		gtk_adjustment_set_value (csel->a[2], rgb[2]);
		gtk_adjustment_set_value (csel->a[3], a);
		csel->updating = FALSE;
		sp_color_selector_update_sliders (csel, CSEL_CHANNELS_ALL);
		gtk_signal_emit (GTK_OBJECT (csel), csel_signals[CHANGED]);
		break;
	case SP_COLOR_SELECTOR_MODE_HSV:
		sp_color_selector_rgb_from_cmyk (rgb, c, m, y, k);
		hsv[0] = csel->a[0]->value;
		sp_color_selector_hsv_from_rgb (hsv, rgb[0], rgb[1], rgb[2]);
		csel->updating = TRUE;
		gtk_adjustment_set_value (csel->a[0], hsv[0]);
		gtk_adjustment_set_value (csel->a[1], hsv[1]);
		gtk_adjustment_set_value (csel->a[2], hsv[2]);
		gtk_adjustment_set_value (csel->a[3], a);
		csel->updating = FALSE;
		sp_color_selector_update_sliders (csel, CSEL_CHANNELS_ALL);
		gtk_signal_emit (GTK_OBJECT (csel), csel_signals[CHANGED]);
		break;
	case SP_COLOR_SELECTOR_MODE_CMYK:
		csel->updating = TRUE;
		gtk_adjustment_set_value (csel->a[0], c);
		gtk_adjustment_set_value (csel->a[1], m);
		gtk_adjustment_set_value (csel->a[2], y);
		gtk_adjustment_set_value (csel->a[3], k);
		gtk_adjustment_set_value (csel->a[4], a);
		csel->updating = FALSE;
		sp_color_selector_update_sliders (csel, CSEL_CHANNELS_ALL);
		gtk_signal_emit (GTK_OBJECT (csel), csel_signals[CHANGED]);
		break;
	default:
		g_warning ("file %s: line %d: Illegal color selector mode", __FILE__, __LINE__);
		break;
	}
}

void
sp_color_selector_set_rgba_uint (SPColorSelector *csel, guint32 rgba)
{
	g_return_if_fail (csel != NULL);
	g_return_if_fail (SP_IS_COLOR_SELECTOR (csel));

	sp_color_selector_set_rgba_float (csel,
					  (rgba >> 24) / 255.0,
					  ((rgba >> 16) & 0xff) / 255.0,
					  ((rgba >> 8) & 0xff) / 255.0,
					  (rgba & 0xff) / 255.0);
}

void
sp_color_selector_get_rgba_double (SPColorSelector *csel, gdouble *rgba)
{
	g_return_if_fail (csel != NULL);
	g_return_if_fail (SP_IS_COLOR_SELECTOR (csel));
	g_return_if_fail (rgba != NULL);

	switch (csel->mode) {
	case SP_COLOR_SELECTOR_MODE_RGB:
		rgba[0] = csel->a[0]->value;
		rgba[1] = csel->a[1]->value;
		rgba[2] = csel->a[2]->value;
		rgba[3] = csel->a[3]->value;
		break;
	case SP_COLOR_SELECTOR_MODE_HSV:
		sp_color_selector_rgb_from_hsv (rgba, csel->a[0]->value, csel->a[1]->value, csel->a[2]->value);
		rgba[3] = csel->a[3]->value;
		break;
	case SP_COLOR_SELECTOR_MODE_CMYK:
		sp_color_selector_rgb_from_cmyk (rgba, csel->a[0]->value, csel->a[1]->value, csel->a[2]->value, csel->a[3]->value);
		rgba[3] = csel->a[4]->value;
		break;
	default:
		g_warning ("file %s: line %d: Illegal color selector mode", __FILE__, __LINE__);
		break;
	}
}

void
sp_color_selector_get_cmyka_double (SPColorSelector *csel, gdouble *cmyka)
{
	gdouble rgb[3];

	g_return_if_fail (csel != NULL);
	g_return_if_fail (SP_IS_COLOR_SELECTOR (csel));
	g_return_if_fail (cmyka != NULL);

	switch (csel->mode) {
	case SP_COLOR_SELECTOR_MODE_RGB:
		sp_color_selector_cmyk_from_rgb (cmyka, csel->a[0]->value, csel->a[1]->value, csel->a[2]->value);
		cmyka[4] = csel->a[3]->value;
		break;
	case SP_COLOR_SELECTOR_MODE_HSV:
		sp_color_selector_rgb_from_hsv (rgb, csel->a[0]->value, csel->a[1]->value, csel->a[2]->value);
		sp_color_selector_cmyk_from_rgb (cmyka, rgb[0], rgb[1], rgb[2]);
		cmyka[4] = csel->a[3]->value;
		break;
	case SP_COLOR_SELECTOR_MODE_CMYK:
		cmyka[0] = csel->a[0]->value;
		cmyka[1] = csel->a[1]->value;
		cmyka[2] = csel->a[2]->value;
		cmyka[3] = csel->a[3]->value;
		cmyka[4] = csel->a[4]->value;
		break;
	default:
		g_warning ("file %s: line %d: Illegal color selector mode", __FILE__, __LINE__);
		break;
	}
}

gfloat
sp_color_selector_get_r (SPColorSelector *csel)
{
	gdouble rgba[4];

	g_return_val_if_fail (csel != NULL, 0.0);
	g_return_val_if_fail (SP_IS_COLOR_SELECTOR (csel), 0.0);

	sp_color_selector_get_rgba_double (csel, rgba);

	return rgba[0];
}

gfloat
sp_color_selector_get_g (SPColorSelector *csel)
{
	gdouble rgba[4];

	g_return_val_if_fail (csel != NULL, 0.0);
	g_return_val_if_fail (SP_IS_COLOR_SELECTOR (csel), 0.0);

	sp_color_selector_get_rgba_double (csel, rgba);

	return rgba[1];
}

gfloat
sp_color_selector_get_b (SPColorSelector *csel)
{
	gdouble rgba[4];

	g_return_val_if_fail (csel != NULL, 0.0);
	g_return_val_if_fail (SP_IS_COLOR_SELECTOR (csel), 0.0);

	sp_color_selector_get_rgba_double (csel, rgba);

	return rgba[2];
}

gfloat
sp_color_selector_get_a (SPColorSelector *csel)
{
	gdouble rgba[4];

	g_return_val_if_fail (csel != NULL, 0.0);
	g_return_val_if_fail (SP_IS_COLOR_SELECTOR (csel), 0.0);

	sp_color_selector_get_rgba_double (csel, rgba);

	return rgba[3];
}

guint32
sp_color_selector_get_color_uint (SPColorSelector *csel)
{
	gdouble c[4];
	guint32 rgba;

	g_return_val_if_fail (csel != NULL, 0.0);
	g_return_val_if_fail (SP_IS_COLOR_SELECTOR (csel), 0.0);

	sp_color_selector_get_rgba_double (csel, c);

	rgba = ((guint) floor (c[0] * 255.9999) << 24) |
		((guint) floor (c[1] * 255.9999) << 16) |
		((guint) floor (c[2] * 255.9999) << 8) |
		(guint) floor (c[3] * 255.9999);

	return rgba;
}

void
sp_color_selector_set_mode (SPColorSelector *csel, SPColorSelectorMode mode)
{
	gfloat r, g, b, a;
	gdouble c[4];

	if (csel->mode == mode) return;

	if ((csel->mode == SP_COLOR_SELECTOR_MODE_RGB) ||
	    (csel->mode == SP_COLOR_SELECTOR_MODE_HSV) ||
	    (csel->mode == SP_COLOR_SELECTOR_MODE_CMYK)) {
		r = sp_color_selector_get_r (csel);
		g = sp_color_selector_get_g (csel);
		b = sp_color_selector_get_b (csel);
		a = sp_color_selector_get_a (csel);
	} else {
		r = g = b = a = 1.0;
	}

	csel->mode = mode;

	switch (mode) {
	case SP_COLOR_SELECTOR_MODE_RGB:
		gtk_label_set_text (GTK_LABEL (csel->l[0]), _("Red:"));
		gtk_label_set_text (GTK_LABEL (csel->l[1]), _("Green:"));
		gtk_label_set_text (GTK_LABEL (csel->l[2]), _("Blue:"));
		gtk_label_set_text (GTK_LABEL (csel->l[3]), _("Alpha:"));
		sp_color_slider_set_map (SP_COLOR_SLIDER (csel->s[0]), NULL);
		gtk_widget_hide (csel->l[4]);
		gtk_widget_hide (csel->s[4]);
		gtk_widget_hide (csel->b[4]);
		csel->updating = TRUE;
		gtk_adjustment_set_value (csel->a[0], r);
		gtk_adjustment_set_value (csel->a[1], g);
		gtk_adjustment_set_value (csel->a[2], b);
		gtk_adjustment_set_value (csel->a[3], a);
		csel->updating = FALSE;
		sp_color_selector_update_sliders (csel, CSEL_CHANNELS_ALL);
		gtk_signal_emit (GTK_OBJECT (csel), csel_signals[CHANGED]);
		break;
	case SP_COLOR_SELECTOR_MODE_HSV:
		gtk_label_set_text (GTK_LABEL (csel->l[0]), _("Hue:"));
		gtk_label_set_text (GTK_LABEL (csel->l[1]), _("Saturation:"));
		gtk_label_set_text (GTK_LABEL (csel->l[2]), _("Value:"));
		gtk_label_set_text (GTK_LABEL (csel->l[3]), _("Alpha:"));
		sp_color_slider_set_map (SP_COLOR_SLIDER (csel->s[0]), sp_color_selector_hue_map ());
		gtk_widget_hide (csel->l[4]);
		gtk_widget_hide (csel->s[4]);
		gtk_widget_hide (csel->b[4]);
		csel->updating = TRUE;
		sp_color_selector_hsv_from_rgb (c, r, g, b);
		gtk_adjustment_set_value (csel->a[0], c[0]);
		gtk_adjustment_set_value (csel->a[1], c[1]);
		gtk_adjustment_set_value (csel->a[2], c[2]);
		gtk_adjustment_set_value (csel->a[3], a);
		csel->updating = FALSE;
		sp_color_selector_update_sliders (csel, CSEL_CHANNELS_ALL);
		gtk_signal_emit (GTK_OBJECT (csel), csel_signals[CHANGED]);
		break;
	case SP_COLOR_SELECTOR_MODE_CMYK:
		gtk_label_set_text (GTK_LABEL (csel->l[0]), _("Cyan:"));
		gtk_label_set_text (GTK_LABEL (csel->l[1]), _("Magenta:"));
		gtk_label_set_text (GTK_LABEL (csel->l[2]), _("Yellow:"));
		gtk_label_set_text (GTK_LABEL (csel->l[3]), _("Black:"));
		gtk_label_set_text (GTK_LABEL (csel->l[4]), _("Alpha:"));
		sp_color_slider_set_map (SP_COLOR_SLIDER (csel->s[0]), NULL);
		gtk_widget_show (csel->l[4]);
		gtk_widget_show (csel->s[4]);
		gtk_widget_show (csel->b[4]);
		csel->updating = TRUE;
		sp_color_selector_cmyk_from_rgb (c, r, g, b);
		gtk_adjustment_set_value (csel->a[0], c[0]);
		gtk_adjustment_set_value (csel->a[1], c[1]);
		gtk_adjustment_set_value (csel->a[2], c[2]);
		gtk_adjustment_set_value (csel->a[3], c[3]);
		gtk_adjustment_set_value (csel->a[4], a);
		csel->updating = FALSE;
		sp_color_selector_update_sliders (csel, CSEL_CHANNELS_ALL);
		gtk_signal_emit (GTK_OBJECT (csel), csel_signals[CHANGED]);
		break;
	default:
		g_warning ("file %s: line %d: Illegal color selector mode", __FILE__, __LINE__);
		break;
	}
}

SPColorSelectorMode
sp_color_selector_get_mode (SPColorSelector *csel)
{
	g_return_val_if_fail (csel != NULL, SP_COLOR_SELECTOR_MODE_NONE);
	g_return_val_if_fail (SP_IS_COLOR_SELECTOR (csel), SP_COLOR_SELECTOR_MODE_NONE);

	return csel->mode;
}

static void
sp_color_selector_adjustment_any_changed (GtkAdjustment *adjustment, SPColorSelector *csel)
{
	gint channel;

	channel = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (adjustment), "channel"));

	sp_color_selector_adjustment_changed (csel, channel);
}

static void
sp_color_selector_slider_any_grabbed (SPColorSlider *slider, SPColorSelector *csel)
{
	if (!csel->dragging) {
		csel->dragging = TRUE;
		gtk_signal_emit (GTK_OBJECT (csel), csel_signals[GRABBED]);
	}
}

static void
sp_color_selector_slider_any_released (SPColorSlider *slider, SPColorSelector *csel)
{
	if (csel->dragging) {
		csel->dragging = FALSE;
		gtk_signal_emit (GTK_OBJECT (csel), csel_signals[RELEASED]);
	}
}

static void
sp_color_selector_slider_any_changed (SPColorSlider *slider, SPColorSelector *csel)
{
	gtk_signal_emit (GTK_OBJECT (csel), csel_signals[CHANGED]);
}

static void
sp_color_selector_adjustment_changed (SPColorSelector *csel, guint channel)
{
	if (csel->updating) return;

	sp_color_selector_update_sliders (csel, (1 << channel));

	if (csel->dragging) {
		gtk_signal_emit (GTK_OBJECT (csel), csel_signals[DRAGGED]);
	} else {
		gtk_signal_emit (GTK_OBJECT (csel), csel_signals[CHANGED]);
	}
}

static void
sp_color_selector_update_sliders (SPColorSelector *csel, guint channels)
{
	gdouble rgb0[3], rgb1[3];

	switch (csel->mode) {
	case SP_COLOR_SELECTOR_MODE_RGB:
		if ((channels != CSEL_CHANNEL_R) && (channels != CSEL_CHANNEL_A)) {
			/* Update red */
			sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[0]),
						    RGBA_FROM_FLOAT (0.0, csel->a[1]->value, csel->a[2]->value, 1.0),
						    RGBA_FROM_FLOAT (1.0, csel->a[1]->value, csel->a[2]->value, 1.0));
		}
		if ((channels != CSEL_CHANNEL_G) && (channels != CSEL_CHANNEL_A)) {
			/* Update green */
			sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[1]),
						    RGBA_FROM_FLOAT (csel->a[0]->value, 0.0, csel->a[2]->value, 1.0),
						    RGBA_FROM_FLOAT (csel->a[0]->value, 1.0, csel->a[2]->value, 1.0));
		}
		if ((channels != CSEL_CHANNEL_B) && (channels != CSEL_CHANNEL_A)) {
			/* Update blue */
			sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[2]),
						    RGBA_FROM_FLOAT (csel->a[0]->value, csel->a[1]->value, 0.0, 1.0),
						    RGBA_FROM_FLOAT (csel->a[0]->value, csel->a[1]->value, 1.0, 1.0));
		}
		if (channels != CSEL_CHANNEL_A) {
			/* Update alpha */
			sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[3]),
						    RGBA_FROM_FLOAT (csel->a[0]->value, csel->a[1]->value, csel->a[2]->value, 0.0),
						    RGBA_FROM_FLOAT (csel->a[0]->value, csel->a[1]->value, csel->a[2]->value, 1.0));
		}
		break;
	case SP_COLOR_SELECTOR_MODE_HSV:
		/* Hue is never updated */
		if ((channels != CSEL_CHANNEL_S) && (channels != CSEL_CHANNEL_A)) {
			/* Update saturation */
			sp_color_selector_rgb_from_hsv (rgb0, csel->a[0]->value, 0.0, csel->a[2]->value);
			sp_color_selector_rgb_from_hsv (rgb1, csel->a[0]->value, 1.0, csel->a[2]->value);
			sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[1]),
						    RGBA_FROM_FLOAT (rgb0[0], rgb0[1], rgb0[2], 1.0),
						    RGBA_FROM_FLOAT (rgb1[0], rgb1[1], rgb1[2], 1.0));
		}
		if ((channels != CSEL_CHANNEL_V) && (channels != CSEL_CHANNEL_A)) {
			/* Update value */
			sp_color_selector_rgb_from_hsv (rgb0, csel->a[0]->value, csel->a[1]->value, 0.0);
			sp_color_selector_rgb_from_hsv (rgb1, csel->a[0]->value, csel->a[1]->value, 1.0);
			sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[2]),
						    RGBA_FROM_FLOAT (rgb0[0], rgb0[1], rgb0[2], 1.0),
						    RGBA_FROM_FLOAT (rgb1[0], rgb1[1], rgb1[2], 1.0));
		}
		if (channels != CSEL_CHANNEL_A) {
			/* Update alpha */
			sp_color_selector_rgb_from_hsv (rgb0, csel->a[0]->value, csel->a[1]->value, csel->a[2]->value);
			sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[3]),
						    RGBA_FROM_FLOAT (rgb0[0], rgb0[1], rgb0[2], 0.0),
						    RGBA_FROM_FLOAT (rgb0[0], rgb0[1], rgb0[2], 1.0));
		}
		break;
	case SP_COLOR_SELECTOR_MODE_CMYK:
		if ((channels != CSEL_CHANNEL_C) && (channels != CSEL_CHANNEL_CMYKA)) {
			/* Update saturation */
			sp_color_selector_rgb_from_cmyk (rgb0, 0.0, csel->a[1]->value, csel->a[2]->value, csel->a[3]->value);
			sp_color_selector_rgb_from_cmyk (rgb1, 1.0, csel->a[1]->value, csel->a[2]->value, csel->a[3]->value);
			sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[0]),
						    RGBA_FROM_FLOAT (rgb0[0], rgb0[1], rgb0[2], 1.0),
						    RGBA_FROM_FLOAT (rgb1[0], rgb1[1], rgb1[2], 1.0));
		}
		if ((channels != CSEL_CHANNEL_M) && (channels != CSEL_CHANNEL_CMYKA)) {
			/* Update saturation */
			sp_color_selector_rgb_from_cmyk (rgb0, csel->a[0]->value, 0.0, csel->a[2]->value, csel->a[3]->value);
			sp_color_selector_rgb_from_cmyk (rgb1, csel->a[0]->value, 1.0, csel->a[2]->value, csel->a[3]->value);
			sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[1]),
						    RGBA_FROM_FLOAT (rgb0[0], rgb0[1], rgb0[2], 1.0),
						    RGBA_FROM_FLOAT (rgb1[0], rgb1[1], rgb1[2], 1.0));
		}
		if ((channels != CSEL_CHANNEL_Y) && (channels != CSEL_CHANNEL_CMYKA)) {
			/* Update saturation */
			sp_color_selector_rgb_from_cmyk (rgb0, csel->a[0]->value, csel->a[1]->value, 0.0, csel->a[3]->value);
			sp_color_selector_rgb_from_cmyk (rgb1, csel->a[0]->value, csel->a[1]->value, 1.0, csel->a[3]->value);
			sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[2]),
						    RGBA_FROM_FLOAT (rgb0[0], rgb0[1], rgb0[2], 1.0),
						    RGBA_FROM_FLOAT (rgb1[0], rgb1[1], rgb1[2], 1.0));
		}
		if ((channels != CSEL_CHANNEL_K) && (channels != CSEL_CHANNEL_CMYKA)) {
			/* Update saturation */
			sp_color_selector_rgb_from_cmyk (rgb0, csel->a[0]->value, csel->a[1]->value, csel->a[2]->value, 0.0);
			sp_color_selector_rgb_from_cmyk (rgb1, csel->a[0]->value, csel->a[1]->value, csel->a[2]->value, 1.0);
			sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[3]),
						    RGBA_FROM_FLOAT (rgb0[0], rgb0[1], rgb0[2], 1.0),
						    RGBA_FROM_FLOAT (rgb1[0], rgb1[1], rgb1[2], 1.0));
		}
		if (channels != CSEL_CHANNEL_CMYKA) {
			/* Update saturation */
			sp_color_selector_rgb_from_cmyk (rgb0, csel->a[0]->value, csel->a[1]->value, csel->a[2]->value, csel->a[3]->value);
			sp_color_slider_set_colors (SP_COLOR_SLIDER (csel->s[4]),
						    RGBA_FROM_FLOAT (rgb0[0], rgb0[1], rgb0[2], 0.0),
						    RGBA_FROM_FLOAT (rgb0[0], rgb0[1], rgb0[2], 1.0));
		}
		break;
	default:
		g_warning ("file %s: line %d: Illegal color selector mode", __FILE__, __LINE__);
		break;
	}
}

static const guchar *
sp_color_selector_hue_map (void)
{
	static guchar *map = NULL;
	guchar *p;
	gint i;

	if (!map) {
		map = g_new (guchar, 4 * 1024);
		p = map;
		for (i = 0; i < 1024; i++) {
			gdouble d;
			d = (gdouble) i * 6.0 / 1024.0;
			if (d < 1.0) {
				/* Red -> Yellow */
				*p++ = 0xff;
				*p++ = (guint) floor ((d - 0.0) * 255.9999);
				*p++ = 0x0;
				*p++ = 0xff;
			} else if (d < 2.0) {
				/* Yellow -> Green */
				*p++ = (guint) floor ((2.0 - d) * 255.9999);
				*p++ = 0xff;
				*p++ = 0x0;
				*p++ = 0xff;
			} else if (d < 3.0) {
				/* Green -> Cyan */
				*p++ = 0x0;
				*p++ = 0xff;
				*p++ = (guint) floor ((d - 2.0) * 255.9999);
				*p++ = 0xff;
			} else if (d < 4.0) {
				/* Cyan -> Blue */
				*p++ = 0x0;
				*p++ = (guint) floor ((4.0 - d) * 255.9999);
				*p++ = 0xff;
				*p++ = 0xff;
			} else if (d < 5.0) {
				/* Blue -> Magenta */
				*p++ = (guint) floor ((d - 4.0) * 255.9999);
				*p++ = 0x0;
				*p++ = 0xff;
				*p++ = 0xff;
			} else {
				/* Magenta -> Red */
				*p++ = 0xff;
				*p++ = 0x0;
				*p++ = (guint) floor ((6.0 - d) * 255.9999);
				*p++ = 0xff;
			}
		}
	}

	return map;
}

/* Taken from Gtk+ code */

static void
sp_color_selector_rgb_from_hsv (gdouble *rgb, gdouble h, gdouble s, gdouble v)
{
	gdouble f, w, q, t, d;

	d = h * 5.9999;
	f = d - floor (d);
	w = v * (1.0 - s);
	q = v * (1.0 - (s * f));
	t = v * (1.0 - (s * (1.0 - f)));

	if (d < 1.0) {
		*rgb++ = v;
		*rgb++ = t;
		*rgb++ = w;
	} else if (d < 2.0) {
		*rgb++ = q;
		*rgb++ = v;
		*rgb++ = w;
	} else if (d < 3.0) {
		*rgb++ = w;
		*rgb++ = v;
		*rgb++ = t;
	} else if (d < 4.0) {
		*rgb++ = w;
		*rgb++ = q;
		*rgb++ = v;
	} else if (d < 5.0) {
		*rgb++ = t;
		*rgb++ = w;
		*rgb++ = v;
	} else {
		*rgb++ = v;
		*rgb++ = w;
		*rgb++ = q;
	}
}

/* This sucks at moment */

static void
sp_color_selector_rgb_from_cmyk (gdouble *rgb, gdouble c, gdouble m, gdouble y, gdouble k)
{
	gdouble kd;

	kd = 1.0 - k;

	c = c * kd;
	m = m * kd;
	y = y * kd;

	c = c + k;
	m = m + k;
	y = y + k;

	rgb[0] = 1.0 - c;
	rgb[1] = 1.0 - m;
	rgb[2] = 1.0 - y;
#if 0
	gdouble min;

	min = MIN (MIN (c, m), y);
	k = CLAMP (k + min, 0.0, 1.0);
	c = CLAMP (c - min, 0.0, 1.0 - k);
	m = CLAMP (m - min, 0.0, 1.0 - k);
	y = CLAMP (y - min, 0.0, 1.0 - k);

	*rgb++ = 1.0 - c - k;
	*rgb++ = 1.0 - m - k;
	*rgb++ = 1.0 - y - k;
#endif
}

/* Taken from Gtk+ code */

static void
sp_color_selector_hsv_from_rgb (gdouble *hsv, gdouble r, gdouble g, gdouble b)
{
	gdouble max, min, delta;

	max = MAX (MAX (r, g), b);
	min = MIN (MIN (r, g), b);
	delta = max - min;

	hsv[2] = max;

	if (max > 0) {
		hsv[1] = delta / max;
	} else {
		hsv[1] = 0.0;
	}

	if (hsv[1] != 0.0) {
		if (r == max) {
			hsv[0] = (g - b) / delta;
		} else if (g == max) {
			hsv[0] = 2.0 + (b - r) / delta;
		} else {
			hsv[0] = 4.0 + (r - g) / delta;
		}

		hsv[0] = hsv[0] / 6.0;

		if (hsv[0] < 0) hsv[0] += 1.0;
	}
}

/* This too sucks at moment */

static void
sp_color_selector_cmyk_from_rgb (gdouble *cmyk, gdouble r, gdouble g, gdouble b)
{
	gfloat c, m, y, k, kd;

	c = 1.0 - r;
	m = 1.0 - g;
	y = 1.0 - b;
	k = MIN (MIN (c, m), y);

	c = c - k;
	m = m - k;
	y = y - k;

	kd = 1.0 - k;

	if (kd > 1e-9) {
		c = c / kd;
		m = m / kd;
		y = y / kd;
	}

	cmyk[0] = c;
	cmyk[1] = m;
	cmyk[2] = y;
	cmyk[3] = k;
#if 0
	cmyk[0] = 1.0 - r;
	cmyk[1] = 1.0 - g;
	cmyk[2] = 1.0 - b;
	cmyk[3] = MIN (MIN (cmyk[0], cmyk[1]), cmyk[2]);
	cmyk[0] -= cmyk[3];
	cmyk[1] -= cmyk[3];
	cmyk[2] -= cmyk[3];
#endif
}


#define __SP_STROKE_STYLE_C__

/*
 * Display settings dialog
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <config.h>

#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>

#include <gtk/gtksignal.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkwindow.h>
#include <libgnomeui/gnome-pixmap.h>

#include "../helper/unit-menu.h"
#include "../svg/svg.h"
#include "../widgets/sp-widget.h"
#include "../widgets/paint-selector.h"
#include "../gradient-chemistry.h"
#include "../document.h"
#include "../desktop-handles.h"
#include "../selection.h"
#include "../sp-gradient.h"
#include "../sodipodi.h"

#include "stroke-style.h"

static void sp_stroke_style_widget_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, SPPaintSelector *psel);
static void sp_stroke_style_widget_change_selection (SPWidget *spw, SPSelection *selection, SPPaintSelector *psel);
static void sp_stroke_style_widget_update (SPWidget *spw, SPSelection *sel);

static void sp_stroke_style_widget_paint_mode_changed (SPPaintSelector *psel, SPPaintSelectorMode mode, SPWidget *spw);
static void sp_stroke_style_widget_paint_dragged (SPPaintSelector *psel, SPWidget *spw);
static void sp_stroke_style_widget_paint_changed (SPPaintSelector *psel, SPWidget *spw);

static void sp_stroke_style_get_average_color_rgba (const GSList *objects, gfloat *c);
static void sp_stroke_style_get_average_color_cmyka (const GSList *objects, gfloat *c);
static SPPaintSelectorMode sp_stroke_style_determine_paint_selector_mode (SPObject *object);

static void sp_stroke_style_set_join_buttons (SPWidget *spw, GtkWidget *active);
static void sp_stroke_style_set_cap_buttons (SPWidget *spw, GtkWidget *active);

static GtkWidget *dialog = NULL;

static void
sp_stroke_style_dialog_destroy (GtkObject *object, gpointer data)
{
	dialog = NULL;
}

void
sp_stroke_style_dialog (void)
{
	if (!dialog) {
		GtkWidget *w;
		dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (dialog), _("Stroke style"));
		gtk_signal_connect (GTK_OBJECT (dialog), "destroy", GTK_SIGNAL_FUNC (sp_stroke_style_dialog_destroy), NULL);
		w = sp_stroke_style_widget_new ();
		gtk_widget_show (w);
		gtk_container_add (GTK_CONTAINER (dialog), w);
	}

	gtk_widget_show (dialog);
}

static void
sp_stroke_style_width_changed (GtkAdjustment *adj, SPWidget *spw)
{
	SPUnitSelector *us;
	const GSList *items, *i;
	SPCSSAttr *css;
	guchar c[32];

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	us = gtk_object_get_data (GTK_OBJECT (spw), "units");
	items = sp_widget_get_item_list (spw);

	/* fixme: Create some standardized method */
	css = sp_repr_css_attr_new ();
	g_snprintf (c, 32, "%g%s", adj->value, sp_unit_selector_get_unit (us)->abbr);
	sp_repr_css_set_property (css, "stroke-width", c);

	for (i = items; i != NULL; i = i->next) {
		sp_repr_css_change_recursive (SP_OBJECT_REPR (i->data), css, "style");
	}

	sp_repr_css_attr_unref (css);
	sp_document_done (spw->document);
}

static void
sp_stroke_style_any_toggled (GtkToggleButton *tb, SPWidget *spw)
{
	if (gtk_toggle_button_get_active (tb)) {
		const GSList *items, *i;
		const guchar *join, *cap;
		SPCSSAttr *css;

		items = sp_widget_get_item_list (spw);
		join = gtk_object_get_data (GTK_OBJECT (tb), "join");
		cap = gtk_object_get_data (GTK_OBJECT (tb), "cap");

		/* fixme: Create some standardized method */
		css = sp_repr_css_attr_new ();

		if (join) {
			sp_repr_css_set_property (css, "stroke-linejoin", join);
			for (i = items; i != NULL; i = i->next) {
				sp_repr_css_change_recursive (SP_OBJECT_REPR (i->data), css, "style");
			}
			sp_stroke_style_set_join_buttons (spw, GTK_WIDGET (tb));
		} else {
			sp_repr_css_set_property (css, "stroke-linecap", cap);
			for (i = items; i != NULL; i = i->next) {
				sp_repr_css_change_recursive (SP_OBJECT_REPR (i->data), css, "style");
			}
			sp_stroke_style_set_cap_buttons (spw, GTK_WIDGET (tb));
		}

		sp_repr_css_attr_unref (css);
		sp_document_done (spw->document);
	}
}

GtkWidget *
sp_stroke_style_widget_new (void)
{
	GtkWidget *spw, *vb, *f, *t, *l, *hb, *sb, *us, *tb, *px, *psel;
	GtkObject *a;

	spw = sp_widget_new (SODIPODI, SP_ACTIVE_DESKTOP, SP_ACTIVE_DOCUMENT);

	vb = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vb);
	gtk_container_add (GTK_CONTAINER (spw), vb);

	/* Stroke settings */
	hb = gtk_hbox_new (FALSE, 4);
	gtk_widget_show (hb);
	gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 0);

	f = gtk_frame_new (_("Stroke settings"));
	gtk_widget_show (f);
	gtk_container_set_border_width (GTK_CONTAINER (f), 4);
	gtk_box_pack_start (GTK_BOX (hb), f, FALSE, FALSE, 0);
	
	t = gtk_table_new (3, 5, FALSE);
	gtk_widget_show (t);
	gtk_container_set_border_width (GTK_CONTAINER (t), 4);
	gtk_table_set_row_spacings (GTK_TABLE (t), 4);
	gtk_container_add (GTK_CONTAINER (f), t);
	gtk_object_set_data (GTK_OBJECT (spw), "stroke", t);

	/* Stroke width */
	l = gtk_label_new (_("Width:"));
	gtk_widget_show (l);
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 0, 1, GTK_FILL, 0, 4, 0);

	hb = gtk_hbox_new (FALSE, 4);
	gtk_widget_show (hb);
	gtk_table_attach (GTK_TABLE (t), hb, 1, 5, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);

	a = gtk_adjustment_new (1.0, 0.0, 100.0, 0.1, 10.0, 10.0);
	gtk_object_set_data (GTK_OBJECT (spw), "width", a);
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 0.1, 2);
	gtk_widget_show (sb);
	gtk_box_pack_start (GTK_BOX (hb), sb, FALSE, FALSE, 0);
	us = sp_unit_selector_new (SP_UNIT_ABSOLUTE);
	gtk_widget_show (us);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	gtk_box_pack_start (GTK_BOX (hb), us, FALSE, FALSE, 0);
	gtk_object_set_data (GTK_OBJECT (spw), "units", us);

	gtk_signal_connect (GTK_OBJECT (a), "value_changed", GTK_SIGNAL_FUNC (sp_stroke_style_width_changed), spw);

	/* Join type */
	l = gtk_label_new (_("Join:"));
	gtk_widget_show (l);
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 1, 2, GTK_FILL, 0, 4, 0);

	tb = gtk_toggle_button_new ();
	gtk_widget_show (tb);
	gtk_table_attach (GTK_TABLE (t), tb, 1, 2, 1, 2, 0, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (spw), "join-miter", tb);
	gtk_object_set_data (GTK_OBJECT (tb), "join", "miter");
	gtk_signal_connect (GTK_OBJECT (tb), "toggled", GTK_SIGNAL_FUNC (sp_stroke_style_any_toggled), spw);
	px = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/join_miter.xpm");
	gtk_widget_show (px);
	gtk_container_add (GTK_CONTAINER (tb), px);

	tb = gtk_toggle_button_new ();
	gtk_widget_show (tb);
	gtk_table_attach (GTK_TABLE (t), tb, 2, 3, 1, 2, 0, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (spw), "join-round", tb);
	gtk_object_set_data (GTK_OBJECT (tb), "join", "round");
	gtk_signal_connect (GTK_OBJECT (tb), "toggled", GTK_SIGNAL_FUNC (sp_stroke_style_any_toggled), spw);
	px = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/join_round.xpm");
	gtk_widget_show (px);
	gtk_container_add (GTK_CONTAINER (tb), px);

	tb = gtk_toggle_button_new ();
	gtk_widget_show (tb);
	gtk_table_attach (GTK_TABLE (t), tb, 3, 4, 1, 2, 0, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (spw), "join-bevel", tb);
	gtk_object_set_data (GTK_OBJECT (tb), "join", "bevel");
	gtk_signal_connect (GTK_OBJECT (tb), "toggled", GTK_SIGNAL_FUNC (sp_stroke_style_any_toggled), spw);
	px = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/join_bevel.xpm");
	gtk_widget_show (px);
	gtk_container_add (GTK_CONTAINER (tb), px);

	/* Cap type */
	l = gtk_label_new (_("Cap:"));
	gtk_widget_show (l);
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (t), l, 0, 1, 2, 3, GTK_FILL, 0, 4, 0);

	tb = gtk_toggle_button_new ();
	gtk_widget_show (tb);
	gtk_table_attach (GTK_TABLE (t), tb, 1, 2, 2, 3, 0, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (spw), "cap-butt", tb);
	gtk_object_set_data (GTK_OBJECT (tb), "cap", "butt");
	gtk_signal_connect (GTK_OBJECT (tb), "toggled", GTK_SIGNAL_FUNC (sp_stroke_style_any_toggled), spw);
	px = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/cap_butt.xpm");
	gtk_widget_show (px);
	gtk_container_add (GTK_CONTAINER (tb), px);

	tb = gtk_toggle_button_new ();
	gtk_widget_show (tb);
	gtk_table_attach (GTK_TABLE (t), tb, 2, 3, 2, 3, 0, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (spw), "cap-round", tb);
	gtk_object_set_data (GTK_OBJECT (tb), "cap", "round");
	gtk_signal_connect (GTK_OBJECT (tb), "toggled", GTK_SIGNAL_FUNC (sp_stroke_style_any_toggled), spw);
	px = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/cap_round.xpm");
	gtk_widget_show (px);
	gtk_container_add (GTK_CONTAINER (tb), px);

	tb = gtk_toggle_button_new ();
	gtk_widget_show (tb);
	gtk_table_attach (GTK_TABLE (t), tb, 3, 4, 2, 3, 0, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (spw), "cap-square", tb);
	gtk_object_set_data (GTK_OBJECT (tb), "cap", "square");
	gtk_signal_connect (GTK_OBJECT (tb), "toggled", GTK_SIGNAL_FUNC (sp_stroke_style_any_toggled), spw);
	px = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/cap_square.xpm");
	gtk_widget_show (px);
	gtk_container_add (GTK_CONTAINER (tb), px);

	/* Paint selector */
	psel = sp_paint_selector_new ();
	gtk_widget_show (psel);
	gtk_box_pack_start (GTK_BOX (vb), psel, FALSE, FALSE, 0);
	gtk_object_set_data (GTK_OBJECT (spw), "paint-selector", psel);

	gtk_signal_connect (GTK_OBJECT (spw), "modify_selection", GTK_SIGNAL_FUNC (sp_stroke_style_widget_modify_selection), psel);
	gtk_signal_connect (GTK_OBJECT (spw), "change_selection", GTK_SIGNAL_FUNC (sp_stroke_style_widget_change_selection), psel);

	gtk_signal_connect (GTK_OBJECT (psel), "mode_changed", GTK_SIGNAL_FUNC (sp_stroke_style_widget_paint_mode_changed), spw);
	gtk_signal_connect (GTK_OBJECT (psel), "dragged", GTK_SIGNAL_FUNC (sp_stroke_style_widget_paint_dragged), spw);
	gtk_signal_connect (GTK_OBJECT (psel), "changed", GTK_SIGNAL_FUNC (sp_stroke_style_widget_paint_changed), spw);

	sp_stroke_style_widget_update (SP_WIDGET (spw), SP_ACTIVE_DESKTOP ? SP_DT_SELECTION (SP_ACTIVE_DESKTOP) : NULL);

	return spw;
}

static void
sp_stroke_style_widget_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, SPPaintSelector *psel)
{
	if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG)) {
		g_print ("StrokeStyleWidget: selection modified\n");

		sp_stroke_style_widget_update (spw, selection);
	}
}

static void
sp_stroke_style_widget_change_selection (SPWidget *spw, SPSelection *selection, SPPaintSelector *psel)
{
	sp_stroke_style_widget_update (spw, selection);
}

static void
sp_stroke_style_widget_update (SPWidget *spw, SPSelection *sel)
{
	GtkWidget *sset, *units;
	GtkObject *width;
	const SPUnit *unit;
	SPPaintSelector *psel;
	SPPaintSelectorMode pselmode;
	const GSList *objects, *l;
	SPObject *object;
	SPGradient *vector;
	gfloat c[5];
	ArtDRect bbox;
	SPLinearGradient *lg;
	gdouble avgwidth;
	gint jointype, captype;
	GtkWidget *tb;

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (TRUE));

	sset = gtk_object_get_data (GTK_OBJECT (spw), "stroke");
	width = gtk_object_get_data (GTK_OBJECT (spw), "width");
	units = gtk_object_get_data (GTK_OBJECT (spw), "units");
	psel = gtk_object_get_data (GTK_OBJECT (spw), "paint-selector");

	if (!sel || sp_selection_is_empty (sel)) {
		/* No objects, set empty */
		gtk_widget_set_sensitive (sset, FALSE);
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_EMPTY);
		gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
		return;
	}

	objects = sp_selection_item_list (sel);

	/* Determine average stroke width */
	avgwidth = 0.0;
	for (l = objects; l != NULL; l = l->next) {
		object = SP_OBJECT (l->data);
		avgwidth += sp_distance_get_points (&object->style->stroke_width);
	}
	avgwidth /= g_slist_length ((GSList *) objects);
	unit = sp_unit_selector_get_unit (SP_UNIT_SELECTOR (units));
	sp_convert_distance (&avgwidth, SP_PS_UNIT, unit);
	gtk_adjustment_set_value (GTK_ADJUSTMENT (width), avgwidth);

	/* Join & Cap */
	object = SP_OBJECT (objects->data);
	jointype = object->style->stroke_linejoin;
	captype = object->style->stroke_linecap;

	for (l = objects->next; l != NULL; l = l->next) {
		SPObject *o;
		o = SP_OBJECT (l->data);
		if (o->style->stroke_linejoin != jointype) jointype = -1;
		if (o->style->stroke_linecap != captype) captype = -1;
	}

	switch (jointype) {
	case ART_PATH_STROKE_JOIN_MITER:
		tb = gtk_object_get_data (GTK_OBJECT (spw), "join-miter");
		break;
	case ART_PATH_STROKE_JOIN_ROUND:
		tb = gtk_object_get_data (GTK_OBJECT (spw), "join-round");
		break;
	case ART_PATH_STROKE_JOIN_BEVEL:
		tb = gtk_object_get_data (GTK_OBJECT (spw), "join-bevel");
		break;
	default:
		tb = NULL;
		break;
	}

	sp_stroke_style_set_join_buttons (spw, tb);

	switch (captype) {
	case ART_PATH_STROKE_CAP_BUTT:
		tb = gtk_object_get_data (GTK_OBJECT (spw), "cap-butt");
		break;
	case ART_PATH_STROKE_CAP_ROUND:
		tb = gtk_object_get_data (GTK_OBJECT (spw), "cap-round");
		break;
	case ART_PATH_STROKE_CAP_SQUARE:
		tb = gtk_object_get_data (GTK_OBJECT (spw), "cap-square");
		break;
	default:
		tb = NULL;
		break;
	}

	sp_stroke_style_set_cap_buttons (spw, tb);

	/* Paint */

	object = SP_OBJECT (objects->data);
	pselmode = sp_stroke_style_determine_paint_selector_mode (object);

	for (l = objects->next; l != NULL; l = l->next) {
		SPPaintSelectorMode nextmode;
		nextmode = sp_stroke_style_determine_paint_selector_mode (SP_OBJECT (l->data));
		if (nextmode != pselmode) {
			/* Multiple styles */
			sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_MULTIPLE);
			gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
			return;
		}
	}

	g_print ("StrokeStyleWidget: paint selector mode %d\n", pselmode);

	gtk_widget_set_sensitive (sset, pselmode != SP_PAINT_SELECTOR_MODE_NONE);

	switch (pselmode) {
	case SP_PAINT_SELECTOR_MODE_NONE:
		/* No paint at all */
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_NONE);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_RGB:
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_COLOR_RGB);
		sp_stroke_style_get_average_color_rgba (objects, c);
		sp_paint_selector_set_color_rgba_floatv (psel, c);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_CMYK:
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_COLOR_CMYK);
		sp_stroke_style_get_average_color_cmyka (objects, c);
		sp_paint_selector_set_color_cmyka_floatv (psel, c);
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR:
		object = SP_OBJECT (objects->data);
		/* We know that all objects have lineargradient stroke style */
		vector = sp_gradient_get_vector (SP_GRADIENT (object->style->stroke.server), FALSE);
		for (l = objects->next; l != NULL; l = l->next) {
			SPObject *next;
			next = SP_OBJECT (l->data);
			if (sp_gradient_get_vector (SP_GRADIENT (next->style->stroke.server), FALSE) != vector) {
				/* Multiple vectors */
				sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_MULTIPLE);
				gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
				return;
			}
		}
		/* fixme: Probably we should set multiple mode here too */
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR);
		sp_paint_selector_set_gradient_linear (psel, vector);
		sp_selection_bbox (sel, &bbox);
		sp_paint_selector_set_gradient_bbox (psel, bbox.x0, bbox.y0, bbox.x1, bbox.y1);
		/* fixme: This is plain wrong */
		lg = SP_LINEARGRADIENT (SP_OBJECT_STYLE_STROKE_SERVER (object));
		/* fixme: Take units into account! */
		sp_paint_selector_set_gradient_position (psel, lg->x1.distance, lg->y1.distance, lg->x2.distance, lg->y2.distance);
		break;
	}

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
}

#if 0
	SPGradient *gradient;
	GtkWidget *vectors, *position;
#if 0
	GtkWidget *menu;
	gint pos;
	GList *children, *l;
#endif
	const GSList *items, *i;
	ArtDRect bbox;
	SPLinearGradient *lg;

	if (!selection) return;

	items = sp_selection_item_list (selection);

	gradient = NULL;
	for (i = items; i != NULL; i = i->next) {
		SPStyle *style;
		style = SP_OBJECT_STYLE (i->data);
		if ((style->stroke.type == SP_PAINT_TYPE_PAINTSERVER) && SP_IS_LINEARGRADIENT (style->stroke.server)) {
			gradient = SP_GRADIENT (style->stroke.server);
			while (gradient) {
				/* Search vector gradient */
				sp_gradient_ensure_vector (gradient);
				if (gradient->has_stops) break;
				gradient = gradient->href;
			}
			if (gradient) break;
		}
	}
	/* Return if no vector gradient */
	if (!gradient) return;

	vectors = gtk_object_get_data (GTK_OBJECT (spw), "vectors");

#if 0
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (vectors));
	children = gtk_container_children (GTK_CONTAINER (menu));

	pos = 0;
	for (l = children; l != NULL; l = l->next) {
		SPGradient *gr;
		gr = gtk_object_get_data (GTK_OBJECT (l->data), "gradient");
		if (gr == gradient) break;
		pos += 1;
	}

	g_list_free (children);

	gtk_option_menu_set_history (GTK_OPTION_MENU (vectors), pos);
#else
	sp_gradient_vector_selector_set_gradient (SP_GRADIENT_VECTOR_SELECTOR (vectors), (gradient) ? SP_OBJECT_DOCUMENT (gradient) : NULL, gradient);
#endif

	gtk_object_set_data (GTK_OBJECT (spw), "gradient", gradient);

	/* Update position */
	position = gtk_object_get_data (GTK_OBJECT (spw), "position");
	gradient = NULL;
	for (i = items; i != NULL; i = i->next) {
		SPStyle *style;
		style = SP_OBJECT_STYLE (i->data);
		if ((style->stroke.type == SP_PAINT_TYPE_PAINTSERVER) && SP_IS_LINEARGRADIENT (style->stroke.server)) {
			gradient = SP_GRADIENT (style->stroke.server);
			break;
		}
	}
	g_assert (gradient != NULL);
	sp_gradient_position_set_gradient (SP_GRADIENT_POSITION (position), SP_GRADIENT (gradient));
	sp_selection_bbox (selection, &bbox);
	sp_gradient_position_set_bbox (SP_GRADIENT_POSITION (position), bbox.x0, bbox.y0, bbox.x1, bbox.y1);
	lg = SP_LINEARGRADIENT (gradient);
	sp_gradient_position_set_vector (SP_GRADIENT_POSITION (position), lg->x1.distance, lg->y1.distance, lg->x2.distance, lg->y2.distance);
#endif

static void
sp_stroke_style_widget_paint_mode_changed (SPPaintSelector *psel, SPPaintSelectorMode mode, SPWidget *spw)
{
	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	/* fixme: Does this work? */
	/* fixme: Not really, here we have to get old color back from object */
	/* Instead of relying on paint widget having meaningful colors set */
	sp_stroke_style_widget_paint_changed (psel, spw);
}

static void
sp_stroke_style_widget_paint_dragged (SPPaintSelector *psel, SPWidget *spw)
{
	const GSList *items, *i;
	SPGradient *vector;
	gfloat c[5];

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	g_print ("StrokeStyleWidget: paint dragged\n");

	switch (psel->mode) {
	case SP_PAINT_SELECTOR_MODE_EMPTY:
	case SP_PAINT_SELECTOR_MODE_MULTIPLE:
	case SP_PAINT_SELECTOR_MODE_NONE:
		g_warning ("file %s: line %d: Paint %d should not emit 'dragged'", __FILE__, __LINE__, psel->mode);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_RGB:
		sp_paint_selector_get_rgba_floatv (psel, c);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			sp_style_set_stroke_color_rgba (SP_OBJECT_STYLE (i->data), c[0], c[1], c[2], c[3], TRUE, TRUE);
			sp_object_style_changed (SP_OBJECT (i->data), SP_OBJECT_MODIFIED_FLAG);
		}
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_CMYK:
		sp_paint_selector_get_cmyka_floatv (psel, c);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			sp_style_set_stroke_color_cmyka (SP_OBJECT_STYLE (i->data), c[0], c[1], c[2], c[3], c[4], TRUE, TRUE);
			sp_object_style_changed (SP_OBJECT (i->data), SP_OBJECT_MODIFIED_FLAG);
		}
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR:
		vector = sp_paint_selector_get_gradient_vector (psel);
		vector = sp_gradient_ensure_vector_normalized (vector);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			gfloat p[4];
			sp_item_force_stroke_lineargradient_vector (SP_ITEM (i->data), vector);
			sp_paint_selector_get_gradient_position_floatv (psel, p);
			sp_lineargradient_set_position (SP_LINEARGRADIENT (SP_OBJECT_STYLE_STROKE_SERVER (i->data)), p[0], p[1], p[2], p[3]);
			/* fixme: Managing selection bbox/item bbox stuff is big mess */
		}
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL:
		g_warning ("file %s: line %d: Paint %d should not emit 'dragged'", __FILE__, __LINE__, psel->mode);
		break;
	default:
		g_warning ("file %s: line %d: Paint selector should not be in mode %d", __FILE__, __LINE__, psel->mode);
		break;
	}
}

static void
sp_stroke_style_widget_paint_changed (SPPaintSelector *psel, SPWidget *spw)
{
	const GSList *items, *i;
	SPCSSAttr *css;
	gfloat rgba[4], cmyka[5];
	SPGradient *vector;
	guchar b[64];

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	g_print ("StrokeStyleWidget: paint changed\n");

	switch (psel->mode) {
	case SP_PAINT_SELECTOR_MODE_EMPTY:
	case SP_PAINT_SELECTOR_MODE_MULTIPLE:
		g_warning ("file %s: line %d: Paint %d should not emit 'changed'", __FILE__, __LINE__, psel->mode);
		break;
	case SP_PAINT_SELECTOR_MODE_NONE:
		css = sp_repr_css_attr_new ();
		sp_repr_css_set_property (css, "stroke", "none");
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			sp_repr_css_change_recursive (SP_OBJECT_REPR (i->data), css, "style");
			sp_repr_set_attr_recursive (SP_OBJECT_REPR (i->data), "stroke-cmyk", NULL);
		}
		sp_repr_css_attr_unref (css);
		sp_document_done (spw->document);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_RGB:
		css = sp_repr_css_attr_new ();
		sp_paint_selector_get_rgba_floatv (psel, rgba);
		sp_svg_write_color (b, 64, SP_RGBA32_F_COMPOSE (rgba[0], rgba[1], rgba[2], 0.0));
		sp_repr_css_set_property (css, "stroke", b);
		g_snprintf (b, 64, "%g", rgba[3]);
		sp_repr_css_set_property (css, "stroke-opacity", b);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			g_print ("StrokeStyleWidget: Set RGBA stroke style\n");
			sp_repr_css_change_recursive (SP_OBJECT_REPR (i->data), css, "style");
			g_print ("StrokeStyleWidget: Set RGBA stroke-cmyk style\n");
			sp_repr_set_attr_recursive (SP_OBJECT_REPR (i->data), "stroke-cmyk", NULL);
		}
		sp_repr_css_attr_unref (css);
		sp_document_done (spw->document);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_CMYK:
		css = sp_repr_css_attr_new ();
		sp_paint_selector_get_cmyka_floatv (psel, cmyka);
		sp_color_cmyk_to_rgb_floatv (rgba, cmyka[0], cmyka[1], cmyka[2], cmyka[3]);
		sp_svg_write_color (b, 64, SP_RGBA32_F_COMPOSE (rgba[0], rgba[1], rgba[2], 0.0));
		sp_repr_css_set_property (css, "stroke", b);
		g_snprintf (b, 64, "%g", cmyka[4]);
		sp_repr_css_set_property (css, "stroke-opacity", b);
		g_snprintf (b, 64, "(%g %g %g %g)", cmyka[0], cmyka[1], cmyka[2], cmyka[3]);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			g_print ("StrokeStyleWidget: Set CMYK stroke style\n");
			sp_repr_css_change_recursive (SP_OBJECT_REPR (i->data), css, "style");
			g_print ("StrokeStyleWidget: Set CMYK stroke-cmyk style\n");
			sp_repr_set_attr_recursive (SP_OBJECT_REPR (i->data), "stroke-cmyk", b);
		}
		sp_repr_css_attr_unref (css);
		sp_document_done (spw->document);
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR:
		items = sp_widget_get_item_list (spw);
		if (items) {
			vector = sp_paint_selector_get_gradient_vector (psel);
			if (!vector) {
				g_warning ("SPStrokeStyleWidget: Got linearGradient mode but NULL gradient in 'changed' handler\n");
				vector = sp_document_default_gradient_vector (spw->document);
			}
			vector = sp_gradient_ensure_vector_normalized (vector);
			for (i = items; i != NULL; i = i->next) {
				gfloat p[4];
				g_print ("StrokeStyleWidget: Set LinearGradient %s stroke style\n", SP_OBJECT_ID (vector));
				sp_item_force_stroke_lineargradient_vector (SP_ITEM (i->data), vector);
				/* fixme: Managing selection bbox/item bbox stuff is big mess */
				sp_paint_selector_get_gradient_position_floatv (psel, p);
				sp_repr_set_double (SP_OBJECT_REPR (SP_OBJECT_STYLE_STROKE_SERVER (i->data)), "x1", p[0]);
				sp_repr_set_double (SP_OBJECT_REPR (SP_OBJECT_STYLE_STROKE_SERVER (i->data)), "y1", p[1]);
				sp_repr_set_double (SP_OBJECT_REPR (SP_OBJECT_STYLE_STROKE_SERVER (i->data)), "x2", p[2]);
				sp_repr_set_double (SP_OBJECT_REPR (SP_OBJECT_STYLE_STROKE_SERVER (i->data)), "y2", p[3]);
			}
			sp_document_done (spw->document);
		}
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL:
		g_warning ("file %s: line %d: Paint %d should not emit 'changed'", __FILE__, __LINE__, psel->mode);
		break;
	default:
		g_warning ("file %s: line %d: Paint selector should not be in mode %d", __FILE__, __LINE__, psel->mode);
		break;
	}
}

static void
sp_stroke_style_get_average_color_rgba (const GSList *objects, gfloat *c)
{
	gint num;

	c[0] = 0.0;
	c[1] = 0.0;
	c[2] = 0.0;
	c[3] = 0.0;
	num = 0;

	while (objects) {
		SPObject *object;
		gfloat d[3];
		object = SP_OBJECT (objects->data);
		if (object->style->stroke.type == SP_PAINT_TYPE_COLOR) {
			sp_color_get_rgb_floatv (&object->style->stroke.color, d);
			c[0] += d[0];
			c[1] += d[1];
			c[2] += d[2];
			c[3] += object->style->stroke_opacity;
		}
		num += 1;
		objects = objects->next;
	}

	c[0] /= num;
	c[1] /= num;
	c[2] /= num;
	c[3] /= num;
}

static void
sp_stroke_style_get_average_color_cmyka (const GSList *objects, gfloat *c)
{
	gint num;

	c[0] = 0.0;
	c[1] = 0.0;
	c[2] = 0.0;
	c[3] = 0.0;
	c[4] = 0.0;
	num = 0;

	while (objects) {
		SPObject *object;
		gfloat d[4];
		object = SP_OBJECT (objects->data);
		if (object->style->stroke.type == SP_PAINT_TYPE_COLOR) {
			sp_color_get_cmyk_floatv (&object->style->stroke.color, d);
			c[0] += d[0];
			c[1] += d[1];
			c[2] += d[2];
			c[3] += d[3];
			c[4] += object->style->stroke_opacity;
		}
		num += 1;
		objects = objects->next;
	}

	c[0] /= num;
	c[1] /= num;
	c[2] /= num;
	c[3] /= num;
	c[4] /= num;
}

static SPPaintSelectorMode
sp_stroke_style_determine_paint_selector_mode (SPObject *object)
{
	SPColorSpaceType cstype;

	switch (object->style->stroke.type) {
	case SP_PAINT_TYPE_NONE:
		return SP_PAINT_SELECTOR_MODE_NONE;
	case SP_PAINT_TYPE_COLOR:
		cstype = sp_color_get_colorspace_type (&object->style->stroke.color);
		switch (cstype) {
		case SP_COLORSPACE_TYPE_RGB:
			return SP_PAINT_SELECTOR_MODE_COLOR_RGB;
		case SP_COLORSPACE_TYPE_CMYK:
			return SP_PAINT_SELECTOR_MODE_COLOR_CMYK;
		default:
			g_warning ("file %s: line %d: Unknown colorspace type %d", __FILE__, __LINE__, cstype);
			return SP_PAINT_SELECTOR_MODE_NONE;
		}
	case SP_PAINT_TYPE_PAINTSERVER:
		if (SP_IS_LINEARGRADIENT (object->style->stroke.server)) {
			return SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR;
		}
		g_warning ("file %s: line %d: Unknown paintserver", __FILE__, __LINE__);
		return SP_PAINT_SELECTOR_MODE_NONE;
	default:
		g_warning ("file %s: line %d: Unknown paint type %d", __FILE__, __LINE__, object->style->stroke.type);
		break;
	}

	return SP_PAINT_SELECTOR_MODE_NONE;
}

static void
sp_stroke_style_set_join_buttons (SPWidget *spw, GtkWidget *active)
{
	GtkWidget *tb;

	tb = gtk_object_get_data (GTK_OBJECT (spw), "join-miter");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));
	gtk_widget_set_sensitive (tb, (active != tb));
	tb = gtk_object_get_data (GTK_OBJECT (spw), "join-round");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));
	gtk_widget_set_sensitive (tb, (active != tb));
	tb = gtk_object_get_data (GTK_OBJECT (spw), "join-bevel");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));
	gtk_widget_set_sensitive (tb, (active != tb));
}

static void
sp_stroke_style_set_cap_buttons (SPWidget *spw, GtkWidget *active)
{
	GtkWidget *tb;

	tb = gtk_object_get_data (GTK_OBJECT (spw), "cap-butt");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));
	gtk_widget_set_sensitive (tb, (active != tb));
	tb = gtk_object_get_data (GTK_OBJECT (spw), "cap-round");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));
	gtk_widget_set_sensitive (tb, (active != tb));
	tb = gtk_object_get_data (GTK_OBJECT (spw), "cap-square");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));
	gtk_widget_set_sensitive (tb, (active != tb));
}


#define __SP_GRADIENT_SELECTOR_C__

/*
 * A simple OptionMenu for choosing gradient vectors visually
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkoptionmenu.h>
#include "../widgets/gradient-image.h"
#include "../widgets/gradient-position.h"
#include "../sodipodi.h"
#include "../document.h"
#include "../desktop-handles.h"
#include "../document-private.h"
#include "../sp-root.h"
#include "../selection.h"
#include "../sp-gradient.h"
#include "../sp-item.h"
#include "../gradient-chemistry.h"
#include "sp-widget.h"
#include "gradient-vector.h"
#include "gradient-selector.h"

static GtkWidget *sp_gradient_selector_vector_menu_new (SPWidget *spw);
static void sp_gradient_selector_vector_menu_refresh (GtkOptionMenu *vectors, SPWidget *spw);
static void sp_gradient_selector_vector_activate (GtkMenuItem *mi, SPWidget *spw);
static void sp_gradient_selector_edit_vector_clicked (GtkWidget *w, SPWidget *spw);
static void sp_gradient_selector_add_vector_clicked (GtkWidget *w, SPWidget *spw);
static void sp_gradient_selector_delete_vector_clicked (GtkWidget *w, SPWidget *spw);
static void sp_gradient_selector_reset_clicked (GtkWidget *w, SPWidget *spw);

static void sp_gradient_selector_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, gpointer data);
static void sp_gradient_selector_change_selection (SPWidget *spw, SPSelection *selection, gpointer data);

static void sp_gradient_selector_load_selection (SPWidget *spw, SPSelection *selection);

static void sp_gradient_selection_position_dragged (SPGradientPosition *pos, SPWidget *spw);
static void sp_gradient_selection_position_changed (SPGradientPosition *pos, SPWidget *spw);

GtkWidget *
sp_gradient_widget_new (void)
{
	GtkWidget *spw, *vb, *hb, *b, *vectors, *pos;

	spw = sp_widget_new (SODIPODI, SP_ACTIVE_DESKTOP, SP_ACTIVE_DOCUMENT);

	vb = gtk_vbox_new (FALSE, 4);
	gtk_widget_show (vb);
	gtk_container_add (GTK_CONTAINER (spw), vb);

	vectors = sp_gradient_selector_vector_menu_new (SP_WIDGET (spw));
	gtk_widget_show (vectors);
	gtk_box_pack_start (GTK_BOX (vb), vectors, FALSE, FALSE, 4);
	gtk_object_set_data (GTK_OBJECT (spw), "vectors", vectors);

	hb = gtk_hbox_new (TRUE, 0);
	gtk_widget_show (hb);
	gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 4);

	b = gtk_button_new_with_label (_("Edit"));
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (hb), b, TRUE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (b), "clicked",
			    GTK_SIGNAL_FUNC (sp_gradient_selector_edit_vector_clicked), spw);
	b = gtk_button_new_with_label (_("Add"));
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (hb), b, TRUE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (b), "clicked",
			    GTK_SIGNAL_FUNC (sp_gradient_selector_add_vector_clicked), spw);
	b = gtk_button_new_with_label (_("Delete"));
	gtk_widget_set_sensitive (b, FALSE);
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (hb), b, TRUE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (b), "clicked",
			    GTK_SIGNAL_FUNC (sp_gradient_selector_delete_vector_clicked), spw);

	/* Create gradient position widget */
	pos = sp_gradient_position_new (NULL);
	gtk_widget_show (pos);
	gtk_box_pack_start (GTK_BOX (vb), pos, TRUE, TRUE, 4);
	gtk_object_set_data (GTK_OBJECT (spw), "position", pos);
	gtk_signal_connect (GTK_OBJECT (pos), "dragged",
			    GTK_SIGNAL_FUNC (sp_gradient_selection_position_dragged), spw);
	gtk_signal_connect (GTK_OBJECT (pos), "changed",
			    GTK_SIGNAL_FUNC (sp_gradient_selection_position_changed), spw);

	/* Reset button */
	b = gtk_button_new_with_label (_("Reset"));
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (vb), b, FALSE, FALSE, 4);
	gtk_signal_connect (GTK_OBJECT (b), "clicked",
			    GTK_SIGNAL_FUNC (sp_gradient_selector_reset_clicked), spw);

	/* Connect selection tracking signal */
	gtk_signal_connect (GTK_OBJECT (spw), "modify_selection",
			    GTK_SIGNAL_FUNC (sp_gradient_selector_modify_selection), NULL);
	gtk_signal_connect (GTK_OBJECT (spw), "change_selection",
			    GTK_SIGNAL_FUNC (sp_gradient_selector_change_selection), NULL);

	sp_gradient_selector_load_selection (SP_WIDGET (spw), SP_ACTIVE_DESKTOP ? SP_DT_SELECTION (SP_ACTIVE_DESKTOP) : NULL);

	return spw;
}

/* Create menu of vectors */

static GtkWidget *
sp_gradient_selector_vector_menu_new (SPWidget *spw)
{
	GtkWidget *vectors;

	vectors = gtk_option_menu_new ();

	sp_gradient_selector_vector_menu_refresh (GTK_OPTION_MENU (vectors), spw);

	return vectors;
}

/* Rebuild menu of vectors */
/* fixme: pick it with signals */

static void
sp_gradient_selector_vector_menu_refresh (GtkOptionMenu *vectors, SPWidget *spw)
{
	GtkWidget *menu;
	const GSList *gradients, *l;
	GSList *gl;

	gtk_option_menu_remove_menu (vectors);

	gl = NULL;
	gradients = sp_document_get_resource_list (spw->document, "gradient");
	/* Pick up all gradients with vectors */
	for (l = gradients; l != NULL; l = l->next) {
		SPGradient *gr;
		gr = SP_GRADIENT (l->data);
		sp_gradient_ensure_vector (gr);
		if (gr->has_stops) gl = g_slist_prepend (gl, gr);
	}
	/* Do not create menu, if there are no gradients */
	if (!gl) return;
	gl = g_slist_reverse (gl);

	menu = gtk_menu_new ();
	gtk_widget_show (menu);

	while (gl) {
		SPGradient *gr;
		GtkWidget *w, *i, *hb, *g;
		gr = SP_GRADIENT (gl->data);
		gl = g_slist_remove (gl, gr);

		i = gtk_menu_item_new ();
		gtk_widget_show (i);
		gtk_object_set_data (GTK_OBJECT (i), "gradient", gr);
		gtk_signal_connect (GTK_OBJECT (i), "activate",
				    GTK_SIGNAL_FUNC (sp_gradient_selector_vector_activate), spw);

		hb = gtk_hbox_new (FALSE, 4);
		gtk_widget_show (hb);
		gtk_container_add (GTK_CONTAINER (i), hb);

		w = gtk_label_new (SP_OBJECT_ID (gr));
		gtk_widget_show (w);
		gtk_box_pack_start (GTK_BOX (hb), w, TRUE, TRUE, 4);

		g = sp_gradient_image_new (gr);
		gtk_widget_show (g);
		gtk_box_pack_start (GTK_BOX (hb), g, FALSE, FALSE, 4);

		gtk_menu_append (GTK_MENU (menu), i);
	}

	gtk_option_menu_set_menu (vectors, menu);
}

static void
sp_gradient_selector_vector_activate (GtkMenuItem *mi, SPWidget *spw)
{
	SPGradient *gr;
	const GSList *selected, *l;

	gr = gtk_object_get_data (GTK_OBJECT (mi), "gradient");
	g_assert (gr != NULL);
	g_assert (SP_IS_GRADIENT (gr));
	/* Hmmm... bad things may happen here, as actual gradient is something new */
	gr = sp_gradient_ensure_vector_normalized (gr);

	gtk_object_set_data (GTK_OBJECT (spw), "gradient", gr);

	selected = sp_selection_item_list (SP_DT_SELECTION (spw->desktop));

	for (l = selected; l != NULL; l = l->next) {
		sp_item_force_fill_lineargradient_vector (SP_ITEM (l->data), gr);
	}

	sp_document_done (spw->document);
}

static void
sp_gradient_selector_edit_vector_clicked (GtkWidget *w, SPWidget *spw)
{
	SPGradient *gradient;

	gradient = gtk_object_get_data (GTK_OBJECT (spw), "gradient");

	sp_gradient_vector_dialog (gradient);
}

static void
sp_gradient_selector_add_vector_clicked (GtkWidget *w, SPWidget *spw)
{
	SPDefs *defs;
	SPRepr *repr, *stop;

	if (!SP_ACTIVE_DOCUMENT) return;

	defs = SP_DOCUMENT_DEFS (SP_ACTIVE_DOCUMENT);

	repr = sp_repr_new ("linearGradient");
	stop = sp_repr_new ("stop");
	sp_repr_set_attr (stop, "offset", "0");
	sp_repr_set_attr (stop, "style", "stop-color:#000;stop-opacity:1;");
	sp_repr_append_child (repr, stop);
	stop = sp_repr_new ("stop");
	sp_repr_set_attr (stop, "offset", "1");
	sp_repr_set_attr (stop, "style", "stop-color:#fff;stop-opacity:1;");
	sp_repr_append_child (repr, stop);

	sp_repr_add_child (SP_OBJECT_REPR (defs), repr, NULL);

	/* fixme: */
	sp_gradient_selector_vector_menu_refresh (gtk_object_get_data (GTK_OBJECT (spw), "vectors"), spw);
	/* fixme: */
	if (spw->desktop) sp_gradient_selector_load_selection (spw, SP_DT_SELECTION (spw->desktop));
}

static void
sp_gradient_selector_delete_vector_clicked (GtkWidget *w, SPWidget *spw)
{
}

static void
sp_gradient_selector_reset_clicked (GtkWidget *w, SPWidget *spw)
{
	SPSelection *sel;
	const GSList *items, *l;
	SPGradient *vector;

	if (!spw->desktop) return;
	sel = SP_DT_SELECTION (spw->desktop);

	items = sp_selection_item_list (sel);
	if (!items) return;

	vector = gtk_object_get_data (GTK_OBJECT (spw), "gradient");
	if (!vector) return;
	vector = sp_gradient_ensure_vector_normalized (vector);
	gtk_object_set_data (GTK_OBJECT (spw), "gradient", vector);

	for (l = items; l != NULL; l = l->next) {
		SPStyle *style;
		sp_item_force_fill_lineargradient_vector (SP_ITEM (l->data), vector);
		style = SP_OBJECT_STYLE (l->data);
		g_return_if_fail (style->fill.type == SP_PAINT_TYPE_PAINTSERVER);
		/* fixme: Managing selection bbox/item bbox stuff is big mess */
		sp_repr_set_double_attribute (SP_OBJECT_REPR (style->fill.server), "x1", 0.0);
		sp_repr_set_double_attribute (SP_OBJECT_REPR (style->fill.server), "y1", 0.0);
		sp_repr_set_double_attribute (SP_OBJECT_REPR (style->fill.server), "x2", 1.0);
		sp_repr_set_double_attribute (SP_OBJECT_REPR (style->fill.server), "y2", 0.0);
	}

	sp_document_done (spw->document);
}

static void
sp_gradient_selector_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, gpointer data)
{
	sp_gradient_selector_load_selection (spw, selection);
}

static void
sp_gradient_selector_change_selection (SPWidget *spw, SPSelection *selection, gpointer data)
{
	sp_gradient_selector_load_selection (spw, selection);
}

static void
sp_gradient_selector_load_selection (SPWidget *spw, SPSelection *selection)
{
	SPGradient *gradient;
	GtkWidget *vectors, *menu, *position;
	const GSList *items, *i;
	GList *children, *l;
	gint pos;
	ArtDRect bbox;
	SPLinearGradient *lg;

	if (!selection) return;

	items = sp_selection_item_list (selection);

	gradient = NULL;
	for (i = items; i != NULL; i = i->next) {
		SPStyle *style;
		style = SP_OBJECT_STYLE (i->data);
		if ((style->fill.type == SP_PAINT_TYPE_PAINTSERVER) && SP_IS_LINEARGRADIENT (style->fill.server)) {
			gradient = SP_GRADIENT (style->fill.server);
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

	gtk_object_set_data (GTK_OBJECT (spw), "gradient", gradient);
	gtk_option_menu_set_history (GTK_OPTION_MENU (vectors), pos);

	/* Update position */
	position = gtk_object_get_data (GTK_OBJECT (spw), "position");
	gradient = NULL;
	for (i = items; i != NULL; i = i->next) {
		SPStyle *style;
		style = SP_OBJECT_STYLE (i->data);
		if ((style->fill.type == SP_PAINT_TYPE_PAINTSERVER) && SP_IS_LINEARGRADIENT (style->fill.server)) {
			gradient = SP_GRADIENT (style->fill.server);
			break;
		}
	}
	g_assert (gradient != NULL);
	sp_gradient_position_set_gradient (SP_GRADIENT_POSITION (position), SP_GRADIENT (gradient));
	sp_selection_bbox (selection, &bbox);
	sp_gradient_position_set_bbox (SP_GRADIENT_POSITION (position), bbox.x0, bbox.y0, bbox.x1, bbox.y1);
	lg = SP_LINEARGRADIENT (gradient);
	sp_gradient_position_set_vector (SP_GRADIENT_POSITION (position), lg->x1.distance, lg->y1.distance, lg->x2.distance, lg->y2.distance);
}

static void
sp_gradient_selection_position_dragged (SPGradientPosition *pos, SPWidget *spw)
{
	SPSelection *sel;
	const GSList *items, *l;
	SPGradient *vector;

	if (!spw->desktop) return;
	sel = SP_DT_SELECTION (spw->desktop);

	items = sp_selection_item_list (sel);
	if (!items) return;

	vector = gtk_object_get_data (GTK_OBJECT (spw), "gradient");
	if (!vector) return;
	vector = sp_gradient_ensure_vector_normalized (vector);
	gtk_object_set_data (GTK_OBJECT (spw), "gradient", vector);

	for (l = items; l != NULL; l = l->next) {
		SPStyle *style;
		sp_item_force_fill_lineargradient_vector (SP_ITEM (l->data), vector);
		style = SP_OBJECT_STYLE (l->data);
		g_return_if_fail (style->fill.type == SP_PAINT_TYPE_PAINTSERVER);
		/* fixme: Managing selection bbox/item bbox stuff is big mess */
		sp_lineargradient_set_position (SP_LINEARGRADIENT (style->fill.server), pos->p0.x, pos->p0.y, pos->p1.x, pos->p1.y);
	}
}

static void
sp_gradient_selection_position_changed (SPGradientPosition *pos, SPWidget *spw)
{
	SPSelection *sel;
	const GSList *items, *l;
	SPGradient *vector;

	if (!spw->desktop) return;
	sel = SP_DT_SELECTION (spw->desktop);

	items = sp_selection_item_list (sel);
	if (!items) return;

	vector = gtk_object_get_data (GTK_OBJECT (spw), "gradient");
	if (!vector) return;
	vector = sp_gradient_ensure_vector_normalized (vector);
	gtk_object_set_data (GTK_OBJECT (spw), "gradient", vector);

	for (l = items; l != NULL; l = l->next) {
		SPStyle *style;
		sp_item_force_fill_lineargradient_vector (SP_ITEM (l->data), vector);
		style = SP_OBJECT_STYLE (l->data);
		g_return_if_fail (style->fill.type == SP_PAINT_TYPE_PAINTSERVER);
		/* fixme: Managing selection bbox/item bbox stuff is big mess */
		sp_repr_set_double_attribute (SP_OBJECT_REPR (style->fill.server), "x1", pos->p0.x);
		sp_repr_set_double_attribute (SP_OBJECT_REPR (style->fill.server), "y1", pos->p0.y);
		sp_repr_set_double_attribute (SP_OBJECT_REPR (style->fill.server), "x2", pos->p1.x);
		sp_repr_set_double_attribute (SP_OBJECT_REPR (style->fill.server), "y2", pos->p1.y);
	}

	sp_document_done (spw->document);
}


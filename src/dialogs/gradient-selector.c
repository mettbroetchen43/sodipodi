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

#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkoptionmenu.h>
#include "../widgets/gradient-image.h"
#include "../sodipodi.h"
#include "../document.h"
#include "../desktop-handles.h"
#include "../selection.h"
#include "../sp-gradient.h"
#include "../sp-item.h"
#include "../gradient-chemistry.h"
#include "sp-widget.h"
#include "gradient-selector.h"

static GtkWidget *sp_gradient_selector_vector_menu_new (SPWidget *spw);
static void sp_gradient_selector_vector_menu_refresh (GtkOptionMenu *vectors, SPWidget *spw);
static void sp_gradient_selector_vector_activate (GtkMenuItem *mi, SPWidget *spw);

GtkWidget *
sp_gradient_widget_new (void)
{
	GtkWidget *spw, *vectors;

	spw = sp_widget_new (SODIPODI, SP_ACTIVE_DESKTOP, SP_ACTIVE_DOCUMENT);

	vectors = sp_gradient_selector_vector_menu_new (SP_WIDGET (spw));
	gtk_widget_show (vectors);
	gtk_container_add (GTK_CONTAINER (spw), vectors);

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
		gtk_box_pack_start (GTK_BOX (hb), g, TRUE, TRUE, 4);

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
	gr = sp_gradient_ensure_vector_normalized (gr);

	selected = sp_selection_item_list (SP_DT_SELECTION (spw->desktop));

	for (l = selected; l != NULL; l = l->next) {
		sp_item_force_fill_lineargradient_vector (SP_ITEM (l->data), gr);
	}
}

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
#include "../sp-gradient.h"
#include "sp-widget.h"
#include "gradient-selector.h"

GtkWidget *
sp_gradient_widget_new (void)
{
	GtkWidget *spw, *m, *om;
	const GSList *gradients, *l;

	spw = sp_widget_new (SODIPODI, SP_ACTIVE_DESKTOP, SP_ACTIVE_DOCUMENT);

	m = gtk_menu_new ();
	gtk_widget_show (m);

	gradients = sp_document_get_resource_list (SP_ACTIVE_DOCUMENT, "gradient");
	for (l = gradients; l != NULL; l = l->next) {
		SPGradient *gradient;
		GtkWidget *w, *i, *hb, *g;
		gradient = SP_GRADIENT (l->data);
		i = gtk_menu_item_new ();
		gtk_widget_show (i);
		hb = gtk_hbox_new (FALSE, 4);
		gtk_widget_show (hb);
		gtk_container_add (GTK_CONTAINER (i), hb);

		g = sp_gradient_image_new (gradient);
		gtk_widget_show (g);
		gtk_box_pack_start (GTK_BOX (hb), g, TRUE, TRUE, 4);

		w = gtk_label_new (SP_OBJECT_ID (gradient));
		gtk_widget_show (w);
		gtk_box_pack_start (GTK_BOX (hb), w, TRUE, TRUE, 4);

		gtk_menu_append (GTK_MENU (m), i);
	}

	om = gtk_option_menu_new ();
	gtk_widget_show (om);
	gtk_option_menu_set_menu (GTK_OPTION_MENU (om), m);

	gtk_container_add (GTK_CONTAINER (spw), om);

	return spw;
}


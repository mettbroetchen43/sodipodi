#define __SP_OBJECT_ATTRIBUTES_C__

/*
 * Link - <a> element - properties editor
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Licensed under GNU GPL
 */

#include <gtk/gtkwindow.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "../sp-anchor.h"
#include "sp-attribute-widget.h"
#include "object-attributes.h"

static const guchar *anchor_labels[] = {"Type:", "Role:", "Arcrole:", "Title:", "Show:", "Actuate:", "Href:", "Target:"};
static const guchar *anchor_attrs[] = {"xlink:type", "xlink:role", "xlink:arcrole", "xlink:title", "xlink:show",
				       "xlink:actuate", "xlink:href", "target"};
#define NUM_ANCHOR_ATTRS (sizeof (anchor_attrs) / sizeof (anchor_attrs[0]))

static const guchar *star_labels[] = {
	N_("Sides:"),
	N_("Center X:"),
	N_("Center Y:"),
	N_("R1:"),
	N_("R2:"),
	N_("ARG1:"),
	N_("ARG2:")};
static const guchar *star_attrs[] = {
	"sodipodi:sides",
	"sodipodi:cx",
	"sodipodi:cy",
	"sodipodi:r1",
	"sodipodi:r2",
	"sodipodi:arg1",
	"sodipodi:arg2"};
#define NUM_STAR_ATTRS (sizeof (star_attrs) / sizeof (star_attrs[0]))


static void
object_destroyed (GtkObject *object, GtkWidget *widget)
{
	gtk_widget_destroy (widget);
}

void
sp_object_attributes_dialog (SPObject *object, const guchar *tag)
{
	const guchar **labels, **attrs;
	GtkWidget *w, *t;
	gint num_attrs;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));
	g_return_if_fail (tag != NULL);

	num_attrs = 0;
	labels = NULL;
	attrs = NULL;

	if (!strcmp (tag, "SPAnchor")) {
		g_return_if_fail (SP_IS_ANCHOR (object));

		num_attrs = NUM_ANCHOR_ATTRS;
		labels = anchor_labels;
		attrs = anchor_attrs;
	} else if (!strcmp (tag, "SPStar")) {
		num_attrs = NUM_STAR_ATTRS;
		labels = star_labels;
		attrs = star_attrs;
	}


	if (num_attrs && labels && attrs) {
		w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (w), _("Link properties"));

		t = sp_attribute_table_new (object, num_attrs, labels, attrs);
		gtk_widget_show (t);
		gtk_container_add (GTK_CONTAINER (w), t);

		gtk_signal_connect_while_alive (GTK_OBJECT (object), "destroy",
						GTK_SIGNAL_FUNC (object_destroyed), w, GTK_OBJECT (w));

		gtk_widget_show (w);
	}
}


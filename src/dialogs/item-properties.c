#define __SP_ITEM_PROPERTIES_C__

/*
 * Display settings dialog
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <glade/glade.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtksignal.h>
#include "../svg/svg.h"
#include "../document.h"
#include "../sp-item.h"
#include "../style.h"
#include "item-properties.h"

static GladeXML  * xml = NULL;
static GtkWidget * dialog = NULL;
static SPItem *dialog_item = NULL;

static void sp_item_dialog_setup (SPItem *item);

static gint sp_item_dialog_delete (GtkWidget *widget, GdkEvent *event);

void sp_item_dialog_sensitive_toggled (GtkWidget *widget, gpointer data);
void sp_item_dialog_id_changed (GtkWidget *widget, gpointer data);
void sp_item_dialog_opacity_changed (GtkWidget *widget, gpointer data);
void sp_item_dialog_transform_changed (GtkWidget *widget, gpointer data);
static void sp_item_dialog_item_destroy (GtkObject *object, gpointer data);

void
sp_item_dialog (SPItem *item)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));

	if (dialog == NULL) {
		g_assert (xml == NULL);
		xml = glade_xml_new (SODIPODI_GLADEDIR "/item.glade", "item_dialog");
		glade_xml_signal_autoconnect (xml);
		dialog = glade_xml_get_widget (xml, "item_dialog");
		gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
				    GTK_SIGNAL_FUNC (sp_item_dialog_delete), NULL);
	} else {
		if (!GTK_WIDGET_VISIBLE (dialog)) gtk_widget_show (dialog);
	}

	if (dialog_item) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (dialog_item), dialog);
		dialog_item = NULL;
	}

	dialog_item = item;
	gtk_signal_connect (GTK_OBJECT (item), "destroy",
			    GTK_SIGNAL_FUNC (sp_item_dialog_item_destroy), dialog);

	sp_item_dialog_setup (item);
}

static void
sp_item_dialog_setup (SPItem *item)
{
	SPObject *object;
	SPRepr *repr;
	const guchar *str;
	GtkWidget * w;

	object = SP_OBJECT (item);
	repr = object->repr;

	/* Sensitive */
	str = sp_repr_attr (repr, "insensitive");
	w = glade_xml_get_widget (xml, "sensitive");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), (str == NULL));
	
	/* Id */
	if (SP_OBJECT_IS_CLONED (object)) {
		w = glade_xml_get_widget (xml, "id");
		gtk_entry_set_text (GTK_ENTRY (w), "");
		gtk_widget_set_sensitive (w, FALSE);
		w = glade_xml_get_widget (xml, "id_comment");
		gtk_label_set_text (GTK_LABEL (w), _("Item is reference"));
	} else {
		w = glade_xml_get_widget (xml, "id");
		gtk_entry_set_text (GTK_ENTRY (w), object->id);
		gtk_widget_set_sensitive (w, TRUE);
		w = glade_xml_get_widget (xml, "id_comment");
		gtk_label_set_text (GTK_LABEL (w), _("The SVG ID of item"));
	}

	/* Opacity */
	w = glade_xml_get_widget (xml, "opacity");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), object->style->opacity);

	/* Transform */
	w = glade_xml_get_widget (xml, "transform_0");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), item->affine[0]);
	w = glade_xml_get_widget (xml, "transform_1");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), item->affine[1]);
	w = glade_xml_get_widget (xml, "transform_2");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), item->affine[2]);
	w = glade_xml_get_widget (xml, "transform_3");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), item->affine[3]);
	w = glade_xml_get_widget (xml, "transform_4");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), item->affine[4]);
	w = glade_xml_get_widget (xml, "transform_5");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), item->affine[5]);

	/* Apply button */
	w = glade_xml_get_widget (xml, "apply");
	gtk_widget_set_sensitive (w, FALSE);
}

void
sp_item_dialog_close (GtkWidget * widget)
{
	g_assert (dialog != NULL);

	if (dialog_item) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (dialog_item), dialog);
		dialog_item = NULL;
	}

	if (GTK_WIDGET_VISIBLE (dialog)) gtk_widget_hide (dialog);
}

static gint
sp_item_dialog_delete (GtkWidget *widget, GdkEvent *event)
{
	sp_item_dialog_close (widget);

	return TRUE;
}

void
sp_item_dialog_apply (GtkWidget * widget)
{
	SPObject *object;
	SPRepr *repr;
	GtkWidget *w;
	const gchar *str;
	SPStyle *style;
	gchar *s;
	gdouble a[6];
	guchar c[256];

	g_assert (dialog != NULL);

	g_return_if_fail (dialog_item != NULL);

	object = SP_OBJECT (dialog_item);
	repr = object->repr;

	/* Sensitive */
	w = glade_xml_get_widget (xml, "sensitive");
	sp_repr_set_attr (repr, "insensitive", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)) ? NULL : "true");

	/* Id */
	w = glade_xml_get_widget (xml, "id");
	s = gtk_entry_get_text (GTK_ENTRY (w));
	if (s && *s && isalnum (*s) && strcmp (s, object->id) && !SP_OBJECT_IS_CLONED (object) && !sp_document_lookup_id (object->document, s)) {
		sp_repr_set_attr (repr, "id", s);
	}

	/* Opacity */
	str = sp_repr_attr (repr, "style");
	style = sp_style_new ();
	sp_style_read_from_string (style, str, object->document);
	w = glade_xml_get_widget (xml, "opacity");
	style->opacity = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	style->opacity_set = TRUE;
	s = sp_style_write_string (style);
	sp_style_unref (style);
	sp_repr_set_attr (repr, "style", s);
	g_free (s);

	/* Transform */
	w = glade_xml_get_widget (xml, "transform_0");
	a[0] = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	w = glade_xml_get_widget (xml, "transform_1");
	a[1] = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	w = glade_xml_get_widget (xml, "transform_2");
	a[2] = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	w = glade_xml_get_widget (xml, "transform_3");
	a[3] = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	w = glade_xml_get_widget (xml, "transform_4");
	a[4] = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	w = glade_xml_get_widget (xml, "transform_5");
	a[5] = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
	sp_svg_write_affine (c, 256, a);
	sp_repr_set_attr (repr, "transform", c);

	sp_document_done (object->document);

	sp_item_dialog_setup (dialog_item);
}

void
sp_item_dialog_sensitive_toggled (GtkWidget *widget, gpointer data)
{
	GtkWidget *w;

	g_assert (dialog != NULL);

	w = glade_xml_get_widget (xml, "apply");
	gtk_widget_set_sensitive (w, TRUE);
}

void
sp_item_dialog_id_changed (GtkWidget *widget, gpointer data)
{
	SPObject *o;
	GtkWidget *w;
	gchar *id;

	g_assert (dialog != NULL);
	g_assert (dialog_item != NULL);

	o = SP_OBJECT (dialog_item);

	w = glade_xml_get_widget (xml, "id");
	id = gtk_entry_get_text (GTK_ENTRY (w));
	w = glade_xml_get_widget (xml, "id_comment");
	if (!strcmp (id, o->id)) {
		gtk_label_set_text (GTK_LABEL (w), _("The SVG ID of item"));
	} else if (!*id || !isalnum (*id)) {
		gtk_label_set_text (GTK_LABEL (w), _("The ID is not valid"));
	} else if (sp_document_lookup_id (o->document, id)) {
		gtk_label_set_text (GTK_LABEL (w), _("The ID is already defined"));
	} else {
		gtk_label_set_text (GTK_LABEL (w), _("The ID is valid"));
	}

	w = glade_xml_get_widget (xml, "apply");
	gtk_widget_set_sensitive (w, TRUE);
}

void
sp_item_dialog_opacity_changed (GtkWidget *widget, gpointer data)
{
	GtkWidget *w;

	g_assert (dialog != NULL);

	w = glade_xml_get_widget (xml, "apply");
	gtk_widget_set_sensitive (w, TRUE);
}

void
sp_item_dialog_transform_changed (GtkWidget *widget, gpointer data)
{
	GtkWidget *w;

	g_assert (dialog != NULL);

	w = glade_xml_get_widget (xml, "apply");
	gtk_widget_set_sensitive (w, TRUE);
}

static void
sp_item_dialog_item_destroy (GtkObject *object, gpointer data)
{
	sp_item_dialog_close (dialog);
}



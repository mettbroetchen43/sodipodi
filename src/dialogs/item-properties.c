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
#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include "../svg/svg.h"
#include "../sodipodi.h"
#include "../document.h"
#include "../desktop-handles.h"
#include "../selection.h"
#include "../sp-item.h"
#include "../style.h"
#include "sp-widget.h"
#include "item-properties.h"

/* fixme: This sucks, we should really use per-widget xml */
static GladeXML *xml = NULL;

static GtkWidget * dialog = NULL;
#if 0
static SPItem *dialog_item = NULL;
#endif

static void sp_item_widget_destroy (GtkObject *object);
static void sp_item_widget_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, GtkWidget *itemw);
static void sp_item_widget_change_selection (SPWidget *spw, SPSelection *selection, GtkWidget *itemw);
static void sp_item_widget_setup (SPWidget *spw, SPSelection *selection);
static void sp_item_widget_sensitivity_toggled (GtkWidget *widget, SPWidget *spw);
static void sp_item_widget_id_changed (GtkWidget *widget, SPWidget *spw);
static void sp_item_widget_opacity_changed (GtkWidget *widget, SPWidget *spw);
static void sp_item_widget_transform_changed (GtkWidget *widget, SPWidget *spw);

static void sp_item_dialog_setup (SPItem *item);

static gint sp_item_dialog_delete (GtkWidget *widget, GdkEvent *event);

void sp_item_dialog_sensitive_toggled (GtkWidget *widget, gpointer data);
void sp_item_dialog_id_changed (GtkWidget *widget, gpointer data);
void sp_item_dialog_opacity_changed (GtkWidget *widget, gpointer data);
void sp_item_dialog_transform_changed (GtkWidget *widget, gpointer data);

static gboolean blocked = FALSE;

/* Creates new instance of item properties widget */

GtkWidget *
sp_item_widget_new (void)
{
	GtkWidget *spw, *itemw, *w;

	/* Read widget backbone from glade file */
	xml = glade_xml_new (SODIPODI_GLADEDIR "/item.glade", "item_widget");
	g_return_val_if_fail (xml != NULL, NULL);
	itemw = glade_xml_get_widget (xml, "item_widget");
	/* Create container widget */
	spw = sp_widget_new (SODIPODI, SP_ACTIVE_DESKTOP, SP_ACTIVE_DOCUMENT);
	gtk_object_set_data (GTK_OBJECT (spw), "xml", xml);
	gtk_signal_connect (GTK_OBJECT (spw), "destroy", GTK_SIGNAL_FUNC (sp_item_widget_destroy), NULL);
	gtk_signal_connect (GTK_OBJECT (spw), "modify_selection", GTK_SIGNAL_FUNC (sp_item_widget_modify_selection), itemw);
	gtk_signal_connect (GTK_OBJECT (spw), "change_selection", GTK_SIGNAL_FUNC (sp_item_widget_change_selection), itemw);
	/* Connect handlers */
	w = glade_xml_get_widget (xml, "sensitive");
	gtk_signal_connect (GTK_OBJECT (w), "toggled", GTK_SIGNAL_FUNC (sp_item_widget_sensitivity_toggled), spw);
	w = glade_xml_get_widget (xml, "id");
	gtk_signal_connect (GTK_OBJECT (w), "changed", GTK_SIGNAL_FUNC (sp_item_widget_id_changed), spw);
	w = glade_xml_get_widget (xml, "opacity");
	gtk_signal_connect (GTK_OBJECT (w), "changed", GTK_SIGNAL_FUNC (sp_item_widget_opacity_changed), spw);
	w = glade_xml_get_widget (xml, "transform_0");
	gtk_object_set_data (GTK_OBJECT (w), "pos", GINT_TO_POINTER (0));
	gtk_signal_connect (GTK_OBJECT (w), "changed", GTK_SIGNAL_FUNC (sp_item_widget_transform_changed), spw);
	w = glade_xml_get_widget (xml, "transform_1");
	gtk_object_set_data (GTK_OBJECT (w), "pos", GINT_TO_POINTER (1));
	gtk_signal_connect (GTK_OBJECT (w), "changed", GTK_SIGNAL_FUNC (sp_item_widget_transform_changed), spw);
	w = glade_xml_get_widget (xml, "transform_2");
	gtk_object_set_data (GTK_OBJECT (w), "pos", GINT_TO_POINTER (2));
	gtk_signal_connect (GTK_OBJECT (w), "changed", GTK_SIGNAL_FUNC (sp_item_widget_transform_changed), spw);
	w = glade_xml_get_widget (xml, "transform_3");
	gtk_object_set_data (GTK_OBJECT (w), "pos", GINT_TO_POINTER (3));
	gtk_signal_connect (GTK_OBJECT (w), "changed", GTK_SIGNAL_FUNC (sp_item_widget_transform_changed), spw);
	w = glade_xml_get_widget (xml, "transform_4");
	gtk_object_set_data (GTK_OBJECT (w), "pos", GINT_TO_POINTER (4));
	gtk_signal_connect (GTK_OBJECT (w), "changed", GTK_SIGNAL_FUNC (sp_item_widget_transform_changed), spw);
	w = glade_xml_get_widget (xml, "transform_5");
	gtk_object_set_data (GTK_OBJECT (w), "pos", GINT_TO_POINTER (5));
	gtk_signal_connect (GTK_OBJECT (w), "changed", GTK_SIGNAL_FUNC (sp_item_widget_transform_changed), spw);
	/* Add widget to container */
	gtk_container_add (GTK_CONTAINER (spw), itemw);

	sp_item_widget_setup (SP_WIDGET (spw), SP_DT_SELECTION (SP_ACTIVE_DESKTOP));

	return (GtkWidget *) spw;
}

static void
sp_item_widget_destroy (GtkObject *object)
{
	GladeXML *xml;

	xml = gtk_object_get_data (GTK_OBJECT (object), "xml");

	gtk_object_unref (GTK_OBJECT (xml));
}

static void
sp_item_widget_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, GtkWidget *itemw)
{
	if (blocked) return;

	sp_item_widget_setup (spw, selection);
}

static void
sp_item_widget_change_selection (SPWidget *spw, SPSelection *selection, GtkWidget *itemw)
{
	if (blocked) return;

	sp_item_widget_setup (spw, selection);
}

static void
sp_item_widget_setup (SPWidget *spw, SPSelection *selection)
{
	GladeXML *xml;
	SPItem *item;
	SPObject *object;
	SPRepr *repr;
	const guchar *str;
	GtkWidget * w;

	if (blocked) return;

	if (!selection || !sp_selection_item (selection)) {
		gtk_widget_set_sensitive (GTK_WIDGET (spw), FALSE);
		return;
	} else {
		gtk_widget_set_sensitive (GTK_WIDGET (spw), TRUE);
	}

	blocked = TRUE;

	xml = gtk_object_get_data (GTK_OBJECT (spw), "xml");
	item = sp_selection_item (selection);
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

	blocked = FALSE;
}

static void
sp_item_widget_sensitivity_toggled (GtkWidget *widget, SPWidget *spw)
{
	SPItem *item;
	SPException ex;

	if (blocked) return;

	item = sp_selection_item (SP_DT_SELECTION (spw->desktop));
	g_return_if_fail (item != NULL);

	blocked = TRUE;

	SP_EXCEPTION_INIT (&ex);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
		sp_object_removeAttribute (SP_OBJECT (item), "insensitive", &ex);
	} else {
		sp_object_setAttribute (SP_OBJECT (item), "insensitive", "true", &ex);
	}

	sp_document_maybe_done (spw->document, "ItemDialog:insensitive");

	blocked = FALSE;
}

static void
sp_item_widget_id_changed (GtkWidget *widget, SPWidget *spw)
{
	GladeXML *xml;
	SPItem *item;
	GtkWidget *w;
	gchar *id;

	if (blocked) return;

	item = sp_selection_item (SP_DT_SELECTION (spw->desktop));
	g_return_if_fail (item != NULL);
	xml = gtk_object_get_data (GTK_OBJECT (spw), "xml");
	g_return_if_fail (xml != NULL);

	blocked = TRUE;

	w = glade_xml_get_widget (xml, "id");
	id = gtk_entry_get_text (GTK_ENTRY (w));
	w = glade_xml_get_widget (xml, "id_comment");
	if (!strcmp (id, ((SPObject *) item)->id)) {
		gtk_label_set_text (GTK_LABEL (w), _("The SVG ID of item"));
	} else if (!*id || !isalnum (*id)) {
		gtk_label_set_text (GTK_LABEL (w), _("The ID is not valid"));
	} else if (sp_document_lookup_id (spw->document, id)) {
		gtk_label_set_text (GTK_LABEL (w), _("The ID is already defined"));
	} else {
		SPException ex;
		gtk_label_set_text (GTK_LABEL (w), _("The ID is valid"));
		SP_EXCEPTION_INIT (&ex);
		sp_object_setAttribute (SP_OBJECT (item), "id", id, &ex);
		sp_document_maybe_done (spw->document, "ItemDialog:id");
	}

	blocked = FALSE;
}

static void
sp_item_widget_opacity_changed (GtkWidget *widget, SPWidget *spw)
{
	SPItem *item;
	const gchar *str;
	SPStyle *style;
	gchar *s;
	SPException ex;

	if (blocked) return;

	item = sp_selection_item (SP_DT_SELECTION (spw->desktop));
	g_return_if_fail (item != NULL);

	blocked = TRUE;

	/* Opacity */
	str = sp_repr_attr (((SPObject *) item)->repr, "style");
	style = sp_style_new ();
	sp_style_read_from_string (style, str, spw->document);
	style->opacity = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (widget));
	style->opacity_set = TRUE;
	s = sp_style_write_string (style);
	sp_style_unref (style);
	SP_EXCEPTION_INIT (&ex);
	sp_object_setAttribute (SP_OBJECT (item), "style", s, &ex);
	g_free (s);

	sp_document_maybe_done (spw->document, "ItemDialog:style");

	blocked = FALSE;
}

static void
sp_item_widget_transform_changed (GtkWidget *widget, SPWidget *spw)
{
	SPItem *item;
	gdouble a[6], t;
	gint pos;
	guchar c[256];
	SPException ex;

	if (blocked) return;

	item = sp_selection_item (SP_DT_SELECTION (spw->desktop));
	g_return_if_fail (item != NULL);
	pos = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), "pos"));

	blocked = TRUE;

	t = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (widget));
	if (t != a[pos]) {
		memcpy (a, item->affine, 6 * sizeof (gdouble));
		a[pos] = t;
		sp_svg_write_affine (c, 256, a);
		SP_EXCEPTION_INIT (&ex);
		sp_object_setAttribute (SP_OBJECT (item), "transform", c, &ex);

		sp_document_maybe_done (spw->document, "ItemDialog:transform");
	}

	blocked = FALSE;
}

void
sp_item_dialog (SPItem *item)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));

	if (dialog == NULL) {
		GtkWidget *itemw;
		dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (dialog), _("Item properties"));
		gtk_signal_connect (GTK_OBJECT (dialog), "delete_event", GTK_SIGNAL_FUNC (sp_item_dialog_delete), NULL);
		itemw = sp_item_widget_new ();
		/* Connect signals */
		gtk_widget_show (itemw);
		gtk_container_add (GTK_CONTAINER (dialog), itemw);
	}

	if (!GTK_WIDGET_VISIBLE (dialog)) gtk_widget_show (dialog);

#if 0
	if (dialog_item) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (dialog_item), dialog);
		dialog_item = NULL;
	}
#endif

#if 0
	dialog_item = item;
	gtk_signal_connect (GTK_OBJECT (item), "destroy",
			    GTK_SIGNAL_FUNC (sp_item_dialog_item_destroy), dialog);
#endif

	sp_item_dialog_setup (item);
}

static void
sp_item_dialog_setup (SPItem *item)
{
	GtkWidget * w;

	/* Apply button */
	w = glade_xml_get_widget (xml, "apply");
	gtk_widget_set_sensitive (w, FALSE);
}

void
sp_item_dialog_close (GtkWidget * widget)
{
	g_assert (dialog != NULL);

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
	SPItem *item;
	SPObject *object;
	SPRepr *repr;
	GtkWidget *w;
	const gchar *str;
	SPStyle *style;
	gchar *s;
	gdouble a[6];
	guchar c[256];

	g_assert (dialog != NULL);

	/* fixme: */
	item = sp_selection_item (SP_DT_SELECTION (SP_ACTIVE_DESKTOP));
	g_return_if_fail (item != NULL);

	object = SP_OBJECT (item);
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

	sp_item_dialog_setup (item);
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
#if 0
	SPItem *item;
	SPObject *o;
	GtkWidget *w;
	gchar *id;

	g_assert (dialog != NULL);

	/* fixme: */
	item = sp_selection_item (SP_DT_SELECTION (SP_ACTIVE_DESKTOP));
	g_return_if_fail (item != NULL);

	o = SP_OBJECT (item);

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
#endif
}

void
sp_item_dialog_opacity_changed (GtkWidget *widget, gpointer data)
{
#if 0
	GtkWidget *w;

	g_assert (dialog != NULL);

	w = glade_xml_get_widget (xml, "apply");
	gtk_widget_set_sensitive (w, TRUE);
#endif
}

void
sp_item_dialog_transform_changed (GtkWidget *widget, gpointer data)
{
	GtkWidget *w;

	g_assert (dialog != NULL);

	w = glade_xml_get_widget (xml, "apply");
	gtk_widget_set_sensitive (w, TRUE);
}

#if 0
static void
sp_item_dialog_item_destroy (GtkObject *object, gpointer data)
{
	sp_item_dialog_close (dialog);
}
#endif



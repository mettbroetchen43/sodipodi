#define SP_TOOLBOX_C

/*
 * Toolbox
 *
 * Authors:
 *   Frank Felfe  <innerspace@iname.com>
 *   Lauris Kaplinski  <lauris@helixcode.com>
 *
 * Copyright (C) 2000-2001 Helix Code, Inc. and authors
 */

#include <config.h>
#include <gnome.h>
#include <libgnomeui/gnome-pixmap.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnomeui/gnome-window-icon.h>
#include <glade/glade.h>
#include "widgets/sp-toolbox.h"
#include "sodipodi-private.h"
#include "document.h"
#include "toolbox.h"
#include "sodipodi.h"
#include "event-broker.h"
#include "dialogs/transformation.h"
#include "zoom-context.h"
#include "selection.h"
#include "sp-item-transform.h"
#include "desktop-handles.h"

GtkWidget * sp_toolbox_create (GladeXML * xml,
			       const gchar * widgetname,
			       const gchar * name,
			       const gchar * internalname,
			       const gchar * pxname);

static gint sp_toolbox_set_state_handler (SPToolBox * t, guint state, gpointer data);
static void sp_update_draw_toolbox (Sodipodi * sodipodi, SPEventContext * eventcontext, gpointer data);
void object_flip (GtkWidget * widget, GdkEventButton * event);

static GladeXML  * toolbox_xml = NULL;
static GtkWidget * toolbox = NULL;
static GtkWidget * fh_pixmap = NULL;
static GtkWidget * fv_pixmap = NULL;
GtkWidget * zoom_any = NULL;

typedef enum {
	FLIP_HOR,
	FLIP_VER,
} SPObjectFlipMode;

SPObjectFlipMode object_flip_mode = FLIP_HOR;


void
sp_maintoolbox_create (void)
{
	if (toolbox == NULL) {
		GtkWidget * vbox, * t, * w;
		GladeXML * xml;

		/* Crete main toolbox */
		toolbox_xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "maintoolbox");
		g_return_if_fail (toolbox_xml != NULL);
		glade_xml_signal_autoconnect (toolbox_xml);

		toolbox = glade_xml_get_widget (toolbox_xml, "maintoolbox");
		g_return_if_fail (toolbox != NULL);

		vbox = glade_xml_get_widget (toolbox_xml, "main_vbox");

		/* File */
		xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "file_table");
		t = sp_toolbox_create (xml, "file_table", _("File"), "file", "toolbox_file.xpm");
		gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
		/* Edit */
		xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "edit_table");
		t = sp_toolbox_create (xml, "edit_table", _("Edit"), "edit", "toolbox_edit.xpm");
		gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
		w = glade_xml_get_widget (xml, "undo");
		gtk_object_set_data (GTK_OBJECT (t), "undo", w);
		w = glade_xml_get_widget (xml, "redo");
		gtk_object_set_data (GTK_OBJECT (t), "redo", w);
		gtk_object_set_data (GTK_OBJECT (toolbox), "edit", t);
		/* Object */
		xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "object_table");
		t = sp_toolbox_create (xml, "object_table", _("Object"), "object", "toolbox_object.xpm");
		gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
		fh_pixmap = glade_xml_get_widget (xml, "fh_pixmap");
		fv_pixmap = glade_xml_get_widget (xml, "fv_pixmap");
		object_flip_mode = FLIP_HOR;
		gtk_widget_show (fh_pixmap);
		/* Select */
		xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "select_table");
		t = sp_toolbox_create (xml, "select_table", _("Selection"), "selection", "toolbox_select.xpm");
		gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
		/* Draw */
		xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "draw_table");
		t = sp_toolbox_create (xml, "draw_table", _("Draw"), "draw", "toolbox_draw.xpm");
		gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
		w = glade_xml_get_widget (xml, "draw_select");
		gtk_object_set_data (GTK_OBJECT (t), "SPSelectContext", w);
		w = glade_xml_get_widget (xml, "draw_node");
		gtk_object_set_data (GTK_OBJECT (t), "SPNodeContext", w);
		w = glade_xml_get_widget (xml, "draw_zoom");
		gtk_object_set_data (GTK_OBJECT (t), "SPZoomContext", w);
		w = glade_xml_get_widget (xml, "draw_text");
		gtk_object_set_data (GTK_OBJECT (t), "SPTextContext", w);
		w = glade_xml_get_widget (xml, "draw_rect");
		gtk_object_set_data (GTK_OBJECT (t), "SPRectContext", w);
		w = glade_xml_get_widget (xml, "draw_arc");
		gtk_object_set_data (GTK_OBJECT (t), "SPArcContext", w);
		w = glade_xml_get_widget (xml, "draw_star");
		gtk_object_set_data (GTK_OBJECT (t), "SPStarContext", w);
		w = glade_xml_get_widget (xml, "draw_spiral");
		gtk_object_set_data (GTK_OBJECT (t), "SPSpiralContext", w);
		w = glade_xml_get_widget (xml, "draw_freehand");
		gtk_object_set_data (GTK_OBJECT (t), "SPDrawContext", w);
		w = glade_xml_get_widget (xml, "draw_dynahand");
		gtk_object_set_data (GTK_OBJECT (t), "SPDynaDrawContext", w);
		if (SP_ACTIVE_DESKTOP) {
			const gchar * tname;
			tname = gtk_type_name (GTK_OBJECT_TYPE (SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP)));
			w = gtk_object_get_data (GTK_OBJECT (t), tname);
			if (w != NULL) {
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
				gtk_object_set_data (GTK_OBJECT (t), "active", t);
			}
		}
		gtk_signal_connect_while_alive (GTK_OBJECT (SODIPODI), "set_eventcontext",
						GTK_SIGNAL_FUNC (sp_update_draw_toolbox), t, GTK_OBJECT (t));
		/* Zoom */
		xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "zoom_table");
		t = sp_toolbox_create (xml, "zoom_table", _("Zoom"), "zoom", "toolbox_zoom.xpm");
		gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
		/* Node */
		xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "node_table");
		t = sp_toolbox_create (xml, "node_table", _("Nodes"), "node", "toolbox_node.xpm");
		gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
	}

	/* Reference our sodipodi engine */
	if (!GTK_WIDGET_VISIBLE (toolbox)) sodipodi_ref ();

	gnome_window_icon_set_from_default (GTK_WINDOW (toolbox));
	gtk_widget_show (toolbox);
}

GtkWidget *
sp_toolbox_create (GladeXML * xml, const gchar * widgetname, const gchar * name, const gchar * internalname, const gchar * pxname)
{
	GtkWidget * t, * contents;
	SPRepr * repr;
	guint state;
	gchar c[256];

	glade_xml_signal_autoconnect (xml);
	contents = glade_xml_get_widget (xml, widgetname);

	g_snprintf (c, 256, SODIPODI_GLADEDIR "/%s", pxname);
	t = sp_toolbox_new (contents, name, internalname, c);

	g_snprintf (c, 256, "toolboxes.%s", internalname);
	repr = sodipodi_get_repr (SODIPODI, c);
	g_return_val_if_fail (repr != NULL, NULL);
	state = sp_repr_get_int_attribute (repr, "state", 0);
	sp_toolbox_set_state (SP_TOOLBOX (t), state);

	gtk_signal_connect (GTK_OBJECT (t), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

	gtk_widget_show (t);

	return t;
}

static gint
sp_toolbox_set_state_handler (SPToolBox * t, guint state, gpointer data)
{
	SPRepr * repr;

	repr = (SPRepr *) data;

	sp_repr_set_int_attribute (repr, "state", state);

	return FALSE;
}

gint
sp_maintoolbox_close (GtkWidget * widget, GdkEventAny * event, gpointer data)
{
	sodipodi_unref ();

	gtk_widget_hide (GTK_WIDGET (toolbox));

	return TRUE;
}

/* 
 * object toolbox
 */

void
object_flip (GtkWidget * widget, GdkEventButton * event) {
  SPDesktop * desktop;
  SPSelection * selection;
  SPItem * item;
  GSList * l, * l2;

  //right click
  if (event->button == 3) {
    if (object_flip_mode == FLIP_HOR) {
      object_flip_mode = FLIP_VER;
      gtk_widget_hide (fh_pixmap);
      gtk_widget_show (fv_pixmap);
    } else {
      object_flip_mode = FLIP_HOR;
      gtk_widget_hide (fv_pixmap);
      gtk_widget_show (fh_pixmap);
    };
  };

  // left click
  if (event->button == 1) {
    desktop = SP_ACTIVE_DESKTOP;
    if (!SP_IS_DESKTOP(desktop)) return;
    selection = SP_DT_SELECTION(desktop);
    if sp_selection_is_empty(selection) return;
    l = selection->items;  
    for (l2 = l; l2 != NULL; l2 = l2-> next) {
      item = SP_ITEM (l2->data);
      if (object_flip_mode == FLIP_HOR) sp_item_scale_rel (item,-1,1);
      else sp_item_scale_rel (item,1,-1);
    }

    sp_selection_changed (selection);
    sp_document_done (SP_DT_DOCUMENT (desktop));
  }

}

static void 
sp_update_draw_toolbox (Sodipodi * sodipodi, SPEventContext * eventcontext, gpointer data)
{
	const gchar * tname;
	gpointer active, new, e, w;

	tname = NULL;

	active = gtk_object_get_data (GTK_OBJECT (data), "active");

	if (eventcontext != NULL) {
		tname = gtk_type_name (GTK_OBJECT_TYPE (eventcontext));
		new = gtk_object_get_data (GTK_OBJECT (data), tname);
	} else {
		new = NULL;
	}

	if (new != active) {
		if (active) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (active), FALSE);
		if (new) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (new), TRUE);
		gtk_object_set_data (GTK_OBJECT (data), "active", new);
		if ((tname) && ((!strcmp (tname, "SPNodeContext")) || (!strcmp (tname, "SPDrawContext")) || (!strcmp (tname, "SPDynaDrawContext")))) {
			e = gtk_object_get_data (GTK_OBJECT (toolbox), "edit");
			w = gtk_object_get_data (GTK_OBJECT (e), "undo");
			gtk_widget_set_sensitive (GTK_WIDGET (w), FALSE);
			w = gtk_object_get_data (GTK_OBJECT (e), "redo");
			gtk_widget_set_sensitive (GTK_WIDGET (w), FALSE);
		} else {
			e = gtk_object_get_data (GTK_OBJECT (toolbox), "edit");
			w = gtk_object_get_data (GTK_OBJECT (e), "undo");
			gtk_widget_set_sensitive (GTK_WIDGET (w), TRUE);
			w = gtk_object_get_data (GTK_OBJECT (e), "redo");
			gtk_widget_set_sensitive (GTK_WIDGET (w), TRUE);
		}
	}
}


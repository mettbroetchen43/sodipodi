#define __SP_TOOLBOX_C__

/*
 * Main toolbox
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>
#include <gnome.h>
#include <libgnomeui/gnome-pixmap.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnomeui/gnome-window-icon.h>
#include <glade/glade.h>
#include "widgets/sp-toolbox.h"
#include "widgets/sp-menu-button.h"
#include "file.h"
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
#include "interface.h"

GtkWidget * sp_toolbox_create (GladeXML *xml, const gchar *widgetname, const gchar *name, const gchar *internalname, const gchar *pxname);
static GtkWidget *sp_toolbox_draw_create (void);

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

/* Drag and Drop */
typedef enum {
	URI_LIST
} toolbox_drop_target_info;

static GtkTargetEntry toolbox_drop_target_entries [] = {
	{"text/uri-list", 0, URI_LIST},
};

#define ENTRIES_SIZE(n) sizeof(n)/sizeof(n[0]) 
static guint ntoolbox_drop_target_entries = ENTRIES_SIZE(toolbox_drop_target_entries);

static void sp_maintoolbox_open_files(gchar * buffer);
static void sp_maintoolbox_open_one_file_with_check(gpointer filename, gpointer unused);

static gint
sp_maintoolbox_delete_event (GtkWidget *widget, GdkEventAny *event, gpointer data)
{
	sodipodi_unref ();

	gtk_widget_hide (GTK_WIDGET (toolbox));

	return TRUE;
}

static gint
sp_maintoolbox_menu_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkWidget *m;

	m = sp_ui_main_menu ();
	gtk_widget_show (m);

	gtk_menu_popup (GTK_MENU (m), NULL, NULL, NULL, NULL, event->button, event->time);

	return TRUE;
}

void
sp_maintoolbox_create (void)
{
	if (toolbox == NULL) {
		GtkWidget *vbox, *mbar, *mi, *t, *w;
		GladeXML *xml;

		/* Crete main toolbox Glade XML */
		toolbox_xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "maintoolbox");
		g_return_if_fail (toolbox_xml != NULL);

		/* Create window */
		toolbox = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (toolbox), _("Sodipodi"));
		gtk_window_set_policy (GTK_WINDOW (toolbox), TRUE, TRUE, TRUE);
		gtk_signal_connect (GTK_OBJECT (toolbox), "delete_event", GTK_SIGNAL_FUNC (sp_maintoolbox_delete_event), NULL);

		vbox = gtk_vbox_new (FALSE, 0);
		gtk_widget_show (vbox);
		gtk_container_add (GTK_CONTAINER (toolbox), vbox);
		mbar = gtk_menu_bar_new ();
		gtk_widget_show (mbar);
		gtk_box_pack_start (GTK_BOX (vbox), mbar, FALSE, FALSE, 0);

		mi = gtk_menu_item_new_with_label (_("Sodipodi"));
		gtk_widget_show (mi);
		gtk_menu_bar_append (GTK_MENU_BAR (mbar), mi);
		gtk_signal_connect (GTK_OBJECT (mi), "button_press_event", GTK_SIGNAL_FUNC (sp_maintoolbox_menu_button_press), NULL);

#if 0
		glade_xml_signal_autoconnect (toolbox_xml);
		toolbox = glade_xml_get_widget (toolbox_xml, "maintoolbox");
		g_return_if_fail (toolbox != NULL);
		vbox = glade_xml_get_widget (toolbox_xml, "main_vbox");
#endif

		/* File */
		xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "file_table");
		t = sp_toolbox_create (xml, "file_table", _("File"), "file", "toolbox_file.xpm");
		gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
		/* Edit */
		xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "edit_table");
		t = sp_toolbox_create (xml, "edit_table", _("Edit"), "edit", "toolbox_edit.xpm");
		gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
		/* fixme: Freehand does not need this anymore, remove if node editing is fixed (Lauris) */
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
		t = sp_toolbox_draw_create ();
		gtk_widget_show (t);
		gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);

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

	gnome_window_icon_set_from_default (GTK_WINDOW (toolbox));
	gtk_drag_dest_set(toolbox, 
			  GTK_DEST_DEFAULT_ALL,
			  toolbox_drop_target_entries,
			  ntoolbox_drop_target_entries,
			  GDK_ACTION_COPY);

	if (!GTK_WIDGET_VISIBLE (toolbox)) {
		/* Reference our sodipodi engine */
		sodipodi_ref ();
		gtk_widget_show (toolbox);
	}
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

enum {
	SP_TOOLBOX_DRAW_RECT,
	SP_TOOLBOX_DRAW_ARC,
	SP_TOOLBOX_DRAW_STAR,
	SP_TOOLBOX_DRAW_SPIRAL,
	SP_TOOLBOX_DRAW_FREEHAND,
	SP_TOOLBOX_DRAW_PEN,
	SP_TOOLBOX_DRAW_DYNAHAND
};

static void
sp_toolbox_draw_set_object (GtkButton *button, gpointer itemdata, gpointer data)
{
	guint mode;

	mode = GPOINTER_TO_UINT (itemdata);

	g_print ("Draw toolbox set object: %d\n", mode);

	switch (mode) {
	case SP_TOOLBOX_DRAW_RECT:
		sp_event_context_set_rect (NULL);
		break;
	case SP_TOOLBOX_DRAW_ARC:
		sp_event_context_set_arc (NULL);
		break;
	case SP_TOOLBOX_DRAW_STAR:
		sp_event_context_set_star (NULL);
		break;
	case SP_TOOLBOX_DRAW_SPIRAL:
		sp_event_context_set_spiral (NULL);
		break;
	case SP_TOOLBOX_DRAW_FREEHAND:
		sp_event_context_set_freehand (NULL);
		break;
	case SP_TOOLBOX_DRAW_PEN:
		sp_event_context_set_pen (NULL);
		break;
	case SP_TOOLBOX_DRAW_DYNAHAND:
		sp_event_context_set_dynahand (NULL);
		break;
	default:
		g_warning ("Illegal draw code %d", mode);
		break;
	}
}

#if 0
static void
select_clicked (GtkButton *button, gpointer data)
{
	g_print ("Select clicked\n");

	sp_event_context_set_select (button);
}
#endif

static GtkWidget *
sp_toolbox_draw_create (void)
{
	GtkWidget *tb, *t, *pm, *b;
	SPRepr *repr;

	t = gtk_table_new (2, 4, TRUE);
	gtk_widget_show (t);

	tb = sp_toolbox_new (t, _("Draw"), "draw", SODIPODI_PIXMAPDIR "/toolbox_draw.xpm");

	/* Select */
	pm = gnome_pixmap_new_from_file (SODIPODI_PIXMAPDIR "/draw_select.xpm");
	gtk_widget_show (pm);
	b = gtk_toggle_button_new ();
	gtk_widget_show (b);
	gtk_container_add (GTK_CONTAINER (b), pm);
	gtk_signal_connect (GTK_OBJECT (b), "released", GTK_SIGNAL_FUNC (sp_event_context_set_select), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_object_set_data (GTK_OBJECT (tb), "SPSelectContext", b);

	/* Node */
	pm = gnome_pixmap_new_from_file (SODIPODI_PIXMAPDIR "/draw_node.xpm");
	gtk_widget_show (pm);
	b = gtk_toggle_button_new ();
	gtk_widget_show (b);
	gtk_container_add (GTK_CONTAINER (b), pm);
	gtk_signal_connect (GTK_OBJECT (b), "released", GTK_SIGNAL_FUNC (sp_event_context_set_node_edit), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_object_set_data (GTK_OBJECT (tb), "SPNodeContext", b);

	/* Object */
	b = sp_menu_button_new ();
	gtk_widget_show (b);
	/* START COMPONENTS */
	/* Rect */
	pm = gnome_pixmap_new_from_file (SODIPODI_PIXMAPDIR "/draw_rect.xpm");
	gtk_widget_show (pm);
	sp_menu_button_append_child (SP_MENU_BUTTON (b), pm, GUINT_TO_POINTER (SP_TOOLBOX_DRAW_RECT));
	/* Arc */
	pm = gnome_pixmap_new_from_file (SODIPODI_PIXMAPDIR "/draw_arc.xpm");
	gtk_widget_show (pm);
	sp_menu_button_append_child (SP_MENU_BUTTON (b), pm, GUINT_TO_POINTER (SP_TOOLBOX_DRAW_ARC));
	/* Star */
	pm = gnome_pixmap_new_from_file (SODIPODI_PIXMAPDIR "/draw_star.xpm");
	gtk_widget_show (pm);
	sp_menu_button_append_child (SP_MENU_BUTTON (b), pm, GUINT_TO_POINTER (SP_TOOLBOX_DRAW_STAR));
	/* Spiral */
	pm = gnome_pixmap_new_from_file (SODIPODI_PIXMAPDIR "/draw_spiral.xpm");
	gtk_widget_show (pm);
	sp_menu_button_append_child (SP_MENU_BUTTON (b), pm, GUINT_TO_POINTER (SP_TOOLBOX_DRAW_SPIRAL));
	/* END COMPONENTS */
	gtk_signal_connect (GTK_OBJECT (b), "activate", GTK_SIGNAL_FUNC (sp_toolbox_draw_set_object), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 2, 3, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	/* fixme: */
	gtk_object_set_data (GTK_OBJECT (tb), "SPRectContext", b);
	gtk_object_set_data (GTK_OBJECT (tb), "SPArcContext", b);
	gtk_object_set_data (GTK_OBJECT (tb), "SPStarContext", b);
	gtk_object_set_data (GTK_OBJECT (tb), "SPSpiralContext", b);

	/* Freehand */
	b = sp_menu_button_new ();
	gtk_widget_show (b);
	/* START COMPONENTS */
	/* Freehand */
	pm = gnome_pixmap_new_from_file (SODIPODI_PIXMAPDIR "/draw_freehand.xpm");
	gtk_widget_show (pm);
	sp_menu_button_append_child (SP_MENU_BUTTON (b), pm, GUINT_TO_POINTER (SP_TOOLBOX_DRAW_FREEHAND));
	/* Pen */
	pm = gnome_pixmap_new_from_file (SODIPODI_PIXMAPDIR "/draw_pen.xpm");
	gtk_widget_show (pm);
	sp_menu_button_append_child (SP_MENU_BUTTON (b), pm, GUINT_TO_POINTER (SP_TOOLBOX_DRAW_PEN));
	/* Dynahand */
	pm = gnome_pixmap_new_from_file (SODIPODI_PIXMAPDIR "/draw_dynahand.xpm");
	gtk_widget_show (pm);
	sp_menu_button_append_child (SP_MENU_BUTTON (b), pm, GUINT_TO_POINTER (SP_TOOLBOX_DRAW_DYNAHAND));
	/* END COMPONENTS */
	gtk_signal_connect (GTK_OBJECT (b), "activate", GTK_SIGNAL_FUNC (sp_toolbox_draw_set_object), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 3, 4, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	/* fixme: */
	gtk_object_set_data (GTK_OBJECT (tb), "SPPencilContext", b);
	gtk_object_set_data (GTK_OBJECT (tb), "SPPenContext", b);
	gtk_object_set_data (GTK_OBJECT (tb), "SPDynaDrawContext", b);

	/* Text */
	pm = gnome_pixmap_new_from_file (SODIPODI_PIXMAPDIR "/draw_text.xpm");
	gtk_widget_show (pm);
	b = gtk_toggle_button_new ();
	gtk_widget_show (b);
	gtk_container_add (GTK_CONTAINER (b), pm);
	gtk_signal_connect (GTK_OBJECT (b), "released", GTK_SIGNAL_FUNC (sp_event_context_set_text), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_object_set_data (GTK_OBJECT (tb), "SPtextContext", b);

	/* Zoom */
	pm = gnome_pixmap_new_from_file (SODIPODI_PIXMAPDIR "/draw_zoom.xpm");
	gtk_widget_show (pm);
	b = gtk_toggle_button_new ();
	gtk_widget_show (b);
	gtk_container_add (GTK_CONTAINER (b), pm);
	gtk_signal_connect (GTK_OBJECT (b), "released", GTK_SIGNAL_FUNC (sp_event_context_set_zoom), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_object_set_data (GTK_OBJECT (tb), "SPZoomContext", b);

	repr = sodipodi_get_repr (SODIPODI, "toolboxes.draw");
	if (repr) {
		gint state;
		state = sp_repr_get_int_attribute (repr, "state", 0);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);
	}

	gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

	return tb;
}

static gint
sp_toolbox_set_state_handler (SPToolBox * t, guint state, gpointer data)
{
	SPRepr * repr;

	repr = (SPRepr *) data;

	sp_repr_set_int_attribute (repr, "state", state);

	return FALSE;
}

void 
sp_maintoolbox_drag_data_received (GtkWidget * widget,
		       GdkDragContext * drag_context,
		       gint x, gint y,
		       GtkSelectionData * data,
		       guint info,
		       guint event_time,
		       gpointer user_data)
{
	gchar * uri;
	
	switch(info) {
	case URI_LIST:
		uri = (gchar *)data->data;
		sp_maintoolbox_open_files(uri);
		break;
	}
}

static void
sp_maintoolbox_open_files(gchar * buffer)
{
  	GList * list = gnome_uri_list_extract_filenames(buffer);
	if (!list)
		return;
	g_list_foreach (list, sp_maintoolbox_open_one_file_with_check, NULL);

}

static void
sp_maintoolbox_open_one_file_with_check(gpointer filename, gpointer unused)
{
	const gchar * svg_suffix   = ".svg";
	gint  svg_suffix_len = strlen(svg_suffix);
	gint  filename_len;
	if (filename) {
		filename_len = strlen(filename);
		if (filename_len > svg_suffix_len
		    && !strcmp((char *)filename + filename_len - svg_suffix_len, svg_suffix))
			sp_file_open (filename);
	}
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
		if (active && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (active))) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (active), FALSE);
		}
		if (new && !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (new))) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (new), TRUE);
		}
		gtk_object_set_data (GTK_OBJECT (data), "active", new);
		if ((tname) && ((!strcmp (tname, "SPNodeContext")) || (!strcmp (tname, "SPDynaDrawContext")))) {
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

	if (tname) {
		if (!strcmp (tname, "SPRectContext")) {
			sp_menu_button_set_active (SP_MENU_BUTTON (new), GUINT_TO_POINTER (SP_TOOLBOX_DRAW_RECT));
		} else if (!strcmp (tname, "SPArcContext")) {
			sp_menu_button_set_active (SP_MENU_BUTTON (new), GUINT_TO_POINTER (SP_TOOLBOX_DRAW_ARC));
		} else if (!strcmp (tname, "SPStarContext")) {
			sp_menu_button_set_active (SP_MENU_BUTTON (new), GUINT_TO_POINTER (SP_TOOLBOX_DRAW_STAR));
		} else if (!strcmp (tname, "SPSpiralContext")) {
			sp_menu_button_set_active (SP_MENU_BUTTON (new), GUINT_TO_POINTER (SP_TOOLBOX_DRAW_SPIRAL));
		} else if (!strcmp (tname, "SPPencilContext")) {
			sp_menu_button_set_active (SP_MENU_BUTTON (new), GUINT_TO_POINTER (SP_TOOLBOX_DRAW_FREEHAND));
		} else if (!strcmp (tname, "SPPenContext")) {
			sp_menu_button_set_active (SP_MENU_BUTTON (new), GUINT_TO_POINTER (SP_TOOLBOX_DRAW_PEN));
		} else if (!strcmp (tname, "SPDynaDrawContext")) {
			sp_menu_button_set_active (SP_MENU_BUTTON (new), GUINT_TO_POINTER (SP_TOOLBOX_DRAW_DYNAHAND));
		} else if (!strcmp (tname, "SPPenContext")) {
			sp_menu_button_set_active (SP_MENU_BUTTON (new), GUINT_TO_POINTER (SP_TOOLBOX_DRAW_PEN));
		}
	}
}


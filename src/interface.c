#define __SP_INTERFACE_C__

/*
 * Main UI stuff
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

#include <string.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-mime.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnomeui/gnome-messagebox.h>
#include <libgnomeui/gnome-window-icon.h>
#include "sodipodi.h"
#include "document.h"
#include "desktop-handles.h"
#include "file.h"
#include "help.h"
#include "interface.h"
#include "desktop.h"
#include "selection-chemistry.h"
#include "path-chemistry.h"
#include "zoom-context.h"
#include "event-broker.h"
#include "svg-view.h"

#include "dir-util.h"
#include "xml/repr-private.h"

#include "dialogs/text-edit.h"
#include "dialogs/export.h"
#include "dialogs/xml-tree.h"
#include "dialogs/align.h"
#include "dialogs/transformation.h"
#include "dialogs/object-properties.h"
#include "dialogs/desktop-properties.h"
#include "dialogs/document-properties.h"
#include "dialogs/display-settings.h"
#include "dialogs/tool-options.h"
#include "dialogs/tool-attributes.h"

void fake_dialogs (void);

static gint sp_ui_delete (GtkWidget *widget, GdkEvent *event, SPView *view);

/* Drag and Drop */
typedef enum {
  URI_LIST
} ui_drop_target_info;
static GtkTargetEntry ui_drop_target_entries [] = {
  {"text/uri-list", 0, URI_LIST},
};
#define ENTRIES_SIZE(n) sizeof(n)/sizeof(n[0]) 
static guint nui_drop_target_entries = ENTRIES_SIZE(ui_drop_target_entries);
static void sp_ui_import_files(gchar * buffer);
static void sp_ui_import_one_file(gchar * filename);
static void sp_ui_import_one_file_with_check(gpointer filename, gpointer unused);
static void sp_ui_drag_data_received (GtkWidget * widget,
				      GdkDragContext * drag_context,
				      gint x, gint y,
				      GtkSelectionData * data,
				      guint info,
				      guint event_time,
				      gpointer user_data);
void
sp_create_window (SPViewWidget *vw, gboolean editable)
{
	GtkWidget *w;

	g_return_if_fail (vw != NULL);
	g_return_if_fail (SP_IS_VIEW_WIDGET (vw));

	w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_object_set_data (GTK_OBJECT (vw), "window", w);

	/* fixme: */
	if (editable) {
		gtk_window_set_default_size ((GtkWindow *) w, 400, 400);
		gtk_object_set_data (GTK_OBJECT (w), "desktop", SP_DESKTOP_WIDGET (vw)->desktop);
		gtk_object_set_data (GTK_OBJECT (w), "desktopwidget", vw);
		gtk_signal_connect (GTK_OBJECT (w), "delete_event", GTK_SIGNAL_FUNC (sp_ui_delete), vw->view);
		gtk_signal_connect (GTK_OBJECT (w), "focus_in_event", GTK_SIGNAL_FUNC (sp_desktop_widget_set_focus), vw);
#if 0
		sp_desktop_set_title (desktop);
#endif
	} else {
		gtk_window_set_policy (GTK_WINDOW (w), TRUE, TRUE, TRUE);
	}

	gtk_container_add (GTK_CONTAINER (w), GTK_WIDGET (vw));
	gtk_widget_show (GTK_WIDGET (vw));

	gnome_window_icon_set_from_default (GTK_WINDOW (w));
	gtk_drag_dest_set(w, 
			  GTK_DEST_DEFAULT_ALL,
			  ui_drop_target_entries,
			  nui_drop_target_entries,
			  GDK_ACTION_COPY);
	gtk_signal_connect(GTK_OBJECT(w),
			   "drag_data_received",
			   GTK_SIGNAL_FUNC(sp_ui_drag_data_received),
			   NULL);
	gtk_widget_show (w);
}

void
sp_ui_new_view (GtkWidget * widget)
{
	SPDocument * document;
	SPViewWidget *dtw;

	document = SP_ACTIVE_DOCUMENT;
	if (!document) return;

	dtw = sp_desktop_widget_new (sp_document_namedview (document, NULL));
	g_return_if_fail (dtw != NULL);

	sp_create_window (dtw, TRUE);
}

/* fixme: */
void
sp_ui_new_view_preview (GtkWidget *widget)
{
	SPDocument *document;
	SPViewWidget *dtw;

	document = SP_ACTIVE_DOCUMENT;
	if (!document) return;

	dtw = (SPViewWidget *) sp_svg_view_widget_new (document);
	g_return_if_fail (dtw != NULL);
#if 1
	sp_svg_view_widget_set_resize (SP_SVG_VIEW_WIDGET (dtw), TRUE, 400.0, 400.0);
#endif

	sp_create_window (dtw, FALSE);
}

void
sp_ui_close_view (GtkWidget * widget)
{
	gpointer w;

	if (SP_ACTIVE_DESKTOP == NULL) return;

	if (sp_ui_delete (NULL, NULL, SP_VIEW (SP_ACTIVE_DESKTOP))) return;

	w = gtk_object_get_data (GTK_OBJECT (SP_ACTIVE_DESKTOP), "window");

	if (w) gtk_object_destroy (GTK_OBJECT (w));

	/*
	 * We have to fake dialog initialization, or corresponding code is
	 * not compiled into application.
	 */

	if (GTK_IS_WIDGET (SODIPODI)) fake_dialogs ();
}

static gint
sp_ui_delete (GtkWidget *widget, GdkEvent *event, SPView *view)
{
	return sp_view_shutdown (view);
}

static GtkWidget *
sp_ui_menu_append_item (GtkMenu *menu, const guchar *stock, const guchar *label, GtkSignalFunc callback, gpointer data)
{
	GtkWidget *item;

	if (stock) {
		item = gnome_stock_menu_item (stock, label);
	} else {
		item = gtk_menu_item_new ();
	}
	gtk_widget_show (item);
	if (callback) {
		gtk_signal_connect (GTK_OBJECT (item), "activate", callback, data);
	}
	gtk_menu_append (GTK_MENU (menu), item);

	return item;
}

static void
sp_ui_file_menu (GtkMenu *fm, SPDocument *doc)
{
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_NEW, _("New"), sp_file_new, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_OPEN, _("Open"), sp_file_open, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_SAVE, _("Save"), sp_file_save, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_SAVE_AS, _("Save as"), sp_file_save_as, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), NULL, NULL, NULL, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_BLANK, _("Import"), sp_file_import, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_BLANK, _("Export"), sp_export_dialog, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), NULL, NULL, NULL, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_PRINT, _("Print"), sp_file_print, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_PRINT, _("Print Preview"), sp_file_print_preview, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), NULL, NULL, NULL, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_ABOUT, _("About Sodipodi"), sp_help_about, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), NULL, NULL, NULL, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_CLOSE, _("Close View"), sp_ui_close_view, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_EXIT, _("Exit Program"), sp_file_exit, NULL);
}

static void
sp_ui_edit_menu (GtkMenu *fm, SPDocument *doc)
{
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_CUT, _("Cut"), sp_selection_cut, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_COPY, _("Copy"), sp_selection_copy, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_PASTE, _("Paste"), sp_selection_paste, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), NULL, NULL, NULL, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_BLANK, _("Duplicate"), sp_selection_duplicate, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_BLANK, _("Delete"), sp_selection_delete, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), NULL, NULL, NULL, NULL);
	sp_ui_menu_append_item (GTK_MENU (fm), GNOME_STOCK_MENU_BLANK, _("Clear all"), sp_edit_clear_all, NULL);
}

static void
sp_ui_selection_menu (GtkMenu *menu, SPDocument *doc)
{
	GtkWidget *i, *sm, *si;

	/* Selection:Group */
	i = gnome_stock_menu_item (GNOME_STOCK_MENU_BLANK, _("Group"));
	gtk_widget_show (i);
	gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (sp_selection_group), NULL);
	gtk_menu_append (GTK_MENU (menu), i);
	/* Selection:Ungroup */
	i = gnome_stock_menu_item (GNOME_STOCK_MENU_BLANK, _("Ungroup"));
	gtk_widget_show (i);
	gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (sp_selection_ungroup), NULL);
	gtk_menu_append (GTK_MENU (menu), i);
	/* Separator */
	i = gtk_menu_item_new ();
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (menu), i);
	/* Selection:Combine */
	i = gnome_stock_menu_item (GNOME_STOCK_MENU_BLANK, _("Combine"));
	gtk_widget_show (i);
	gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (sp_selected_path_combine), NULL);
	gtk_menu_append (GTK_MENU (menu), i);
	/* Selection:Break apart */
	i = gnome_stock_menu_item (GNOME_STOCK_MENU_BLANK, _("Break apart"));
	gtk_widget_show (i);
	gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (sp_selected_path_break_apart), NULL);
	gtk_menu_append (GTK_MENU (menu), i);
	/* Separator */
	i = gtk_menu_item_new ();
	gtk_widget_show (i);
	gtk_menu_append (GTK_MENU (menu), i);
	/* Selection:Order */
	i = gnome_stock_menu_item (GNOME_STOCK_MENU_BLANK, _("Order"));
	gtk_widget_show (i);
	sm = gtk_menu_new ();
	gtk_widget_show (sm);
	si = gnome_stock_menu_item (GNOME_STOCK_MENU_BLANK, _("Bring to Front"));
	gtk_widget_show (si);
	gtk_signal_connect (GTK_OBJECT (si), "activate", GTK_SIGNAL_FUNC (sp_selection_raise_to_top), NULL);
	gtk_menu_append (GTK_MENU (sm), si);
	si = gnome_stock_menu_item (GNOME_STOCK_MENU_BLANK, _("Send to Back"));
	gtk_widget_show (si);
	gtk_signal_connect (GTK_OBJECT (si), "activate", GTK_SIGNAL_FUNC (sp_selection_lower_to_bottom), NULL);
	gtk_menu_append (GTK_MENU (sm), si);
	si = gnome_stock_menu_item (GNOME_STOCK_MENU_BLANK, _("Raise"));
	gtk_widget_show (si);
	gtk_signal_connect (GTK_OBJECT (si), "activate", GTK_SIGNAL_FUNC (sp_selection_raise), NULL);
	gtk_menu_append (GTK_MENU (sm), si);
	si = gnome_stock_menu_item (GNOME_STOCK_MENU_BLANK, _("Lower"));
	gtk_widget_show (si);
	gtk_signal_connect (GTK_OBJECT (si), "activate", GTK_SIGNAL_FUNC (sp_selection_lower), NULL);
	gtk_menu_append (GTK_MENU (sm), si);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), sm);
	gtk_menu_append (GTK_MENU (menu), i);
}

static void
sp_ui_view_menu (GtkMenu *menu, SPDocument *doc)
{
	GtkWidget *zm, *zi, *sm, *si;

	/* View:Zoom */
	zi = sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Zoom"), NULL, NULL);
	zm = gtk_menu_new ();
	gtk_widget_show (zm);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (zi), zm);
	sp_ui_menu_append_item (GTK_MENU (zm), GNOME_STOCK_MENU_BLANK, _("Selection"), sp_zoom_selection, NULL);
	sp_ui_menu_append_item (GTK_MENU (zm), GNOME_STOCK_MENU_BLANK, _("Drawing"), sp_zoom_drawing, NULL);
	sp_ui_menu_append_item (GTK_MENU (zm), GNOME_STOCK_MENU_BLANK, _("Page"), sp_zoom_page, NULL);
	sp_ui_menu_append_item (GTK_MENU (zm), NULL, NULL, NULL, NULL);
	si = sp_ui_menu_append_item (GTK_MENU (zm), GNOME_STOCK_MENU_BLANK, _("Scale"), NULL, NULL);
	sm = gtk_menu_new ();
	gtk_widget_show (sm);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (si), sm);
	sp_ui_menu_append_item (GTK_MENU (sm), GNOME_STOCK_MENU_BLANK, _("1:2"), sp_zoom_1_to_2, NULL);
	sp_ui_menu_append_item (GTK_MENU (sm), GNOME_STOCK_MENU_BLANK, _("1:1"), sp_zoom_1_to_1, NULL);
	sp_ui_menu_append_item (GTK_MENU (sm), GNOME_STOCK_MENU_BLANK, _("2:1"), sp_zoom_2_to_1, NULL);
	/* View:New View*/
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("New View"), sp_ui_new_view, NULL);
	/* View:New Preview*/
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("New Preview"), sp_ui_new_view_preview, NULL);
}

static void
sp_ui_event_context_menu (GtkMenu *menu, SPDocument *doc)
{
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Select"), sp_event_context_set_select, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Node"), sp_event_context_set_node_edit, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Rectangle"), sp_event_context_set_rect, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Arc"), sp_event_context_set_arc, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Star"), sp_event_context_set_star, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Spiral"), sp_event_context_set_spiral, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Freehand"), sp_event_context_set_freehand, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Dynahand"), sp_event_context_set_dynahand, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Text"), sp_event_context_set_text, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Zoom"), sp_event_context_set_zoom, NULL);
}

static void
sp_ui_dialog_menu (GtkMenu *menu, SPDocument *doc)
{
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Fill and Stroke"), sp_object_properties_dialog, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Size and Position"), sp_object_properties_layout, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Align Objects"), sp_quick_align_dialog, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Text Editing"), sp_text_edit_dialog, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Transformations"), sp_transformation_dialog, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Tool Options"), sp_tool_options_dialog, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Tool Attributes"), sp_tool_attributes_dialog, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Document"), sp_document_dialog, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Editing Window"), sp_desktop_dialog, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("XML Editor"), sp_xml_tree_dialog, NULL);
	sp_ui_menu_append_item (menu, GNOME_STOCK_MENU_BLANK, _("Display Properties"), sp_display_dialog, NULL);
}

/* Menus */

GtkWidget *
sp_ui_main_menu (void)
{
	GtkWidget *m;

	m = gtk_menu_new ();

	sp_ui_menu_append_item (GTK_MENU (m), GNOME_STOCK_MENU_NEW, _("New"), sp_file_new, NULL);
	sp_ui_menu_append_item (GTK_MENU (m), GNOME_STOCK_MENU_OPEN, _("Open"), sp_file_open, NULL);
	sp_ui_menu_append_item (GTK_MENU (m), NULL, NULL, NULL, NULL);
	sp_ui_menu_append_item (GTK_MENU (m), GNOME_STOCK_MENU_ABOUT, _("About Sodipodi"), sp_help_about, NULL);
	sp_ui_menu_append_item (GTK_MENU (m), NULL, NULL, NULL, NULL);
	sp_ui_menu_append_item (GTK_MENU (m), GNOME_STOCK_MENU_EXIT, _("Exit Program"), sp_file_exit, NULL);

	return m;
}

GtkWidget *
sp_ui_generic_menu (SPView *v, SPItem *item)
{
	GtkWidget *m, *i, *sm;
	SPDesktop *dt;

	dt = (SP_IS_DESKTOP (v)) ? SP_DESKTOP (v) : NULL;

	m = gtk_menu_new ();

	/* Undo */
	sp_ui_menu_append_item (GTK_MENU (m), GNOME_STOCK_MENU_UNDO, _("Undo"), GTK_SIGNAL_FUNC (sp_undo), NULL);
	/* File:Print Preview */
	sp_ui_menu_append_item (GTK_MENU (m), GNOME_STOCK_MENU_REDO, _("Redo"), GTK_SIGNAL_FUNC (sp_redo), NULL);
	/* Separator */
	sp_ui_menu_append_item (GTK_MENU (m), NULL, NULL, NULL, NULL);

	/* Item menu */
	if (item) {
		sp_item_menu (item, dt, GTK_MENU (m));
		/* Separator */
		sp_ui_menu_append_item (GTK_MENU (m), NULL, NULL, NULL, NULL);
	}

#if 0
	/* Desktop menu */
	if (vw) {
		sp_view_widget_menu (vw, m);
		i = gtk_menu_item_new ();
		gtk_widget_show (i);
		gtk_menu_append (GTK_MENU (m), i);
	}
#endif

	/* Generic menu */
	/* File submenu */
	i = sp_ui_menu_append_item (GTK_MENU (m), GNOME_STOCK_MENU_BLANK, _("File"), NULL, NULL);
	sm = gtk_menu_new ();
	sp_ui_file_menu (GTK_MENU (sm), NULL);
	gtk_widget_show (sm);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), sm);
	/* Edit submenu */
	i = sp_ui_menu_append_item (GTK_MENU (m), GNOME_STOCK_MENU_BLANK, _("Edit"), NULL, NULL);
	sm = gtk_menu_new ();
	sp_ui_edit_menu (GTK_MENU (sm), NULL);
	gtk_widget_show (sm);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), sm);
	/* Selection submenu */
	i = sp_ui_menu_append_item (GTK_MENU (m), GNOME_STOCK_MENU_BLANK, _("Selection"), NULL, NULL);
	sm = gtk_menu_new ();
	sp_ui_selection_menu (GTK_MENU (sm), NULL);
	gtk_widget_show (sm);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), sm);
	/* View submenu */
	i = sp_ui_menu_append_item (GTK_MENU (m), GNOME_STOCK_MENU_BLANK, _("View"), NULL, NULL);
	sm = gtk_menu_new ();
	sp_ui_view_menu (GTK_MENU (sm), NULL);
	gtk_widget_show (sm);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), sm);
	/* Drawing mode submenu */
	i = sp_ui_menu_append_item (GTK_MENU (m), GNOME_STOCK_MENU_BLANK, _("Drawing Mode"), NULL, NULL);
	sm = gtk_menu_new ();
	sp_ui_event_context_menu (GTK_MENU (sm), NULL);
	gtk_widget_show (sm);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), sm);
	/* Dialog submenu */
	i = sp_ui_menu_append_item (GTK_MENU (m), GNOME_STOCK_MENU_BLANK, _("Dialogs"), NULL, NULL);
	sm = gtk_menu_new ();
	sp_ui_dialog_menu (GTK_MENU (sm), NULL);
	gtk_widget_show (sm);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), sm);

	return m;
}

void
fake_dialogs (void)
{
	sp_object_properties_dialog ();
    	sp_object_properties_fill ();
    	sp_object_properties_stroke ();
    	sp_object_properties_layout ();
	sp_text_edit_dialog ();
	sp_export_dialog ();
	sp_xml_tree_dialog ();
	sp_transformation_dialog ();
	sp_desktop_dialog ();
	sp_document_dialog ();
	sp_display_dialog ();
}

/* Drag and Drop */
void 
sp_ui_drag_data_received (GtkWidget * widget,
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
		sp_ui_import_files(uri);
		break;
	}
}

static void
sp_ui_import_files(gchar * buffer)
{
	GList * list = gnome_uri_list_extract_filenames(buffer);
	if (!list)
		return;
	g_list_foreach (list, sp_ui_import_one_file_with_check, NULL);
}

static void
sp_ui_import_one_file_with_check(gpointer filename, gpointer unused)
{
	if (filename) {
		if (strlen(filename) > 2)
			sp_ui_import_one_file(filename);
	}
}

/* Cut&Paste'ed from file.c:file_import_ok */
static void
sp_ui_import_one_file(gchar * filename)
{
	SPDocument * doc;
	SPRepr * rdoc;
	const gchar * e, * docbase, * relname;
	SPRepr * repr;
	SPReprDoc * rnewdoc;

	doc = SP_ACTIVE_DOCUMENT;
	if (!SP_IS_DOCUMENT(doc)) return;
	
	if (filename == NULL) return;  

	rdoc = sp_document_repr_root (doc);

	docbase = sp_repr_attr (rdoc, "sodipodi:docbase");
	relname = sp_relative_path_from_path (filename, docbase);
	/* fixme: this should be implemented with mime types */
	e = sp_extension_from_path (filename);

	if ((e == NULL) || (strcmp (e, "svg") == 0) || (strcmp (e, "xml") == 0)) {
		SPRepr * newgroup;
		const gchar * style;
		SPRepr * child;

		rnewdoc = sp_repr_read_file (filename);
		if (rnewdoc == NULL) return;
		repr = sp_repr_document_root (rnewdoc);
		style = sp_repr_attr (repr, "style");

		newgroup = sp_repr_new ("g");
		sp_repr_set_attr (newgroup, "style", style);

		for (child = repr->children; child != NULL; child = child->next) {
			SPRepr * newchild;
			newchild = sp_repr_duplicate (child);
			sp_repr_append_child (newgroup, newchild);
		}

		sp_repr_document_unref (rnewdoc);

		sp_document_add_repr (doc, newgroup);
		sp_repr_unref (newgroup);
		sp_document_done (doc);
		return;
	}

	if ((strcmp (e, "png") == 0) ||
	    (strcmp (e, "jpg") == 0) ||
	    (strcmp (e, "jpeg") == 0) ||
	    (strcmp (e, "bmp") == 0) ||
	    (strcmp (e, "gif") == 0) ||
	    (strcmp (e, "tiff") == 0) ||
	    (strcmp (e, "xpm") == 0)) {
		/* Try pixbuf */
		GdkPixbuf *pb;
		pb = gdk_pixbuf_new_from_file (filename);
		if (pb) {
			/* We are readable */
			repr = sp_repr_new ("image");
			sp_repr_set_attr (repr, "xlink:href", relname);
			sp_repr_set_attr (repr, "sodipodi:absref", filename);
			sp_repr_set_double_attribute (repr, "width", gdk_pixbuf_get_width (pb));
			sp_repr_set_double_attribute (repr, "height", gdk_pixbuf_get_height (pb));
			sp_document_add_repr (doc, repr);
			sp_repr_unref (repr);
			sp_document_done (doc);
			gdk_pixbuf_unref (pb);
		}
	}
}

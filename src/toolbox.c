#define __SP_MAINTOOLBOX_C__

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkselection.h>
#include <gtk/gtktable.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtktooltips.h>
#include <gtk/gtkdnd.h>

#include "macros.h"
#include "helper/window.h"
#include "widgets/icon.h"
#include "widgets/button.h"
#include "widgets/sp-toolbox.h"

#include "dialogs/object-properties.h"
#include "dialogs/transformation.h"
#include "dialogs/text-edit.h"
#include "dialogs/align.h"
#include "dialogs/export.h"
#include "dialogs/node-edit.h"

#include "verbs.h"

#include "file.h"
#include "selection-chemistry.h"
#include "path-chemistry.h"
#include "sodipodi-private.h"
#include "document.h"
#include "sodipodi.h"
#include "sp-item-transform.h"
#include "desktop-handles.h"
#include "interface.h"
#include "toolbox.h"
#include "xml/repr-private.h"
#include "helper/gnome-utils.h"
#include "helper/sp-intl.h"

static GtkWidget *sp_toolbox_file_create (void);
static GtkWidget *sp_toolbox_edit_create (void);
static GtkWidget *sp_toolbox_draw_create (void);
static GtkWidget *sp_toolbox_object_create (void);
static GtkWidget *sp_toolbox_selection_create (void);
static GtkWidget *sp_toolbox_zoom_create (void);
static GtkWidget *sp_toolbox_node_create (void);

static gint sp_toolbox_set_state_handler (SPToolBox * t, guint state, gpointer data);
static void sp_maintoolbox_drag_data_received (GtkWidget * widget,
					       GdkDragContext * drag_context,
					       gint x, gint y,
					       GtkSelectionData * data,
					       guint info,
					       guint event_time,
					       gpointer user_data);

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

static GSList *toolboxes = NULL;

static void
sp_maintoolbox_window_destroy (void)
{
	toolboxes = g_slist_remove (toolboxes, toolboxes->data);
}

void
sp_maintoolbox_create_toplevel (void)
{
	GtkWidget *window, *toolbox;

	/* Create window */
	window = sp_window_new (_("Sodipodi"), FALSE, FALSE);
	g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (sp_maintoolbox_window_destroy), NULL);

	toolbox = sp_maintoolbox_new ();
	gtk_widget_show (toolbox);
	gtk_container_add (GTK_CONTAINER (window), toolbox);

	gtk_widget_show (window);

	/* Reference our sodipodi engine */
	toolboxes = g_slist_prepend (toolboxes, window);
};

static void
sp_maintoolbox_destroy (GtkObject *object, gpointer data)
{
	/* Disconnect tool tracking signals */
	sp_signal_disconnect_by_data (sodipodi, object);
	sodipodi_unref ();
}

#ifdef WITH_DOCK
#include <widgets/sp-dock.h>
#include <widgets/sp-dock-item.h>
#include <widgets/sp-dock-notebook.h>
#endif

GtkWidget *
sp_maintoolbox_new (void)
{
#ifdef WITH_DOCK
	GtkWidget *toolbox, *vbox, *mbar, *mi, *dock, *t, *item;

	vbox = gtk_vbox_new (FALSE, 0);
	toolbox = vbox;

	mbar = gtk_menu_bar_new ();
	gtk_box_pack_start (GTK_BOX (vbox), mbar, FALSE, FALSE, 0);

	mi = gtk_menu_item_new_with_label (_("Sodipodi"));
	gtk_menu_bar_append (GTK_MENU_BAR (mbar), mi);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM(mi), sp_ui_main_menu ());

	dock = sp_dock_new ();
	gtk_box_pack_start (GTK_BOX (vbox), dock, FALSE, FALSE, 0);

	t = sp_toolbox_file_create ();
	/* item = sp_dock_item_new ("file", "File Toolbox", SP_DOCK_ITEM_BEH_NORMAL); */
	item = sp_dock_notebook_new ();
	gtk_container_add (GTK_CONTAINER (item), t);
	sp_dock_add_item (SP_DOCK (dock), SP_DOCK_ITEM (item), SP_DOCK_TOP);

	t = sp_toolbox_edit_create ();
	item = sp_dock_item_new ("edit", "Edit Toolbox", SP_DOCK_ITEM_BEH_NORMAL);
	gtk_container_add (GTK_CONTAINER (item), t);
	sp_dock_add_item (SP_DOCK (dock), SP_DOCK_ITEM (item), SP_DOCK_TOP);

	t = sp_toolbox_object_create ();
	item = sp_dock_item_new ("object", "Object Toolbox", SP_DOCK_ITEM_BEH_NORMAL);
	gtk_container_add (GTK_CONTAINER (item), t);
	sp_dock_add_item (SP_DOCK (dock), SP_DOCK_ITEM (item), SP_DOCK_TOP);

	gtk_widget_show_all (vbox);
#else
	GtkWidget *toolbox, *vbox, *mbar, *mi, *t, *w;

	vbox = gtk_vbox_new (FALSE, 0);
	toolbox = vbox;

	mbar = gtk_menu_bar_new ();
	gtk_widget_show (mbar);
	gtk_box_pack_start (GTK_BOX (vbox), mbar, FALSE, FALSE, 0);

	mi = gtk_menu_item_new_with_label (_("Sodipodi"));
	gtk_widget_show (mi);
	gtk_menu_bar_append (GTK_MENU_BAR (mbar), mi);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM(mi), sp_ui_main_menu ());
	/* gtk_signal_connect (GTK_OBJECT (mi), "button_press_event", GTK_SIGNAL_FUNC (sp_maintoolbox_menu_button_press), NULL); */

	/* File */
	t = sp_toolbox_file_create ();
	gtk_widget_show (t);
	gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
	/* Edit */
	t = sp_toolbox_edit_create ();
	gtk_widget_show (t);
	gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
	/* fixme: Freehand does not need this anymore, remove if node editing is fixed (Lauris) */
	gtk_object_set_data (GTK_OBJECT (toolbox), "edit", t);
	/* Object */
	t = sp_toolbox_object_create ();
	gtk_widget_show (t);
	gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
	/* Select */
	t = sp_toolbox_selection_create ();
	gtk_widget_show (t);
	gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
	/* Draw */
	t = sp_toolbox_draw_create ();
	gtk_widget_show (t);
	gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
	g_object_set_data (G_OBJECT (toolbox), "draw", t);
	
	if (SP_ACTIVE_DESKTOP) {
		const gchar * tname;
		tname = gtk_type_name (GTK_OBJECT_TYPE (SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP)));
		w = g_object_get_data (G_OBJECT (t), tname);
		if (w != NULL) {
			sp_button_toggle_set_down (SP_BUTTON (w), TRUE, TRUE);
			g_object_set_data (G_OBJECT (t), "active", w);
		}
	}

	/* Zoom */
	t = sp_toolbox_zoom_create ();
	gtk_widget_show (t);
	gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
	/* Node */
	t = sp_toolbox_node_create ();
	gtk_widget_show (t);
	gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
#endif

	/* gnome_window_icon_set_from_default (GTK_WINDOW (toolbox)); */
	gtk_drag_dest_set (toolbox, 
			  GTK_DEST_DEFAULT_ALL,
			  toolbox_drop_target_entries,
			  ntoolbox_drop_target_entries,
			  GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (toolbox), "destroy", G_CALLBACK (sp_maintoolbox_destroy), NULL);
	/* g_signal_connect (G_OBJECT (toolbox), "delete_event", G_CALLBACK (sp_maintoolbox_delete_event), NULL); */
	g_signal_connect (G_OBJECT (toolbox), "drag_data_received", G_CALLBACK (sp_maintoolbox_drag_data_received), NULL);

	/* Keep sodipodi alive */
	sodipodi_ref ();

	return toolbox;
}

void
sp_maintoolbox_close_all (void)
{
	while (toolboxes) {
		gtk_widget_destroy ((GtkWidget *) toolboxes->data);
	}
}

static GtkWidget *
sp_toolbox_button_new (GtkWidget *t, int pos, const unsigned char *pxname, GtkSignalFunc handler, GtkTooltips *tt, unsigned char *tip)
{
	GtkWidget *b;
	int xpos, ypos;

	b = sp_button_new_from_data (24, SP_BUTTON_TYPE_NORMAL, pxname, tip, tt);
	gtk_widget_show (b);
	if (handler) gtk_signal_connect (GTK_OBJECT (b), "clicked", handler, NULL);
	xpos = pos % 4;
	ypos = pos / 4;
	gtk_table_attach (GTK_TABLE (t), b, xpos, xpos + 1, ypos, ypos + 1, 0, 0, 0, 0);

	return b;
}

static GtkWidget *
sp_toolbox_button_new_from_verb (GtkWidget *t, int pos, unsigned int type, unsigned int verb, GtkTooltips *tt)
{
	SPAction *action;
	GtkWidget *b;
	int xpos, ypos;

	action = sp_verb_get_action (verb);
	if (!action) return NULL;
	/* fixme: Handle sensitive/unsensitive */
	/* fixme: Implement sp_button_new_from_action */
	b = sp_button_new (24, type, action, tt);
	gtk_widget_show (b);
	xpos = pos % 4;
	ypos = pos / 4;
	gtk_table_attach (GTK_TABLE (t), b, xpos, xpos + 1, ypos, ypos + 1, 0, 0, 0, 0);
	return b;
}

static GtkWidget *
sp_toolbox_file_create (void)
{
	GtkWidget *t, *tb, *b;
	GtkTooltips *tt;
	SPRepr *repr;
#ifdef WIN32
#define PDIRECT
#endif
#ifdef WITH_KDE
#define PDIRECT
#endif
#ifdef WITH_GNOME_PRINT
#define PDIRECT
#endif
#ifdef PDIRECT
	SPAction *action;
#endif

	t = gtk_table_new (2, 4, TRUE);
	gtk_widget_show (t);

	tt = gtk_tooltips_new ();
	tb = sp_toolbox_new (t, _("File"), "file", "toolbox_file", tt);

	sp_toolbox_button_new_from_verb (t, 0, SP_BUTTON_TYPE_NORMAL, SP_VERB_FILE_NEW, tt);
	sp_toolbox_button_new_from_verb (t, 4, SP_BUTTON_TYPE_NORMAL, SP_VERB_FILE_OPEN, tt);
	sp_toolbox_button_new_from_verb (t, 1, SP_BUTTON_TYPE_NORMAL, SP_VERB_FILE_SAVE, tt);
	sp_toolbox_button_new_from_verb (t, 5, SP_BUTTON_TYPE_NORMAL, SP_VERB_FILE_SAVE_AS, tt);
#ifdef PDIRECT
	b = sp_button_menu_new (24, SP_BUTTON_TYPE_NORMAL, 2, tt);
	gtk_widget_show (b);
	action = sp_verb_get_action (SP_VERB_FILE_PRINT);
	sp_button_add_option (SP_BUTTON (b), 0, action);
	action = sp_verb_get_action (SP_VERB_FILE_PRINT_DIRECT);
	sp_button_add_option (SP_BUTTON (b), 1, action);
	gtk_table_attach (GTK_TABLE (t), b, 2, 3, 0, 1, 0, 0, 0, 0);
#else
	sp_toolbox_button_new_from_verb (t, 2, SP_BUTTON_TYPE_NORMAL, SP_VERB_FILE_PRINT, tt);
#endif
	b = sp_toolbox_button_new_from_verb (t, 6, SP_BUTTON_TYPE_NORMAL, SP_VERB_FILE_PRINT_PREVIEW, tt);
	gtk_widget_set_sensitive (b, FALSE);
#ifdef WITH_KDE
	gtk_widget_set_sensitive (b, TRUE);
#endif
	sp_toolbox_button_new_from_verb (t, 3, SP_BUTTON_TYPE_NORMAL, SP_VERB_FILE_IMPORT, tt);
	sp_toolbox_button_new_from_verb (t, 7, SP_BUTTON_TYPE_NORMAL, SP_VERB_FILE_EXPORT, tt);

	repr = sodipodi_get_repr (SODIPODI, "toolboxes.file");
	if (repr) {
		int state;
		state = 0;
		sp_repr_get_int (repr, "state", &state);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);
	}

	gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

        gtk_object_sink(GTK_OBJECT(tt));

	return tb;
}

static GtkWidget *
sp_toolbox_edit_create (void)
{
	GtkWidget *t, *tb;
	GtkTooltips *tt;
	SPRepr *repr;

	t = gtk_table_new (2, 4, TRUE);
	gtk_widget_show (t);

	tt = gtk_tooltips_new ();
	tb = sp_toolbox_new (t, _("Edit"), "edit", "toolbox_edit", tt);

	sp_toolbox_button_new_from_verb (t, 0, SP_BUTTON_TYPE_NORMAL, SP_VERB_EDIT_UNDO, tt);
	sp_toolbox_button_new_from_verb (t, 1, SP_BUTTON_TYPE_NORMAL, SP_VERB_EDIT_REDO, tt);
	sp_toolbox_button_new_from_verb (t, 3, SP_BUTTON_TYPE_NORMAL, SP_VERB_EDIT_DELETE, tt);
	sp_toolbox_button_new_from_verb (t, 4, SP_BUTTON_TYPE_NORMAL, SP_VERB_EDIT_CUT, tt);
	sp_toolbox_button_new_from_verb (t, 5, SP_BUTTON_TYPE_NORMAL, SP_VERB_EDIT_COPY, tt);
	sp_toolbox_button_new_from_verb (t, 6, SP_BUTTON_TYPE_NORMAL, SP_VERB_EDIT_PASTE, tt);
	sp_toolbox_button_new_from_verb (t, 7, SP_BUTTON_TYPE_NORMAL, SP_VERB_EDIT_DUPLICATE, tt);

	repr = sodipodi_get_repr (SODIPODI, "toolboxes.edit");
	if (repr) {
		int state;
		state = 0;
		sp_repr_get_int (repr, "state", &state);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);
	}

	gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

        gtk_object_sink(GTK_OBJECT(tt));

	return tb;
}

static GtkWidget *
sp_toolbox_object_create (void)
{
	GtkWidget *t, *tb, *b;
	GtkTooltips *tt;
	SPAction *action;
	SPRepr *repr;

	t = gtk_table_new (3, 4, TRUE);
	gtk_widget_show (t);

	tt = gtk_tooltips_new ();
	tb = sp_toolbox_new (t, _("Object"), "object", "toolbox_object", tt);

	sp_toolbox_button_new (t, 0, "object_stroke", GTK_SIGNAL_FUNC (sp_object_properties_stroke), tt, _("Stroke settings"));
	sp_toolbox_button_new (t, 1, "object_fill", GTK_SIGNAL_FUNC (sp_object_properties_fill), tt, _("Fill settings"));
	sp_toolbox_button_new (t, 2, "object_layout", GTK_SIGNAL_FUNC (sp_object_properties_layout), tt, _("Object size and position"));
	sp_toolbox_button_new (t, 3, "object_font", GTK_SIGNAL_FUNC (sp_text_edit_dialog), tt, _("Text and font settings"));
	sp_toolbox_button_new (t, 4, "object_align", GTK_SIGNAL_FUNC (sp_quick_align_dialog), tt, _("Align objects"));
	sp_toolbox_button_new (t, 5, "object_trans", GTK_SIGNAL_FUNC (sp_transformation_dialog_move), tt, _("Object transformations"));

	/* Mirror */
	b = sp_button_menu_new (24, SP_BUTTON_TYPE_NORMAL, 2, tt);
	gtk_widget_show (b);
	/* START COMPONENTS */
	action = sp_verb_get_action (SP_VERB_OBJECT_FLIP_HORIZONTAL);
	sp_button_add_option (SP_BUTTON (b), 0, action);
	action = sp_verb_get_action (SP_VERB_OBJECT_FLIP_VERTICAL);
	sp_button_add_option (SP_BUTTON (b), 1, action);
	gtk_table_attach (GTK_TABLE (t), b, 2, 3, 1, 2, 0, 0, 0, 0);

	sp_toolbox_button_new_from_verb (t, 7, SP_BUTTON_TYPE_NORMAL, SP_VERB_OBJECT_ROTATE_90, tt);
	sp_toolbox_button_new_from_verb (t, 8, SP_BUTTON_TYPE_NORMAL, SP_VERB_OBJECT_FLATTEN, tt);
	sp_toolbox_button_new_from_verb (t, 9, SP_BUTTON_TYPE_NORMAL, SP_VERB_OBJECT_TO_CURVE, tt);

	repr = sodipodi_get_repr (SODIPODI, "toolboxes.object");
	if (repr) {
		int state;
		state = 0;
		sp_repr_get_int (repr, "state", &state);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);
	}

	gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

        gtk_object_sink(GTK_OBJECT(tt));

	return tb;
}

static GtkWidget *
sp_toolbox_selection_create (void)
{
	GtkWidget *t, *tb;
	GtkTooltips *tt;
	SPRepr *repr;

	t = gtk_table_new (2, 4, TRUE);
	gtk_widget_show (t);

	tt = gtk_tooltips_new ();
	tb = sp_toolbox_new (t, _("Selection"), "selection", "toolbox_select", tt);

	sp_toolbox_button_new_from_verb (t, 0, SP_BUTTON_TYPE_NORMAL, SP_VERB_SELECTION_TO_FRONT, tt);
	sp_toolbox_button_new_from_verb (t, 1, SP_BUTTON_TYPE_NORMAL, SP_VERB_SELECTION_RAISE, tt);
	sp_toolbox_button_new_from_verb (t, 2, SP_BUTTON_TYPE_NORMAL, SP_VERB_SELECTION_COMBINE, tt);
	sp_toolbox_button_new_from_verb (t, 3, SP_BUTTON_TYPE_NORMAL, SP_VERB_SELECTION_GROUP, tt);
	sp_toolbox_button_new_from_verb (t, 4, SP_BUTTON_TYPE_NORMAL, SP_VERB_SELECTION_TO_BACK, tt);
	sp_toolbox_button_new_from_verb (t, 5, SP_BUTTON_TYPE_NORMAL, SP_VERB_SELECTION_LOWER, tt);
	sp_toolbox_button_new_from_verb (t, 6, SP_BUTTON_TYPE_NORMAL, SP_VERB_SELECTION_BREAK_APART, tt);
	sp_toolbox_button_new_from_verb (t, 7, SP_BUTTON_TYPE_NORMAL, SP_VERB_SELECTION_UNGROUP, tt);

	repr = sodipodi_get_repr (SODIPODI, "toolboxes.selection");
	if (repr) {
		int state;
		state = 0;
		sp_repr_get_int (repr, "state", &state);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);
	}

	gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

        gtk_object_sink(GTK_OBJECT(tt));

	return tb;
}

static GtkWidget *
sp_toolbox_draw_create (void)
{
	GtkWidget *tb, *t, *b;
	SPRepr *repr;
	GtkTooltips *tt;
	
	t = gtk_table_new (2, 4, TRUE);
	gtk_widget_show (t);
	
	tt = gtk_tooltips_new ();
	tb = sp_toolbox_new (t, _("Draw"), "draw", "toolbox_draw", tt);
	
	sp_toolbox_button_new_from_verb (t, 0, SP_BUTTON_TYPE_SELECT, SP_VERB_CONTEXT_SELECT, tt);
	sp_toolbox_button_new_from_verb (t, 1, SP_BUTTON_TYPE_SELECT, SP_VERB_CONTEXT_NODE, tt);
	b = sp_button_menu_new (24, SP_BUTTON_TYPE_SELECT, 4, tt);
	gtk_widget_show (b);
	sp_button_add_option (SP_BUTTON (b), 0, sp_verb_get_action (SP_VERB_CONTEXT_RECT));
	sp_button_add_option (SP_BUTTON (b), 1, sp_verb_get_action (SP_VERB_CONTEXT_ARC));
	sp_button_add_option (SP_BUTTON (b), 2, sp_verb_get_action (SP_VERB_CONTEXT_STAR));
	sp_button_add_option (SP_BUTTON (b), 3, sp_verb_get_action (SP_VERB_CONTEXT_SPIRAL));
	gtk_table_attach (GTK_TABLE (t), b, 2, 3, 0, 1, 0, 0, 0, 0);
	b = sp_button_menu_new (24, SP_BUTTON_TYPE_SELECT, 3, tt);
	gtk_widget_show (b);
	sp_button_add_option (SP_BUTTON (b), 0, sp_verb_get_action (SP_VERB_CONTEXT_PENCIL));
	sp_button_add_option (SP_BUTTON (b), 1, sp_verb_get_action (SP_VERB_CONTEXT_PEN));
	sp_button_add_option (SP_BUTTON (b), 2, sp_verb_get_action (SP_VERB_CONTEXT_CALLIGRAPHIC));
	gtk_table_attach (GTK_TABLE (t), b, 3, 4, 0, 1, 0, 0, 0, 0);
	sp_toolbox_button_new_from_verb (t, 4, SP_BUTTON_TYPE_SELECT, SP_VERB_CONTEXT_TEXT, tt);
	sp_toolbox_button_new_from_verb (t, 5, SP_BUTTON_TYPE_SELECT, SP_VERB_CONTEXT_ZOOM, tt);
	sp_toolbox_button_new_from_verb (t, 6, SP_BUTTON_TYPE_SELECT, SP_VERB_CONTEXT_DROPPER, tt);
	
	repr = sodipodi_get_repr (SODIPODI, "toolboxes.draw");
	if (repr) {
		int state;
		state = 0;
		sp_repr_get_int (repr, "state", &state);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);
	}

	gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

        gtk_object_sink(GTK_OBJECT(tt));

	return tb;
}

#if 0

static GtkWidget *
sp_toolbox_extension_create (void)
{
	GtkWidget *t, *tb, *button;
	GtkTooltips *tt;
	SPRepr *repr;
	SPRepr *extensions_repr;
	SPRepr *ext;
	int pos = 0;

	t = gtk_table_new (2, 4, TRUE);
	gtk_widget_show (t);

	/* Todo:  Generalize all the toolbox routines to have general purpose
	   routine for creating them */

	/* Create the extension toolbox */
	/* Todo: Make it able to create required boxes dynamically */
	tt = gtk_tooltips_new ();
	tb = sp_toolbox_new (t, _("Extension"), "extension", SODIPODI_PIXMAPDIR "/toolbox_zoom.xpm", tt);

	/* Loop over the list of extensions in spx structure */
	extensions_repr = sodipodi_get_repr (SODIPODI, "extensions");
	for (ext = extensions_repr->children; ext != NULL; ext = ext->next) {
	  if (strcmp(sp_repr_attr(ext, "state"),"active")==0) {
	    printf("Loading extension:  %s\n", sp_repr_attr(ext, "id"));
	    button = sp_toolbox_button_new (t, pos, 
					    sp_repr_attr(ext, "toolbutton_filename"), 
					    GTK_SIGNAL_FUNC (sp_extension), 
					    tt, 
					    _(sp_repr_attr(ext, "description")));
	    gtk_widget_set_name(button, sp_repr_attr(ext, "id"));
	    pos++;
	  }
	}

	repr = sodipodi_get_repr (SODIPODI, "toolboxes.extension");

	if (repr) {
		int state;
		state = 0;
		sp_repr_get_int (repr, "state", &state);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);

		gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);
	}

        gtk_object_sink(GTK_OBJECT(tt));

	return tb;
}
#endif

static GtkWidget *
sp_toolbox_zoom_create (void)
{
	GtkWidget *t, *tb;
	GtkTooltips *tt;
	SPRepr *repr;

	t = gtk_table_new (2, 4, TRUE);
	gtk_widget_show (t);

	tt = gtk_tooltips_new ();
	tb = sp_toolbox_new (t, _("Zoom"), "zoom", "toolbox_zoom", tt);

	sp_toolbox_button_new_from_verb (t, 0, SP_BUTTON_TYPE_NORMAL, SP_VERB_ZOOM_IN, tt);
	sp_toolbox_button_new_from_verb (t, 1, SP_BUTTON_TYPE_NORMAL, SP_VERB_ZOOM_2_1, tt);
	sp_toolbox_button_new_from_verb (t, 2, SP_BUTTON_TYPE_NORMAL, SP_VERB_ZOOM_1_1, tt);
	sp_toolbox_button_new_from_verb (t, 3, SP_BUTTON_TYPE_NORMAL, SP_VERB_ZOOM_1_2, tt);
	sp_toolbox_button_new_from_verb (t, 4, SP_BUTTON_TYPE_NORMAL, SP_VERB_ZOOM_OUT, tt);
	sp_toolbox_button_new_from_verb (t, 5, SP_BUTTON_TYPE_NORMAL, SP_VERB_ZOOM_PAGE, tt);
	sp_toolbox_button_new_from_verb (t, 6, SP_BUTTON_TYPE_NORMAL, SP_VERB_ZOOM_DRAWING, tt);
	sp_toolbox_button_new_from_verb (t, 7, SP_BUTTON_TYPE_NORMAL, SP_VERB_ZOOM_SELECTION, tt);

	repr = sodipodi_get_repr (SODIPODI, "toolboxes.zoom");
	if (repr) {
		int state;
		state = 0;
		sp_repr_get_int (repr, "state", &state);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);
	}

	gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

        gtk_object_sink(GTK_OBJECT(tt));

	return tb;
}

static GtkWidget *
sp_toolbox_node_create (void)
{
	GtkWidget *t, *tb;
	GtkTooltips *tt;
	SPRepr *repr;

	t = gtk_table_new (3, 4, TRUE);
	gtk_widget_show (t);

	tt = gtk_tooltips_new ();
	tb = sp_toolbox_new (t, _("Nodes"), "node", "toolbox_node", tt);

	sp_toolbox_button_new (t, 0, "node_insert", GTK_SIGNAL_FUNC (sp_node_path_edit_add),
			       tt, _("Insert new nodes into selected segments"));
	sp_toolbox_button_new (t, 1, "node_break", GTK_SIGNAL_FUNC (sp_node_path_edit_break),
			       tt, _("Break line at selected nodes"));
	sp_toolbox_button_new (t, 2, "node_cusp", GTK_SIGNAL_FUNC (sp_node_path_edit_cusp),
			       tt, _("Make selected nodes corner"));
	sp_toolbox_button_new (t, 3, "node_line", GTK_SIGNAL_FUNC (sp_node_path_edit_toline),
			       tt, _("Make selected segments lines"));
	sp_toolbox_button_new (t, 4, "node_delete", GTK_SIGNAL_FUNC (sp_node_path_edit_delete),
			       tt, _("Delete selected nodes"));
	sp_toolbox_button_new (t, 5, "node_join", GTK_SIGNAL_FUNC (sp_node_path_edit_join),
			       tt, _("Join lines at selected nodes"));
	sp_toolbox_button_new (t, 6, "node_join_segment",
			       GTK_SIGNAL_FUNC (sp_node_path_edit_join_segment),
			       tt, _("Join lines at selected nodes with new segment"));
	sp_toolbox_button_new (t, 7, "node_smooth", GTK_SIGNAL_FUNC (sp_node_path_edit_smooth),
			       tt, _("Make selected nodes smooth"));
	sp_toolbox_button_new (t, 8, "node_curve", GTK_SIGNAL_FUNC (sp_node_path_edit_tocurve),
			       tt, _("Make selected segments curves"));

	repr = sodipodi_get_repr (SODIPODI, "toolboxes.node");
	if (repr) {
		int state;
		state = 0;
		sp_repr_get_int (repr, "state", &state);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);
	}

	gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

        gtk_object_sink(GTK_OBJECT(tt));

	return tb;
}

static gint
sp_toolbox_set_state_handler (SPToolBox * t, guint state, gpointer data)
{
	SPRepr *repr;

	repr = (SPRepr *) data;

	sp_repr_set_int (repr, "state", state);

	return FALSE;
}

static void 
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
sp_maintoolbox_open_files (gchar *buffer)
{
  	GList *list;

	list = gnome_uri_list_extract_filenames (buffer);
	if (!list) return;
	g_list_foreach (list, sp_maintoolbox_open_one_file_with_check, NULL);
	g_list_foreach (list, (GFunc) g_free, NULL);
	g_list_free (list);
}

static void
sp_maintoolbox_open_one_file_with_check (gpointer filename, gpointer unused)
{
	static const gchar *svg_suffix = ".svg";

	if (filename) {
		int len;
		len = strlen (filename);
		if (len > 4 && !strcmp ((char *) filename + len - 4, svg_suffix)) {
			sp_file_open (filename, NULL);
		}
	}
}

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

#include <config.h>

#include <string.h>
#include <glib.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkselection.h>
#include <gtk/gtktable.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtktooltips.h>
#include <gtk/gtkdnd.h>

#include "macros.h"
#include "widgets/icon.h"
#include "widgets/button.h"
#include "widgets/sp-toolbox.h"
#include "file.h"
#include "selection-chemistry.h"
#include "path-chemistry.h"
#include "sodipodi-private.h"
#include "document.h"
#include "sodipodi.h"
#include "event-broker.h"
#include "dialogs/object-properties.h"
#include "dialogs/transformation.h"
#include "dialogs/text-edit.h"
#include "dialogs/align.h"
#include "dialogs/export.h"
#include "dialogs/node-edit.h"
#include "zoom-context.h"
#include "selection.h"
#include "extension.h"
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
#ifdef WITH_EXTENSIONS_TOOLBOX
static GtkWidget *sp_toolbox_extension_create (void);
#endif
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
static void sp_update_draw_toolbox (Sodipodi * sodipodi, SPEventContext * eventcontext, GObject *toolbox);

static void sp_toolbox_object_flip_clicked (SPButton *button, gpointer data);

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

static void
sp_maintoolbox_destroy (GtkObject *object, gpointer data)
{
	sp_signal_disconnect_by_data (sodipodi, object);
	sodipodi_unref ();
}

void
sp_maintoolbox_create_toplevel (void)
{
	GtkWidget *window, *toolbox;

	/* Create window */
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), _("Sodipodi"));
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

	toolbox = sp_maintoolbox_new ();
	gtk_widget_show (toolbox);
	gtk_container_add (GTK_CONTAINER (window), toolbox);

	gtk_widget_show (window);
};

GtkWidget *
sp_maintoolbox_new (void)
{
	GtkWidget *toolbox, *vbox, *mbar, *mi, *t, *w;

	vbox = gtk_vbox_new (FALSE, 0);
	toolbox = vbox;

	mbar = gtk_menu_bar_new ();
	gtk_widget_show (mbar);
	gtk_box_pack_start (GTK_BOX (vbox), mbar, FALSE, FALSE, 0);

	mi = gtk_menu_item_new_with_label (_("Sodipodi"));
	gtk_widget_show (mi);
	gtk_menu_bar_append (GTK_MENU_BAR (mbar), mi);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), sp_ui_main_menu ());
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
	g_signal_connect (G_OBJECT (SODIPODI), "set_eventcontext", G_CALLBACK (sp_update_draw_toolbox), toolbox);
#ifdef WITH_EXTENSIONS_TOOLBOX
	/* Extension */
	t = sp_toolbox_extension_create ();
	gtk_widget_show (t);
	gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
#endif
	/* Zoom */
	t = sp_toolbox_zoom_create ();
	gtk_widget_show (t);
	gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
	/* Node */
	t = sp_toolbox_node_create ();
	gtk_widget_show (t);
	gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);

	/* gnome_window_icon_set_from_default (GTK_WINDOW (toolbox)); */
	gtk_drag_dest_set (toolbox, 
			  GTK_DEST_DEFAULT_ALL,
			  toolbox_drop_target_entries,
			  ntoolbox_drop_target_entries,
			  GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (toolbox), "destroy", G_CALLBACK (sp_maintoolbox_destroy), NULL);
	/* g_signal_connect (G_OBJECT (toolbox), "delete_event", G_CALLBACK (sp_maintoolbox_delete_event), NULL); */
	g_signal_connect (G_OBJECT (toolbox), "drag_data_received", G_CALLBACK (sp_maintoolbox_drag_data_received), NULL);

	/* Reference our sodipodi engine */
	sodipodi_ref ();

	return toolbox;
}

enum {
	SP_TOOLBOX_DRAW_RECT,
	SP_TOOLBOX_DRAW_ARC,
	SP_TOOLBOX_DRAW_STAR,
	SP_TOOLBOX_DRAW_SPIRAL
};

enum {
	SP_TOOLBOX_DRAW_FREEHAND,
	SP_TOOLBOX_DRAW_PEN,
	SP_TOOLBOX_DRAW_DYNAHAND
};

static void
sp_toolbox_draw_set_object (SPButton *button, gpointer data)
{
	unsigned int mode;

	mode = sp_button_get_option (button);

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
	default:
		g_warning ("Illegal draw code %d", mode);
		break;
	}
}

static void
sp_toolbox_draw_set_freehand (SPButton *button, gpointer data)
{
	unsigned int mode;

	mode = sp_button_get_option (button);

	switch (mode) {
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

static GtkWidget *
sp_toolbox_button_new (GtkWidget *t, int pos, const unsigned char *pxname, GtkSignalFunc handler, GtkTooltips *tt, unsigned char *tip)
{
	GtkWidget *b;
	int xpos, ypos;

	b = sp_button_new (24, pxname, tip);
	sp_button_set_tooltips (SP_BUTTON (b), tt);
	gtk_widget_show (b);
	if (handler) gtk_signal_connect (GTK_OBJECT (b), "clicked", handler, NULL);
	xpos = pos % 4;
	ypos = pos / 4;
	gtk_table_attach (GTK_TABLE (t), b, xpos, xpos + 1, ypos, ypos + 1, 0, 0, 0, 0);

	return b;
}

static GtkWidget *
sp_toolbox_toggle_button_new (const unsigned char *pxname, GtkTooltips *tt, const unsigned char *tip)
{
	GtkWidget *b;

	b = sp_button_toggle_new (24, pxname, tip);
	sp_button_set_tooltips (SP_BUTTON (b), tt);
	gtk_widget_show (b);

	return b;
}

static GtkWidget *
sp_toolbox_file_create (void)
{
	GtkWidget *t, *tb, *b;
	GtkTooltips *tt;
	SPRepr *repr;

	t = gtk_table_new (2, 4, TRUE);
	gtk_widget_show (t);

	tb = sp_toolbox_new (t, _("File"), "file", "toolbox_file");
	tt = gtk_tooltips_new ();

	sp_toolbox_button_new (t, 0, "file_new", GTK_SIGNAL_FUNC (sp_file_new), tt, _("Create new SVG document"));
	sp_toolbox_button_new (t, 4, "file_open", GTK_SIGNAL_FUNC (sp_file_open_dialog), tt, _("Open existing SVG document"));
	sp_toolbox_button_new (t, 1, "file_save", GTK_SIGNAL_FUNC (sp_file_save), tt, _("Save document"));
	sp_toolbox_button_new (t, 5, "file_save_as", GTK_SIGNAL_FUNC (sp_file_save_as), tt, _("Save document under new name"));
	b = sp_toolbox_button_new (t, 2, "file_print", GTK_SIGNAL_FUNC (sp_file_print), tt, _("Print document"));
	b = sp_toolbox_button_new (t, 6, "file_print_preview", GTK_SIGNAL_FUNC (sp_file_print_preview), tt, _("Preview document printout"));
#ifndef WITH_GNOME_PRINT
	gtk_widget_set_sensitive (b, FALSE);
#endif
	sp_toolbox_button_new (t, 3, "file_import", GTK_SIGNAL_FUNC (sp_file_import), tt, _("Import bitmap or SVG image into document"));
	sp_toolbox_button_new (t, 7, "file_export", GTK_SIGNAL_FUNC (sp_file_export_dialog), tt, _("Export document as PNG bitmap"));

	repr = sodipodi_get_repr (SODIPODI, "toolboxes.file");
	if (repr) {
		gint state;
		state = sp_repr_get_int_attribute (repr, "state", 0);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);
	}

	gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

	return tb;
}

static GtkWidget *
sp_toolbox_edit_create (void)
{
	GtkWidget *t, *tb, *b;
	GtkTooltips *tt;
	SPRepr *repr;

	t = gtk_table_new (2, 4, TRUE);
	gtk_widget_show (t);

	tb = sp_toolbox_new (t, _("Edit"), "edit", "toolbox_edit");
	tt = gtk_tooltips_new ();

	b = sp_toolbox_button_new (t, 0, "edit_undo", GTK_SIGNAL_FUNC (sp_undo), tt, _("Revert last action"));
	/* fixme: Freehand does not need this anymore, remove if node editing is fixed (Lauris) */
	gtk_object_set_data (GTK_OBJECT (tb), "undo", b);
	b = sp_toolbox_button_new (t, 1, "edit_redo", GTK_SIGNAL_FUNC (sp_redo), tt, _("Do again undone action"));
	gtk_object_set_data (GTK_OBJECT (tb), "redo", b);
	sp_toolbox_button_new (t, 3, "edit_delete", GTK_SIGNAL_FUNC (sp_selection_delete), tt, _("Delete selected objects"));
	sp_toolbox_button_new (t, 4, "edit_cut", GTK_SIGNAL_FUNC (sp_selection_cut), tt, _("Cut selected objects to clipboard"));
	sp_toolbox_button_new (t, 5, "edit_copy", GTK_SIGNAL_FUNC (sp_selection_copy), tt, _("Copy selected objects to clipboard"));
	sp_toolbox_button_new (t, 6, "edit_paste", GTK_SIGNAL_FUNC (sp_selection_paste), tt, _("Paste objects from clipboard"));
	sp_toolbox_button_new (t, 7, "edit_duplicate", GTK_SIGNAL_FUNC (sp_selection_duplicate), tt, _("Duplicate selected objects"));

	repr = sodipodi_get_repr (SODIPODI, "toolboxes.edit");
	if (repr) {
		gint state;
		state = sp_repr_get_int_attribute (repr, "state", 0);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);
	}

	gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

	return tb;
}

static GtkWidget *
sp_toolbox_object_create (void)
{
	GtkWidget *t, *tb, *b;
	GtkTooltips *tt;
	SPRepr *repr;

	t = gtk_table_new (3, 4, TRUE);
	gtk_widget_show (t);

	tb = sp_toolbox_new (t, _("Object"), "object", "toolbox_object");
	tt = gtk_tooltips_new ();

	sp_toolbox_button_new (t, 0, "object_stroke", GTK_SIGNAL_FUNC (sp_object_properties_stroke), tt, _("Stroke settings"));
	sp_toolbox_button_new (t, 1, "object_fill", GTK_SIGNAL_FUNC (sp_object_properties_fill), tt, _("Fill settings"));
	sp_toolbox_button_new (t, 2, "object_layout", GTK_SIGNAL_FUNC (sp_object_properties_layout), tt, _("Object size and position"));
	sp_toolbox_button_new (t, 3, "object_font", GTK_SIGNAL_FUNC (sp_text_edit_dialog), tt, _("Text and font settings"));
	sp_toolbox_button_new (t, 4, "object_align", GTK_SIGNAL_FUNC (sp_quick_align_dialog), tt, _("Align objects"));
	sp_toolbox_button_new (t, 5, "object_trans", GTK_SIGNAL_FUNC (sp_transformation_dialog_move), tt, _("Object transformations"));

	/* Mirror */
	b = sp_button_menu_new (24, 2);
	sp_button_set_tooltips (SP_BUTTON (b), tt);
	gtk_widget_show (b);
	/* START COMPONENTS */
	sp_button_add_option (SP_BUTTON (b), 0, "object_flip_hor", _("Flip selected objects horizontally"));
	sp_button_add_option (SP_BUTTON (b), 1, "object_flip_ver", _("Flip selected objects vertically"));
	gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (sp_toolbox_object_flip_clicked), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 2, 3, 1, 2, 0, 0, 0, 0);

	sp_toolbox_button_new (t, 7, "object_rotate", GTK_SIGNAL_FUNC (sp_selection_rotate_90), tt, _("Rotate object 90 deg clockwise"));
	sp_toolbox_button_new (t, 8, "object_reset", GTK_SIGNAL_FUNC (sp_selection_remove_transform), tt, _("Remove transformations"));
	sp_toolbox_button_new (t, 9, "object_tocurve", GTK_SIGNAL_FUNC (sp_selected_path_to_curves), tt, _("Convert object to curve"));

	repr = sodipodi_get_repr (SODIPODI, "toolboxes.object");
	if (repr) {
		gint state;
		state = sp_repr_get_int_attribute (repr, "state", 0);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);
	}

	gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

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

	tb = sp_toolbox_new (t, _("Selection"), "selection", "toolbox_select");
	tt = gtk_tooltips_new ();

	sp_toolbox_button_new (t, 0, "selection_top", GTK_SIGNAL_FUNC (sp_selection_raise_to_top), tt, _("Raise selected objects to top"));
	sp_toolbox_button_new (t, 1, "selection_up", GTK_SIGNAL_FUNC (sp_selection_raise), tt, _("Raise selected objects one level"));
	sp_toolbox_button_new (t, 2, "selection_combine", GTK_SIGNAL_FUNC (sp_selected_path_combine), tt, _("Combine multiple paths"));
	sp_toolbox_button_new (t, 3, "selection_group", GTK_SIGNAL_FUNC (sp_selection_group), tt, _("Group selected objects"));
	sp_toolbox_button_new (t, 4, "selection_bot", GTK_SIGNAL_FUNC (sp_selection_lower_to_bottom), tt, _("Lower selected objects to bottom"));
	sp_toolbox_button_new (t, 5, "selection_down", GTK_SIGNAL_FUNC (sp_selection_lower), tt, _("Lower selected objects one level"));
	sp_toolbox_button_new (t, 6, "selection_break", GTK_SIGNAL_FUNC (sp_selected_path_break_apart), tt, _("Break selected path to subpaths"));
	sp_toolbox_button_new (t, 7, "selection_ungroup", GTK_SIGNAL_FUNC (sp_selection_ungroup), tt, _("Ungroup selected group"));

	repr = sodipodi_get_repr (SODIPODI, "toolboxes.selection");
	if (repr) {
		gint state;
		state = sp_repr_get_int_attribute (repr, "state", 0);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);
	}

	gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

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
	tb = sp_toolbox_new (t, _("Draw"), "draw", "toolbox_draw");
	
	tt = gtk_tooltips_new ();
	
	/* Select */
	b = sp_toolbox_toggle_button_new ("draw_select", tt, _("Select tool - select and transform objects"));
	sp_button_set_tooltips (SP_BUTTON (b), tt);
	gtk_signal_connect (GTK_OBJECT (b), "released", GTK_SIGNAL_FUNC (sp_event_context_set_select), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 0, 1, 0, 1, 0, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (tb), "SPSelectContext", b);

	/* Node */
	b = sp_toolbox_toggle_button_new ("draw_node", tt, _("Node tool - modify different aspects of existing objects"));
	sp_button_set_tooltips (SP_BUTTON (b), tt);
	gtk_signal_connect (GTK_OBJECT (b), "released", GTK_SIGNAL_FUNC (sp_event_context_set_node_edit), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 1, 2, 0, 1, 0, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (tb), "SPNodeContext", b);

	/* Draw */
	b = sp_button_toggle_menu_new (24, 4);
	sp_button_set_tooltips (SP_BUTTON (b), tt);
	gtk_widget_show (b);
	/* START COMPONENTS */
	/* Rect */
	sp_button_add_option (SP_BUTTON (b), 0, "draw_rect", _("Rectangle tool - create rectangles and squares with optional rounded corners"));
	sp_button_add_option (SP_BUTTON (b), 1, "draw_arc", _("Arc tool - create circles, ellipses and arcs"));
	sp_button_add_option (SP_BUTTON (b), 2, "draw_star", _("Star tool - create stars and polygons"));
	sp_button_add_option (SP_BUTTON (b), 3, "draw_spiral", _("Spiral tool - create spirals"));
	gtk_signal_connect (GTK_OBJECT (b), "released", GTK_SIGNAL_FUNC (sp_toolbox_draw_set_object), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 2, 3, 0, 1, 0, 0, 0, 0);
	/* fixme: */
	gtk_object_set_data (GTK_OBJECT (tb), "SPRectContext", b);
	gtk_object_set_data (GTK_OBJECT (tb), "SPArcContext", b);
	gtk_object_set_data (GTK_OBJECT (tb), "SPStarContext", b);
	gtk_object_set_data (GTK_OBJECT (tb), "SPSpiralContext", b);

	/* Freehand */
	b = sp_button_toggle_menu_new (24, 3);
	sp_button_set_tooltips (SP_BUTTON (b), tt);
	gtk_widget_show (b);
	/* START COMPONENTS */
	/* Rect */
	sp_button_add_option (SP_BUTTON (b), 0, "draw_freehand", _("Pencil tool - draw freehand lines and straight segments"));
	sp_button_add_option (SP_BUTTON (b), 1, "draw_pen", _("Pen tool - draw exactly positioned curved lines"));
	sp_button_add_option (SP_BUTTON (b), 2, "draw_dynahand", _("Calligraphic tool - draw calligraphic lines"));
	gtk_signal_connect (GTK_OBJECT (b), "released", GTK_SIGNAL_FUNC (sp_toolbox_draw_set_freehand), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 3, 4, 0, 1, 0, 0, 0, 0);
	/* fixme: */
	gtk_object_set_data (GTK_OBJECT (tb), "SPPencilContext", b);
	gtk_object_set_data (GTK_OBJECT (tb), "SPPenContext", b);
	gtk_object_set_data (GTK_OBJECT (tb), "SPDynaDrawContext", b);

	/* Text */
	b = sp_toolbox_toggle_button_new ("draw_text", tt, _("Text tool - create editable text objects"));
	gtk_signal_connect (GTK_OBJECT (b), "released", GTK_SIGNAL_FUNC (sp_event_context_set_text), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 0, 1, 1, 2, 0, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (tb), "SPTextContext", b);

	/* Zoom */
	b = sp_toolbox_toggle_button_new ("draw_zoom", tt, _("Zoom tool - zoom into choosen area"));
	gtk_signal_connect (GTK_OBJECT (b), "released", GTK_SIGNAL_FUNC (sp_event_context_set_zoom), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 1, 2, 1, 2, 0, 0, 0, 0);
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

#ifdef WITH_EXTENSIONS_TOOLBOX

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
	tb = sp_toolbox_new (t, _("Extension"), "extension", SODIPODI_PIXMAPDIR "/toolbox_zoom.xpm");
	tt = gtk_tooltips_new ();

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
		gint state;
		state = sp_repr_get_int_attribute (repr, "state", 0);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);

		gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);
	}

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

	tb = sp_toolbox_new (t, _("Zoom"), "zoom", "toolbox_zoom");
	tt = gtk_tooltips_new ();

	sp_toolbox_button_new (t, 0, "zoom_in", GTK_SIGNAL_FUNC (sp_zoom_in), tt, _("Zoom in drawing"));
	sp_toolbox_button_new (t, 1, "zoom_2_to_1", GTK_SIGNAL_FUNC (sp_zoom_2_to_1), tt, _("Set zoom factor to 2:1"));
	sp_toolbox_button_new (t, 2, "zoom_1_to_1", GTK_SIGNAL_FUNC (sp_zoom_1_to_1), tt, _("Set zoom factor to 1:1"));
	sp_toolbox_button_new (t, 3, "zoom_1_to_2", GTK_SIGNAL_FUNC (sp_zoom_1_to_2), tt, _("Set zoom factor to 1:2"));
	sp_toolbox_button_new (t, 4, "zoom_out", GTK_SIGNAL_FUNC (sp_zoom_out), tt, _("Zoom out drawing"));
	sp_toolbox_button_new (t, 5, "zoom_page", GTK_SIGNAL_FUNC (sp_zoom_page), tt, _("Fit the whole page into window"));
	sp_toolbox_button_new (t, 6, "zoom_draw", GTK_SIGNAL_FUNC (sp_zoom_drawing), tt, _("Fit the whole drawing into window"));
	sp_toolbox_button_new (t, 7, "zoom_select", GTK_SIGNAL_FUNC (sp_zoom_selection), tt, _("Fit the whole selection into window"));

	repr = sodipodi_get_repr (SODIPODI, "toolboxes.zoom");
	if (repr) {
		gint state;
		state = sp_repr_get_int_attribute (repr, "state", 0);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);
	}

	gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

	return tb;
}

static GtkWidget *
sp_toolbox_node_create (void)
{
	GtkWidget *t, *tb;
	GtkTooltips *tt;
	SPRepr *repr;

	t = gtk_table_new (2, 4, TRUE);
	gtk_widget_show (t);

	tb = sp_toolbox_new (t, _("Nodes"), "node", "toolbox_node");
	tt = gtk_tooltips_new ();

	sp_toolbox_button_new (t, 0, "node_insert", GTK_SIGNAL_FUNC (sp_node_path_edit_add), tt, _("Insert new nodes into selected segments"));
	sp_toolbox_button_new (t, 1, "node_break", GTK_SIGNAL_FUNC (sp_node_path_edit_break), tt, _("Break line at selected nodes"));
	sp_toolbox_button_new (t, 2, "node_cusp", GTK_SIGNAL_FUNC (sp_node_path_edit_cusp), tt, _("Make selected nodes corner"));
	sp_toolbox_button_new (t, 3, "node_line", GTK_SIGNAL_FUNC (sp_node_path_edit_toline), tt, _("Make selected segments lines"));
	sp_toolbox_button_new (t, 4, "node_delete", GTK_SIGNAL_FUNC (sp_node_path_edit_delete), tt, _("Delete selected nodes"));
	sp_toolbox_button_new (t, 5, "node_join", GTK_SIGNAL_FUNC (sp_node_path_edit_join), tt, _("Join lines at selected nodes"));
	sp_toolbox_button_new (t, 6, "node_smooth", GTK_SIGNAL_FUNC (sp_node_path_edit_smooth), tt, _("Make selected nodes smooth"));
	sp_toolbox_button_new (t, 7, "node_curve", GTK_SIGNAL_FUNC (sp_node_path_edit_tocurve), tt, _("Make selected segments curves"));

	repr = sodipodi_get_repr (SODIPODI, "toolboxes.node");
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
}

static void
sp_maintoolbox_open_one_file_with_check (gpointer filename, gpointer unused)
{
	static const gchar *svg_suffix = ".svg";

	if (filename) {
		int len;
		len = strlen (filename);
		if (len > 4 && !strcmp ((char *) filename + len - 4, svg_suffix)) {
			sp_file_open (filename);
		}
	}
}

/* 
 * object toolbox
 */

static void
sp_toolbox_object_flip_clicked (SPButton *button, gpointer data)
{
	SPDesktop *desktop;
	SPSelection *selection;
	SPItem *item;
	GSList *l, *l2;
	unsigned int axis;

	desktop = SP_ACTIVE_DESKTOP;
	if (!desktop) return;
	selection = SP_DT_SELECTION (desktop);
	if sp_selection_is_empty (selection) return;

	axis = sp_button_get_option (button);

	l = selection->items;  
	for (l2 = l; l2 != NULL; l2 = l2-> next) {
		item = SP_ITEM (l2->data);
		sp_item_scale_rel (item, (axis) ? 1.0 : -1.0, (axis) ? -1.0 : 1.0);
	}
	sp_selection_changed (selection);
	sp_document_done (SP_DT_DOCUMENT (desktop));
}

static void 
sp_update_draw_toolbox (Sodipodi * sodipodi, SPEventContext * eventcontext, GObject *toolbox)
{
	GObject *draw;
	const gchar * tname;
	gpointer active, new, e, w;

	tname = NULL;

	draw = g_object_get_data (toolbox, "draw");
	active = g_object_get_data (draw, "active");

	if (eventcontext != NULL) {
		tname = gtk_type_name (GTK_OBJECT_TYPE (eventcontext));
		new = g_object_get_data (G_OBJECT (draw), tname);
	} else {
		new = NULL;
	}

	if (new != active) {
		if (active && SP_BUTTON_IS_DOWN (SP_BUTTON (active))) {
			sp_button_toggle_set_down (SP_BUTTON (active), FALSE, TRUE);
		}
		if (new && !SP_BUTTON_IS_DOWN (SP_BUTTON (new))) {
			sp_button_toggle_set_down (SP_BUTTON (new), TRUE, TRUE);
		}
		g_object_set_data (draw, "active", new);
		if (tname && !strcmp (tname, "SPNodeContext")) {
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
			sp_button_set_option (SP_BUTTON (new), SP_TOOLBOX_DRAW_RECT);
		} else if (!strcmp (tname, "SPArcContext")) {
			sp_button_set_option (SP_BUTTON (new), SP_TOOLBOX_DRAW_ARC);
		} else if (!strcmp (tname, "SPStarContext")) {
			sp_button_set_option (SP_BUTTON (new), SP_TOOLBOX_DRAW_STAR);
		} else if (!strcmp (tname, "SPSpiralContext")) {
			sp_button_set_option (SP_BUTTON (new), SP_TOOLBOX_DRAW_SPIRAL);
		} else if (!strcmp (tname, "SPPencilContext")) {
			sp_button_set_option (SP_BUTTON (new), SP_TOOLBOX_DRAW_FREEHAND);
		} else if (!strcmp (tname, "SPPenContext")) {
			sp_button_set_option (SP_BUTTON (new), SP_TOOLBOX_DRAW_PEN);
		} else if (!strcmp (tname, "SPDynaDrawContext")) {
			sp_button_set_option (SP_BUTTON (new), SP_TOOLBOX_DRAW_DYNAHAND);
		}
	}
}


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
#include <gtk/gtkimage.h>
#include <gtk/gtktooltips.h>
#include <gtk/gtkdnd.h>

#include "macros.h"
#include "widgets/sp-toolbox.h"
#include "widgets/sp-menu-button.h"
#include "widgets/sp-hwrap-box.h"
#include "file.h"
#include "selection-chemistry.h"
#include "path-chemistry.h"
#include "sodipodi-private.h"
#include "document.h"
#include "sodipodi.h"
#include "event-broker.h"
#include "dialogs/object-properties.h"
#if 0
#include "dialogs/transformation.h"
#endif
#include "dialogs/text-edit.h"
#include "dialogs/align.h"
#include "dialogs/export.h"
#include "dialogs/node-edit.h"
#include "zoom-context.h"
#include "selection.h"
#include "sp-item-transform.h"
#include "desktop-handles.h"
#include "interface.h"
#include "toolbox.h"
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
static void sp_update_draw_toolbox (Sodipodi * sodipodi, SPEventContext * eventcontext, gpointer data);
void object_flip (GtkWidget * widget, GdkEventButton * event);

/* helper function */
static GtkWidget *
gtk_event_box_new_with_image_file_and_tooltips(const gchar   *image_file,
					       const gchar   *tip_text,
					       const gchar   *tip_private);
static guint gtk_event_box_force_draw_parent(GtkWidget * widget, 
					     GdkEventExpose *event, 
					     gpointer user_data);

static GtkWidget * toolbox = NULL;

static GtkWidget * fh_pixmap = NULL;
static GtkWidget * fv_pixmap = NULL;

#if 0
GtkWidget * zoom_any = NULL;
#endif

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

static void
sp_maintoolbox_destroy (GtkObject *object, gpointer data)
{
	sp_signal_disconnect_by_data (sodipodi, object);
}

static gint
sp_maintoolbox_delete_event (GtkWidget *widget, GdkEventAny *event, gpointer data)
{
	/* fixme: i do not like that hide/show game (Lauris) */

	sodipodi_unref ();

	gtk_widget_hide (GTK_WIDGET (toolbox));

	return TRUE;
}

#if 0
static gint
sp_maintoolbox_menu_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkWidget *m;

	m = sp_ui_main_menu ();
	gtk_widget_show (m);

	gtk_menu_popup (GTK_MENU (m), NULL, NULL, NULL, NULL, event->button, event->time);

	return TRUE;
}
#endif

void
sp_maintoolbox_create (void)
{
	if (toolbox == NULL) {
		GtkWidget *vbox, *mbar, *mi, *t, *w;

		/* Create window */
		toolbox = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (toolbox), _("Sodipodi"));
		/* Hmmm, this does not shrink, but set_policy is deprecated. */
		gtk_window_set_resizable (GTK_WINDOW (toolbox), TRUE);
/*  		gtk_window_set_policy (GTK_WINDOW (toolbox), TRUE, TRUE, TRUE); */
		g_signal_connect (G_OBJECT (toolbox), "destroy", G_CALLBACK (sp_maintoolbox_destroy), NULL);
		g_signal_connect (G_OBJECT (toolbox), "delete_event", G_CALLBACK (sp_maintoolbox_delete_event), NULL);
		g_signal_connect (G_OBJECT (toolbox), "drag_data_received", G_CALLBACK (sp_maintoolbox_drag_data_received), NULL);
				    
		vbox = gtk_vbox_new (FALSE, 0);
		gtk_widget_show (vbox);
		gtk_container_add (GTK_CONTAINER (toolbox), vbox);
		mbar = gtk_menu_bar_new ();
		gtk_widget_show (mbar);
		gtk_box_pack_start (GTK_BOX (vbox), mbar, FALSE, FALSE, 0);

		mi = gtk_menu_item_new_with_label (_("Sodipodi"));
		gtk_widget_show (mi);
		gtk_menu_bar_append (GTK_MENU_BAR (mbar), mi);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), sp_ui_main_menu ());
/*  		gtk_signal_connect (GTK_OBJECT (mi), "button_press_event", GTK_SIGNAL_FUNC (sp_maintoolbox_menu_button_press), NULL); */

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

		if (SP_ACTIVE_DESKTOP) {
			const gchar * tname;
			tname = gtk_type_name (GTK_OBJECT_TYPE (SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP)));
			w = gtk_object_get_data (GTK_OBJECT (t), tname);
			if (w != NULL) {
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
				gtk_object_set_data (GTK_OBJECT (t), "active", t);
			}
		}
		g_signal_connect (G_OBJECT (SODIPODI), "set_eventcontext", G_CALLBACK (sp_update_draw_toolbox), t);

		/* Zoom */
		t = sp_toolbox_zoom_create ();
		gtk_widget_show (t);
		gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
		/* Node */
		t = sp_toolbox_node_create ();
		gtk_widget_show (t);
		gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
	}

/* 	gnome_window_icon_set_from_default (GTK_WINDOW (toolbox)); */
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

static GtkWidget *
sp_toolbox_button_new (GtkWidget *t, int pos, const unsigned char *pxname, GtkSignalFunc handler, GtkTooltips *tt, unsigned char *tip)
{
	GtkWidget *pm, *b;
	unsigned char c[1024];
	int xpos, ypos;

	g_snprintf (c, 1024, "%s/%s", SODIPODI_PIXMAPDIR, pxname);
	pm = gtk_image_new_from_file (c);
	gtk_widget_show (pm);
	b = gtk_button_new ();
	gtk_widget_show (b);
	gtk_container_add (GTK_CONTAINER (b), pm);
	if (handler) gtk_signal_connect (GTK_OBJECT (b), "clicked", handler, NULL);
	xpos = pos % 4;
	ypos = pos / 4;
	gtk_table_attach (GTK_TABLE (t), b, xpos, xpos + 1, ypos, ypos + 1, 0, 0, 0, 0);

	gtk_tooltips_set_tip (tt, b, tip, NULL);

	return b;
}

static GtkWidget *
sp_toolbox_toggle_button_new (const unsigned char *pixmap, unsigned int show)
{
	GtkWidget *pm, *b;

	pm = gtk_image_new_from_file (pixmap);
	gtk_widget_show (pm);
	b = gtk_toggle_button_new ();
	if (show) gtk_widget_show (b);
	gtk_container_add (GTK_CONTAINER (b), pm);

	return b;
}

static GtkWidget *
sp_toolbox_file_create (void)
{
	GtkWidget *t, *tb;
	GtkTooltips *tt;
	SPRepr *repr;

	t = gtk_table_new (2, 4, TRUE);
	gtk_widget_show (t);

	tb = sp_toolbox_new (t, _("File"), "file", SODIPODI_PIXMAPDIR "/toolbox_file.xpm");
	tt = gtk_tooltips_new ();

	sp_toolbox_button_new (t, 0, "file_new.xpm", GTK_SIGNAL_FUNC (sp_file_new), tt, _("Create new SVG document"));
	sp_toolbox_button_new (t, 4, "file_open.xpm", GTK_SIGNAL_FUNC (sp_file_open_dialog), tt, _("Open existing SVG document"));
	sp_toolbox_button_new (t, 1, "file_save.xpm", GTK_SIGNAL_FUNC (sp_file_save), tt, _("Save document"));
	sp_toolbox_button_new (t, 5, "file_save_as.xpm", GTK_SIGNAL_FUNC (sp_file_save_as), tt, _("Save document under new name"));
	sp_toolbox_button_new (t, 2, "file_print.xpm", GTK_SIGNAL_FUNC (sp_file_print), tt, _("Print document"));
	sp_toolbox_button_new (t, 6, "file_print_preview.xpm", GTK_SIGNAL_FUNC (sp_file_print_preview), tt, _("Preview document printout"));
	sp_toolbox_button_new (t, 3, "file_import.xpm", GTK_SIGNAL_FUNC (sp_file_import), tt, _("Import bitmap or SVG image into document"));
	sp_toolbox_button_new (t, 7, "file_export.xpm", GTK_SIGNAL_FUNC (sp_file_export_dialog), tt, _("Export document as PNG bitmap"));

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

	tb = sp_toolbox_new (t, _("Edit"), "edit", SODIPODI_PIXMAPDIR "/toolbox_edit.xpm");
	tt = gtk_tooltips_new ();

	b = sp_toolbox_button_new (t, 0, "edit_undo.xpm", GTK_SIGNAL_FUNC (sp_undo), tt, _("Revert last action"));
	/* fixme: Freehand does not need this anymore, remove if node editing is fixed (Lauris) */
	gtk_object_set_data (GTK_OBJECT (tb), "undo", b);
	b = sp_toolbox_button_new (t, 1, "edit_redo.xpm", GTK_SIGNAL_FUNC (sp_redo), tt, _("Do again undone action"));
	gtk_object_set_data (GTK_OBJECT (tb), "redo", b);
	sp_toolbox_button_new (t, 3, "edit_delete.xpm", GTK_SIGNAL_FUNC (sp_selection_delete), tt, _("Delete selected objects"));
	sp_toolbox_button_new (t, 4, "edit_cut.xpm", GTK_SIGNAL_FUNC (sp_selection_cut), tt, _("Cut selected objects to clipboard"));
	sp_toolbox_button_new (t, 5, "edit_copy.xpm", GTK_SIGNAL_FUNC (sp_selection_copy), tt, _("Copy selected objects to clipboard"));
	sp_toolbox_button_new (t, 6, "edit_paste.xpm", GTK_SIGNAL_FUNC (sp_selection_paste), tt, _("Paste objects from clipboard"));
	sp_toolbox_button_new (t, 7, "edit_duplicate.xpm", GTK_SIGNAL_FUNC (sp_selection_duplicate), tt, _("Duplicate selected objects"));

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
	GtkWidget *t, *tb;
	GtkTooltips *tt;
	SPRepr *repr;

	t = gtk_table_new (3, 4, TRUE);
	gtk_widget_show (t);

	tb = sp_toolbox_new (t, _("Object"), "object", SODIPODI_PIXMAPDIR "/toolbox_object.xpm");
	tt = gtk_tooltips_new ();

	sp_toolbox_button_new (t, 0, "object_stroke.xpm", GTK_SIGNAL_FUNC (sp_object_properties_stroke), tt, _("Stroke settings"));
	sp_toolbox_button_new (t, 1, "object_fill.xpm", GTK_SIGNAL_FUNC (sp_object_properties_fill), tt, _("Fill settings"));
	sp_toolbox_button_new (t, 2, "object_layout.xpm", GTK_SIGNAL_FUNC (sp_object_properties_layout), tt, _("Object size and position"));
	sp_toolbox_button_new (t, 3, "object_font.xpm", GTK_SIGNAL_FUNC (sp_text_edit_dialog), tt, _("Text and font settings"));
	sp_toolbox_button_new (t, 4, "object_align.xpm", GTK_SIGNAL_FUNC (sp_quick_align_dialog), tt, _("Align objects"));
#if 0
	sp_toolbox_button_new (t, 5, "object_trans.xpm", GTK_SIGNAL_FUNC (sp_transformation_dialog_move), tt, _("Object transformations"));
#endif
	sp_toolbox_button_new (t, 7, "object_rotate.xpm", GTK_SIGNAL_FUNC (sp_selection_rotate_90), tt, _("Rotate object 90 deg clockwise"));
	sp_toolbox_button_new (t, 8, "object_reset.xpm", GTK_SIGNAL_FUNC (sp_selection_remove_transform), tt, _("Remove transformations"));
	sp_toolbox_button_new (t, 9, "object_tocurve.xpm", GTK_SIGNAL_FUNC (sp_selected_path_to_curves), tt, _("Convert object to curve"));

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

	tb = sp_toolbox_new (t, _("Selection"), "selection", SODIPODI_PIXMAPDIR "/toolbox_select.xpm");
	tt = gtk_tooltips_new ();

	sp_toolbox_button_new (t, 0, "selection_top.xpm", GTK_SIGNAL_FUNC (sp_selection_raise_to_top), tt, _("Raise selected objects to top"));
	sp_toolbox_button_new (t, 1, "selection_up.xpm", GTK_SIGNAL_FUNC (sp_selection_raise), tt, _("Raise selected objects one level"));
	sp_toolbox_button_new (t, 2, "selection_combine.xpm", GTK_SIGNAL_FUNC (sp_selected_path_combine), tt, _("Combine multiple paths"));
	sp_toolbox_button_new (t, 3, "selection_group.xpm", GTK_SIGNAL_FUNC (sp_selection_group), tt, _("Group selected objects"));
	sp_toolbox_button_new (t, 4, "selection_bot.xpm", GTK_SIGNAL_FUNC (sp_selection_lower_to_bottom), tt, _("Lower selected objects to bottom"));
	sp_toolbox_button_new (t, 5, "selection_down.xpm", GTK_SIGNAL_FUNC (sp_selection_lower), tt, _("Lower selected objects one level"));
	sp_toolbox_button_new (t, 6, "selection_break.xpm", GTK_SIGNAL_FUNC (sp_selected_path_break_apart), tt, _("Break selected path to subpaths"));
	sp_toolbox_button_new (t, 7, "selection_ungroup.xpm", GTK_SIGNAL_FUNC (sp_selection_ungroup), tt, _("Ungroup selected group"));

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
	GtkWidget *tb, *t, *ev, *b;
	SPRepr *repr;
	GtkTooltips *tt;
	
	t = gtk_table_new (2, 4, TRUE);
	gtk_widget_show (t);
	tb = sp_toolbox_new (t, _("Draw"), "draw", SODIPODI_PIXMAPDIR "/toolbox_draw.xpm");
	
	tt = gtk_tooltips_new ();
	
	/* Select */
	b = sp_toolbox_toggle_button_new (SODIPODI_PIXMAPDIR "/draw_select.xpm", TRUE);
	gtk_signal_connect (GTK_OBJECT (b), "released", GTK_SIGNAL_FUNC (sp_event_context_set_select), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 0, 1, 0, 1, 0, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (tb), "SPSelectContext", b);
	gtk_tooltips_set_tip (tt, b, _("Select tool - select and transform objects"), NULL);

	/* Node */
	b = sp_toolbox_toggle_button_new (SODIPODI_PIXMAPDIR "/draw_node.xpm", TRUE);
	gtk_signal_connect (GTK_OBJECT (b), "released", GTK_SIGNAL_FUNC (sp_event_context_set_node_edit), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 1, 2, 0, 1, 0, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (tb), "SPNodeContext", b);
	gtk_tooltips_set_tip (tt, b, _("Node tool - modify different aspects of existing objects"), NULL);

#if 1
	/* Object */
	b = sp_menu_button_new ();
	gtk_widget_show (b);
	/* START COMPONENTS */
	/* Rect */
	ev = gtk_event_box_new_with_image_file_and_tooltips(SODIPODI_PIXMAPDIR "/draw_rect.xpm",
							    _("Rectangle tool - create rectangles and squares with optional rounded corners"),
							    NULL);
	gtk_widget_show (ev);
	sp_menu_button_append_child (SP_MENU_BUTTON (b), ev, GUINT_TO_POINTER (SP_TOOLBOX_DRAW_RECT));
	/* Arc */
	ev = gtk_event_box_new_with_image_file_and_tooltips(SODIPODI_PIXMAPDIR "/draw_arc.xpm",
							    _("Arc tool - create circles, ellipses and arcs"),
							    NULL);
	gtk_widget_show (ev);
	sp_menu_button_append_child (SP_MENU_BUTTON (b), ev, GUINT_TO_POINTER (SP_TOOLBOX_DRAW_ARC));
	/* Star */
	ev = gtk_event_box_new_with_image_file_and_tooltips(SODIPODI_PIXMAPDIR "/draw_star.xpm",
							    _("Star tool - create stars and polygons"),
							    NULL);
	gtk_widget_show (ev);
	sp_menu_button_append_child (SP_MENU_BUTTON (b), ev, GUINT_TO_POINTER (SP_TOOLBOX_DRAW_STAR));
	/* Spiral */
	ev = gtk_event_box_new_with_image_file_and_tooltips(SODIPODI_PIXMAPDIR "/draw_spiral.xpm",
							    _("Spiral tool - create spirals"),
							    NULL);
	gtk_widget_show (ev);
	sp_menu_button_append_child (SP_MENU_BUTTON (b), ev, GUINT_TO_POINTER (SP_TOOLBOX_DRAW_SPIRAL));
	/* END COMPONENTS */
	gtk_signal_connect (GTK_OBJECT (b), "activate-item", GTK_SIGNAL_FUNC (sp_toolbox_draw_set_object), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 2, 3, 0, 1, 0, 0, 0, 0);
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
	ev = gtk_event_box_new_with_image_file_and_tooltips (SODIPODI_PIXMAPDIR "/draw_freehand.xpm",
							     _("Pencil tool - draw freehand lines and straight segments"),
							     NULL);
	gtk_widget_show (ev);
	sp_menu_button_append_child (SP_MENU_BUTTON (b), ev, GUINT_TO_POINTER (SP_TOOLBOX_DRAW_FREEHAND));
	/* Pen */
	ev = gtk_event_box_new_with_image_file_and_tooltips (SODIPODI_PIXMAPDIR "/draw_pen.xpm",
							     _("Pen tool - draw exactly positioned curved lines"),
							     NULL);
	gtk_widget_show (ev);
	sp_menu_button_append_child (SP_MENU_BUTTON (b), ev, GUINT_TO_POINTER (SP_TOOLBOX_DRAW_PEN));
	/* Dynahand */
	ev =  gtk_event_box_new_with_image_file_and_tooltips (SODIPODI_PIXMAPDIR "/draw_dynahand.xpm",
							      _("Calligraphic tool - draw calligraphic lines"),
							      NULL);
	gtk_widget_show (ev);
	sp_menu_button_append_child (SP_MENU_BUTTON (b), ev, GUINT_TO_POINTER (SP_TOOLBOX_DRAW_DYNAHAND));
	/* END COMPONENTS */
	gtk_signal_connect (GTK_OBJECT (b), "activate-item", GTK_SIGNAL_FUNC (sp_toolbox_draw_set_object), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 3, 4, 0, 1, 0, 0, 0, 0);
	/* fixme: */
	gtk_object_set_data (GTK_OBJECT (tb), "SPPencilContext", b);
	gtk_object_set_data (GTK_OBJECT (tb), "SPPenContext", b);
	gtk_object_set_data (GTK_OBJECT (tb), "SPDynaDrawContext", b);
#endif

	/* Text */
	b = sp_toolbox_toggle_button_new (SODIPODI_PIXMAPDIR "/draw_text.xpm", TRUE);
	gtk_signal_connect (GTK_OBJECT (b), "released", GTK_SIGNAL_FUNC (sp_event_context_set_text), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 0, 1, 1, 2, 0, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (tb), "SPTextContext", b);
	gtk_tooltips_set_tip (tt, b, _("Text tool - create editable text objects"), NULL);

	/* Zoom */
	b = sp_toolbox_toggle_button_new (SODIPODI_PIXMAPDIR "/draw_zoom.xpm", TRUE);
	gtk_signal_connect (GTK_OBJECT (b), "released", GTK_SIGNAL_FUNC (sp_event_context_set_zoom), NULL);
	gtk_table_attach (GTK_TABLE (t), b, 1, 2, 1, 2, 0, 0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (tb), "SPZoomContext", b);
	gtk_tooltips_set_tip (tt, b, _("Zoom tool - zoom into choosen area"), NULL);
	
	repr = sodipodi_get_repr (SODIPODI, "toolboxes.draw");
	if (repr) {
		gint state;
		state = sp_repr_get_int_attribute (repr, "state", 0);
		sp_toolbox_set_state (SP_TOOLBOX (tb), state);
	}

	gtk_signal_connect (GTK_OBJECT (tb), "set_state", GTK_SIGNAL_FUNC (sp_toolbox_set_state_handler), repr);

	return tb;
}

static GtkWidget *
sp_toolbox_zoom_create (void)
{
	GtkWidget *t, *tb;
	GtkTooltips *tt;
	SPRepr *repr;

	t = gtk_table_new (2, 4, TRUE);
	gtk_widget_show (t);

	tb = sp_toolbox_new (t, _("Zoom"), "zoom", SODIPODI_PIXMAPDIR "/toolbox_zoom.xpm");
	tt = gtk_tooltips_new ();

	sp_toolbox_button_new (t, 0, "zoom_in.xpm", GTK_SIGNAL_FUNC (sp_zoom_in), tt, _("Zoom in drawing"));
	sp_toolbox_button_new (t, 1, "zoom_2_to_1.xpm", GTK_SIGNAL_FUNC (sp_zoom_2_to_1), tt, _("Set zoom factor to 2:1"));
	sp_toolbox_button_new (t, 2, "zoom_1_to_1.xpm", GTK_SIGNAL_FUNC (sp_zoom_1_to_1), tt, _("Set zoom factor to 1:1"));
	sp_toolbox_button_new (t, 3, "zoom_1_to_2.xpm", GTK_SIGNAL_FUNC (sp_zoom_1_to_2), tt, _("Set zoom factor to 1:2"));
	sp_toolbox_button_new (t, 4, "zoom_out.xpm", GTK_SIGNAL_FUNC (sp_zoom_out), tt, _("Zoom out drawing"));
	sp_toolbox_button_new (t, 5, "zoom_page.xpm", GTK_SIGNAL_FUNC (sp_zoom_page), tt, _("Fit the whole page into window"));
	sp_toolbox_button_new (t, 6, "zoom_draw.xpm", GTK_SIGNAL_FUNC (sp_zoom_drawing), tt, _("Fit the whole drawing into window"));
	sp_toolbox_button_new (t, 7, "zoom_select.xpm", GTK_SIGNAL_FUNC (sp_zoom_selection), tt, _("Fit the whole selection into window"));

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

	tb = sp_toolbox_new (t, _("Nodes"), "node", SODIPODI_PIXMAPDIR "/toolbox_node.xpm");
	tt = gtk_tooltips_new ();

	sp_toolbox_button_new (t, 0, "node_insert.xpm", GTK_SIGNAL_FUNC (sp_node_path_edit_add), tt, _("Insert new nodes into selected segments"));
	sp_toolbox_button_new (t, 1, "node_break.xpm", GTK_SIGNAL_FUNC (sp_node_path_edit_break), tt, _("Break line at selected nodes"));
	sp_toolbox_button_new (t, 2, "node_cusp.xpm", GTK_SIGNAL_FUNC (sp_node_path_edit_cusp), tt, _("Make selected nodes corner"));
	sp_toolbox_button_new (t, 3, "node_line.xpm", GTK_SIGNAL_FUNC (sp_node_path_edit_toline), tt, _("Make selected segments lines"));
	sp_toolbox_button_new (t, 4, "node_delete.xpm", GTK_SIGNAL_FUNC (sp_node_path_edit_delete), tt, _("Delete selected nodes"));
	sp_toolbox_button_new (t, 5, "node_join.xpm", GTK_SIGNAL_FUNC (sp_node_path_edit_join), tt, _("Join lines at selected nodes"));
	sp_toolbox_button_new (t, 6, "node_smooth.xpm", GTK_SIGNAL_FUNC (sp_node_path_edit_smooth), tt, _("Make selected nodes smooth"));
	sp_toolbox_button_new (t, 7, "node_curve.xpm", GTK_SIGNAL_FUNC (sp_node_path_edit_tocurve), tt, _("Make selected segments curves"));

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

static GtkWidget *
gtk_event_box_new_with_image_file_and_tooltips(const gchar   *image_file,
					       const gchar   *tip_text,
					       const gchar   *tip_private)
{
	GtkWidget * pm;
	GtkWidget * ev;
	GtkTooltips *tt;
	
	pm = gtk_image_new_from_file (image_file);
	tt = gtk_tooltips_new ();
	ev = gtk_event_box_new();
	gtk_tooltips_set_tip (tt, ev, tip_text, tip_private);
	gtk_container_add(GTK_CONTAINER(ev), pm);
	gtk_widget_show(pm);

#if 0
	/* arrow on the button is redraw by the parent. */
	gtk_signal_connect_after(GTK_OBJECT(ev),
				 "expose_event",
				 GTK_SIGNAL_FUNC(gtk_event_box_force_draw_parent),
				 NULL);
#endif		   
			   
	return ev;
}

static guint
gtk_event_box_force_draw_parent(GtkWidget * widget, 
				GdkEventExpose *event,
				gpointer user_data)
{
	/* FIXME: I should calculate the area necessarily redrawn. */
	if (widget->parent)
		gtk_widget_draw(widget->parent, NULL);
	return FALSE;
}

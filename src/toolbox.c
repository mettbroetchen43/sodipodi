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

#include <gnome.h>
#include <libgnomeui/gnome-pixmap.h>
#include <libgnomeui/gnome-stock.h>
#include <glade/glade.h>
#include "widgets/sp-toolbox.h"
#include "sodipodi-private.h"
#include "toolbox.h"
#include "sodipodi.h"
#include "event-broker.h"
#include "dialogs/transformation.h"
#include "zoom-context.h"
#include "selection.h"
#include "sp-item-transform.h"
#include "desktop-handles.h"

#if 0
/* needed for draw toolbox update */
#include "select-context.h"
#include "node-context.h"
#include "rect-context.h"
#include "ellipse-context.h"
#include "draw-context.h"
#include "text-context.h"
#include "zoom-context.h"
#endif

GtkWidget * sp_toolbox_create (GladeXML * xml, const gchar * widgetname, const gchar * name, const gchar * internalname, const gchar * pxname);

static gint sp_toolbox_set_state_handler (SPToolBox * t, guint state, gpointer data);
static void sp_update_draw_toolbox (Sodipodi * sodipodi, SPEventContext * eventcontext, gpointer data);

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

		/* Reference our sodipodi engine */
		sodipodi_ref ();

		vbox = glade_xml_get_widget (toolbox_xml, "main_vbox");

		/* File */
		xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "file_table");
		t = sp_toolbox_create (xml, "file_table", _("File"), "file", "toolbox_file.xpm");
		gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
		/* Edit */
		xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "edit_table");
		t = sp_toolbox_create (xml, "edit_table", _("Edit"), "edit", "toolbox_edit.xpm");
		gtk_box_pack_start (GTK_BOX (vbox), t, FALSE, FALSE, 0);
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
		w = glade_xml_get_widget (xml, "draw_ellipse");
		gtk_object_set_data (GTK_OBJECT (t), "SPEllipseContext", w);
		w = glade_xml_get_widget (xml, "draw_freehand");
		gtk_object_set_data (GTK_OBJECT (t), "SPDrawContext", w);
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

	return FALSE;
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
	gpointer active, new;

	active = gtk_object_get_data (GTK_OBJECT (data), "active");

	if (eventcontext != NULL) {
		new = gtk_object_get_data (GTK_OBJECT (data), gtk_type_name (GTK_OBJECT_TYPE (eventcontext)));
	} else {
		new = NULL;
	}

	if (new != active) {
		if (active) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (active), FALSE);
		if (new) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (new), TRUE);
		gtk_object_set_data (GTK_OBJECT (data), "active", new);
	}
}

#if 0
static GladeXML  * main_toolbox_xml = NULL;
static GtkWidget * main_toolbox_dialog = NULL;
static GtkWidget * main_vbox = NULL;
static GladeXML  * quit_xml = NULL;
static GtkWidget * quit_dialog = NULL;

static GtkWidget * draw_select = NULL;
static GtkWidget * draw_node = NULL;
static GtkWidget * draw_zoom = NULL;
static GtkWidget * draw_text = NULL;
static GtkWidget * draw_rect = NULL;
static GtkWidget * draw_ellipse = NULL;
static GtkWidget * draw_freehand = NULL;

static GtkWidget * flip_button = NULL;
static GtkWidget * fh_pixmap = NULL ;
static GtkWidget * fv_pixmap = NULL;
GtkWidget * zoom_any = NULL;

typedef enum {
  FLIP_HOR,
  FLIP_VER,
} SPObjectFlipMode;

SPObjectFlipMode object_flip_mode = FLIP_HOR;

typedef struct _SPToolBox SPToolBox;

struct _SPToolBox {
	const gchar * name;
	const gchar * internalname;
	const gchar * pixmapname;
	SPToolBoxState state;
#if 0
	gboolean visible;     
	gboolean standalone;
	gboolean init ;
	gboolean inmain ;
	char* dialog_name;
	char* icon;
#endif
	GtkWidget * MainVBox;
	GtkWidget * DialogVBox;
	GtkWidget * Dialog;
	GtkWidget * BoxTable;
	GtkWidget * Hide;
	GtkWidget * Seperate;
	GtkWidget * Close;
	GladeXML * DialogXML;
};

void sp_toolbox_init (SPToolBox * toolbox);
void initialize_toolbox (SPToolBox * toolbox);
void toolbox_toggle_main (GtkWidget * widget, SPToolBox * toolbox);
void toolbox_close (GtkWidget * widget, SPToolBox * toolbox);
void maintoolbox_destroy (GtkWidget * widget, SPToolBox * toolbox);
void toolbox_delete (GtkWidget * widget, SPToolBox * toolbox);
void toolbox_toggle_seperate (GtkWidget * widget, SPToolBox * toolbox);
//void sp_select_draw_mode (GtkWidget * widget);
void quit_yep (void);
void quit_nope (void);
void sp_toolbox_create_widgets (SPToolBox * toolbox);
void sp_toolbox_expose (SPToolBox * toolbox);
//void create_draw_box (void);
void object_flip (GtkWidget * widget, GdkEventButton * event);
void object_rotate_90 (void);

static void sp_update_draw_toolbox (Sodipodi * sodipodi, SPEventContext * eventcontext, gpointer data);

// our toolboxen
static SPToolBox draw_box, zoom_box, node_box, select_box, file_box, edit_box, object_box;

SPToolBox * sp_toolbox_new (const gchar * name, const gchar * internalname, const gchar * widgetname, const gchar * pixmapname);

void
sp_maintoolbox_create (void)
{
	if (main_toolbox_dialog == NULL) {
		SPToolBox * t;

	        /* Crete main toolbox */
		main_toolbox_xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "maintoolbox");
		g_return_if_fail (main_toolbox_xml != NULL);
		glade_xml_signal_autoconnect (main_toolbox_xml);
		main_toolbox_dialog = glade_xml_get_widget (main_toolbox_xml, "maintoolbox");
		g_return_if_fail (main_toolbox_dialog != NULL);

		/* Reference our sodipodi engine */
		sodipodi_ref ();

		main_vbox = glade_xml_get_widget (main_toolbox_xml, "main_vbox");

		/* File toolbox */
		t = sp_toolbox_create (_("File"), "file", "file_table", "toolbox_file.xpm");
		// edit toolbox
		edit_box.DialogXML = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "edit_table");
       		glade_xml_signal_autoconnect (edit_box.DialogXML);

		edit_box.visible = TRUE;
		edit_box.standalone = FALSE;
		edit_box.inmain = TRUE;
		edit_box.dialog_name = _("Edit");
		edit_box.icon = SODIPODI_GLADEDIR "/toolbox_edit.xpm";

		sp_toolbox_create_widgets (&edit_box);
		edit_box.BoxTable = glade_xml_get_widget (edit_box.DialogXML, "edit_table");
		sp_toolbox_expose (&edit_box);

		// objects toolbox
		object_box.DialogXML = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "object_table");
		glade_xml_signal_autoconnect (object_box.DialogXML);

		object_box.visible = TRUE;
		object_box.standalone = FALSE;
		object_box.inmain = TRUE;
		object_box.dialog_name = _("Object");
		object_box.icon = SODIPODI_GLADEDIR "/toolbox_object.xpm";

		flip_button = glade_xml_get_widget (object_box.DialogXML, "flip_button");
		fh_pixmap = glade_xml_get_widget (object_box.DialogXML, "fh_pixmap");
		fv_pixmap = glade_xml_get_widget (object_box.DialogXML, "fv_pixmap");
		object_flip_mode = FLIP_HOR;
		gtk_widget_show (fh_pixmap);

		sp_toolbox_create_widgets (&object_box);
		object_box.BoxTable = glade_xml_get_widget (object_box.DialogXML, "object_table");
		sp_toolbox_expose (&object_box);
		// select toolbox
		select_box.DialogXML = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "select_table");
		glade_xml_signal_autoconnect (select_box.DialogXML);

		select_box.visible = FALSE;
		select_box.standalone = FALSE;
		select_box.inmain = TRUE;
		select_box.dialog_name = _("Selection");
		select_box.icon = SODIPODI_GLADEDIR "/toolbox_select.xpm";

		sp_toolbox_create_widgets (&select_box);
		select_box.BoxTable = glade_xml_get_widget (select_box.DialogXML, "select_table");
		sp_toolbox_expose (&select_box);
		//draw_box
		xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "draw_table");
		glade_xml_signal_autoconnect (xml);

 		draw_box.visible = TRUE;
		draw_box.standalone = FALSE;
		draw_box.inmain = TRUE;
		draw_box.dialog_name = _("Draw context");
		draw_box.icon = SODIPODI_GLADEDIR "/toolbox_draw.xpm";
		sp_toolbox_create_widgets (&draw_box);
		draw_box.BoxTable = glade_xml_get_widget (xml, "draw_table");
		draw_select = glade_xml_get_widget (xml, "draw_select");
		draw_node = glade_xml_get_widget (xml, "draw_node");
		draw_zoom = glade_xml_get_widget (xml, "draw_zoom");
		draw_text = glade_xml_get_widget (xml, "draw_text");
		draw_rect = glade_xml_get_widget (xml, "draw_rect");
		draw_ellipse = glade_xml_get_widget (xml, "draw_ellipse");
		draw_freehand = glade_xml_get_widget (xml, "draw_freehand");
		gtk_toggle_button_set_active ((GtkToggleButton *) draw_select, TRUE);
		sp_toolbox_expose (&draw_box);

		gtk_signal_connect_while_alive (GTK_OBJECT (SODIPODI), "set_eventcontext",
						GTK_SIGNAL_FUNC (sp_update_draw_toolbox), NULL,
						GTK_OBJECT (draw_box.BoxTable));

		// zoom toolbox
		zoom_box.DialogXML = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "zoom_table");
		glade_xml_signal_autoconnect (zoom_box.DialogXML);

		zoom_box.visible = FALSE;
		zoom_box.standalone = FALSE;
		zoom_box.inmain = TRUE;
		zoom_box.dialog_name = _("Zoom");
		zoom_box.icon = SODIPODI_GLADEDIR "/toolbox_zoom.xpm";

		sp_toolbox_create_widgets (&zoom_box);
		zoom_box.BoxTable = glade_xml_get_widget (zoom_box.DialogXML, "zoom_table");
		zoom_any = glade_xml_get_widget (zoom_box.DialogXML, "zoom_any_entry");
		sp_toolbox_expose (&zoom_box);
		// node toolbox
		node_box.DialogXML = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "node_table");
		glade_xml_signal_autoconnect (node_box.DialogXML);

		node_box.visible = FALSE;
		node_box.standalone = FALSE;
		node_box.inmain = TRUE;
		node_box.dialog_name = _("Node edit");
		node_box.icon = SODIPODI_GLADEDIR "/toolbox_node.xpm";

		sp_toolbox_create_widgets (&node_box);
		node_box.BoxTable = glade_xml_get_widget (node_box.DialogXML, "node_table");
		sp_toolbox_expose (&node_box);


		gtk_widget_show (main_toolbox_dialog);
	}
}

SPToolBox *
sp_toolbox_new (const gchar * name, const gchar * internalname, const gchar * widgetname, const gchar * pixmapname)
{
	SPToolBox * t;
	char c[256];

	t = g_new (SPToolBox, 1);

	t->name = name;
	t->internalname = internalname;
	t->pixmapname = pixmapname;

	t->DialogXML = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", widgetname);
	glade_xml_signal_autoconnect (t->DialogXML);

	g_snprintf (c, 256, "/toolbox/%s/state", internalname);
	t->state = sodipodi_get_key_as_int (SODIPODI, c);

#if 0
	file_box.visible = TRUE;
	file_box.standalone = FALSE;
	file_box.inmain = TRUE;
	file_box.dialog_name = _("File");
	t->icon = SODIPODI_GLADEDIR "/toolbox_file.xpm";
#endif

	sp_toolbox_create_widgets (t);
	t->BoxTable = glade_xml_get_widget (t->DialogXML, widgetname);
	sp_toolbox_expose (t);

	return t;
}

void sp_toolbox_create_widgets (SPToolBox * toolbox) {
	GtkWidget * vbox, * hbox;
	GtkWidget * gnpixmap;
	GtkWidget * sep, * label;
	gchar c[256];

	/* Dialog window */
	toolbox->Dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title ((GtkWindow *) toolbox->Dialog, toolbox->name);
	gtk_window_set_policy ((GtkWindow *) toolbox->Dialog, FALSE, FALSE, FALSE);
	g_snprintf (c, 256, "toolbox_%s", toolbox->internalname);
	gtk_window_set_wmclass (GTK_WINDOW (toolbox->Dialog), c, "Sodipodi");

	/* Main vbox */
	toolbox->MainVBox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start ((GtkBox *) main_vbox, toolbox->MainVBox, TRUE, TRUE, 0);
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start ((GtkBox *) toolbox->MainVBox, hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);

	/* Seperate button */
	toolbox->Seperate = gtk_button_new ();
	gtk_button_set_relief ((GtkButton *) toolbox->Seperate, GTK_RELIEF_NONE);
	gtk_box_pack_end ((GtkBox *) hbox, toolbox->Seperate, FALSE, FALSE, 0);
	gtk_widget_show (toolbox->Seperate);
	gnpixmap = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/seperate_tool.xpm");
	gtk_container_add ((GtkContainer *) toolbox->Seperate, gnpixmap);
	gtk_widget_show (gnpixmap);

	/* Hide button */
	toolbox->Hide = gtk_button_new  ();
	gtk_button_set_relief ((GtkButton *) toolbox->Hide, GTK_RELIEF_NONE);
	gtk_box_pack_start ((GtkBox *) hbox, toolbox->Hide, TRUE, TRUE, 0);
	gtk_widget_show (toolbox->Hide);

	hbox = gtk_hbox_new (FALSE,0);
	gtk_container_add ((GtkContainer *) toolbox->Hide, hbox);
	gtk_widget_show (hbox);
	gnpixmap = gnome_pixmap_new_from_file (toolbox->icon);
	gtk_box_pack_start ((GtkBox *) hbox, gnpixmap, FALSE, FALSE, 0);
	gtk_widget_show (gnpixmap);
	label = gtk_label_new (toolbox->dialog_name);
	gtk_box_pack_start ((GtkBox *) hbox, label, FALSE, FALSE, 0);
	gtk_widget_show (label);  
	sep = gtk_hseparator_new ();
	gtk_box_pack_start ((GtkBox *) hbox, sep, TRUE, TRUE, 0);
	gtk_widget_show (sep);  
	toolbox->Hide->allocation.height = 12;
	gtk_object_set(GTK_OBJECT(toolbox->Hide),  "GtkWidget::height", (gulong) 16, NULL);

	// dialog vbox
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add ((GtkContainer *) toolbox->Dialog, vbox);
	toolbox->DialogVBox = gtk_vbox_new (FALSE, 0);
	gtk_container_add ((GtkContainer *) vbox, toolbox->DialogVBox);
	gtk_widget_show (vbox);
	gtk_widget_show (toolbox->DialogVBox);

	// close button
	toolbox->Close = gnome_stock_button ("Button_Close");
	gtk_container_add ((GtkContainer *) vbox, toolbox->Close);
	gtk_widget_show (toolbox->Close);

	// signals
	gtk_signal_connect (GTK_OBJECT (toolbox->Hide), "clicked", GTK_SIGNAL_FUNC (toolbox_toggle_main), toolbox);
	gtk_signal_connect (GTK_OBJECT (toolbox->Seperate), "clicked", GTK_SIGNAL_FUNC (toolbox_toggle_seperate), toolbox);
	gtk_signal_connect (GTK_OBJECT (toolbox->Close), "clicked", GTK_SIGNAL_FUNC (toolbox_toggle_seperate), toolbox);
	gtk_signal_connect (GTK_OBJECT (toolbox->Dialog), "delete_event", GTK_SIGNAL_FUNC (toolbox_delete), toolbox);
}

void sp_toolbox_expose (SPToolBox * toolbox) {
	if (toolbox->inmain) gtk_widget_show (toolbox->MainVBox);
	if (toolbox->standalone) {
		gtk_box_pack_start ((GtkBox *) toolbox->DialogVBox, toolbox->BoxTable, TRUE, TRUE, 0);
		if (toolbox->visible) gtk_widget_show (toolbox->Dialog);
	} else {
		gtk_box_pack_end ((GtkBox *) toolbox->MainVBox, toolbox->BoxTable, TRUE, TRUE, 0);
		if (toolbox->visible) gtk_widget_show (toolbox->BoxTable);
	}
}


gint
sp_maintoolbox_close (GtkWidget * widget, GdkEventAny * event, gpointer data)
{
	sodipodi_unref ();

	return FALSE;
#if 0
  // we need save changes and stuff
  	if (!GTK_IS_WIDGET (quit_dialog)) {
  		quit_xml = glade_xml_new (SODIPODI_GLADEDIR "/toolbox.glade", "quit");
  		if (quit_xml == NULL) return;
		glade_xml_signal_autoconnect (quit_xml);
		quit_dialog = glade_xml_get_widget (quit_xml, "quit");
		if (quit_dialog == NULL) return;
	}else {
	  if (!GTK_WIDGET_VISIBLE (quit_dialog)) gtk_widget_show (quit_dialog);
	}
#endif
}

void quit_yep () {	gtk_main_quit();  }

void quit_nope () {  if (GTK_WIDGET_VISIBLE (quit_dialog)) gtk_widget_hide (quit_dialog); }

void toolbox_toggle_main (GtkWidget * widget, SPToolBox * toolbox) {
  if (!GTK_IS_WIDGET (toolbox->BoxTable)) g_print("table weg  ");
  if ((*toolbox).standalone) {
      (*toolbox).visible = TRUE;
      (*toolbox).standalone = FALSE;
      if (GTK_WIDGET_VISIBLE (toolbox->Dialog)) gtk_widget_hide (toolbox->Dialog);
      gtk_widget_reparent (toolbox->BoxTable, toolbox->MainVBox);
      if (!GTK_WIDGET_VISIBLE (toolbox->BoxTable)) gtk_widget_show (toolbox->BoxTable);
    }  else {
    if ((*toolbox).visible) {
      (*toolbox).visible = FALSE;
      if (GTK_WIDGET_VISIBLE (toolbox->BoxTable)) gtk_widget_hide (toolbox->BoxTable);
    } else {
      (*toolbox).visible = TRUE;
      if (!GTK_WIDGET_VISIBLE (toolbox->BoxTable)) gtk_widget_show (toolbox->BoxTable);
    }      
  }
}

void toolbox_close (GtkWidget * widget, SPToolBox * toolbox) {
  (*toolbox).visible = FALSE;
  if (GTK_WIDGET_VISIBLE (toolbox->Dialog)) gtk_widget_hide (toolbox->Dialog);
}

void toolbox_delete (GtkWidget * widget, SPToolBox * toolbox) {
  gtk_widget_hide (widget);
  (*toolbox).visible = FALSE;
}

void maintoolbox_destroy (GtkWidget * widget, SPToolBox * toolbox) {
  g_print ("goodbye !! ... ");
}

void toolbox_toggle_seperate (GtkWidget * widget, SPToolBox * toolbox) {
  if ((*toolbox).standalone) {
    if (GTK_WIDGET_VISIBLE (toolbox->Dialog)) {
      (*toolbox).visible = FALSE;
      gtk_widget_hide (toolbox->Dialog);
    } else {
      (*toolbox).visible = TRUE;
      if (!GTK_WIDGET_VISIBLE (toolbox->Dialog)) gtk_widget_show (toolbox->Dialog);
    }
  } else {
    (*toolbox).standalone = TRUE;
    (*toolbox).visible = TRUE;
    gtk_widget_reparent (toolbox->BoxTable, toolbox->DialogVBox);
    if (!GTK_WIDGET_VISIBLE (toolbox->BoxTable)) gtk_widget_show (toolbox->BoxTable);
    if (!GTK_WIDGET_VISIBLE (toolbox->Dialog)) gtk_widget_show (toolbox->Dialog);
  }
    
  g_print ("seperate ");
}

#endif

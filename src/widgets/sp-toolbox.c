#define __SP_TOOLBOX_C__

/*
 * Toolbox widget
 *
 * Authors:
 *   Frank Felfe  <innerspace@iname.com>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000-2002 Authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkarrow.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtktogglebutton.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnomeui/gnome-pixmap.h>
#include <libgnomeui/gnome-window-icon.h>
#include "sp-toolbox.h"

enum {
	SET_STATE,
	LAST_SIGNAL
};

static void sp_toolbox_class_init (SPToolBoxClass * klass);
static void sp_toolbox_init (SPToolBox * toolbox);
static void sp_toolbox_destroy (GtkObject * object);

static void sp_toolbox_size_request (GtkWidget * widget, GtkRequisition * requisition);

static void sp_toolbox_hide (GtkButton * button, gpointer data);
//static void sp_toolbox_separate (GtkButton * button, gpointer data);
static void sp_toolbox_separate (GtkToggleButton * button, gpointer data);
static void sp_toolbox_close (GtkButton * button, gpointer data);
static gint sp_toolbox_delete (GtkWidget * widget, GdkEventAny * event, gpointer data);

static GtkVBoxClass * parent_class;
static guint toolbox_signals[LAST_SIGNAL] = {0};

GtkType
sp_toolbox_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPToolBox",
			sizeof (SPToolBox),
			sizeof (SPToolBoxClass),
			(GtkClassInitFunc) sp_toolbox_class_init,
			(GtkObjectInitFunc) sp_toolbox_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_VBOX, &info);
	}
	return type;
}

static void
sp_toolbox_class_init (SPToolBoxClass * klass)
{
	GtkObjectClass * object_class;
	GtkWidgetClass * widget_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	parent_class = gtk_type_class (gtk_vbox_get_type ());

	toolbox_signals[SET_STATE] = gtk_signal_new ("set_state",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPToolBoxClass, set_state),
		gtk_marshal_INT__INT,
		GTK_TYPE_BOOL, 1,
		GTK_TYPE_ENUM);

	gtk_object_class_add_signals (object_class, toolbox_signals, LAST_SIGNAL);

	object_class->destroy = sp_toolbox_destroy;

	widget_class->size_request = sp_toolbox_size_request;
}

static void
sp_toolbox_init (SPToolBox * toolbox)
{
	toolbox->state = 1;//SP_TOOLBOX_VISIBLE;

	toolbox->contents = NULL;
	toolbox->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	toolbox->windowvbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (toolbox->windowvbox);
	gtk_container_add (GTK_CONTAINER (toolbox->window), toolbox->windowvbox);

	toolbox->width = 0;
	toolbox->name = NULL;
	toolbox->internalname = NULL;
}

static void
sp_toolbox_destroy (GtkObject * object)
{
	SPToolBox * toolbox;

	toolbox = (SPToolBox *) object;

	if (toolbox->internalname) {
		g_free (toolbox->internalname);
		toolbox->internalname = NULL;
	}
	if (toolbox->name) {
		g_free (toolbox->name);
		toolbox->name = NULL;
	}
	if (toolbox->window) {
		gtk_widget_destroy (toolbox->window);
		toolbox->window = NULL;
		toolbox->windowvbox = NULL;
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_toolbox_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	SPToolBox * t;
	GtkRequisition r;

	t = (SPToolBox *) widget;

	if (((GtkWidgetClass *) (parent_class))->size_request)
		(* ((GtkWidgetClass *) (parent_class))->size_request) (widget, &r);

	t->width = MAX (t->width, r.width);
	requisition->width = t->width;
	requisition->height = r.height;
}

void
sp_toolbox_set_state (SPToolBox * toolbox, guint state)
{
	gboolean consumed;

	g_return_if_fail (toolbox != NULL);
	g_return_if_fail (SP_IS_TOOLBOX (toolbox));

	if (state == toolbox->state) return;

	gtk_object_ref (GTK_OBJECT (toolbox));
	consumed = FALSE;
	gtk_signal_emit (GTK_OBJECT (toolbox), toolbox_signals[SET_STATE], state, &consumed);
	if (!consumed) {
		if ((state & SP_TOOLBOX_STANDALONE) && (!(toolbox->state & SP_TOOLBOX_STANDALONE))) {
			gtk_widget_reparent (toolbox->contents, toolbox->windowvbox);
		} else if ((!(state & SP_TOOLBOX_STANDALONE)) && (toolbox->state & SP_TOOLBOX_STANDALONE)) {
			gtk_widget_reparent (toolbox->contents, GTK_WIDGET (toolbox));
			gtk_widget_hide (toolbox->window);
		}
		if (state & SP_TOOLBOX_VISIBLE) {
			gtk_widget_show (toolbox->contents);
			if (state & SP_TOOLBOX_STANDALONE) gtk_widget_show (toolbox->window);
		} else {
			gtk_widget_hide (toolbox->contents);
			if (state & SP_TOOLBOX_STANDALONE) gtk_widget_hide (toolbox->window);
		}
		if ((state & SP_TOOLBOX_VISIBLE) && (!(state & SP_TOOLBOX_STANDALONE))) {
			gtk_arrow_set (GTK_ARROW (toolbox->arrow), GTK_ARROW_DOWN, GTK_SHADOW_OUT);
		} else {
			gtk_arrow_set (GTK_ARROW (toolbox->arrow), GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
		}
		gtk_signal_handler_block_by_func (GTK_OBJECT (toolbox->standalonetoggle), sp_toolbox_separate, toolbox);
		if (state & SP_TOOLBOX_STANDALONE) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbox->standalonetoggle), TRUE);
			gtk_button_set_relief (GTK_BUTTON (toolbox->standalonetoggle), GTK_RELIEF_NORMAL);
		} else {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbox->standalonetoggle), FALSE);
			gtk_button_set_relief (GTK_BUTTON (toolbox->standalonetoggle), GTK_RELIEF_NONE);
		}
		gtk_signal_handler_unblock_by_func (GTK_OBJECT (toolbox->standalonetoggle), sp_toolbox_separate, toolbox);
		toolbox->state = state;

	}

	gtk_object_unref (GTK_OBJECT (toolbox));
}

GtkWidget *
sp_toolbox_new (GtkWidget * contents, const gchar * name, const gchar * internalname, const gchar * pixmapname)
{
	SPToolBox * t;
	GtkWidget * hbox, * hbb, * b, * w;
	gchar c[256];

	g_return_val_if_fail (contents != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (contents), NULL);
	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (internalname != NULL, NULL);
	g_return_val_if_fail (pixmapname != NULL, NULL);

	t = gtk_type_new (SP_TYPE_TOOLBOX);

	t->contents = contents;
	t->name = g_strdup (name);
	t->internalname = g_strdup (internalname);

	/* Main widget */
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (t), hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);
	/* Hide button */
	b = gtk_button_new  ();
	gtk_button_set_relief (GTK_BUTTON (b), GTK_RELIEF_NONE);
	gtk_box_pack_start (GTK_BOX (hbox), b, TRUE, TRUE, 0);
	gtk_widget_show (b);
	hbb = gtk_hbox_new (FALSE,0);
	gtk_container_add (GTK_CONTAINER (b), hbb);
	gtk_widget_show (hbb);
	w = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
	gtk_box_pack_start (GTK_BOX (hbb), w, FALSE, FALSE, 2);
	gtk_widget_show (w);
	t->arrow = w;
	w = gnome_pixmap_new_from_file (pixmapname);
	gtk_box_pack_start (GTK_BOX (hbb), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	w = gtk_label_new (t->name);
	gtk_box_pack_start (GTK_BOX (hbb), w, FALSE, FALSE, 4);
	gtk_widget_show (w);
	w = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (hbb), w, TRUE, TRUE, 0);
	gtk_widget_show (w);
	gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (sp_toolbox_hide), t);
	/* Separate button */
	//b = gtk_button_new ();
	//gtk_button_set_relief (GTK_BUTTON (b), GTK_RELIEF_NONE);
        b = gtk_toggle_button_new ();
	gtk_box_pack_start (GTK_BOX (hbox), b, FALSE, FALSE, 0);
	gtk_widget_show (b);
        t->standalonetoggle = b;
	w = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/seperate_tool.xpm");
	gtk_container_add (GTK_CONTAINER (b), w);
	gtk_widget_show (w);
	//gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (sp_toolbox_separate), t);
        gtk_signal_connect (GTK_OBJECT (b), "toggled", GTK_SIGNAL_FUNC (sp_toolbox_separate), t);
	/* Contents */
	gtk_box_pack_start (GTK_BOX (t), contents, TRUE, TRUE, 0);
	gtk_widget_show (contents);

	/* Window */
	gtk_window_set_title (GTK_WINDOW (t->window), t->name);
	gtk_window_set_policy (GTK_WINDOW (t->window), FALSE, FALSE, FALSE);
	g_snprintf (c, 256, "toolbox_%s", t->internalname);
	gtk_window_set_wmclass (GTK_WINDOW (t->window), c, "Sodipodi");
	gnome_window_icon_set_from_default (GTK_WINDOW (t->window));
	gtk_signal_connect (GTK_OBJECT (t->window), "delete_event", GTK_SIGNAL_FUNC (sp_toolbox_delete), t);
	/* Window vbox */
	gtk_widget_show (t->windowvbox);
	/* Close button */
	b = gnome_stock_button ("Button_Close");
	gtk_box_pack_end (GTK_BOX (t->windowvbox), b, TRUE, TRUE, 0);
	gtk_widget_show (b);
	gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (sp_toolbox_close), t);

	return GTK_WIDGET (t);
}

static void
sp_toolbox_hide (GtkButton * button, gpointer data)
{
	SPToolBox * toolbox;
	guint newstate;

	toolbox = SP_TOOLBOX (data);

	//if (toolbox->state & SP_TOOLBOX_STANDALONE) {
	//	newstate = SP_TOOLBOX_VISIBLE;
	if (toolbox->state == SP_TOOLBOX_VISIBLE) {
		newstate = 0;
	} else {
        //	newstate = toolbox->state ^ SP_TOOLBOX_VISIBLE;
		newstate = SP_TOOLBOX_VISIBLE;
	}

	sp_toolbox_set_state (toolbox, newstate);
}

static void
//sp_toolbox_separate (GtkButton * button, gpointer data)
sp_toolbox_separate (GtkToggleButton * button, gpointer data)
{
	SPToolBox * toolbox;
	guint newstate;

	toolbox = SP_TOOLBOX (data);

	//if ((toolbox->state & SP_TOOLBOX_STANDALONE) && (toolbox->state & SP_TOOLBOX_VISIBLE)) {
	if (toolbox->state & SP_TOOLBOX_STANDALONE) {
		newstate = 0;
	} else {
		//newstate = SP_TOOLBOX_VISIBLE | SP_TOOLBOX_STANDALONE;
		newstate = SP_TOOLBOX_STANDALONE | SP_TOOLBOX_VISIBLE;
	}

	sp_toolbox_set_state (toolbox, newstate);
}

static void
sp_toolbox_close (GtkButton * button, gpointer data)
{
	SPToolBox * toolbox;

	toolbox = SP_TOOLBOX (data);

	sp_toolbox_set_state (toolbox, 0);
}

static gint
sp_toolbox_delete (GtkWidget * widget, GdkEventAny * event, gpointer data)
{
	SPToolBox * toolbox;

	toolbox = SP_TOOLBOX (data);

	sp_toolbox_set_state (toolbox, 0);

	return TRUE;
}




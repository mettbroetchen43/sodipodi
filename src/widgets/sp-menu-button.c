#define __SP_MENU_BUTTON_C__

/*
 * Button with menu
 *
 * Authors:
 *   The Gtk+ Team
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 */

#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>

#include "sp-menu-button.h"

enum {ACTIVATE_ITEM, LAST_SIGNAL};

static void sp_menu_button_class_init (SPMenuButtonClass *klass);
static void sp_menu_button_init (SPMenuButton *mb);
static void sp_menu_button_destroy (GtkObject *object);

static void sp_menu_button_draw_arrow (GtkWidget * widget);
static void sp_menu_button_size_request (GtkWidget *widget, GtkRequisition *req);
static gint sp_menu_button_expose (GtkWidget *widget, GdkEventExpose *event);
static gint sp_menu_button_button_press (GtkWidget *widget, GdkEventButton *event);
static gint sp_menu_button_button_release (GtkWidget *widget, GdkEventButton *event);
static void sp_menu_button_released (GtkButton *button);

static gint sp_menu_button_button_timeout (gpointer data);
static void sp_menu_button_menu_deactivate (GtkMenuShell *shell, SPMenuButton *mb);

static void sp_menu_button_menu_position (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer data);

static GtkToggleButtonClass *parent_class = NULL;
static guint button_signals[LAST_SIGNAL] = {0};

GtkType
sp_menu_button_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		static const GtkTypeInfo info = {
			"SPMenuButton",
			sizeof (SPMenuButton),
			sizeof (SPMenuButtonClass),
			(GtkClassInitFunc) sp_menu_button_class_init,
			(GtkObjectInitFunc) sp_menu_button_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_TOGGLE_BUTTON, &info);
	}

	return type;
}

static void
sp_menu_button_class_init (SPMenuButtonClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkButtonClass *button_class;

	object_class = (GtkObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;
	button_class = (GtkButtonClass*) klass;

	parent_class = gtk_type_class (GTK_TYPE_TOGGLE_BUTTON);

	object_class->destroy = sp_menu_button_destroy;

	button_signals[ACTIVATE_ITEM] = gtk_signal_new ("activate_item",
						   GTK_RUN_FIRST,
						   GTK_CLASS_TYPE(object_class),
						   GTK_SIGNAL_OFFSET (SPMenuButtonClass, activate_item),
						   gtk_marshal_NONE__POINTER,
						   GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

	widget_class->size_request = sp_menu_button_size_request;
	widget_class->expose_event = sp_menu_button_expose;
	widget_class->button_press_event = sp_menu_button_button_press;
	widget_class->button_release_event = sp_menu_button_button_release;

	button_class->released = sp_menu_button_released;
}

static void
sp_menu_button_init (SPMenuButton *mb)
{
	GTK_WIDGET_SET_FLAGS (mb, GTK_CAN_FOCUS);
	GTK_WIDGET_UNSET_FLAGS (mb, GTK_CAN_DEFAULT | GTK_RECEIVES_DEFAULT);

	mb->menu = gtk_menu_new ();
	mb->menuitem = NULL;
	gtk_signal_connect (GTK_OBJECT (mb->menu), "deactivate", GTK_SIGNAL_FUNC (sp_menu_button_menu_deactivate), mb);

	mb->cbid = 0;

	mb->width = 0;
	mb->height = 0;
}

static void
sp_menu_button_destroy (GtkObject *object)
{
	SPMenuButton *mb;

	mb = SP_MENU_BUTTON (object);

	if (mb->cbid) {
		gtk_timeout_remove (mb->cbid);
		mb->cbid = 0;
	}

	if (mb->menu) {
		gtk_widget_destroy (mb->menu);
		mb->menu = NULL;
	}

	mb->menuitem = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);


}

static void
sp_menu_button_size_request (GtkWidget *widget, GtkRequisition *req)
{
	SPMenuButton *mb;

	mb = SP_MENU_BUTTON (widget);

	if ((mb->width > 0) || (mb->height > 0)) {
		req->width = mb->width;
		req->height = mb->height;
	} else {
		if (GTK_WIDGET_CLASS (parent_class)->size_request)
			GTK_WIDGET_CLASS (parent_class)->size_request (widget, req);
	}
}

static gint
sp_menu_button_expose (GtkWidget *widget, GdkEventExpose *event)
{
	gint ret;

	if (GTK_WIDGET_CLASS (parent_class)->expose_event) {
		ret = GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
	} else {
		ret = FALSE;
	}

	sp_menu_button_draw_arrow(widget);

	return ret;
}

static gint
sp_menu_button_button_press (GtkWidget *widget, GdkEventButton *event)
{
	SPMenuButton *mb;

	mb = SP_MENU_BUTTON (widget);

	g_print ("Press\n");

	if ((event->button == 1) && !mb->cbid) {
		mb->cbid = gtk_timeout_add (100, sp_menu_button_button_timeout, mb);
	}

	if (GTK_WIDGET_CLASS (parent_class)->button_press_event)
		return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);

	return FALSE;
}

static gint
sp_menu_button_button_release (GtkWidget *widget, GdkEventButton *event)
{
	SPMenuButton *mb;

	mb = SP_MENU_BUTTON (widget);

	g_print ("Release\n");

	if ((event->button == 1) && mb->cbid) {
		gtk_timeout_remove (mb->cbid);
		mb->cbid = 0;
	}

	if (GTK_WIDGET_CLASS (parent_class)->button_release_event)
		return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);

	return FALSE;
}

static void
sp_menu_button_released (GtkButton *button)
{
	SPMenuButton *mb;
	gpointer data;

	mb = SP_MENU_BUTTON (button);

	if (GTK_BUTTON_CLASS (parent_class)->released)
		GTK_BUTTON_CLASS (parent_class)->released (button);

	if (mb->menuitem) {
		data = gtk_object_get_data (GTK_OBJECT (mb->menuitem), "user");
		gtk_signal_emit (GTK_OBJECT (mb), button_signals[ACTIVATE_ITEM], data);
	}
}

GtkWidget*
sp_menu_button_new (void)
{
	SPMenuButton *mb;

	mb = gtk_type_new (SP_TYPE_MENU_BUTTON);

	return (GtkWidget *) mb;
}

void
sp_menu_button_append_child (SPMenuButton *mb, GtkWidget *child, gpointer data)
{
	GtkWidget *mi;

	g_return_if_fail (mb != NULL);
	g_return_if_fail (SP_IS_MENU_BUTTON (mb));
	g_return_if_fail (child != NULL);
	g_return_if_fail (GTK_IS_WIDGET (child));

	mi = gtk_menu_item_new ();
	gtk_widget_show (mi);
	gtk_object_set_data (GTK_OBJECT (mi), "user", data);

	if (mb->menuitem) {
		/* Put child into menu now */
		gtk_container_add (GTK_CONTAINER (mi), child);
	} else {
		/* Put child into main button */
		gtk_container_add (GTK_CONTAINER (mb), child);
		mb->menuitem = mi;
	}

	gtk_menu_append (GTK_MENU (mb->menu), mi);
}

void
sp_menu_button_set_active (SPMenuButton *mb, gpointer data)
{
	g_return_if_fail (mb != NULL);
	g_return_if_fail (SP_IS_MENU_BUTTON (mb));

	if (mb->menu) {
		GList *children;
		children = GTK_MENU_SHELL (mb->menu)->children;
		while (children) {
			gpointer user;
			user = gtk_object_get_data (GTK_OBJECT (children->data), "user");
			if (user == data) {
				if (children->data != mb->menuitem) {
					gtk_widget_reparent (gtk_bin_get_child(GTK_BIN (mb)), mb->menuitem);
					mb->menuitem = GTK_WIDGET (children->data);
					gtk_widget_reparent (gtk_bin_get_child(GTK_BIN (mb->menuitem)), GTK_WIDGET (mb));
					gtk_signal_emit (GTK_OBJECT (mb), button_signals[ACTIVATE_ITEM], data);
				}
				return;
			}
			children = children->next;
		}

		g_warning ("Data not associated with any child");
	}

	g_warning ("No menu");
}

gpointer
sp_menu_button_get_active_data (SPMenuButton *mb)
{
	g_return_val_if_fail (mb != NULL, NULL);
	g_return_val_if_fail (SP_IS_MENU_BUTTON (mb), NULL);

	if (mb->menuitem) {
		return gtk_object_get_data (GTK_OBJECT (mb->menuitem), "user");
	}

	return NULL;
}

static gint
sp_menu_button_button_timeout (gpointer data)
{
	SPMenuButton *mb;

	mb = SP_MENU_BUTTON (data);

	g_print ("Menu button timeout\n");

	/* fixme: Think (lauris) */
	/* We keep original values to prevent resizing */
	mb->width = GTK_WIDGET (mb)->allocation.width;
	mb->height = GTK_WIDGET (mb)->allocation.height;
	gtk_widget_reparent (gtk_bin_get_child(GTK_BIN (mb)), mb->menuitem);
	gtk_menu_popup (GTK_MENU (mb->menu), NULL, NULL, sp_menu_button_menu_position, mb, 1, GDK_CURRENT_TIME);

	mb->cbid = 0;

	return FALSE;
}

static void
sp_menu_button_menu_deactivate (GtkMenuShell *shell, SPMenuButton *mb)
{
	/* fixme: Think (Lauris) */
	mb->menuitem = gtk_menu_get_active (GTK_MENU (shell));

	if (mb->menuitem) {
		GtkWidget *child;
		gpointer data;

		child = GTK_BIN (mb->menuitem)->child;

		if (child) {
			gtk_widget_reparent (child, GTK_WIDGET (mb));
		}

		mb->width = 0;
		mb->height = 0;

		gtk_grab_remove (GTK_WIDGET (mb));
		gtk_button_released (GTK_BUTTON (mb));

		data = gtk_object_get_data (GTK_OBJECT (mb->menuitem), "user");
		gtk_signal_emit (GTK_OBJECT (mb), button_signals[ACTIVATE_ITEM], data);
	}
}

static void
sp_menu_button_menu_position (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer data)
{
	SPMenuButton *menu_button;
	GtkWidget *active;
	GtkWidget *child;
	GtkRequisition requisition;
	GList *children;
	gint shift_menu;
	gint screen_width;
	gint screen_height;
	gint menu_xpos;
	gint menu_ypos;
	gint width;
	gint height;
	
	g_return_if_fail (data != NULL);
	g_return_if_fail (SP_IS_MENU_BUTTON (data));

	menu_button = SP_MENU_BUTTON (data);

	gtk_widget_get_child_requisition (GTK_WIDGET (menu), &requisition);
	width = requisition.width;
	height = requisition.height;
	
	active = gtk_menu_get_active (GTK_MENU (menu_button->menu));
	children = GTK_MENU_SHELL (menu_button->menu)->children;
	gdk_window_get_origin (GTK_WIDGET (menu_button)->window, &menu_xpos, &menu_ypos);
	menu_xpos += GTK_WIDGET (menu_button)->allocation.x;
	menu_ypos += GTK_WIDGET (menu_button)->allocation.y;

#if 1	/* if 0 to compatible with sodipodi with gtk+1.2 */
	menu_ypos += GTK_WIDGET (menu_button)->allocation.height / 2 - 2;
	
	if (active != NULL)
	{
		gtk_widget_get_child_requisition (active, &requisition);
		menu_ypos -= requisition.height / 2;
	}
	
	while (children)
	{
		child = children->data;
		
		if (active == child)
			break;
		
		if (GTK_WIDGET_VISIBLE (child))
		{
			gtk_widget_get_child_requisition (child, &requisition);
			menu_ypos -= requisition.height;
		}
		
		children = children->next;
	}
#endif
	
	screen_width = gdk_screen_width ();
	screen_height = gdk_screen_height ();
	
	shift_menu = FALSE;
	if (menu_ypos < 0)
	{
		menu_ypos = 0;
		shift_menu = TRUE;
	}
	else if ((menu_ypos + height) > screen_height)
	{
		menu_ypos -= ((menu_ypos + height) - screen_height);
		shift_menu = TRUE;
	}
	
	if (shift_menu)
	{
		if ((menu_xpos + GTK_WIDGET (menu_button)->allocation.width + width) <= screen_width)
			menu_xpos += GTK_WIDGET (menu_button)->allocation.width;
		else
			menu_xpos -= width;
	}
	
	if (menu_xpos < 0)
		menu_xpos = 0;
	else if ((menu_xpos + width) > screen_width)
		menu_xpos -= ((menu_xpos + width) - screen_width);
	
	*x = menu_xpos;
	*y = menu_ypos;
}

#define ASIZE 8
static void
sp_menu_button_draw_arrow (GtkWidget * widget)
{
	if (GTK_WIDGET_DRAWABLE (widget)) {
		gint w, h, bw, tx, ty;
		w = widget->allocation.width;
		h = widget->allocation.height;
		bw = GTK_CONTAINER (widget)->border_width;
		tx = widget->style->xthickness;
		ty = widget->style->ythickness;
		gtk_draw_arrow (widget->style, widget->window,
				widget->state, GTK_SHADOW_IN,
				GTK_ARROW_DOWN, TRUE,
				widget->allocation.x + tx,
				widget->allocation.y + h - 2 * bw - ty - ASIZE,
				ASIZE, ASIZE);
	}
}

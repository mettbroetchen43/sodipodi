/**
 * sp-dock-item-grip.c
 * Copyright (C) 2002 Gustavo Gir�ldez <gustavo.giraldez@gmx.net>
 * Copyright (C) 2003 Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
 *
 * Based on bonobo-dock-item-grip.  Original copyright notice follows.
 *
 * Author:
 *    Michael Meeks
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <helper/sp-intl.h>
#include <string.h>
#include <glib-object.h>
#include <atk/atkstateset.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkaccessible.h>
#include <gtk/gtkbindings.h>
#include "sp-dock-macros.h"
#include "sp-dock-item.h"
#include "sp-dock-item-grip.h"
#include "sp-dock.h"

/* Keep this for future API/ABI compatibility - Biswa */
struct _SPDockItemPrivate {
	gpointer unused;
};

enum {
    ACTIVATE,
    LAST_SIGNAL
};
static guint signals [LAST_SIGNAL];

SP_CLASS_BOILERPLATE (SPDockItemGrip, sp_dock_item_grip,
			 GtkWidget, GTK_TYPE_WIDGET);

static gint
sp_dock_item_grip_expose (GtkWidget      *widget,
			   GdkEventExpose *event)
{
    GdkRectangle *clip = &event->area;
    GdkRectangle *rect = &widget->allocation;
    SPDockItemGrip *grip = (SPDockItemGrip *) widget;
    GtkShadowType shadow = GTK_SHADOW_OUT;

    gtk_paint_handle (widget->style,
                      widget->window,
                      GTK_WIDGET_STATE (widget),
                      shadow,
                      clip, widget, "dockitem",
                      rect->x, rect->y, rect->width, rect->height, 
                      grip->item->orientation);

    if (GTK_WIDGET_HAS_FOCUS (widget)) {
        gint focus_width;
        gint focus_pad;
        GdkRectangle focus;
		
        gtk_widget_style_get (GTK_WIDGET (widget),
                              "focus-line-width", &focus_width,
                              "focus-padding", &focus_pad,
                              NULL); 
		
        focus = *rect;
        focus.x += widget->style->xthickness + focus_pad;
        focus.y += widget->style->ythickness + focus_pad;
        focus.width -= 2 * (widget->style->xthickness + focus_pad);
        focus.height -= 2 * (widget->style->xthickness + focus_pad);
		
        gtk_paint_focus (widget->style, widget->window,
                         GTK_WIDGET_STATE (widget),
                         clip, widget, "dockitem",
                         focus.x, focus.y,
                         focus.width, focus.height);
    }

    return FALSE;
}

static AtkObject *
sp_dock_item_grip_get_accessible (GtkWidget *widget)
{
    return NULL;
}

static void
sp_dock_item_grip_activate (SPDockItemGrip *grip)
{
}

static void
sp_dock_item_grip_dispose (GObject *object)
{
    SP_CALL_PARENT (G_OBJECT_CLASS, dispose, (object));
}

static void
sp_dock_item_grip_instance_init (SPDockItemGrip *grip)
{
    GTK_WIDGET_SET_FLAGS (grip, GTK_CAN_FOCUS);
    GTK_WIDGET_SET_FLAGS (grip, GTK_NO_WINDOW);
}

static gint
sp_dock_item_grip_key_press_event (GtkWidget   *widget,
                                    GdkEventKey *event)
{
    return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
}

static void
sp_dock_item_grip_class_init (SPDockItemGripClass *klass)
{
    GtkBindingSet  *binding_set;
    GObjectClass   *gobject_class = (GObjectClass *) klass;
    GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

    parent_class = g_type_class_peek_parent (klass);

    gobject_class->dispose = sp_dock_item_grip_dispose;

    widget_class->expose_event = sp_dock_item_grip_expose;
    widget_class->get_accessible = sp_dock_item_grip_get_accessible;
    widget_class->key_press_event = sp_dock_item_grip_key_press_event;

    klass->activate = sp_dock_item_grip_activate;

    binding_set = gtk_binding_set_by_class (klass);

    signals [ACTIVATE] =
        g_signal_new ("activate",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (
                          SPDockItemGripClass, activate),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
    widget_class->activate_signal = signals [ACTIVATE];

    gtk_binding_entry_add_signal (binding_set, GDK_Return, 0,
                                  "activate", 0);
    gtk_binding_entry_add_signal (binding_set, GDK_KP_Enter, 0,
                                  "activate", 0);
}

GtkWidget *
sp_dock_item_grip_new (SPDockItem *item)
{
    SPDockItemGrip *grip = g_object_new (SP_TYPE_DOCK_ITEM_GRIP, NULL);

    grip->item = item;

    return GTK_WIDGET (grip);
}

/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- 
 *
 * sp-dock-item.c
 *
 * Copyright (C) 2002 Gustavo Gir�ldez <gustavo.giraldez@gmx.net>
 * Copyright (C) 2003 Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
 *
 * Based on GnomeDockItem/BonoboDockItem.  Original copyright notice follows.
 *
 * Copyright (C) 1998 Ettore Perazzoli
 * Copyright (C) 1998 Elliot Lee
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald 
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <helper/sp-intl.h>
#include <string.h>
#include "sp-dock-macros.h"
#include <gdk/gdkkeysyms.h>

#include "sp-dock.h"
#include "sp-dock-item.h"
#include "sp-dock-item-grip.h"
#include "sp-dock-notebook.h"
#include "sp-dock-paned.h"
#include "sp-dock-tablabel.h"
#include "sp-dock-placeholder.h"
#include "sp-dock-master.h"
#include "sp-dock-typebuiltins.h"
#include <helper/sp-marshal.h>


/* ----- Private prototypes ----- */

static void  sp_dock_item_class_init    (SPDockItemClass *class);
static void  sp_dock_item_instance_init (SPDockItem *item);

static GObject *sp_dock_item_constructor (GType                  type,
                                           guint                  n_construct_properties,
                                           GObjectConstructParam *construct_param);

static void  sp_dock_item_set_property  (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void  sp_dock_item_get_property  (GObject      *object,
                                          guint         prop_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);

static void  sp_dock_item_destroy       (GtkObject *object);

static void  sp_dock_item_add           (GtkContainer *container,
                                          GtkWidget    *widget);
static void  sp_dock_item_remove        (GtkContainer *container,
                                          GtkWidget    *widget);
static void  sp_dock_item_forall        (GtkContainer *container,
                                          gboolean      include_internals,
                                          GtkCallback   callback,
                                          gpointer      callback_data);
static GtkType sp_dock_item_child_type  (GtkContainer *container);

static void  sp_dock_item_size_request  (GtkWidget *widget,
                                          GtkRequisition *requisition);
static void  sp_dock_item_size_allocate (GtkWidget *widget,
                                          GtkAllocation *allocation);
static void  sp_dock_item_map           (GtkWidget *widget);
static void  sp_dock_item_unmap         (GtkWidget *widget);
static void  sp_dock_item_realize       (GtkWidget *widget);
static void  sp_dock_item_style_set     (GtkWidget *widget,
                                          GtkStyle  *previous_style);
static gint  sp_dock_item_expose        (GtkWidget *widget,
                                          GdkEventExpose *event);

static gint  sp_dock_item_button_changed (GtkWidget *widget,
                                           GdkEventButton *event);
static gint  sp_dock_item_motion         (GtkWidget *widget,
                                           GdkEventMotion *event);
static gboolean  sp_dock_item_key_press  (GtkWidget *widget,
                                           GdkEventKey *event);

static gboolean sp_dock_item_dock_request (SPDockObject    *object,
                                            gint              x,
                                            gint              y,
                                            SPDockRequest   *request);
static void     sp_dock_item_dock         (SPDockObject    *object,
                                            SPDockObject    *requestor,
                                            SPDockPlacement  position,
                                            GValue           *other_data);

static void  sp_dock_item_popup_menu    (SPDockItem *item, 
                                          guint        button,
                                          guint32      time);
static void  sp_dock_item_drag_start    (SPDockItem *item);
static void  sp_dock_item_drag_end      (SPDockItem *item,
                                          gboolean     cancel);

static void  sp_dock_item_tab_button    (GtkWidget      *widget,
                                          GdkEventButton *event,
                                          gpointer        data);
                                          
static void  sp_dock_item_hide_cb       (GtkWidget   *widget,
                                          SPDockItem *item);

static void  sp_dock_item_lock_cb       (GtkWidget   *widget,
                                          SPDockItem *item);

static void  sp_dock_item_showhide_grip (SPDockItem *item);

static void  sp_dock_item_real_set_orientation (SPDockItem    *item,
                                                 GtkOrientation  orientation);

static void sp_dock_param_export_gtk_orientation (const GValue *src,
                                                   GValue       *dst);
static void sp_dock_param_import_gtk_orientation (const GValue *src,
                                                   GValue       *dst);



/* ----- Class variables and definitions ----- */

enum {
    PROP_0,
    PROP_ORIENTATION,
    PROP_RESIZE,
    PROP_BEHAVIOR,
    PROP_GRIP_SIZE,
    PROP_LOCKED,
    PROP_PREFERRED_WIDTH,
    PROP_PREFERRED_HEIGHT
};

enum {
    DOCK_DRAG_BEGIN,
    DOCK_DRAG_MOTION,
    DOCK_DRAG_END,
    LAST_SIGNAL
};

static guint sp_dock_item_signals [LAST_SIGNAL] = { 0 };

#define DEFAULT_GRIP_SIZE  10
#define SP_DOCK_ITEM_NOT_LOCKED(item) !((item)->behavior & SP_DOCK_ITEM_BEH_LOCKED)
#define SP_DOCK_ITEM_GRIP_SHOWN(item) \
    (SP_DOCK_ITEM_HAS_GRIP (item) && \
     SP_DOCK_ITEM_NOT_LOCKED (item) && \
     (item)->_priv->grip_shown)


struct _SPDockItemPrivate {
    GtkWidget *menu;

    gboolean   grip_shown;
    GtkWidget *grip;
    guint      grip_size;
    
    GtkWidget *tab_label;

    gint       preferred_width;
    gint       preferred_height;

    SPDockPlaceholder *ph;

    gint       start_x, start_y;
};

/* FIXME: implement the rest of the behaviors */

#define SPLIT_RATIO  0.4


/* ----- Private functions ----- */

SP_CLASS_BOILERPLATE (SPDockItem, sp_dock_item, SPDockObject, SP_TYPE_DOCK_OBJECT);

static void
sp_dock_item_class_init (SPDockItemClass *klass)
{
    GObjectClass       *g_object_class;
    GtkObjectClass     *gtk_object_class;
    GtkWidgetClass     *widget_class;
    GtkContainerClass  *container_class;
    SPDockObjectClass *object_class;
    
    g_object_class = G_OBJECT_CLASS (klass);
    gtk_object_class = GTK_OBJECT_CLASS (klass);
    widget_class = GTK_WIDGET_CLASS (klass);
    container_class = GTK_CONTAINER_CLASS (klass);
    object_class = SP_DOCK_OBJECT_CLASS (klass);

    g_object_class->constructor = sp_dock_item_constructor;
    g_object_class->set_property = sp_dock_item_set_property;
    g_object_class->get_property = sp_dock_item_get_property;

    gtk_object_class->destroy = sp_dock_item_destroy;

    widget_class->realize = sp_dock_item_realize;
    widget_class->map = sp_dock_item_map;
    widget_class->unmap = sp_dock_item_unmap;
    widget_class->size_request = sp_dock_item_size_request;
    widget_class->size_allocate = sp_dock_item_size_allocate;
    widget_class->style_set = sp_dock_item_style_set;
    widget_class->expose_event = sp_dock_item_expose;
    widget_class->button_press_event = sp_dock_item_button_changed;
    widget_class->button_release_event = sp_dock_item_button_changed;
    widget_class->motion_notify_event = sp_dock_item_motion;
    widget_class->key_press_event = sp_dock_item_key_press;
    
    container_class->add = sp_dock_item_add;
    container_class->remove = sp_dock_item_remove;
    container_class->forall = sp_dock_item_forall;
    container_class->child_type = sp_dock_item_child_type;
    
    object_class->is_compound = FALSE;

    object_class->dock_request = sp_dock_item_dock_request;
    object_class->dock = sp_dock_item_dock;

    /* properties */

    g_object_class_install_property (
        g_object_class, PROP_ORIENTATION,
        g_param_spec_enum ("orientation", _("Orientation"),
                           _("Orientation of the docking item"),
                           GTK_TYPE_ORIENTATION,
                           GTK_ORIENTATION_HORIZONTAL,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                           SP_DOCK_PARAM_EXPORT));

    /* --- register exporter/importer for GTK_ORIENTATION */
    g_value_register_transform_func (GTK_TYPE_ORIENTATION, SP_TYPE_DOCK_PARAM,
                                     sp_dock_param_export_gtk_orientation);
    g_value_register_transform_func (SP_TYPE_DOCK_PARAM, GTK_TYPE_ORIENTATION,
                                     sp_dock_param_import_gtk_orientation);
    /* --- end of registration */
    
    g_object_class_install_property (
        g_object_class, PROP_RESIZE,
        g_param_spec_boolean ("resize", _("Resizable"),
                              _("If set, the dock item can be resized when "
                                "docked in a paned"),
                              TRUE,
                              G_PARAM_READWRITE));
                                     
    g_object_class_install_property (
        g_object_class, PROP_BEHAVIOR,
        g_param_spec_flags ("behavior", _("Item behavior"),
                            _("General behavior for the dock item (i.e. "
                              "whether it can float, if it's locked, etc.)"),
                            SP_TYPE_DOCK_ITEM_BEHAVIOR,
                            SP_DOCK_ITEM_BEH_NORMAL,
                            G_PARAM_READWRITE));
                                     
    g_object_class_install_property (
        g_object_class, PROP_GRIP_SIZE,
        g_param_spec_uint ("grip_size", _("Grip size"),
                           _("Size in pixels of the grip to drag the dock item"),
                           0, 100, DEFAULT_GRIP_SIZE,
                           G_PARAM_READWRITE));

    g_object_class_install_property (
        g_object_class, PROP_LOCKED,
        g_param_spec_boolean ("locked", _("Locked"),
                              _("If set, the dock item cannot be dragged around "
                                "and it doesn't show a grip"),
                              FALSE,
                              G_PARAM_READWRITE |
                              SP_DOCK_PARAM_EXPORT));

    g_object_class_install_property (
        g_object_class, PROP_PREFERRED_WIDTH,
        g_param_spec_int ("preferred_width", _("Preferred width"),
                          _("Preferred width for the dock item"),
                          -1, G_MAXINT, -1,
                          G_PARAM_READWRITE));

    g_object_class_install_property (
        g_object_class, PROP_PREFERRED_HEIGHT,
        g_param_spec_int ("preferred_height", _("Preferred height"),
                          _("Preferred height for the dock item"),
                          -1, G_MAXINT, -1,
                          G_PARAM_READWRITE));

    /* signals */
    
    sp_dock_item_signals [DOCK_DRAG_BEGIN] = 
        g_signal_new ("dock_drag_begin",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (SPDockItemClass, dock_drag_begin),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      sp_marshal_NONE__NONE,
                      G_TYPE_NONE, 
                      0);

    sp_dock_item_signals [DOCK_DRAG_MOTION] = 
        g_signal_new ("dock_drag_motion",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (SPDockItemClass, dock_drag_motion),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      sp_marshal_VOID__INT_INT,
                      G_TYPE_NONE, 
                      2,
                      G_TYPE_INT,
                      G_TYPE_INT);

    sp_dock_item_signals [DOCK_DRAG_END] = 
        g_signal_new ("dock_drag_end",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (SPDockItemClass, dock_drag_end),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      sp_marshal_VOID__BOOLEAN,
                      G_TYPE_NONE, 
                      1,
                      G_TYPE_BOOLEAN);

    klass->has_grip = TRUE;
    klass->dock_drag_begin = NULL;
    klass->dock_drag_motion = NULL;
    klass->dock_drag_end = NULL;
    klass->set_orientation = sp_dock_item_real_set_orientation;
}

static void
sp_dock_item_instance_init (SPDockItem *item)
{
    GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET (item), GTK_NO_WINDOW);

    item->child = NULL;
    
    item->orientation = GTK_ORIENTATION_HORIZONTAL;
    item->behavior = SP_DOCK_ITEM_BEH_NORMAL;

    item->resize = TRUE;

    item->dragoff_x = item->dragoff_y = 0;

    item->_priv = g_new0 (SPDockItemPrivate, 1);
    item->_priv->menu = NULL;

    item->_priv->preferred_width = item->_priv->preferred_height = -1;
    item->_priv->grip_size = DEFAULT_GRIP_SIZE;
    item->_priv->tab_label = NULL;

    item->_priv->ph = NULL;
}

static GObject *
sp_dock_item_constructor (GType                  type,
                           guint                  n_construct_properties,
                           GObjectConstructParam *construct_param)
{
    GObject *g_object;
    
    g_object = SP_CALL_PARENT_WITH_DEFAULT (G_OBJECT_CLASS, 
                                               constructor, 
                                               (type,
                                                n_construct_properties,
                                                construct_param),
                                               NULL);
    if (g_object) {
        SPDockItem *item = SP_DOCK_ITEM (g_object);

        if (SP_DOCK_ITEM_HAS_GRIP (item)) {
            item->_priv->grip_shown = TRUE;
            item->_priv->grip = sp_dock_item_grip_new (item);
            gtk_widget_set_parent (item->_priv->grip, GTK_WIDGET (item));
            gtk_widget_show (item->_priv->grip);
        }
        else {
            item->_priv->grip_shown = FALSE;
        }
    }

    return g_object;
}

static void
sp_dock_item_set_property  (GObject      *g_object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
    SPDockItem *item = SP_DOCK_ITEM (g_object);

    switch (prop_id) {
        case PROP_ORIENTATION:
            sp_dock_item_set_orientation (item, g_value_get_enum (value));
            break;
        case PROP_RESIZE:
            item->resize = g_value_get_boolean (value);
            gtk_widget_queue_resize (GTK_WIDGET (item));
            break;
        case PROP_BEHAVIOR:
        {
            SPDockItemBehavior old_beh = item->behavior;
            item->behavior = g_value_get_flags (value);

            if ((old_beh ^ item->behavior) & SP_DOCK_ITEM_BEH_LOCKED) {
                if (SP_DOCK_OBJECT_GET_MASTER (item))
                    g_signal_emit_by_name (SP_DOCK_OBJECT_GET_MASTER (item),
                                           "layout_changed");
                g_object_notify (g_object, "locked");
                sp_dock_item_showhide_grip (item);
            }
            
            break;
        }
        case PROP_GRIP_SIZE:
            item->_priv->grip_size = g_value_get_uint (value);
            if (SP_DOCK_ITEM_GRIP_SHOWN (item))
                gtk_widget_queue_resize (GTK_WIDGET (item));
            break;
        case PROP_LOCKED:
        {
            SPDockItemBehavior old_beh = item->behavior;

            if (g_value_get_boolean (value))
                item->behavior |= SP_DOCK_ITEM_BEH_LOCKED;
            else
                item->behavior &= ~SP_DOCK_ITEM_BEH_LOCKED;

            if (old_beh ^ item->behavior) {
                sp_dock_item_showhide_grip (item);
                g_object_notify (g_object, "behavior");

                if (SP_DOCK_OBJECT_GET_MASTER (item))
                    g_signal_emit_by_name (SP_DOCK_OBJECT_GET_MASTER (item),
                                           "layout_changed");
            }
            break;
        }
        case PROP_PREFERRED_WIDTH:
            item->_priv->preferred_width = g_value_get_int (value);
            break;
        case PROP_PREFERRED_HEIGHT:
            item->_priv->preferred_height = g_value_get_int (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, prop_id, pspec);
            break;
    }
}

static void
sp_dock_item_get_property  (GObject      *g_object,
                             guint         prop_id,
                             GValue       *value,
                             GParamSpec   *pspec)
{
    SPDockItem *item = SP_DOCK_ITEM (g_object);
    
    switch (prop_id) {
        case PROP_ORIENTATION:
            g_value_set_enum (value, item->orientation);
            break;
        case PROP_RESIZE:
            g_value_set_boolean (value, item->resize);
            break;
        case PROP_BEHAVIOR:
            g_value_set_flags (value, item->behavior);
            break;
        case PROP_GRIP_SIZE:
            g_value_set_uint (value, item->_priv->grip_size);
            break;
        case PROP_LOCKED:
            g_value_set_boolean (value, !SP_DOCK_ITEM_NOT_LOCKED (item));
            break;
        case PROP_PREFERRED_WIDTH:
            g_value_set_int (value, item->_priv->preferred_width);
            break;
        case PROP_PREFERRED_HEIGHT:
            g_value_set_int (value, item->_priv->preferred_height);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, prop_id, pspec);
            break;
    }
}

static void
sp_dock_item_destroy (GtkObject *object)
{
    SPDockItem *item = SP_DOCK_ITEM (object);

    if (item->_priv) {
        SPDockItemPrivate *priv = item->_priv;
        
        if (priv->tab_label) {
            sp_dock_item_set_tablabel (item, NULL);
        };
        if (priv->menu) {
            gtk_menu_detach (GTK_MENU (priv->menu));
            priv->menu = NULL;
        };
        if (priv->grip) {
            gtk_container_remove (GTK_CONTAINER (item), priv->grip);
            priv->grip = NULL;
        }
        if (priv->ph) {
            g_object_unref (priv->ph);
            priv->ph = NULL;
        }
        
        item->_priv = NULL;
        g_free (priv);
    }

    SP_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void 
sp_dock_item_add (GtkContainer *container,
                   GtkWidget    *widget)
{
    SPDockItem *item;
    
    g_return_if_fail (SP_IS_DOCK_ITEM (container));

    item = SP_DOCK_ITEM (container);
    if (SP_IS_DOCK_OBJECT (widget)) {
        g_warning (_("You can't add a dock object (%p of type %s) inside a %s. "
                     "Use a SPDock or some other compound dock object."),
                   widget, G_OBJECT_TYPE_NAME (widget), G_OBJECT_TYPE_NAME (item));
        return;
    }

    if (item->child != NULL) {
        g_warning (_("Attempting to add a widget with type %s to a %s, "
                     "but it can only contain one widget at a time; "
                     "it already contains a widget of type %s"),
                     G_OBJECT_TYPE_NAME (widget),
                     G_OBJECT_TYPE_NAME (item),
                     G_OBJECT_TYPE_NAME (item->child));
        return;
    }

    gtk_widget_set_parent (widget, GTK_WIDGET (item));
    item->child = widget;
}

static void  
sp_dock_item_remove (GtkContainer *container,
                      GtkWidget    *widget)
{
    SPDockItem *item;
    gboolean     was_visible;
    
    g_return_if_fail (SP_IS_DOCK_ITEM (container));
    
    item = SP_DOCK_ITEM (container);
    if (item->_priv && widget == item->_priv->grip) {
        gboolean grip_was_visible = GTK_WIDGET_VISIBLE (widget);
        gtk_widget_unparent (widget);
        item->_priv->grip = NULL;
        if (grip_was_visible)
            gtk_widget_queue_resize (GTK_WIDGET (item));
        return;
    }
    
    if (SP_DOCK_ITEM_IN_DRAG (item)) {
        sp_dock_item_drag_end (item, TRUE);
    }
    
    g_return_if_fail (item->child == widget);
    
    was_visible = GTK_WIDGET_VISIBLE (widget);

    gtk_widget_unparent (widget);
    item->child = NULL;
    
    if (was_visible)
        gtk_widget_queue_resize (GTK_WIDGET (container));
}

static void
sp_dock_item_forall (GtkContainer *container,
                      gboolean      include_internals,
                      GtkCallback   callback,
                      gpointer      callback_data)
{
    SPDockItem *item = (SPDockItem *) container;
    
    g_return_if_fail (callback != NULL);
    
    if (include_internals && item->_priv->grip)
        (* callback) (item->_priv->grip, callback_data);
    
    if (item->child)
        (* callback) (item->child, callback_data);
}

static GtkType
sp_dock_item_child_type (GtkContainer *container)
{
    g_return_val_if_fail (SP_IS_DOCK_ITEM (container), G_TYPE_NONE);
    
    if (!SP_DOCK_ITEM (container)->child)
        return GTK_TYPE_WIDGET;
    else
        return G_TYPE_NONE;
}

static void
sp_dock_item_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
    GtkRequisition  child_requisition;
    SPDockItem    *item;

    g_return_if_fail (SP_IS_DOCK_ITEM (widget));
    g_return_if_fail (requisition != NULL);

    item = SP_DOCK_ITEM (widget);

    /* If our child is not visible, we still request its size, since
       we won't have any useful hint for our size otherwise.  */
    if (item->child)
        gtk_widget_size_request (item->child, &child_requisition);
    else {
        child_requisition.width = 0;
        child_requisition.height = 0;
    }

    if (item->orientation == GTK_ORIENTATION_HORIZONTAL) {
        requisition->width = 
            SP_DOCK_ITEM_GRIP_SHOWN (item) ? item->_priv->grip_size : 0;
        if (item->child) {
            requisition->width += child_requisition.width;
            requisition->height = child_requisition.height;
        } else
            requisition->height = 0;
    } else {
        requisition->height = 
            SP_DOCK_ITEM_GRIP_SHOWN (item) ? item->_priv->grip_size : 0;
        if (item->child) {
            requisition->width = child_requisition.width;
            requisition->height += child_requisition.height;
        } else
            requisition->width = 0;
    }

    requisition->width += GTK_CONTAINER (widget)->border_width * 2;
    requisition->height += GTK_CONTAINER (widget)->border_width * 2;

    widget->requisition = *requisition;
}

static void
sp_dock_item_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
    SPDockItem *item;
  
    g_return_if_fail (SP_IS_DOCK_ITEM (widget));
    g_return_if_fail (allocation != NULL);
  
    item = SP_DOCK_ITEM (widget);

    widget->allocation = *allocation;

    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize (widget->window,
                                widget->allocation.x,
                                widget->allocation.y,
                                widget->allocation.width,
                                widget->allocation.height);

    if (item->child && GTK_WIDGET_VISIBLE (item->child)) {
        GtkAllocation  child_allocation;
        int            border_width;

        border_width = GTK_CONTAINER (widget)->border_width;

        child_allocation.x = border_width;
        child_allocation.y = border_width;
        child_allocation.width = allocation->width - 2 * border_width;
        child_allocation.height = allocation->height - 2 * border_width;
        
        if (SP_DOCK_ITEM_GRIP_SHOWN (item)) {
            GtkAllocation grip_alloc = *allocation;
            
            grip_alloc.x = grip_alloc.y = 0;
            
            if (item->orientation == GTK_ORIENTATION_HORIZONTAL) {
                child_allocation.width -= item->_priv->grip_size;
                grip_alloc.width = item->_priv->grip_size;
                if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
                    grip_alloc.x = child_allocation.x + child_allocation.width;
                else 
                    child_allocation.x += item->_priv->grip_size;
            } else {
                child_allocation.y += item->_priv->grip_size;
                child_allocation.height -= item->_priv->grip_size;
                grip_alloc.height = item->_priv->grip_size;
            }
            if (item->_priv->grip)
                gtk_widget_size_allocate (item->_priv->grip, &grip_alloc);
        };
        gtk_widget_size_allocate (item->child, &child_allocation);
    }
}

static void
sp_dock_item_map (GtkWidget *widget)
{
    SPDockItem *item;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (SP_IS_DOCK_ITEM (widget));

    GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

    item = SP_DOCK_ITEM (widget);

    gdk_window_show (widget->window);

    if (item->child
        && GTK_WIDGET_VISIBLE (item->child)
        && !GTK_WIDGET_MAPPED (item->child))
        gtk_widget_map (item->child);

    if (item->_priv->grip
        && GTK_WIDGET_VISIBLE (item->_priv->grip)
        && !GTK_WIDGET_MAPPED (item->_priv->grip))
        gtk_widget_map (item->_priv->grip);
}

static void
sp_dock_item_unmap (GtkWidget *widget)
{
    SPDockItem *item;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (SP_IS_DOCK_ITEM (widget));

    GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
    
    item = SP_DOCK_ITEM (widget);

    gdk_window_hide (widget->window);

    if (item->_priv->grip)
        gtk_widget_unmap (item->_priv->grip);
}

static void
sp_dock_item_realize (GtkWidget *widget)
{
    GdkWindowAttr  attributes;
    gint           attributes_mask;
    SPDockItem   *item;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (SP_IS_DOCK_ITEM (widget));

    item = SP_DOCK_ITEM (widget);

    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

    /* widget window */
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual (widget);
    attributes.colormap = gtk_widget_get_colormap (widget);
    attributes.event_mask = (gtk_widget_get_events (widget) |
                             GDK_EXPOSURE_MASK |
                             GDK_BUTTON1_MOTION_MASK |
                             GDK_BUTTON_PRESS_MASK |
                             GDK_BUTTON_RELEASE_MASK);
    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), 
                                     &attributes, attributes_mask);
    gdk_window_set_user_data (widget->window, widget);
  
    widget->style = gtk_style_attach (widget->style, widget->window);
    gtk_style_set_background (widget->style, widget->window, 
                              GTK_WIDGET_STATE (item));
    gdk_window_set_back_pixmap (widget->window, NULL, TRUE);

    if (item->child)
        gtk_widget_set_parent_window (item->child, widget->window);
    
    if (item->_priv->grip)
        gtk_widget_set_parent_window (item->_priv->grip, widget->window);
}

static void
sp_dock_item_style_set (GtkWidget *widget,
                         GtkStyle  *previous_style)
{
    g_return_if_fail (widget != NULL);
    g_return_if_fail (SP_IS_DOCK_ITEM (widget));

    /* FIXME: maybe remove this method altogether and use the default implementation */
    if (GTK_WIDGET_REALIZED (widget) && !GTK_WIDGET_NO_WINDOW (widget)) {
        gtk_style_set_background (widget->style, widget->window,
                                  widget->state);
        if (GTK_WIDGET_DRAWABLE (widget))
            gdk_window_clear (widget->window);
    }
}

static void
sp_dock_item_paint (GtkWidget      *widget,
                     GdkEventExpose *event)
{
    SPDockItem  *item;

    item = SP_DOCK_ITEM (widget);

    gtk_paint_box (widget->style,
                   widget->window,
                   GTK_WIDGET_STATE (widget),
                   GTK_SHADOW_NONE,
                   &event->area, widget,
                   "dockitem",
                   0, 0, -1, -1);
}

static gint
sp_dock_item_expose (GtkWidget      *widget,
                      GdkEventExpose *event)
{
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (SP_IS_DOCK_ITEM (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    if (GTK_WIDGET_DRAWABLE (widget) && event->window == widget->window) {
        sp_dock_item_paint (widget, event);
        SP_CALL_PARENT (GTK_WIDGET_CLASS, expose_event, (widget, event));
    }
  
    return FALSE;
}

#define EVENT_IN_TABLABEL_EVENT_WINDOW(ev,tl) \
    ((tl) != NULL && (ev)->window == SP_DOCK_TABLABEL (tl)->event_window)

static gint
sp_dock_item_button_changed (GtkWidget      *widget,
                              GdkEventButton *event)
{
    SPDockItem *item;
    gboolean     event_handled;
    gboolean     in_handle;
  
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (SP_IS_DOCK_ITEM (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);
    
    item = SP_DOCK_ITEM (widget);
    
    if (!(event->window == widget->window ||
          EVENT_IN_TABLABEL_EVENT_WINDOW (event, item->_priv->tab_label)))
        return FALSE;
    
    /* Verify that the item is not locked. */
    if (!SP_DOCK_ITEM_NOT_LOCKED (item))
        return FALSE;

    event_handled = FALSE;

    /* Check if user clicked on the drag handle. */      
    switch (item->orientation) {
    case GTK_ORIENTATION_HORIZONTAL:
        if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
            in_handle = event->x > widget->allocation.width - item->_priv->grip_size;
        else
            in_handle = event->x < item->_priv->grip_size;
        break;
    case GTK_ORIENTATION_VERTICAL:
        in_handle = event->y < item->_priv->grip_size;
        break;
    default:
        in_handle = FALSE;
        break;
    }

    /* Left mousebutton click on dockitem. */
    if (event->button == 1 && event->type == GDK_BUTTON_PRESS) {
        /* Set in_drag flag, grab pointer and call begin drag operation. */      
        if (in_handle) {
            item->_priv->start_x = event->x;
            item->_priv->start_y = event->y;

            SP_DOCK_ITEM_SET_FLAGS (item, SP_DOCK_IN_PREDRAG);
        
            event_handled = TRUE;
        };
        
    } else if (event->type == GDK_BUTTON_RELEASE && event->button == 1) {
        if (SP_DOCK_ITEM_IN_DRAG (item)) {
            /* User dropped widget somewhere. */
            sp_dock_item_drag_end (item, FALSE);
            event_handled = TRUE;
        }
        else if (SP_DOCK_ITEM_IN_PREDRAG (item)) {
            SP_DOCK_ITEM_UNSET_FLAGS (item, SP_DOCK_IN_PREDRAG);
            event_handled = TRUE;
        }
             
    } else if (event->button == 3 && event->type == GDK_BUTTON_PRESS && in_handle) {
        sp_dock_item_popup_menu (item, event->button, event->time);
        event_handled = TRUE;    	
    }

    return event_handled;
}

static gint
sp_dock_item_motion (GtkWidget      *widget,
                      GdkEventMotion *event)
{
    SPDockItem *item;
    gint         new_x, new_y;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (SP_IS_DOCK_ITEM (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    item = SP_DOCK_ITEM (widget);

    if (!(event->window == widget->window ||
          EVENT_IN_TABLABEL_EVENT_WINDOW (event, item->_priv->tab_label)))
        return FALSE;

    if (SP_DOCK_ITEM_IN_PREDRAG (item)) {
        if (gtk_drag_check_threshold (widget,
                                      item->_priv->start_x,
                                      item->_priv->start_y,
                                      event->x,
                                      event->y)) {
            SP_DOCK_ITEM_UNSET_FLAGS (item, SP_DOCK_IN_PREDRAG);
            item->dragoff_x = item->_priv->start_x;
            item->dragoff_y = item->_priv->start_y;

            sp_dock_item_drag_start (item);
        }
    }
    
    if (!SP_DOCK_ITEM_IN_DRAG (item))
        return FALSE;

    new_x = event->x_root;
    new_y = event->y_root;
    
    g_signal_emit (item, sp_dock_item_signals [DOCK_DRAG_MOTION], 
                   0, new_x, new_y);

    return TRUE;
}

static gboolean
sp_dock_item_key_press (GtkWidget   *widget,
                         GdkEventKey *event)
{
    gboolean event_handled = FALSE;
    
    if (SP_DOCK_ITEM_IN_DRAG (widget)) {
        if (event->keyval == GDK_Escape) {
            sp_dock_item_drag_end (SP_DOCK_ITEM (widget), TRUE);
            event_handled = TRUE;
        }
    }

    if (event_handled)
        return TRUE;
    else
        return SP_CALL_PARENT_WITH_DEFAULT (GTK_WIDGET_CLASS,
                                               key_press_event,
                                               (widget, event),
                                               FALSE);
}

static gboolean
sp_dock_item_dock_request (SPDockObject  *object,
                            gint            x,
                            gint            y,
                            SPDockRequest *request)
{
    GtkAllocation *alloc;
    gint           rel_x, rel_y;

    /* we get (x,y) in our allocation coordinates system */
    
    /* Get item's allocation. */
    alloc = &(GTK_WIDGET (object)->allocation);
    
    /* Get coordinates relative to our window. */
    rel_x = x - alloc->x;
    rel_y = y - alloc->y;

    /* Location is inside. */
    if (rel_x > 0 && rel_x < alloc->width &&
        rel_y > 0 && rel_y < alloc->height) {
        float rx, ry;
        GtkRequisition my, other;
        gint divider = -1;
        
        /* this are for calculating the extra docking parameter */
        sp_dock_item_preferred_size (SP_DOCK_ITEM (request->applicant), &other);
        sp_dock_item_preferred_size (SP_DOCK_ITEM (object), &my);
        
        /* Calculate location in terms of the available space (0-100%). */
        rx = (float) rel_x / alloc->width;
        ry = (float) rel_y / alloc->height;

        /* Determine dock location. */
        if (rx < SPLIT_RATIO) {
            request->position = SP_DOCK_LEFT;
            divider = other.width;
        }
        else if (rx > (1 - SPLIT_RATIO)) {
            request->position = SP_DOCK_RIGHT;
            rx = 1 - rx;
            divider = MAX (0, my.width - other.width);
        }
        else if (ry < SPLIT_RATIO && ry < rx) {
            request->position = SP_DOCK_TOP;
            divider = other.height;
        }
        else if (ry > (1 - SPLIT_RATIO) && (1 - ry) < rx) {
            request->position = SP_DOCK_BOTTOM;
            divider = MAX (0, my.height - other.height);
        }
        else
            request->position = SP_DOCK_CENTER;

        /* Reset rectangle coordinates to entire item. */
        request->rect.x = 0;
        request->rect.y = 0;
        request->rect.width = alloc->width;
        request->rect.height = alloc->height;

        /* Calculate docking indicator rectangle size for new locations. Only
           do this when we're not over the item's current location. */
        if (request->applicant != object) {
            switch (request->position) {
                case SP_DOCK_TOP:
                    request->rect.height *= SPLIT_RATIO;
                    break;
                case SP_DOCK_BOTTOM:
                    request->rect.y += request->rect.height * (1 - SPLIT_RATIO);
                    request->rect.height *= SPLIT_RATIO;
                    break;
                case SP_DOCK_LEFT:
                    request->rect.width *= SPLIT_RATIO;
                    break;
                case SP_DOCK_RIGHT:
                    request->rect.x += request->rect.width * (1 - SPLIT_RATIO);
                    request->rect.width *= SPLIT_RATIO;
                    break;
                case SP_DOCK_CENTER:
                    request->rect.x = request->rect.width * SPLIT_RATIO/2;
                    request->rect.y = request->rect.height * SPLIT_RATIO/2;
                    request->rect.width = (request->rect.width *
                                           (1 - SPLIT_RATIO/2)) - request->rect.x;
                    request->rect.height = (request->rect.height *
                                            (1 - SPLIT_RATIO/2)) - request->rect.y;
                    break;
                default:
                    break;
            }
        }

        /* adjust returned coordinates so they are have the same
           origin as our window */
        request->rect.x += alloc->x;
        request->rect.y += alloc->y;
        
        /* Set possible target location and return TRUE. */            
        request->target = object;

        /* fill-in other dock information */
        if (request->position != SP_DOCK_CENTER && divider >= 0) {
            if (G_IS_VALUE (&request->extra))
                g_value_unset (&request->extra);
            g_value_init (&request->extra, G_TYPE_UINT);
            g_value_set_uint (&request->extra, (guint) divider);
        }
        
        return TRUE;         
    }
    else /* No docking possible at this location. */            
        return FALSE;
}

static void
sp_dock_item_dock (SPDockObject    *object,
                    SPDockObject    *requestor,
                    SPDockPlacement  position,
                    GValue           *other_data)
{
    SPDockObject *new_parent, *parent;
    gboolean       add_ourselves_first;
    
    parent = sp_dock_object_get_parent_object (object);

    switch (position) {
        case SP_DOCK_TOP:
        case SP_DOCK_BOTTOM:
            /* get a paned style dock object */
            new_parent = g_object_new (sp_dock_object_type_from_nick ("paned"),
                                       "orientation", GTK_ORIENTATION_VERTICAL,
                                       NULL);
            add_ourselves_first = (position == SP_DOCK_BOTTOM);
            break;
        case SP_DOCK_LEFT:
        case SP_DOCK_RIGHT:
            new_parent = g_object_new (sp_dock_object_type_from_nick ("paned"),
                                       "orientation", GTK_ORIENTATION_HORIZONTAL,
                                       NULL);
            add_ourselves_first = (position == SP_DOCK_RIGHT);
            break;
        case SP_DOCK_CENTER:
            new_parent = g_object_new (sp_dock_object_type_from_nick ("notebook"),
                                       NULL);
            add_ourselves_first = TRUE;
            break;
        default: 
        {
            GEnumClass *enum_class = G_ENUM_CLASS (g_type_class_ref (SP_TYPE_DOCK_PLACEMENT));
            GEnumValue *enum_value = g_enum_get_value (enum_class, position);
            gchar *name = enum_value ? enum_value->value_name : NULL;
            
            g_warning (_("Unsupported docking strategy %s in dock object of type %s"),
                       name,  G_OBJECT_TYPE_NAME (object));
            g_type_class_unref (enum_class);
            return;
        }
    }

    /* freeze the parent so it doesn't reduce automatically */
    if (parent)
        sp_dock_object_freeze (parent);

    /* ref ourselves since we could be destroyed when detached */
    g_object_ref (object);
    SP_DOCK_OBJECT_SET_FLAGS (object, SP_DOCK_IN_REFLOW);
    sp_dock_object_detach (object, FALSE);

    /* freeze the new parent, so reduce won't get called before it's
       actually added to our parent */
    sp_dock_object_freeze (new_parent);
    
    /* bind the new parent to our master, so the following adds work */
    sp_dock_object_bind (new_parent, G_OBJECT (SP_DOCK_OBJECT_GET_MASTER (object)));
    
    /* add the objects */
    if (add_ourselves_first) {
        gtk_container_add (GTK_CONTAINER (new_parent), GTK_WIDGET (object));
        gtk_container_add (GTK_CONTAINER (new_parent), GTK_WIDGET (requestor));
    } else {
        gtk_container_add (GTK_CONTAINER (new_parent), GTK_WIDGET (requestor));
        gtk_container_add (GTK_CONTAINER (new_parent), GTK_WIDGET (object));
    }

    /* add the new parent to the parent */
    if (parent)
        gtk_container_add (GTK_CONTAINER (parent), GTK_WIDGET (new_parent));

    /* show automatic object */
    if (GTK_WIDGET_VISIBLE (object))
        gtk_widget_show (GTK_WIDGET (new_parent));
    
    /* use extra docking parameter */
    if (position != SP_DOCK_CENTER && other_data &&
        G_VALUE_HOLDS (other_data, G_TYPE_UINT)) {
        
        g_object_set (G_OBJECT (new_parent),
                      "position", g_value_get_uint (other_data),
                      NULL);
    }
    
    SP_DOCK_OBJECT_UNSET_FLAGS (object, SP_DOCK_IN_REFLOW);
    g_object_unref (object);

    sp_dock_object_thaw (new_parent);
    if (parent)
        sp_dock_object_thaw (parent);
}

static void
sp_dock_item_detach_menu (GtkWidget *widget,
                           GtkMenu   *menu)
{
    SPDockItem *item;
   
    item = SP_DOCK_ITEM (widget);
    item->_priv->menu = NULL;
}

static void
sp_dock_item_popup_menu (SPDockItem  *item, 
                          guint         button,
                          guint32       time)
{
    GtkWidget *mitem;

    if (!item->_priv->menu) {
        /* Create popup menu and attach it to the dock item */
        item->_priv->menu = gtk_menu_new ();
        gtk_menu_attach_to_widget (GTK_MENU (item->_priv->menu),
                                   GTK_WIDGET (item),
                                   sp_dock_item_detach_menu);
        
        /* Hide menuitem. */
        mitem = gtk_menu_item_new_with_label (_("Hide"));
        gtk_menu_shell_append (GTK_MENU_SHELL (item->_priv->menu), mitem);
        g_signal_connect (mitem, "activate", 
                          G_CALLBACK (sp_dock_item_hide_cb), item);

        /* Lock menuitem */
        mitem = gtk_menu_item_new_with_label (_("Lock"));
        gtk_menu_shell_append (GTK_MENU_SHELL (item->_priv->menu), mitem);
        g_signal_connect (mitem, "activate",
                          G_CALLBACK (sp_dock_item_lock_cb), item);

    }

    /* Show popup menu. */
    gtk_widget_show_all (item->_priv->menu);
    gtk_menu_popup (GTK_MENU (item->_priv->menu), NULL, NULL, NULL, NULL, 
                    button, time);
}

static void
sp_dock_item_drag_start (SPDockItem *item)
{
    GdkCursor *fleur;

    if (!GTK_WIDGET_REALIZED (item))
        gtk_widget_realize (GTK_WIDGET (item));
    
    SP_DOCK_ITEM_SET_FLAGS (item, SP_DOCK_IN_DRAG);
            
    /* grab the pointer so we receive all mouse events */
    fleur = gdk_cursor_new (GDK_FLEUR);

    /* grab the keyboard & pointer */
    gtk_grab_add (GTK_WIDGET (item));
    
    gdk_cursor_unref (fleur);
            
    g_signal_emit (item, sp_dock_item_signals [DOCK_DRAG_BEGIN], 0);
}

static void
sp_dock_item_drag_end (SPDockItem *item,
                        gboolean     cancel)
{
    /* Release pointer & keyboard. */
    gtk_grab_remove (gtk_grab_get_current ());
    
    g_signal_emit (item, sp_dock_item_signals [DOCK_DRAG_END], 0, cancel);
    
    SP_DOCK_ITEM_UNSET_FLAGS (item, SP_DOCK_IN_DRAG);
}

static void 
sp_dock_item_tab_button (GtkWidget      *widget,
                          GdkEventButton *event,
                          gpointer        data)
{
    SPDockItem *item;

    item = SP_DOCK_ITEM (data);

    if (!SP_DOCK_ITEM_NOT_LOCKED (item))
        return;

    switch (event->button) {
    case 1:
        /* set dragoff_{x,y} as we the user clicked on the middle of the 
           drag handle */
        switch (item->orientation) {
        case GTK_ORIENTATION_HORIZONTAL:
            item->dragoff_x = item->_priv->grip_size / 2;
            item->dragoff_y = GTK_WIDGET (data)->allocation.height / 2;
            break;
        case GTK_ORIENTATION_VERTICAL:
            item->dragoff_x = GTK_WIDGET (data)->allocation.width / 2;
            item->dragoff_y = item->_priv->grip_size / 2;
            break;
        };
        sp_dock_item_drag_start (item);
        break;

    case 3:
        sp_dock_item_popup_menu (item, event->button, event->time);
        break;

    default:
        break;
    };
}

static void
sp_dock_item_hide_cb (GtkWidget   *widget, 
                       SPDockItem *item)
{
    SPDockMaster *master;
    
    g_return_if_fail (item != NULL);

    master = SP_DOCK_OBJECT_GET_MASTER (item);
    sp_dock_item_hide_item (item);
}

static void
sp_dock_item_lock_cb (GtkWidget   *widget,
                       SPDockItem *item)
{
    g_return_if_fail (item != NULL);

    sp_dock_item_lock (item);
}

static void
sp_dock_item_showhide_grip (SPDockItem *item)
{
    if (item->_priv->grip) {
        if (SP_DOCK_ITEM_GRIP_SHOWN (item)) {
            gtk_widget_show (item->_priv->grip);
            GTK_WIDGET_SET_FLAGS (item->_priv->grip, GTK_CAN_FOCUS);
        } else {
            gtk_widget_hide (item->_priv->grip);
            GTK_WIDGET_UNSET_FLAGS (item->_priv->grip, GTK_CAN_FOCUS);
        }
    }
    gtk_widget_queue_resize (GTK_WIDGET (item));
}

static void
sp_dock_item_real_set_orientation (SPDockItem    *item,
                                    GtkOrientation  orientation)
{
    item->orientation = orientation;
    
    if (GTK_WIDGET_DRAWABLE (item))
        gtk_widget_queue_draw (GTK_WIDGET (item));
    gtk_widget_queue_resize (GTK_WIDGET (item));
}


/* ----- Public interface ----- */

GtkWidget *
sp_dock_item_new (const gchar         *name,
                   const gchar         *long_name,
                   SPDockItemBehavior  behavior)
{
    SPDockItem *item;

    item = SP_DOCK_ITEM (g_object_new (SP_TYPE_DOCK_ITEM, 
                                        "name", name, 
                                        "long_name", long_name,
                                        "behavior", behavior,
                                        NULL));
    SP_DOCK_OBJECT_UNSET_FLAGS (item, SP_DOCK_AUTOMATIC);
    sp_dock_item_set_tablabel (item, sp_dock_tablabel_new (item));

    return GTK_WIDGET (item);
}

/* convenient function (and to preserve source compat) */
void
sp_dock_item_dock_to (SPDockItem      *item,
                       SPDockItem      *target,
                       SPDockPlacement  position,
                       gint              docking_param)
{
    g_return_if_fail (item != NULL);
    g_return_if_fail (item != target);
    g_return_if_fail (target != NULL || position == SP_DOCK_FLOATING);
    
    if (position == SP_DOCK_FLOATING || !target) {
        SPDockObject *controller;
        
        if (!sp_dock_object_is_bound (SP_DOCK_OBJECT (item))) {
            g_warning (_("Attempt to bind an unbound item %p"), item);
            return;
        }

        controller = sp_dock_master_get_controller (SP_DOCK_OBJECT_GET_MASTER (item));
        
        /* FIXME: save previous docking position for later
           re-docking... does this make sense now? */

        /* Create new floating dock for widget. */
        item->dragoff_x = item->dragoff_y = 0;
        sp_dock_add_floating_item (SP_DOCK (controller),
                                    item, 0, 0, -1, -1);

    } else
        sp_dock_object_dock (SP_DOCK_OBJECT (target),
                              SP_DOCK_OBJECT (item),
                              position, NULL);
}

void
sp_dock_item_set_orientation (SPDockItem    *item,
                               GtkOrientation  orientation)
{
    GParamSpec *pspec;

    g_return_if_fail (item != NULL);

    if (item->orientation != orientation) {
        /* push the property down the hierarchy if our child supports it */
        if (item->child != NULL) {
            pspec = g_object_class_find_property (
                G_OBJECT_GET_CLASS (item->child), "orientation");
            if (pspec && pspec->value_type == GTK_TYPE_ORIENTATION)
                g_object_set (G_OBJECT (item->child),
                              "orientation", orientation,
                              NULL);
        };

        SP_CALL_VIRTUAL (item, SP_DOCK_ITEM_GET_CLASS, set_orientation, (item, orientation));
        g_object_notify (G_OBJECT (item), "orientation");
    }
}

GtkWidget *
sp_dock_item_get_tablabel (SPDockItem *item)
{
    g_return_val_if_fail (item != NULL, NULL);
    g_return_val_if_fail (SP_IS_DOCK_ITEM (item), NULL);

    return item->_priv->tab_label;
}

void
sp_dock_item_set_tablabel (SPDockItem *item,
                            GtkWidget   *tablabel)
{
    g_return_if_fail (item != NULL);

    if (item->_priv->tab_label) {
        /* disconnect and unref the previous tablabel */
        if (SP_IS_DOCK_TABLABEL (item->_priv->tab_label)) {
            g_signal_handlers_disconnect_matched (item->_priv->tab_label,
                                                  G_SIGNAL_MATCH_DATA,
                                                  0, 0, NULL,
                                                  NULL, item);
            g_object_set (item->_priv->tab_label, "item", NULL, NULL);
        }
        gtk_widget_unref (item->_priv->tab_label);
        item->_priv->tab_label = NULL;
    }
    
    if (tablabel) {
        gtk_widget_ref (tablabel);
        gtk_object_sink (GTK_OBJECT (tablabel));
        item->_priv->tab_label = tablabel;
        if (SP_IS_DOCK_TABLABEL (tablabel)) {
            g_object_set (tablabel, "item", item, NULL);
            /* connect to tablabel signal */
            g_signal_connect (tablabel, "button_pressed_handle",
                              G_CALLBACK (sp_dock_item_tab_button), item);
        }
    }
}

void 
sp_dock_item_hide_grip (SPDockItem *item)
{
    g_return_if_fail (item != NULL);
    if (item->_priv->grip_shown) {
        item->_priv->grip_shown = FALSE;
        sp_dock_item_showhide_grip (item);
    };
}

void
sp_dock_item_show_grip (SPDockItem *item)
{
    g_return_if_fail (item != NULL);
    if (!item->_priv->grip_shown) {
        item->_priv->grip_shown = TRUE;
        sp_dock_item_showhide_grip (item);
    };
}

/* convenient function (and to preserve source compat) */
void
sp_dock_item_bind (SPDockItem *item,
                    GtkWidget   *dock)
{
    g_return_if_fail (item != NULL);
    g_return_if_fail (dock == NULL || SP_IS_DOCK (dock));
    
    sp_dock_object_bind (SP_DOCK_OBJECT (item),
                          G_OBJECT (SP_DOCK_OBJECT_GET_MASTER (dock)));
}

/* convenient function (and to preserve source compat) */
void
sp_dock_item_unbind (SPDockItem *item)
{
    g_return_if_fail (item != NULL);

    sp_dock_object_unbind (SP_DOCK_OBJECT (item));
}

void
sp_dock_item_hide_item (SPDockItem *item)
{
    g_return_if_fail (item != NULL);

    if (!SP_DOCK_OBJECT_ATTACHED (item))
        /* already hidden/detached */
        return;
    
    /* if the object is manual, create a new placeholder to be able to
       restore the position later */
    if (!SP_DOCK_OBJECT_AUTOMATIC (item)) {
        if (item->_priv->ph)
            g_object_unref (item->_priv->ph);
        
        item->_priv->ph = SP_DOCK_PLACEHOLDER (
            g_object_new (SP_TYPE_DOCK_PLACEHOLDER,
                          "sticky", FALSE,
                          "host", item,
                          NULL));
        g_object_ref (item->_priv->ph);
        gtk_object_sink (GTK_OBJECT (item->_priv->ph));
    }
    
    sp_dock_object_freeze (SP_DOCK_OBJECT (item));
    
    /* hide our children first, so they can also set placeholders */
    if (sp_dock_object_is_compound (SP_DOCK_OBJECT (item))) 
        gtk_container_foreach (GTK_CONTAINER (item),
                               (GtkCallback) sp_dock_item_hide_item,
                               NULL);
    
    /* detach the item recursively */
    sp_dock_object_detach (SP_DOCK_OBJECT (item), TRUE);

    sp_dock_object_thaw (SP_DOCK_OBJECT (item));
}

void
sp_dock_item_show_item (SPDockItem *item)
{
    g_return_if_fail (item != NULL);

    if (item->_priv->ph) {
        gtk_container_add (GTK_CONTAINER (item->_priv->ph), GTK_WIDGET (item));
        g_object_unref (item->_priv->ph);
        item->_priv->ph = NULL;
    }
    else if (sp_dock_object_is_bound (SP_DOCK_OBJECT (item))) {
        SPDockObject *toplevel = sp_dock_master_get_controller (
            SP_DOCK_OBJECT_GET_MASTER (item));
        if (toplevel) {
            sp_dock_object_dock (toplevel, SP_DOCK_OBJECT (item),
                                  SP_DOCK_FLOATING, NULL);
        }
    }
}

void
sp_dock_item_lock (SPDockItem *item)
{
    g_object_set (item, "locked", TRUE, NULL);
}

void
sp_dock_item_unlock (SPDockItem *item)
{
    g_object_set (item, "locked", FALSE, NULL);
}

void 
sp_dock_item_set_default_position (SPDockItem   *item,
                                    SPDockObject *reference)
{
    g_return_if_fail (item != NULL);

    if (item->_priv->ph) {
        g_object_unref (item->_priv->ph);
        item->_priv->ph = NULL;
    }

    if (reference && SP_DOCK_OBJECT_ATTACHED (reference)) {
        if (SP_IS_DOCK_PLACEHOLDER (reference)) {
            g_object_ref (reference);
            gtk_object_sink (GTK_OBJECT (reference));
            item->_priv->ph = SP_DOCK_PLACEHOLDER (reference);
        }
        else {
            item->_priv->ph = SP_DOCK_PLACEHOLDER (
                g_object_new (SP_TYPE_DOCK_PLACEHOLDER,
                              "sticky", TRUE,
                              "host", reference,
                              NULL));
            g_object_ref (item->_priv->ph);
            gtk_object_sink (GTK_OBJECT (item->_priv->ph));
        }
    }
}

void 
sp_dock_item_preferred_size (SPDockItem    *item,
                              GtkRequisition *req)
{
    if (!req)
        return;

    req->width = MAX (item->_priv->preferred_width,
                      GTK_WIDGET (item)->allocation.width);
    req->height = MAX (item->_priv->preferred_height,
                       GTK_WIDGET (item)->allocation.height);
}


/* ----- gtk orientation type exporter/importer ----- */

static void 
sp_dock_param_export_gtk_orientation (const GValue *src,
                                       GValue       *dst)
{
    dst->data [0].v_pointer =
        g_strdup_printf ("%s", (src->data [0].v_int == GTK_ORIENTATION_HORIZONTAL) ?
                         "horizontal" : "vertical");
}

static void 
sp_dock_param_import_gtk_orientation (const GValue *src,
                                       GValue       *dst)
{
    if (!strcmp (src->data [0].v_pointer, "horizontal"))
        dst->data [0].v_int = GTK_ORIENTATION_HORIZONTAL;
    else
        dst->data [0].v_int = GTK_ORIENTATION_VERTICAL;
}

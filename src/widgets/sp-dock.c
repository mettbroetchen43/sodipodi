/*
 * Copyright (C) 2002 Gustavo Giráldez <gustavo.giraldez@gmx.net>
 * Copyright (C) 2003 Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <helper/sp-intl.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdkx.h>
#include "sp-dock-macros.h"

#include "sp-dock.h"
#include "sp-dock-master.h"
#include "sp-dock-paned.h"
#include "sp-dock-notebook.h"
#include "sp-dock-placeholder.h"

#include "shortcuts.h"

#include <helper/sp-marshal.h>


/* ----- Private prototypes ----- */

static void  sp_dock_class_init      (SPDockClass *class);
static void  sp_dock_instance_init   (SPDock *dock);

static GObject *sp_dock_constructor  (GType                  type,
                                       guint                  n_construct_properties,
                                       GObjectConstructParam *construct_param);
static void  sp_dock_set_property    (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);
static void  sp_dock_get_property    (GObject      *object,
                                       guint         prop_id,
                                       GValue       *value,
                                       GParamSpec   *pspec);
static void  sp_dock_notify_cb       (GObject      *object,
                                       GParamSpec   *pspec,
                                       gpointer      user_data);

static void  sp_dock_set_title       (SPDock      *dock);

static void  sp_dock_destroy         (GtkObject    *object);

static void  sp_dock_size_request    (GtkWidget      *widget,
                                       GtkRequisition *requisition);
static void  sp_dock_size_allocate   (GtkWidget      *widget,
                                       GtkAllocation  *allocation);
static void  sp_dock_map             (GtkWidget      *widget);
static void  sp_dock_unmap           (GtkWidget      *widget);
static void  sp_dock_show            (GtkWidget      *widget);
static void  sp_dock_hide            (GtkWidget      *widget);

static void  sp_dock_add             (GtkContainer *container,
                                       GtkWidget    *widget);
static void  sp_dock_remove          (GtkContainer *container,
                                       GtkWidget    *widget);
static void  sp_dock_forall          (GtkContainer *container,
                                       gboolean      include_internals,
                                       GtkCallback   callback,
                                       gpointer      callback_data);
static GtkType  sp_dock_child_type   (GtkContainer *container);

static void     sp_dock_detach       (SPDockObject    *object,
                                       gboolean          recursive);
static void     sp_dock_reduce       (SPDockObject    *object);
static gboolean sp_dock_dock_request (SPDockObject    *object,
                                       gint              x,
                                       gint              y,
                                       SPDockRequest   *request);
static void     sp_dock_dock         (SPDockObject    *object,
                                       SPDockObject    *requestor,
                                       SPDockPlacement  position,
                                       GValue           *other_data);
static gboolean sp_dock_reorder      (SPDockObject    *object,
                                       SPDockObject    *requestor,
                                       SPDockPlacement  new_position,
                                       GValue           *other_data);

static gboolean sp_dock_floating_window_delete_event_cb (GtkWidget *widget);

static gboolean sp_dock_child_placement  (SPDockObject    *object,
                                           SPDockObject    *child,
                                           SPDockPlacement *placement);

static void     sp_dock_present          (SPDockObject    *object,
                                           SPDockObject    *child);


/* ----- Class variables and definitions ----- */

struct _SPDockPrivate
{
    /* for floating docks */
    gboolean            floating;
    GtkWidget          *window;
    gboolean            auto_title;
    
    gint                float_x;
    gint                float_y;
    gint                width;
    gint                height;
    
    /* auxiliary fields */
    GdkGC              *xor_gc;
};

enum {
    LAYOUT_CHANGED,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_FLOATING,
    PROP_DEFAULT_TITLE,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_FLOAT_X,
    PROP_FLOAT_Y
};

static guint dock_signals [LAST_SIGNAL] = { 0 };

#define SPLIT_RATIO  0.3


/* ----- Private functions ----- */

SP_CLASS_BOILERPLATE (SPDock, sp_dock, SPDockObject, SP_TYPE_DOCK_OBJECT);

static void
sp_dock_class_init (SPDockClass *klass)
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
    
    g_object_class->constructor = sp_dock_constructor;
    g_object_class->set_property = sp_dock_set_property;
    g_object_class->get_property = sp_dock_get_property;
    
    /* properties */

    g_object_class_install_property (
        g_object_class, PROP_FLOATING,
        g_param_spec_boolean ("floating", _("Floating"),
                              _("Whether the dock is floating in its own window"),
                              FALSE,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              SP_DOCK_PARAM_EXPORT));
    
    g_object_class_install_property (
        g_object_class, PROP_DEFAULT_TITLE,
        g_param_spec_string ("default_title", _("Default title"),
                             _("Default title for the newly created floating docks"),
                             NULL,
                             G_PARAM_READWRITE));
    
    g_object_class_install_property (
        g_object_class, PROP_WIDTH,
        g_param_spec_int ("width", _("Width"),
                          _("Width for the dock when it's of floating type"),
                          -1, G_MAXINT, -1,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          SP_DOCK_PARAM_EXPORT));
    
    g_object_class_install_property (
        g_object_class, PROP_HEIGHT,
        g_param_spec_int ("height", _("Height"),
                          _("Height for the dock when it's of floating type"),
                          -1, G_MAXINT, -1,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          SP_DOCK_PARAM_EXPORT));
    
    g_object_class_install_property (
        g_object_class, PROP_FLOAT_X,
        g_param_spec_int ("floatx", _("Float X"),
                          _("X coordinate for a floating dock"),
                          G_MININT, G_MAXINT, 0,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          SP_DOCK_PARAM_EXPORT));
    
    g_object_class_install_property (
        g_object_class, PROP_FLOAT_Y,
        g_param_spec_int ("floaty", _("Float Y"),
                          _("Y coordinate for a floating dock"),
                          G_MININT, G_MAXINT, 0,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          SP_DOCK_PARAM_EXPORT));
    
    gtk_object_class->destroy = sp_dock_destroy;

    widget_class->size_request = sp_dock_size_request;
    widget_class->size_allocate = sp_dock_size_allocate;
    widget_class->map = sp_dock_map;
    widget_class->unmap = sp_dock_unmap;
    widget_class->show = sp_dock_show;
    widget_class->hide = sp_dock_hide;
    
    container_class->add = sp_dock_add;
    container_class->remove = sp_dock_remove;
    container_class->forall = sp_dock_forall;
    container_class->child_type = sp_dock_child_type;
    
    object_class->is_compound = TRUE;
    
    object_class->detach = sp_dock_detach;
    object_class->reduce = sp_dock_reduce;
    object_class->dock_request = sp_dock_dock_request;
    object_class->dock = sp_dock_dock;
    object_class->reorder = sp_dock_reorder;    
    object_class->child_placement = sp_dock_child_placement;
    object_class->present = sp_dock_present;
    
    /* signals */

    dock_signals [LAYOUT_CHANGED] = 
        g_signal_new ("layout_changed", 
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (SPDockClass, layout_changed),
                      NULL, /* accumulator */
                      NULL, /* accu_data */
                      sp_marshal_NONE__NONE,
                      G_TYPE_NONE, /* return type */
                      0);

    klass->layout_changed = NULL;
}

static void
sp_dock_instance_init (SPDock *dock)
{
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (dock), GTK_NO_WINDOW);

    dock->root = NULL;
    dock->_priv = g_new0 (SPDockPrivate, 1);
    dock->_priv->width = -1;
    dock->_priv->height = -1;
}

static gboolean 
sp_dock_floating_configure_event_cb (GtkWidget         *widget,
                                      GdkEventConfigure *event,
                                      gpointer           user_data)
{
    SPDock *dock;
    
    g_return_val_if_fail (user_data != NULL && SP_IS_DOCK (user_data), TRUE);

    dock = SP_DOCK (user_data);
    dock->_priv->float_x = event->x;
    dock->_priv->float_y = event->y;
    dock->_priv->width = event->width;
    dock->_priv->height = event->height;

    return FALSE;
}

/* fixme: integrate this with sp-window helper */
static int
sp_dock_window_key_press (GtkWidget *widgt, GdkEventKey *event)
{
	unsigned int shortcut;
	shortcut = event->keyval;
	if (event->state & GDK_SHIFT_MASK) shortcut |= SP_SHORTCUT_SHIFT_MASK;
	if (event->state & GDK_CONTROL_MASK) shortcut |= SP_SHORTCUT_CONTROL_MASK;
	if (event->state & GDK_MOD1_MASK) shortcut |= SP_SHORTCUT_ALT_MASK;
	return sp_shortcut_run (shortcut);
}


static GObject *
sp_dock_constructor (GType                  type,
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
        SPDock *dock = SP_DOCK (g_object);
        SPDockMaster *master;
        
        /* create a master for the dock if none was provided in the construction */
        master = SP_DOCK_OBJECT_GET_MASTER (SP_DOCK_OBJECT (dock));
        if (!master) {
            SP_DOCK_OBJECT_UNSET_FLAGS (dock, SP_DOCK_AUTOMATIC);
            master = g_object_new (SP_TYPE_DOCK_MASTER, NULL);
            /* the controller owns the master ref */
            sp_dock_object_bind (SP_DOCK_OBJECT (dock), G_OBJECT (master));
        }

        if (dock->_priv->floating) {
            SPDockObject *controller;
            
            /* create floating window for this dock */
            dock->_priv->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
            g_object_set_data (G_OBJECT (dock->_priv->window), "dock", dock);
            
            /* set position and default size */
            gtk_window_set_position (GTK_WINDOW (dock->_priv->window),
                                     GTK_WIN_POS_MOUSE);
            gtk_window_set_default_size (GTK_WINDOW (dock->_priv->window),
                                         dock->_priv->width,
                                         dock->_priv->height);
            gtk_window_set_type_hint (GTK_WINDOW (dock->_priv->window),
                                      GDK_WINDOW_TYPE_HINT_NORMAL);
            
            /* metacity ignores this */
            gtk_window_move (GTK_WINDOW (dock->_priv->window),
                             dock->_priv->float_x,
                             dock->_priv->float_y);
            
            /* connect to the configure event so we can track down window geometry */
            g_signal_connect (dock->_priv->window, "configure_event",
                              (GCallback) sp_dock_floating_configure_event_cb,
                              dock);
            
            /* set the title and connect to the long_name notify queue
               so we can reset the title when this prop changes */
            sp_dock_set_title (dock);
            g_signal_connect (dock, "notify::long_name",
                              (GCallback) sp_dock_notify_cb, NULL);
            
			/* key press event */
			g_signal_connect_after ((GObject *) dock->_priv->window, "key_press_event", (GCallback) sp_dock_window_key_press, NULL);
			
            /* set transient for the first dock if that is a non-floating dock */
            controller = sp_dock_master_get_controller (master);
            if (controller && SP_IS_DOCK (controller)) {
                gboolean first_is_floating;
                g_object_get (controller, "floating", &first_is_floating, NULL);
                if (!first_is_floating) {
                    GtkWidget *toplevel =
                        gtk_widget_get_toplevel (GTK_WIDGET (controller));

                    if (GTK_IS_WINDOW (toplevel))
                        gtk_window_set_transient_for (GTK_WINDOW (dock->_priv->window),
                                                      GTK_WINDOW (toplevel));
                }
            }

            gtk_container_add (GTK_CONTAINER (dock->_priv->window), GTK_WIDGET (dock));
    
            g_signal_connect (dock->_priv->window, "delete_event",
                              G_CALLBACK (sp_dock_floating_window_delete_event_cb), 
                              NULL);
        }
        SP_DOCK_OBJECT_SET_FLAGS (dock, SP_DOCK_ATTACHED);
    }
    
    return g_object;
}

static void
sp_dock_set_property  (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
    SPDock *dock = SP_DOCK (object);
    
    switch (prop_id) {
        case PROP_FLOATING:
            dock->_priv->floating = g_value_get_boolean (value);
            break;
        case PROP_DEFAULT_TITLE:
            if (SP_DOCK_OBJECT (object)->master)
                g_object_set (SP_DOCK_OBJECT (object)->master,
                              "default_title", g_value_get_string (value),
                              NULL);
            break;
        case PROP_WIDTH:
            dock->_priv->width = g_value_get_int (value);
            break;
        case PROP_HEIGHT:
            dock->_priv->height = g_value_get_int (value);
            break;
        case PROP_FLOAT_X:
            dock->_priv->float_x = g_value_get_int (value);
            break;
        case PROP_FLOAT_Y:
            dock->_priv->float_y = g_value_get_int (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }

    switch (prop_id) {
        case PROP_WIDTH:
        case PROP_HEIGHT:
        case PROP_FLOAT_X:
        case PROP_FLOAT_Y:
            if (dock->_priv->floating && dock->_priv->window) {
                gtk_window_resize (GTK_WINDOW (dock->_priv->window),
                                   dock->_priv->width,
                                   dock->_priv->height);
            }
            break;
    }
}

static void
sp_dock_get_property  (GObject      *object,
                        guint         prop_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
    SPDock *dock = SP_DOCK (object);

    switch (prop_id) {
        case PROP_FLOATING:
            g_value_set_boolean (value, dock->_priv->floating);
            break;
        case PROP_DEFAULT_TITLE:
            if (SP_DOCK_OBJECT (object)->master) {
                gchar *default_title;
                g_object_get (SP_DOCK_OBJECT (object)->master,
                              "default_title", &default_title,
                              NULL);
                g_value_set_string_take_ownership (value, default_title);
            }
            else
                g_value_set_string (value, NULL);
            break;
        case PROP_WIDTH:
            g_value_set_int (value, dock->_priv->width);
            break;
        case PROP_HEIGHT:
            g_value_set_int (value, dock->_priv->height);
            break;
        case PROP_FLOAT_X:
            g_value_set_int (value, dock->_priv->float_x);
            break;
        case PROP_FLOAT_Y:
            g_value_set_int (value, dock->_priv->float_y);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
sp_dock_set_title (SPDock *dock)
{
    SPDockObject *object = SP_DOCK_OBJECT (dock);
    gchar         *title = NULL;
    gboolean       free_title = FALSE;
    
    if (!dock->_priv->window)
        return;
    
    if (!dock->_priv->auto_title && object->long_name) {
        title = object->long_name;
    }
    else if (object->master) {
        g_object_get (object->master, "default_title", &title, NULL);
        free_title = TRUE;
    }

    if (!title && dock->root) {
        g_object_get (dock->root, "long_name", &title, NULL);
        free_title = TRUE;
    }
    
    if (!title) {
        /* set a default title in the long_name */
        dock->_priv->auto_title = TRUE;
        free_title = FALSE;
        title = object->long_name = g_strdup_printf (
            _("Dock #%d"), SP_DOCK_MASTER (object->master)->dock_number++);
    }

    gtk_window_set_title (GTK_WINDOW (dock->_priv->window), title);
    if (free_title)
        g_free (title);
}

static void
sp_dock_notify_cb (GObject    *object,
                    GParamSpec *pspec,
                    gpointer    user_data)
{
    SPDock *dock;
    
    g_return_if_fail (object != NULL || SP_IS_DOCK (object));
    
    dock = SP_DOCK (object);
    dock->_priv->auto_title = FALSE;
    sp_dock_set_title (dock);
}

static void
sp_dock_destroy (GtkObject *object)
{
    SPDock *dock = SP_DOCK (object);

    if (dock->_priv) {
        SPDockPrivate *priv = dock->_priv;
        dock->_priv = NULL;

        if (priv->window) {
            gtk_widget_destroy (priv->window);
            priv->floating = FALSE;
            priv->window = NULL;
        }
        
        /* destroy the xor gc */
        if (priv->xor_gc) {
            g_object_unref (priv->xor_gc);
            priv->xor_gc = NULL;
        }

        g_free (priv);
    }
    
    SP_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
sp_dock_size_request (GtkWidget      *widget,
                       GtkRequisition *requisition)
{
    SPDock      *dock;
    GtkContainer *container;
    guint         border_width;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (SP_IS_DOCK (widget));

    dock = SP_DOCK (widget);
    container = GTK_CONTAINER (widget);
    border_width = container->border_width;

    /* make request to root */
    if (dock->root && GTK_WIDGET_VISIBLE (dock->root))
        gtk_widget_size_request (GTK_WIDGET (dock->root), requisition);
    else {
        requisition->width = 0;
        requisition->height = 0;
    };

    requisition->width += 2 * border_width;
    requisition->height += 2 * border_width;

    widget->requisition = *requisition;
}

static void
sp_dock_size_allocate (GtkWidget     *widget,
                        GtkAllocation *allocation)
{
    SPDock      *dock;
    GtkContainer *container;
    guint         border_width;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (SP_IS_DOCK (widget));
    
    dock = SP_DOCK (widget);
    container = GTK_CONTAINER (widget);
    border_width = container->border_width;

    widget->allocation = *allocation;

    /* reduce allocation by border width */
    allocation->x += border_width;
    allocation->y += border_width;
    allocation->width = MAX (1, allocation->width - 2 * border_width);
    allocation->height = MAX (1, allocation->height - 2 * border_width);

    if (dock->root && GTK_WIDGET_VISIBLE (dock->root))
        gtk_widget_size_allocate (GTK_WIDGET (dock->root), allocation);
}

static void
sp_dock_map (GtkWidget *widget)
{
    GtkWidget *child;
    SPDock   *dock;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (SP_IS_DOCK (widget));

    dock = SP_DOCK (widget);

    SP_CALL_PARENT (GTK_WIDGET_CLASS, map, (widget));

    if (dock->root) {
        child = GTK_WIDGET (dock->root);
        if (GTK_WIDGET_VISIBLE (child) && !GTK_WIDGET_MAPPED (child))
            gtk_widget_map (child);
    }
}

static void
sp_dock_unmap (GtkWidget *widget)
{
    GtkWidget *child;
    SPDock   *dock;
    
    g_return_if_fail (widget != NULL);
    g_return_if_fail (SP_IS_DOCK (widget));

    dock = SP_DOCK (widget);

    SP_CALL_PARENT (GTK_WIDGET_CLASS, unmap, (widget));

    if (dock->root) {
        child = GTK_WIDGET (dock->root);
        if (GTK_WIDGET_VISIBLE (child) && GTK_WIDGET_MAPPED (child))
            gtk_widget_unmap (child);
    }
    
    if (dock->_priv->window)
        gtk_widget_unmap (dock->_priv->window);
}

static void
sp_dock_foreach_automatic (SPDockObject *object,
                            gpointer       user_data)
{
    void (* function) (GtkWidget *) = user_data;

    if (SP_DOCK_OBJECT_AUTOMATIC (object))
        (* function) (GTK_WIDGET (object));
}

static void
sp_dock_show (GtkWidget *widget)
{
    SPDock *dock;
    
    g_return_if_fail (widget != NULL);
    g_return_if_fail (SP_IS_DOCK (widget));
    
    SP_CALL_PARENT (GTK_WIDGET_CLASS, show, (widget));
    
    dock = SP_DOCK (widget);
    if (dock->_priv->floating && dock->_priv->window)
        gtk_widget_show (dock->_priv->window);

    if (SP_DOCK_IS_CONTROLLER (dock)) {
        sp_dock_master_foreach_toplevel (SP_DOCK_OBJECT_GET_MASTER (dock),
                                          FALSE, (GFunc) sp_dock_foreach_automatic,
                                          gtk_widget_show);
    }
}

static void
sp_dock_hide (GtkWidget *widget)
{
    SPDock *dock;
    
    g_return_if_fail (widget != NULL);
    g_return_if_fail (SP_IS_DOCK (widget));
    
    SP_CALL_PARENT (GTK_WIDGET_CLASS, hide, (widget));
    
    dock = SP_DOCK (widget);
    if (dock->_priv->floating && dock->_priv->window)
        gtk_widget_hide (dock->_priv->window);

    if (SP_DOCK_IS_CONTROLLER (dock)) {
        sp_dock_master_foreach_toplevel (SP_DOCK_OBJECT_GET_MASTER (dock),
                                          FALSE, (GFunc) sp_dock_foreach_automatic,
                                          gtk_widget_hide);
    }
}

static void
sp_dock_add (GtkContainer *container,
              GtkWidget    *widget)
{
    g_return_if_fail (container != NULL);
    g_return_if_fail (SP_IS_DOCK (container));
    g_return_if_fail (SP_IS_DOCK_ITEM (widget));

    sp_dock_add_item (SP_DOCK (container), 
                       SP_DOCK_ITEM (widget), 
                       SP_DOCK_TOP);  /* default position */
}

static void
sp_dock_remove (GtkContainer *container,
                 GtkWidget    *widget)
{
    SPDock  *dock;
    gboolean  was_visible;

    g_return_if_fail (container != NULL);
    g_return_if_fail (widget != NULL);

    dock = SP_DOCK (container);
    was_visible = GTK_WIDGET_VISIBLE (widget);

    if (GTK_WIDGET (dock->root) == widget) {
        dock->root = NULL;
        SP_DOCK_OBJECT_UNSET_FLAGS (widget, SP_DOCK_ATTACHED);
        gtk_widget_unparent (widget);

        if (was_visible && GTK_WIDGET_VISIBLE (GTK_WIDGET (container)))
            gtk_widget_queue_resize (GTK_WIDGET (dock));
    }
}

static void
sp_dock_forall (GtkContainer *container,
                 gboolean      include_internals,
                 GtkCallback   callback,
                 gpointer      callback_data)
{
    SPDock *dock;

    g_return_if_fail (container != NULL);
    g_return_if_fail (SP_IS_DOCK (container));
    g_return_if_fail (callback != NULL);

    dock = SP_DOCK (container);

    if (dock->root)
        (*callback) (GTK_WIDGET (dock->root), callback_data);
}

static GtkType
sp_dock_child_type (GtkContainer *container)
{
    return SP_TYPE_DOCK_ITEM;
}

static void
sp_dock_detach (SPDockObject *object,
                 gboolean       recursive)
{
    SPDock *dock = SP_DOCK (object);
    
    /* detach children */
    if (recursive && dock->root) {
        sp_dock_object_detach (dock->root, recursive);
    }
    SP_DOCK_OBJECT_UNSET_FLAGS (object, SP_DOCK_ATTACHED);
}

static void
sp_dock_reduce (SPDockObject *object)
{
    SPDock *dock = SP_DOCK (object);
    
    if (dock->root)
        return;
    
    if (SP_DOCK_OBJECT_AUTOMATIC (dock)) {
        gtk_widget_destroy (GTK_WIDGET (dock));

    } else if (!SP_DOCK_OBJECT_ATTACHED (dock)) {
        /* if the user explicitly detached the object */
        if (dock->_priv->floating)
            gtk_widget_hide (GTK_WIDGET (dock));
        else {
            GtkWidget *widget = GTK_WIDGET (object);
            if (widget->parent) 
                gtk_container_remove (GTK_CONTAINER (widget->parent), widget);
        }
    }
}

static gboolean
sp_dock_dock_request (SPDockObject  *object,
                       gint            x,
                       gint            y,
                       SPDockRequest *request)
{
    SPDock            *dock;
    guint               bw;
    gint                rel_x, rel_y;
    GtkAllocation      *alloc;
    gboolean            may_dock = FALSE;
    SPDockRequest      my_request;

    g_return_val_if_fail (SP_IS_DOCK (object), FALSE);

    /* we get (x,y) in our allocation coordinates system */
    
    dock = SP_DOCK (object);
    
    /* Get dock size. */
    alloc = &(GTK_WIDGET (dock)->allocation);
    bw = GTK_CONTAINER (dock)->border_width;

    /* Get coordinates relative to our allocation area. */
    rel_x = x - alloc->x;
    rel_y = y - alloc->y;

    if (request)
        my_request = *request;
        
    /* Check if coordinates are in SPDock widget. */
    if (rel_x > 0 && rel_x < alloc->width &&
        rel_y > 0 && rel_y < alloc->height) {

        /* It's inside our area. */
        may_dock = TRUE;

	/* Set docking indicator rectangle to the SPDock size. */
        my_request.rect.x = alloc->x + bw;
        my_request.rect.y = alloc->y + bw;
        my_request.rect.width = alloc->width - 2*bw;
        my_request.rect.height = alloc->height - 2*bw;

	/* If SPDock has no root item yet, set the dock itself as 
	   possible target. */
        if (!dock->root) {
            my_request.position = SP_DOCK_TOP;
            my_request.target = object;
        } else {
            my_request.target = dock->root;

            /* See if it's in the border_width band. */
            if (rel_x < bw) {
                my_request.position = SP_DOCK_LEFT;
                my_request.rect.width *= SPLIT_RATIO;
            } else if (rel_x > alloc->width - bw) {
                my_request.position = SP_DOCK_RIGHT;
                my_request.rect.x += my_request.rect.width * (1 - SPLIT_RATIO);
                my_request.rect.width *= SPLIT_RATIO;
            } else if (rel_y < bw) {
                my_request.position = SP_DOCK_TOP;
                my_request.rect.height *= SPLIT_RATIO;
            } else if (rel_y > alloc->height - bw) {
                my_request.position = SP_DOCK_BOTTOM;
                my_request.rect.y += my_request.rect.height * (1 - SPLIT_RATIO);
                my_request.rect.height *= SPLIT_RATIO;
            } else {
                /* Otherwise try our children. */
                /* give them allocation coordinates (we are a
                   GTK_NO_WINDOW) widget */
                may_dock = sp_dock_object_dock_request (SP_DOCK_OBJECT (dock->root), 
                                                         x, y, &my_request);
            }
        }
    }

    if (may_dock && request)
        *request = my_request;
    
    return may_dock;
}

static void
sp_dock_dock (SPDockObject    *object,
               SPDockObject    *requestor,
               SPDockPlacement  position,
               GValue           *user_data)
{
    SPDock *dock;
    
    g_return_if_fail (SP_IS_DOCK (object));
    /* only dock items allowed at this time */
    g_return_if_fail (SP_IS_DOCK_ITEM (requestor));

    dock = SP_DOCK (object);
    
    if (position == SP_DOCK_FLOATING) {
        SPDockItem *item = SP_DOCK_ITEM (requestor);
        gint x, y, width, height;

        if (user_data && G_VALUE_HOLDS (user_data, GDK_TYPE_RECTANGLE)) {
            GdkRectangle *rect;

            rect = g_value_get_boxed (user_data);
            x = rect->x;
            y = rect->y;
            width = rect->width;
            height = rect->height;
        }
        else {
            x = y = 0;
            width = height = -1;
        }
        
        sp_dock_add_floating_item (dock, item,
                                    x, y, width, height);
    }
    else if (dock->root) {
        /* This is somewhat a special case since we know which item to
           pass the request on because we only have on child */
        sp_dock_object_dock (dock->root, requestor, position, NULL);
        sp_dock_set_title (dock);
        
    }
    else { /* Item about to be added is root item. */
        GtkWidget *widget = GTK_WIDGET (requestor);
        
        dock->root = requestor;
        SP_DOCK_OBJECT_SET_FLAGS (requestor, SP_DOCK_ATTACHED);
        gtk_widget_set_parent (widget, GTK_WIDGET (dock));
        
        sp_dock_item_show_grip (SP_DOCK_ITEM (requestor));

        /* Realize the item (create its corresponding GdkWindow) when 
           SPDock has been realized. */
        if (GTK_WIDGET_REALIZED (dock))
            gtk_widget_realize (widget);
        
        /* Map the widget if it's visible and the parent is visible and has 
           been mapped. This is done to make sure that the GdkWindow is 
           visible. */
        if (GTK_WIDGET_VISIBLE (dock) && 
            GTK_WIDGET_VISIBLE (widget)) {
            if (GTK_WIDGET_MAPPED (dock))
                gtk_widget_map (widget);
            
            /* Make the widget resize. */
            gtk_widget_queue_resize (widget);
        }
        sp_dock_set_title (dock);
    }
}
    
static gboolean
sp_dock_floating_window_delete_event_cb (GtkWidget *widget)
{
    SPDock *dock;
    
    g_return_val_if_fail (GTK_IS_WINDOW (widget), FALSE);
    
    dock = SP_DOCK (g_object_get_data (G_OBJECT (widget), "dock"));
    if (dock->root) {
        /* this will call reduce on ourselves, hiding the window if appropiate */
        sp_dock_item_hide_item (SP_DOCK_ITEM (dock->root));
    }

    return TRUE;
}

static void
_sp_dock_foreach_build_list (SPDockObject *object,
                              gpointer       user_data)
{
    GList **l = (GList **) user_data;

    if (SP_IS_DOCK_ITEM (object))
        *l = g_list_prepend (*l, object);
}

static gboolean
sp_dock_reorder (SPDockObject    *object,
                  SPDockObject    *requestor,
                  SPDockPlacement  new_position,
                  GValue           *other_data)
{
    SPDock *dock = SP_DOCK (object);
    gboolean handled = FALSE;
    
    if (dock->_priv->floating &&
        new_position == SP_DOCK_FLOATING &&
        dock->root == requestor) {
        
        if (other_data && G_VALUE_HOLDS (other_data, GDK_TYPE_RECTANGLE)) {
            GdkRectangle *rect;

            rect = g_value_get_boxed (other_data);
            gtk_window_move (GTK_WINDOW (dock->_priv->window),
                             rect->x,
                             rect->y);
            handled = TRUE;
        }
    }
    
    return handled;
}

static gboolean 
sp_dock_child_placement (SPDockObject    *object,
                          SPDockObject    *child,
                          SPDockPlacement *placement)
{
    SPDock *dock = SP_DOCK (object);
    gboolean retval = TRUE;
    
    if (dock->root == child) {
        if (placement) {
            if (*placement == SP_DOCK_NONE || *placement == SP_DOCK_FLOATING)
                *placement = SP_DOCK_TOP;
        }
    } else 
        retval = FALSE;

    return retval;
}

static void 
sp_dock_present (SPDockObject *object,
                  SPDockObject *child)
{
    SPDock *dock = SP_DOCK (object);

    if (dock->_priv->floating)
        gtk_window_present (GTK_WINDOW (dock->_priv->window));
}


/* ----- Public interface ----- */

GtkWidget *
sp_dock_new (void)
{
    GObject *dock;

    dock = g_object_new (SP_TYPE_DOCK, NULL);
    SP_DOCK_OBJECT_UNSET_FLAGS (dock, SP_DOCK_AUTOMATIC);
    
    return GTK_WIDGET (dock);
}

GtkWidget *
sp_dock_new_from (SPDock  *original,
                   gboolean  floating)
{
    GObject *new_dock;
    
    g_return_val_if_fail (original != NULL, NULL);
    
    new_dock = g_object_new (SP_TYPE_DOCK, 
                             "master", SP_DOCK_OBJECT_GET_MASTER (original), 
                             "floating", floating,
                             NULL);
    SP_DOCK_OBJECT_UNSET_FLAGS (new_dock, SP_DOCK_AUTOMATIC);
    
    return GTK_WIDGET (new_dock);
}

void
sp_dock_add_item (SPDock          *dock,
                   SPDockItem      *item,
                   SPDockPlacement  placement)
{
    g_return_if_fail (dock != NULL);
    g_return_if_fail (item != NULL);

    if (placement == SP_DOCK_FLOATING)
        /* Add the item to a new floating dock */
        sp_dock_add_floating_item (dock, item, 0, 0, -1, -1);

    else {
        /* Non-floating item. */
        sp_dock_object_dock (SP_DOCK_OBJECT (dock),
                              SP_DOCK_OBJECT (item),
                              placement, NULL);
    }
}

void
sp_dock_add_floating_item (SPDock        *dock,
                            SPDockItem    *item,
                            gint            x,
                            gint            y,
                            gint            width,
                            gint            height)
{
    SPDock *new_dock;
    
    g_return_if_fail (dock != NULL);
    g_return_if_fail (item != NULL);
    
    new_dock = SP_DOCK (g_object_new (SP_TYPE_DOCK, 
                                       "master", SP_DOCK_OBJECT_GET_MASTER (dock), 
                                       "floating", TRUE,
                                       "width", width,
                                       "height", height,
                                       "floatx", x,
                                       "floaty", y,
                                       NULL));
    
    if (GTK_WIDGET_VISIBLE (dock)) {
        gtk_widget_show (GTK_WIDGET (new_dock));
        if (GTK_WIDGET_MAPPED (dock))
            gtk_widget_map (GTK_WIDGET (new_dock));
        
        /* Make the widget resize. */
        gtk_widget_queue_resize (GTK_WIDGET (new_dock));
    }

    sp_dock_add_item (SP_DOCK (new_dock), item, SP_DOCK_TOP);
}

SPDockItem *
sp_dock_get_item_by_name (SPDock     *dock,
                           const gchar *name)
{
    SPDockObject *found;
    
    g_return_val_if_fail (dock != NULL && name != NULL, NULL);
    
    /* proxy the call to our master */
    found = sp_dock_master_get_object (SP_DOCK_OBJECT_GET_MASTER (dock), name);

    return (found && SP_IS_DOCK_ITEM (found)) ? SP_DOCK_ITEM (found) : NULL;
}

SPDockPlaceholder *
sp_dock_get_placeholder_by_name (SPDock     *dock,
                                  const gchar *name)
{
    SPDockObject *found;
    
    g_return_val_if_fail (dock != NULL && name != NULL, NULL);
    
    /* proxy the call to our master */
    found = sp_dock_master_get_object (SP_DOCK_OBJECT_GET_MASTER (dock), name);

    return (found && SP_IS_DOCK_PLACEHOLDER (found)) ?
        SP_DOCK_PLACEHOLDER (found) : NULL;
}

GList *
sp_dock_get_named_items (SPDock *dock)
{
    GList *list = NULL;
    
    g_return_val_if_fail (dock != NULL, NULL);

    sp_dock_master_foreach (SP_DOCK_OBJECT_GET_MASTER (dock),
                             (GFunc) _sp_dock_foreach_build_list, &list);

    return list;
}

SPDock *
sp_dock_object_get_toplevel (SPDockObject *object)
{
    SPDockObject *parent = object;
    
    g_return_val_if_fail (object != NULL, NULL);

    while (parent && !SP_IS_DOCK (parent))
        parent = sp_dock_object_get_parent_object (parent);

    return parent ? SP_DOCK (parent) : NULL;
}

void
sp_dock_xor_rect (SPDock      *dock,
                   GdkRectangle *rect)
{
    GtkWidget *widget;
    gint8      dash_list [2];

    widget = GTK_WIDGET (dock);

    if (!dock->_priv->xor_gc) {
        if (GTK_WIDGET_REALIZED (widget)) {
            GdkGCValues values;

            values.function = GDK_INVERT;
            values.subwindow_mode = GDK_INCLUDE_INFERIORS;
            dock->_priv->xor_gc = gdk_gc_new_with_values 
                (widget->window, &values, GDK_GC_FUNCTION | GDK_GC_SUBWINDOW);
        } else 
            return;
    };

    gdk_gc_set_line_attributes (dock->_priv->xor_gc, 1,
                                GDK_LINE_ON_OFF_DASH,
                                GDK_CAP_NOT_LAST,
                                GDK_JOIN_BEVEL);
    
    dash_list [0] = 1;
    dash_list [1] = 1;
    
    gdk_gc_set_dashes (dock->_priv->xor_gc, 1, dash_list, 2);

    gdk_draw_rectangle (widget->window, dock->_priv->xor_gc, 0, 
                        rect->x, rect->y,
                        rect->width, rect->height);

    gdk_gc_set_dashes (dock->_priv->xor_gc, 0, dash_list, 2);

    gdk_draw_rectangle (widget->window, dock->_priv->xor_gc, 0, 
                        rect->x + 1, rect->y + 1,
                        rect->width - 2, rect->height - 2);
}

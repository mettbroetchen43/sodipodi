/*
 * sp-dock-object.c - Abstract base class for all dock related objects
 *
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
#include "sp-dock-macros.h"
#include <stdlib.h>
#include <string.h>

#include <helper/sp-marshal.h>

#include "sp-dock-object.h"
#include "sp-dock-master.h"
#include "sp-dock-typebuiltins.h"

/* for later use by the registry */
#include "sp-dock.h"
#include "sp-dock-item.h"
#include "sp-dock-paned.h"
#include "sp-dock-notebook.h"
#include "sp-dock-placeholder.h"

#if 0
struct _SPDockObjectPrivate {
	gpointer unused;
};
#endif

/* ----- Private prototypes ----- */

static void     sp_dock_object_class_init         (SPDockObjectClass *klass);
static void     sp_dock_object_instance_init      (SPDockObject      *object);

static void     sp_dock_object_set_property       (GObject            *g_object,
                                                    guint               prop_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);
static void     sp_dock_object_get_property       (GObject            *g_object,
                                                    guint               prop_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);
static void     sp_dock_object_finalize           (GObject            *g_object);

static void     sp_dock_object_destroy            (GtkObject          *gtk_object);

static void     sp_dock_object_show               (GtkWidget          *widget);
static void     sp_dock_object_hide               (GtkWidget          *widget);

static void     sp_dock_object_real_detach        (SPDockObject      *object,
                                                    gboolean            recursive);
static void     sp_dock_object_real_reduce        (SPDockObject      *object);
static void     sp_dock_object_dock_unimplemented (SPDockObject     *object,
                                                    SPDockObject     *requestor,
                                                    SPDockPlacement   position,
                                                    GValue            *other_data);
static void     sp_dock_object_real_present       (SPDockObject     *object,
                                                    SPDockObject     *child);


/* ----- Private data types and variables ----- */

enum {
    PROP_0,
    PROP_NAME,
    PROP_LONG_NAME,
    PROP_MASTER,
    PROP_EXPORT_PROPERTIES
};

enum {
    DETACH,
    DOCK,
    LAST_SIGNAL
};

static guint sp_dock_object_signals [LAST_SIGNAL] = { 0 };

/* ----- Private interface ----- */

SP_CLASS_BOILERPLATE (SPDockObject, sp_dock_object, GtkContainer, GTK_TYPE_CONTAINER);

static void
sp_dock_object_class_init (SPDockObjectClass *klass)
{
    GObjectClass      *g_object_class;
    GtkObjectClass    *object_class;
    GtkWidgetClass    *widget_class;
    GtkContainerClass *container_class;

    g_object_class = G_OBJECT_CLASS (klass);
    object_class = GTK_OBJECT_CLASS (klass);
    widget_class = GTK_WIDGET_CLASS (klass);
    container_class = GTK_CONTAINER_CLASS (klass);

    g_object_class->set_property = sp_dock_object_set_property;
    g_object_class->get_property = sp_dock_object_get_property;
    g_object_class->finalize = sp_dock_object_finalize;

    g_object_class_install_property (
        g_object_class, PROP_NAME,
        g_param_spec_string (SP_DOCK_NAME_PROPERTY, _("Name"),
                             _("Unique name for identifying the dock object"),
                             NULL,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                             SP_DOCK_PARAM_EXPORT));

    g_object_class_install_property (
        g_object_class, PROP_LONG_NAME,
        g_param_spec_string ("long_name", _("Long name"),
                             _("Human readable name for the dock object"),
                             NULL,
                             G_PARAM_READWRITE));

    g_object_class_install_property (
        g_object_class, PROP_MASTER,
        g_param_spec_object ("master", _("Dock master"),
                             _("Dock master this dock object is bound to"),
                             SP_TYPE_DOCK_MASTER,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
    
    object_class->destroy = sp_dock_object_destroy;
    
    widget_class->show = sp_dock_object_show;
    widget_class->hide = sp_dock_object_hide;
    
    klass->is_compound = TRUE;
    
    klass->detach = sp_dock_object_real_detach;
    klass->reduce = sp_dock_object_real_reduce;
    klass->dock_request = NULL;
    klass->dock = sp_dock_object_dock_unimplemented;
    klass->reorder = NULL;
    klass->present = sp_dock_object_real_present;
    klass->child_placement = NULL;
    
    sp_dock_object_signals [DETACH] =
        g_signal_new ("detach",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (SPDockObjectClass, detach),
                      NULL,
                      NULL,
                      sp_marshal_VOID__BOOLEAN,
                      G_TYPE_NONE,
                      1,
                      G_TYPE_BOOLEAN);

    sp_dock_object_signals [DOCK] =
        g_signal_new ("dock",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (SPDockObjectClass, dock),
                      NULL,
                      NULL,
                      sp_marshal_VOID__OBJECT_ENUM_BOXED,
                      G_TYPE_NONE,
                      3,
                      SP_TYPE_DOCK_OBJECT,
                      SP_TYPE_DOCK_PLACEMENT,
                      G_TYPE_VALUE);
}

static void
sp_dock_object_instance_init (SPDockObject *object)
{
    object->flags = SP_DOCK_AUTOMATIC;
    object->freeze_count = 0;
}

static void
sp_dock_object_set_property  (GObject      *g_object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
    SPDockObject *object = SP_DOCK_OBJECT (g_object);

    switch (prop_id) {
    case PROP_NAME:
        g_free (object->name);
        object->name = g_value_dup_string (value);
        break;
    case PROP_LONG_NAME:
        g_free (object->long_name);
        object->long_name = g_value_dup_string (value);
        break;
    case PROP_MASTER:
        if (g_value_get_object (value)) 
            sp_dock_object_bind (object, g_value_get_object (value));
        else
            sp_dock_object_unbind (object);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
sp_dock_object_get_property  (GObject      *g_object,
                               guint         prop_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
    SPDockObject *object = SP_DOCK_OBJECT (g_object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, object->name);
        break;
    case PROP_LONG_NAME:
        g_value_set_string (value, object->long_name);
        break;
    case PROP_MASTER:
        g_value_set_object (value, object->master);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
sp_dock_object_finalize (GObject *g_object)
{
    SPDockObject *object;
    
    g_return_if_fail (g_object != NULL && SP_IS_DOCK_OBJECT (g_object));

    object = SP_DOCK_OBJECT (g_object);

    g_free (object->name);
    object->name = NULL;
    g_free (object->long_name);
    object->long_name = NULL;

    SP_CALL_PARENT (G_OBJECT_CLASS, finalize, (g_object));
}

static void
sp_dock_object_foreach_detach (SPDockObject *object,
                                gpointer       user_data)
{
    sp_dock_object_detach (object, TRUE);
}

static void
sp_dock_object_destroy (GtkObject *gtk_object)
{
    SPDockObject *object;

    g_return_if_fail (SP_IS_DOCK_OBJECT (gtk_object));

    object = SP_DOCK_OBJECT (gtk_object);
    if (sp_dock_object_is_compound (object)) {
        /* detach our dock object children if we have some, and even
           if we are not attached, so they can get notification */
        sp_dock_object_freeze (object);
        gtk_container_foreach (GTK_CONTAINER (object),
                               (GtkCallback) sp_dock_object_foreach_detach,
                               NULL);
        object->reduce_pending = FALSE;
        sp_dock_object_thaw (object);
    }
    if (SP_DOCK_OBJECT_ATTACHED (object)) {
        /* detach ourselves */
        sp_dock_object_detach (object, FALSE);
    }
    
    /* finally unbind us */
    if (object->master)
        sp_dock_object_unbind (object);
        
    SP_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (gtk_object));
}

static void
sp_dock_object_foreach_automatic (SPDockObject *object,
                                   gpointer       user_data)
{
    void (* function) (GtkWidget *) = user_data;

    if (SP_DOCK_OBJECT_AUTOMATIC (object))
        (* function) (GTK_WIDGET (object));
}

static void
sp_dock_object_show (GtkWidget *widget)
{
    if (sp_dock_object_is_compound (SP_DOCK_OBJECT (widget))) {
        gtk_container_foreach (GTK_CONTAINER (widget),
                               (GtkCallback) sp_dock_object_foreach_automatic,
                               gtk_widget_show);
    }
    SP_CALL_PARENT (GTK_WIDGET_CLASS, show, (widget));
}

static void
sp_dock_object_hide (GtkWidget *widget)
{
    if (sp_dock_object_is_compound (SP_DOCK_OBJECT (widget))) {
        gtk_container_foreach (GTK_CONTAINER (widget),
                               (GtkCallback) sp_dock_object_foreach_automatic,
                               gtk_widget_hide);
    }
    SP_CALL_PARENT (GTK_WIDGET_CLASS, hide, (widget));
}

static void
sp_dock_object_real_detach (SPDockObject *object,
                             gboolean       recursive)
{
    SPDockObject *parent;
    GtkWidget     *widget;
    
    g_return_if_fail (object != NULL);

    /* detach children */
    if (recursive && sp_dock_object_is_compound (object)) {
        gtk_container_foreach (GTK_CONTAINER (object),
                               (GtkCallback) sp_dock_object_detach,
                               (gpointer) recursive);
    }
    
    /* detach the object itself */
    SP_DOCK_OBJECT_UNSET_FLAGS (object, SP_DOCK_ATTACHED);
    parent = sp_dock_object_get_parent_object (object);
    widget = GTK_WIDGET (object);
    if (widget->parent)
        gtk_container_remove (GTK_CONTAINER (widget->parent), widget);
    if (parent)
        sp_dock_object_reduce (parent);
}

static void
sp_dock_object_real_reduce (SPDockObject *object)
{
    SPDockObject *parent;
    GList         *children;
    
    g_return_if_fail (object != NULL);

    if (!sp_dock_object_is_compound (object))
        return;

    parent = sp_dock_object_get_parent_object (object);
    children = gtk_container_get_children (GTK_CONTAINER (object));
    if (g_list_length (children) <= 1) {
        GList *l;
        
        /* detach ourselves and then re-attach our children to our
           current parent.  if we are not currently attached, the
           children are detached */
        if (parent)
            sp_dock_object_freeze (parent);
        sp_dock_object_freeze (object);
        sp_dock_object_detach (object, FALSE);
        for (l = children; l; l = l->next) {
            SPDockObject *child = SP_DOCK_OBJECT (l->data);

            g_object_ref (child);
            SP_DOCK_OBJECT_SET_FLAGS (child, SP_DOCK_IN_REFLOW);
            sp_dock_object_detach (child, FALSE);
            if (parent)
                gtk_container_add (GTK_CONTAINER (parent), GTK_WIDGET (child));
            SP_DOCK_OBJECT_UNSET_FLAGS (child, SP_DOCK_IN_REFLOW);
            g_object_unref (child);
        }
        /* sink the widget, so any automatic floating widget is destroyed */
        gtk_object_sink (GTK_OBJECT (object));
        /* don't reenter */
        object->reduce_pending = FALSE;
        sp_dock_object_thaw (object);
        if (parent)
            sp_dock_object_thaw (parent);
    }
    g_list_free (children);
}

static void
sp_dock_object_dock_unimplemented (SPDockObject    *object,
                                    SPDockObject    *requestor,
                                    SPDockPlacement  position,
                                    GValue           *other_data)
{
    g_warning (_("Call to sp_dock_object_dock in a dock object %p "
                 "(object type is %s) which hasn't implemented this method"),
               object, G_OBJECT_TYPE_NAME (object));
}

static void 
sp_dock_object_real_present (SPDockObject *object,
                              SPDockObject *child)
{
    gtk_widget_show (GTK_WIDGET (object));
}


/* ----- Public interface ----- */

gboolean
sp_dock_object_is_compound (SPDockObject *object)
{
    SPDockObjectClass *klass;

    g_return_val_if_fail (object != NULL, FALSE);
    g_return_val_if_fail (SP_IS_DOCK_OBJECT (object), FALSE);

    klass = SP_DOCK_OBJECT_GET_CLASS (object);
    return klass->is_compound;
}

void
sp_dock_object_detach (SPDockObject *object,
                        gboolean       recursive)
{
    g_return_if_fail (object != NULL);

    if (!SP_DOCK_OBJECT_ATTACHED (object))
        return;
    
    /* freeze the object to avoid reducing while detaching children */
    sp_dock_object_freeze (object);
    SP_DOCK_OBJECT_SET_FLAGS (object, SP_DOCK_IN_DETACH);
    g_signal_emit (object, sp_dock_object_signals [DETACH], 0, recursive);
    SP_DOCK_OBJECT_UNSET_FLAGS (object, SP_DOCK_IN_DETACH);
    sp_dock_object_thaw (object);
}

SPDockObject *
sp_dock_object_get_parent_object (SPDockObject *object)
{
    GtkWidget *parent;
    
    g_return_val_if_fail (object != NULL, NULL);

    parent = GTK_WIDGET (object)->parent;
    while (parent && !SP_IS_DOCK_OBJECT (parent)) {
        parent = parent->parent;
    }
    
    return parent ? SP_DOCK_OBJECT (parent) : NULL;
}

void
sp_dock_object_freeze (SPDockObject *object)
{
    g_return_if_fail (object != NULL);
    
    if (object->freeze_count == 0) {
        g_object_ref (object);   /* dock objects shouldn't be
                                    destroyed if they are frozen */
    }
    object->freeze_count++;
}

void
sp_dock_object_thaw (SPDockObject *object)
{
    g_return_if_fail (object != NULL);
    g_return_if_fail (object->freeze_count > 0);
    
    object->freeze_count--;
    if (object->freeze_count == 0) {
        if (object->reduce_pending) {
            object->reduce_pending = FALSE;
            sp_dock_object_reduce (object);
        }
        g_object_unref (object);
    }
}

void
sp_dock_object_reduce (SPDockObject *object)
{
    g_return_if_fail (object != NULL);

    if (SP_DOCK_OBJECT_FROZEN (object)) {
        object->reduce_pending = TRUE;
        return;
    }

    SP_CALL_VIRTUAL (object, SP_DOCK_OBJECT_GET_CLASS, reduce, (object));
}

gboolean
sp_dock_object_dock_request (SPDockObject  *object,
                              gint            x,
                              gint            y,
                              SPDockRequest *request)
{
    g_return_val_if_fail (object != NULL && request != NULL, FALSE);
    
    return SP_CALL_VIRTUAL_WITH_DEFAULT (object,
                                          SP_DOCK_OBJECT_GET_CLASS,
                                          dock_request,
                                          (object, x, y, request),
                                          FALSE);
}

void
sp_dock_object_dock (SPDockObject    *object,
                      SPDockObject    *requestor,
                      SPDockPlacement  position,
                      GValue           *other_data)
{
    SPDockObject *parent;
    
    g_return_if_fail (object != NULL && requestor != NULL);
        
    if (object == requestor)
        return;
    
    if (!object->master)
        g_warning (_("Dock operation requested in a non-bound object %p. "
                     "The application might crash"), object);
        
    if (!sp_dock_object_is_bound (requestor))
        sp_dock_object_bind (requestor, object->master);

    if (requestor->master != object->master) {
        g_warning (_("Cannot dock %p to %p because they belong to different masters"),
                   requestor, object);
        return;
    }

    /* first, see if we can optimize things by reordering */
    if (position != SP_DOCK_NONE) {
        parent = sp_dock_object_get_parent_object (object);
        if (sp_dock_object_reorder (object, requestor, position, other_data) ||
            (parent && sp_dock_object_reorder (parent, requestor, position, other_data)))
            return;
    }
    
    /* freeze the object, since under some conditions it might be destroyed when
       detaching the requestor */
    sp_dock_object_freeze (object);

    /* detach the requestor before docking */
    g_object_ref (requestor);
    if (SP_DOCK_OBJECT_ATTACHED (requestor))
        sp_dock_object_detach (requestor, FALSE);
    
    if (position != SP_DOCK_NONE)
        g_signal_emit (object, sp_dock_object_signals [DOCK], 0,
                       requestor, position, other_data);

    g_object_unref (requestor);
    sp_dock_object_thaw (object);
}

void
sp_dock_object_bind (SPDockObject *object,
                      GObject       *master)
{
    g_return_if_fail (object != NULL && master != NULL);
    g_return_if_fail (SP_IS_DOCK_MASTER (master));
    
    if (object->master == master)
        /* nothing to do here */
        return;
    
    if (object->master) {
        g_warning (_("Attempt to bind to %p an already bound dock object %p "
                     "(current master: %p)"), master, object, object->master);
        return;
    }

    sp_dock_master_add (SP_DOCK_MASTER (master), object);
    object->master = master;
    g_object_add_weak_pointer (master, (gpointer *) &object->master);

    g_object_notify (G_OBJECT (object), "master");
}

void
sp_dock_object_unbind (SPDockObject *object)
{
    g_return_if_fail (object != NULL);

    g_object_ref (object);

    /* detach the object first */
    if (SP_DOCK_OBJECT_ATTACHED (object))
        sp_dock_object_detach (object, TRUE);
    
    if (object->master) {
        GObject *master = object->master;
        g_object_remove_weak_pointer (master, (gpointer *) &object->master);
        object->master = NULL;
        sp_dock_master_remove (SP_DOCK_MASTER (master), object);
        g_object_notify (G_OBJECT (object), "master");
    }
    g_object_unref (object);
}

gboolean
sp_dock_object_is_bound (SPDockObject *object)
{
    g_return_val_if_fail (object != NULL, FALSE);
    return (object->master != NULL);
}

gboolean
sp_dock_object_reorder (SPDockObject    *object,
                         SPDockObject    *child,
                         SPDockPlacement  new_position,
                         GValue           *other_data)
{
    g_return_val_if_fail (object != NULL && child != NULL, FALSE);

    return SP_CALL_VIRTUAL_WITH_DEFAULT (object,
                                          SP_DOCK_OBJECT_GET_CLASS,
                                          reorder,
                                          (object, child, new_position, other_data),
                                          FALSE);
}

void 
sp_dock_object_present (SPDockObject *object,
                         SPDockObject *child)
{
    SPDockObject *parent;
    
    g_return_if_fail (object != NULL && SP_IS_DOCK_OBJECT (object));

    parent = sp_dock_object_get_parent_object (object);
    if (parent)
        /* chain the call to our parent */
        sp_dock_object_present (parent, object);

    SP_CALL_VIRTUAL (object, SP_DOCK_OBJECT_GET_CLASS, present, (object, child));
}

/**
 * sp_dock_object_child_placement:
 * @object: the dock object we are asking for child placement
 * @child: the child of the @object we want the placement for
 * @placement: where to return the placement information
 *
 * This function returns information about placement of a child dock
 * object inside another dock object.  The function returns %TRUE if
 * @child is effectively a child of @object.  @placement should
 * normally be initially setup to %SP_DOCK_NONE.  If it's set to some
 * other value, this function will not touch the stored value if the
 * specified placement is "compatible" with the actual placement of
 * the child.
 *
 * @placement can be %NULL, in which case the function simply tells if
 * @child is attached to @object.
 *
 * Returns: %TRUE if @child is a child of @object.
 */
gboolean 
sp_dock_object_child_placement (SPDockObject    *object,
                                 SPDockObject    *child,
                                 SPDockPlacement *placement)
{
    g_return_val_if_fail (object != NULL && child != NULL, FALSE);

    /* simple case */
    if (!sp_dock_object_is_compound (object))
        return FALSE;
    
    return SP_CALL_VIRTUAL_WITH_DEFAULT (object, SP_DOCK_OBJECT_GET_CLASS,
                                          child_placement,
                                          (object, child, placement),
                                          FALSE);
}


/* ----- dock param type functions start here ------ */

static void 
sp_dock_param_export_int (const GValue *src,
                           GValue       *dst)
{
    dst->data [0].v_pointer = g_strdup_printf ("%d", src->data [0].v_int);
}

static void 
sp_dock_param_export_uint (const GValue *src,
                            GValue       *dst)
{
    dst->data [0].v_pointer = g_strdup_printf ("%u", src->data [0].v_uint);
}

static void 
sp_dock_param_export_string (const GValue *src,
                              GValue       *dst)
{
    dst->data [0].v_pointer = g_strdup (src->data [0].v_pointer);
}

static void 
sp_dock_param_export_bool (const GValue *src,
                            GValue       *dst)
{
    dst->data [0].v_pointer = g_strdup_printf ("%s", src->data [0].v_int ? "yes" : "no");
}

static void 
sp_dock_param_export_placement (const GValue *src,
                                 GValue       *dst)
{
    switch (src->data [0].v_int) {
        case SP_DOCK_NONE:
            dst->data [0].v_pointer = g_strdup ("");
            break;
        case SP_DOCK_TOP:
            dst->data [0].v_pointer = g_strdup ("top");
            break;
        case SP_DOCK_BOTTOM:
            dst->data [0].v_pointer = g_strdup ("bottom");
            break;
        case SP_DOCK_LEFT:
            dst->data [0].v_pointer = g_strdup ("left");
            break;
        case SP_DOCK_RIGHT:
            dst->data [0].v_pointer = g_strdup ("right");
            break;
        case SP_DOCK_CENTER:
            dst->data [0].v_pointer = g_strdup ("center");
            break;
        case SP_DOCK_FLOATING:
            dst->data [0].v_pointer = g_strdup ("floating");
            break;
    }
}

static void 
sp_dock_param_import_int (const GValue *src,
                           GValue       *dst)
{
    dst->data [0].v_int = atoi (src->data [0].v_pointer);
}

static void 
sp_dock_param_import_uint (const GValue *src,
                            GValue       *dst)
{
    dst->data [0].v_uint = (guint) atoi (src->data [0].v_pointer);
}

static void 
sp_dock_param_import_string (const GValue *src,
                              GValue       *dst)
{
    dst->data [0].v_pointer = g_strdup (src->data [0].v_pointer);
}

static void 
sp_dock_param_import_bool (const GValue *src,
                            GValue       *dst)
{
    dst->data [0].v_int = !strcmp (src->data [0].v_pointer, "yes");
}

static void 
sp_dock_param_import_placement (const GValue *src,
                                 GValue       *dst)
{
    if (!strcmp (src->data [0].v_pointer, "top"))
        dst->data [0].v_int = SP_DOCK_TOP;
    else if (!strcmp (src->data [0].v_pointer, "bottom"))
        dst->data [0].v_int = SP_DOCK_BOTTOM;
    else if (!strcmp (src->data [0].v_pointer, "center"))
        dst->data [0].v_int = SP_DOCK_CENTER;
    else if (!strcmp (src->data [0].v_pointer, "left"))
        dst->data [0].v_int = SP_DOCK_LEFT;
    else if (!strcmp (src->data [0].v_pointer, "right"))
        dst->data [0].v_int = SP_DOCK_RIGHT;
    else if (!strcmp (src->data [0].v_pointer, "floating"))
        dst->data [0].v_int = SP_DOCK_FLOATING;
    else
        dst->data [0].v_int = SP_DOCK_NONE;
}

GType
sp_dock_param_get_type (void)
{
    static GType our_type = 0;

    if (our_type == 0) {
        GTypeInfo tinfo = { 0, };
        our_type = g_type_register_static (G_TYPE_STRING, "SPDockParam", &tinfo, 0);

        /* register known transform functions */
        /* exporters */
        g_value_register_transform_func (G_TYPE_INT, our_type, sp_dock_param_export_int);
        g_value_register_transform_func (G_TYPE_UINT, our_type, sp_dock_param_export_uint);
        g_value_register_transform_func (G_TYPE_STRING, our_type, sp_dock_param_export_string);
        g_value_register_transform_func (G_TYPE_BOOLEAN, our_type, sp_dock_param_export_bool);
        g_value_register_transform_func (SP_TYPE_DOCK_PLACEMENT, our_type, sp_dock_param_export_placement);
        /* importers */
        g_value_register_transform_func (our_type, G_TYPE_INT, sp_dock_param_import_int);
        g_value_register_transform_func (our_type, G_TYPE_UINT, sp_dock_param_import_uint);
        g_value_register_transform_func (our_type, G_TYPE_STRING, sp_dock_param_import_string);
        g_value_register_transform_func (our_type, G_TYPE_BOOLEAN, sp_dock_param_import_bool);
        g_value_register_transform_func (our_type, SP_TYPE_DOCK_PLACEMENT, sp_dock_param_import_placement);
    }

    return our_type;
}

/* -------------- nick <-> type conversion functions --------------- */

static GRelation *dock_register = NULL;

enum {
    INDEX_NICK = 0,
    INDEX_TYPE
};

static void
sp_dock_object_register_init (void)
{
    if (dock_register)
        return;
    
    /* FIXME: i don't know if GRelation is efficient */
    dock_register = g_relation_new (2);
    g_relation_index (dock_register, INDEX_NICK, g_str_hash, g_str_equal);
    g_relation_index (dock_register, INDEX_TYPE, g_direct_hash, g_direct_equal);

    /* add known types */
    g_relation_insert (dock_register, "dock", (gpointer) SP_TYPE_DOCK);
    g_relation_insert (dock_register, "item", (gpointer) SP_TYPE_DOCK_ITEM);
    g_relation_insert (dock_register, "paned", (gpointer) SP_TYPE_DOCK_PANED);
    g_relation_insert (dock_register, "notebook", (gpointer) SP_TYPE_DOCK_NOTEBOOK);
    g_relation_insert (dock_register, "placeholder", (gpointer) SP_TYPE_DOCK_PLACEHOLDER);
}

G_CONST_RETURN gchar *
sp_dock_object_nick_from_type (GType type)
{
    GTuples *tuples;
    gchar *nick = NULL;
    
    if (!dock_register)
        sp_dock_object_register_init ();

    if (g_relation_count (dock_register, (gpointer) type, INDEX_TYPE) > 0) {
        tuples = g_relation_select (dock_register, (gpointer) type, INDEX_TYPE);
        nick = (gchar *) g_tuples_index (tuples, 0, INDEX_NICK);
        g_tuples_destroy (tuples);
    }
    
    return nick ? nick : g_type_name (type);
}

GType
sp_dock_object_type_from_nick (const gchar *nick)
{
    GTuples *tuples;
    GType type = G_TYPE_NONE;
    
    if (!dock_register)
        sp_dock_object_register_init ();

    if (g_relation_count (dock_register, (gpointer) nick, INDEX_NICK) > 0) {
        tuples = g_relation_select (dock_register, (gpointer) nick, INDEX_NICK);
        type = (GType) g_tuples_index (tuples, 0, INDEX_TYPE);
        g_tuples_destroy (tuples);
    }
    else {
        /* try searching in the glib type system */
        type = g_type_from_name (nick);
    }
    
    return type;
}

GType
sp_dock_object_set_type_for_nick (const gchar *nick,
                                   GType        type)
{
    GType old_type = G_TYPE_NONE;
    
    if (!dock_register)
        sp_dock_object_register_init ();

    g_return_val_if_fail (g_type_is_a (type, SP_TYPE_DOCK_OBJECT), G_TYPE_NONE);
    
    if (g_relation_count (dock_register, (gpointer) nick, INDEX_NICK) > 0) {
        old_type = sp_dock_object_type_from_nick (nick);
        g_relation_delete (dock_register, (gpointer) nick, INDEX_NICK);
    }
    
    g_relation_insert (dock_register, nick, type);

    return old_type;
}

/*
 *
 * sp-dock-item.h
 *
 * Copyright (C) 2002 Gustavo Giráldez <gustavo.giraldez@gmx.net>
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

#ifndef __SP_DOCK_ITEM_H__
#define __SP_DOCK_ITEM_H__

#include <widgets/sp-dock-object.h>

G_BEGIN_DECLS

/* standard macros */
#define SP_TYPE_DOCK_ITEM            (sp_dock_item_get_type ())
#define SP_DOCK_ITEM(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_DOCK_ITEM, SPDockItem))
#define SP_DOCK_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DOCK_ITEM, SPDockItemClass))
#define SP_IS_DOCK_ITEM(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_DOCK_ITEM))
#define SP_IS_DOCK_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DOCK_ITEM))
#define SP_DOCK_ITEM_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GTK_TYPE_DOCK_ITEM, SPDockItemClass))

/* data types & structures */
typedef enum {  /*< prefix=SP >*/
    SP_DOCK_ITEM_BEH_NORMAL           = 0,
    SP_DOCK_ITEM_BEH_NEVER_FLOATING   = 1 << 0,
    SP_DOCK_ITEM_BEH_NEVER_VERTICAL   = 1 << 1,
    SP_DOCK_ITEM_BEH_NEVER_HORIZONTAL = 1 << 2,
    SP_DOCK_ITEM_BEH_LOCKED           = 1 << 3,
    SP_DOCK_ITEM_BEH_CANT_DOCK_TOP    = 1 << 4,
    SP_DOCK_ITEM_BEH_CANT_DOCK_BOTTOM = 1 << 5,
    SP_DOCK_ITEM_BEH_CANT_DOCK_LEFT   = 1 << 6,
    SP_DOCK_ITEM_BEH_CANT_DOCK_RIGHT  = 1 << 7,
    SP_DOCK_ITEM_BEH_CANT_DOCK_CENTER = 1 << 8
} SPDockItemBehavior;

typedef enum { /*< prefix=SP >*/
    SP_DOCK_IN_DRAG             = 1 << SP_DOCK_OBJECT_FLAGS_SHIFT,
    SP_DOCK_IN_PREDRAG          = 1 << (SP_DOCK_OBJECT_FLAGS_SHIFT + 1),
    /* for general use: indicates the user has started an action on
       the dock item */
    SP_DOCK_USER_ACTION         = 1 << (SP_DOCK_OBJECT_FLAGS_SHIFT + 2)
} SPDockItemFlags;

typedef struct _SPDockItem        SPDockItem;
typedef struct _SPDockItemClass   SPDockItemClass;
typedef struct _SPDockItemPrivate SPDockItemPrivate;

struct _SPDockItem {
    SPDockObject        object;

    GtkWidget           *child;
    SPDockItemBehavior  behavior;
    GtkOrientation       orientation;

    guint                resize : 1;

    gint                 dragoff_x, dragoff_y;    /* these need to be
                                                     accesible from
                                                     outside */
    SPDockItemPrivate  *_priv;
};

struct _SPDockItemClass {
    SPDockObjectClass  parent_class;

    gboolean            has_grip;
    
    /* virtuals */
    void     (* dock_drag_begin)  (SPDockItem    *item);
    void     (* dock_drag_motion) (SPDockItem    *item,
                                   gint            x,
                                   gint            y);
    void     (* dock_drag_end)    (SPDockItem    *item,
                                   gboolean        cancelled);
                                   
    void     (* set_orientation)  (SPDockItem    *item,
                                   GtkOrientation  orientation);
	gpointer unused[2]; /* Future expansion without breaking ABI */
};

/* additional macros */
#define SP_DOCK_ITEM_FLAGS(item)     (SP_DOCK_OBJECT (item)->flags)
#define SP_DOCK_ITEM_IN_DRAG(item) \
    ((SP_DOCK_ITEM_FLAGS (item) & SP_DOCK_IN_DRAG) != 0)
#define SP_DOCK_ITEM_IN_PREDRAG(item) \
    ((SP_DOCK_ITEM_FLAGS (item) & SP_DOCK_IN_PREDRAG) != 0)
#define SP_DOCK_ITEM_USER_ACTION(item) \
    ((SP_DOCK_ITEM_FLAGS (item) & SP_DOCK_USER_ACTION) != 0)
   
#define SP_DOCK_ITEM_SET_FLAGS(item,flag) \
    G_STMT_START { (SP_DOCK_ITEM_FLAGS (item) |= (flag)); } G_STMT_END
#define SP_DOCK_ITEM_UNSET_FLAGS(item,flag) \
    G_STMT_START { (SP_DOCK_ITEM_FLAGS (item) &= ~(flag)); } G_STMT_END

#define SP_DOCK_ITEM_HAS_GRIP(item) (SP_DOCK_ITEM_GET_CLASS (item)->has_grip)

/* public interface */
 
GtkWidget     *sp_dock_item_new               (const gchar         *name,
                                                const gchar         *long_name,
                                                SPDockItemBehavior  behavior);

GType          sp_dock_item_get_type          (void);

void           sp_dock_item_dock_to           (SPDockItem      *item,
                                                SPDockItem      *target,
                                                SPDockPlacement  position,
                                                gint              docking_param);

void           sp_dock_item_set_orientation   (SPDockItem    *item,
                                                GtkOrientation  orientation);

GtkWidget     *sp_dock_item_get_tablabel      (SPDockItem *item);
void           sp_dock_item_set_tablabel      (SPDockItem *item,
                                                GtkWidget   *tablabel);
void           sp_dock_item_hide_grip         (SPDockItem *item);
void           sp_dock_item_show_grip         (SPDockItem *item);

/* bind and unbind items to a dock */
void           sp_dock_item_bind              (SPDockItem *item,
                                                GtkWidget   *dock);

void           sp_dock_item_unbind            (SPDockItem *item);

void           sp_dock_item_hide_item         (SPDockItem *item);

void           sp_dock_item_show_item         (SPDockItem *item);

void           sp_dock_item_lock              (SPDockItem *item);

void           sp_dock_item_unlock            (SPDockItem *item);

void        sp_dock_item_set_default_position (SPDockItem      *item,
                                                SPDockObject    *reference);

void        sp_dock_item_preferred_size       (SPDockItem      *item,
                                                GtkRequisition   *req);


G_END_DECLS

#endif

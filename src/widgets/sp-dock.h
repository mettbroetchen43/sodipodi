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

#ifndef __SP_DOCK_H__
#define __SP_DOCK_H__

#include <gtk/gtk.h>
#include <widgets/sp-dock-object.h>
#include <widgets/sp-dock-item.h>
#include <widgets/sp-dock-placeholder.h>

G_BEGIN_DECLS

/* standard macros */
#define SP_TYPE_DOCK            (sp_dock_get_type ())
#define SP_DOCK(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_DOCK, SPDock))
#define SP_DOCK_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DOCK, SPDockClass))
#define SP_IS_DOCK(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_DOCK))
#define SP_IS_DOCK_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DOCK))
#define SP_DOCK_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GTK_TYPE_DOCK, SPDockClass))

/* data types & structures */
typedef struct _SPDock        SPDock;
typedef struct _SPDockClass   SPDockClass;
typedef struct _SPDockPrivate SPDockPrivate;

struct _SPDock {
    SPDockObject    object;

    SPDockObject   *root;

    SPDockPrivate  *_priv;
};

struct _SPDockClass {
    SPDockObjectClass parent_class;

    void  (* layout_changed)  (SPDock *dock);    /* proxy signal for the master */
	gpointer unused[2]; /* Future expansion without ABI breakage */
};

/* additional macros */
#define SP_DOCK_IS_CONTROLLER(dock)  \
    (sp_dock_master_get_controller (SP_DOCK_OBJECT_GET_MASTER (dock)) == \
     SP_DOCK_OBJECT (dock))

/* public interface */
 
GtkWidget     *sp_dock_new               (void);

GtkWidget     *sp_dock_new_from          (SPDock          *original,
                                           gboolean          floating);

GType          sp_dock_get_type          (void);

void           sp_dock_add_item          (SPDock          *dock,
                                           SPDockItem      *item,
                                           SPDockPlacement  place);

void           sp_dock_add_floating_item (SPDock        *dock,
                                           SPDockItem    *item,
                                           gint            x,
                                           gint            y,
                                           gint            width,
                                           gint            height);

SPDockItem   *sp_dock_get_item_by_name  (SPDock     *dock,
                                           const gchar *name);

SPDockPlaceholder *sp_dock_get_placeholder_by_name (SPDock     *dock,
                                                      const gchar *name);

GList         *sp_dock_get_named_items   (SPDock    *dock);

SPDock       *sp_dock_object_get_toplevel (SPDockObject *object);

void           sp_dock_xor_rect            (SPDock       *dock,
                                             GdkRectangle  *rect);

G_END_DECLS

#endif

/*
 * sp-dock-master.h - Object which manages a dock ring
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

#ifndef __SP_DOCK_MASTER_H__
#define __SP_DOCK_MASTER_H__

#include <glib-object.h>
#include <gtk/gtktypeutils.h>
#include <widgets/sp-dock-object.h>


G_BEGIN_DECLS

/* standard macros */
#define SP_TYPE_DOCK_MASTER             (sp_dock_master_get_type ())
#define SP_DOCK_MASTER(obj)             (GTK_CHECK_CAST ((obj), SP_TYPE_DOCK_MASTER, SPDockMaster))
#define SP_DOCK_MASTER_CLASS(klass)     (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DOCK_MASTER, SPDockMasterClass))
#define SP_IS_DOCK_MASTER(obj)          (GTK_CHECK_TYPE ((obj), SP_TYPE_DOCK_MASTER))
#define SP_IS_DOCK_MASTER_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DOCK_MASTER))
#define SP_DOCK_MASTER_GET_CLASS(obj)   (GTK_CHECK_GET_CLASS ((obj), GTK_TYPE_DOCK_MASTER, SPDockMasterClass))

/* data types & structures */
typedef struct _SPDockMaster        SPDockMaster;
typedef struct _SPDockMasterClass   SPDockMasterClass;
typedef struct _SPDockMasterPrivate SPDockMasterPrivate;

struct _SPDockMaster {
    GObject               object;

    GHashTable           *dock_objects;
    GList                *toplevel_docks;
    SPDockObject        *controller;      /* GUI root object */
    
    gint                  dock_number;     /* for toplevel dock numbering */
    
    SPDockMasterPrivate *_priv;
};

struct _SPDockMasterClass {
    GObjectClass parent_class;

    void (* layout_changed) (SPDockMaster *master);
	gpointer unused[2]; /* Future expansion without ABI breakage */
};

/* additional macros */

#define SP_DOCK_OBJECT_GET_MASTER(object) \
    (SP_DOCK_OBJECT (object)->master ? \
        SP_DOCK_MASTER (SP_DOCK_OBJECT (object)->master) : NULL)

/* public interface */
 
GType          sp_dock_master_get_type         (void);

void           sp_dock_master_add              (SPDockMaster *master,
                                                 SPDockObject *object);
void           sp_dock_master_remove           (SPDockMaster *master,
                                                 SPDockObject *object);
void           sp_dock_master_foreach          (SPDockMaster *master,
                                                 GFunc          function,
                                                 gpointer       user_data);

void           sp_dock_master_foreach_toplevel (SPDockMaster *master,
                                                 gboolean       include_controller,
                                                 GFunc          function,
                                                 gpointer       user_data);

SPDockObject *sp_dock_master_get_object       (SPDockMaster *master,
                                                 const gchar   *nick_name);

SPDockObject *sp_dock_master_get_controller   (SPDockMaster *master);
void           sp_dock_master_set_controller   (SPDockMaster *master,
                                                 SPDockObject *new_controller);

G_END_DECLS

#endif /* __SP_DOCK_MASTER_H__ */

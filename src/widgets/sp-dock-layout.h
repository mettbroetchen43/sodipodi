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


#ifndef __SP_DOCK_LAYOUT_H__
#define __SP_DOCK_LAYOUT_H__

#include <glib-object.h>
#include <widgets/sp-dock-master.h>
#include <widgets/sp-dock.h>

G_BEGIN_DECLS

/* standard macros */
#define	SP_TYPE_DOCK_LAYOUT		  (sp_dock_layout_get_type ())
#define SP_DOCK_LAYOUT(object)		  (GTK_CHECK_CAST ((object), SP_TYPE_DOCK_LAYOUT, SPDockLayout))
#define SP_DOCK_LAYOUT_CLASS(klass)	  (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DOCK_LAYOUT, SPDockLayoutClass))
#define SP_IS_DOCK_LAYOUT(object)	  (GTK_CHECK_TYPE ((object), SP_TYPE_DOCK_LAYOUT))
#define SP_IS_DOCK_LAYOUT_CLASS(klass)	  (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DOCK_LAYOUT))
#define	SP_DOCK_LAYOUT_GET_CLASS(object) (GTK_CHECK_GET_CLASS ((object), SP_TYPE_DOCK_LAYOUT, SPDockLayoutClass))

/* data types & structures */
typedef struct _SPDockLayout SPDockLayout;
typedef struct _SPDockLayoutClass SPDockLayoutClass;
typedef struct _SPDockLayoutPrivate SPDockLayoutPrivate;

struct _SPDockLayout {
    GObject               g_object;

    gboolean              dirty;
    SPDockMaster        *master;

    SPDockLayoutPrivate *_priv;
};

struct _SPDockLayoutClass {
    GObjectClass  g_object_class;
	gpointer unused[2]; /* Future expansion without ABI breakage */
};


/* public interface */
 
GType            sp_dock_layout_get_type       (void);

SPDockLayout   *sp_dock_layout_new            (SPDock       *dock);

void             sp_dock_layout_attach         (SPDockLayout *layout,
                                                 SPDockMaster *master);

gboolean         sp_dock_layout_load_layout    (SPDockLayout *layout,
                                                 const gchar   *name);

void             sp_dock_layout_save_layout    (SPDockLayout *layout,
                                                 const gchar   *name);

void             sp_dock_layout_delete_layout  (SPDockLayout *layout,
                                                 const gchar   *name);

GList           *sp_dock_layout_get_layouts    (SPDockLayout *layout,
                                                 gboolean       include_default);

void             sp_dock_layout_run_manager    (SPDockLayout *layout);

gboolean         sp_dock_layout_load_from_file (SPDockLayout *layout,
                                                 const gchar   *filename);

gboolean         sp_dock_layout_save_to_file   (SPDockLayout *layout,
                                                 const gchar   *filename);

gboolean         sp_dock_layout_is_dirty       (SPDockLayout *layout);

GtkWidget       *sp_dock_layout_get_ui         (SPDockLayout *layout);
GtkWidget       *sp_dock_layout_get_items_ui   (SPDockLayout *layout);
GtkWidget       *sp_dock_layout_get_layouts_ui (SPDockLayout *layout);

G_END_DECLS

#endif

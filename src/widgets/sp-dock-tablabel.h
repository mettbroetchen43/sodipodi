/*
 * sp-dock-tablabel.h
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

#ifndef __SP_DOCK_TABLABEL_H__
#define __SP_DOCK_TABLABEL_H__

#include <gtk/gtk.h>
#include <widgets/sp-dock-item.h>


G_BEGIN_DECLS

/* standard macros */
#define SP_TYPE_DOCK_TABLABEL            (sp_dock_tablabel_get_type ())
#define SP_DOCK_TABLABEL(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_DOCK_TABLABEL, SPDockTablabel))
#define SP_DOCK_TABLABEL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DOCK_TABLABEL, SPDockTablabelClass))
#define SP_IS_DOCK_TABLABEL(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_DOCK_TABLABEL))
#define SP_IS_DOCK_TABLABEL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DOCK_TABLABEL))
#define SP_DOCK_TABLABEL_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GTK_TYPE_DOCK_TABLABEL, SPDockTablabelClass))

/* data types & structures */
typedef struct _SPDockTablabel      SPDockTablabel;
typedef struct _SPDockTablabelClass SPDockTablabelClass;
typedef struct _SPDockTablabelPrivate SPDockTablabelPrivate;

struct _SPDockTablabel {
    GtkBin          parent;

    guint           drag_handle_size;
    GtkWidget      *item;
    GdkWindow      *event_window;
    gboolean        active;

    GdkEventButton  drag_start_event;
    gboolean        pre_drag;
	SPDockTablabelPrivate *_priv;
};

struct _SPDockTablabelClass {
    GtkBinClass      parent_class;

    void            (*button_pressed_handle)  (SPDockTablabel *tablabel,
                                               GdkEventButton  *event);
	gpointer unused[2];
};

/* public interface */
 
GtkWidget     *sp_dock_tablabel_new           (SPDockItem *item);
GType          sp_dock_tablabel_get_type      (void);

void           sp_dock_tablabel_activate      (SPDockTablabel *tablabel);
void           sp_dock_tablabel_deactivate    (SPDockTablabel *tablabel);

G_END_DECLS

#endif

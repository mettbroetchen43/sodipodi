/*
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

#ifndef __SP_DOCK_NOTEBOOK_H__
#define __SP_DOCK_NOTEBOOK_H__

#include <widgets/sp-dock-item.h>

G_BEGIN_DECLS

/* standard macros */
#define SP_TYPE_DOCK_NOTEBOOK            (sp_dock_notebook_get_type ())
#define SP_DOCK_NOTEBOOK(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_DOCK_NOTEBOOK, SPDockNotebook))
#define SP_DOCK_NOTEBOOK_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DOCK_NOTEBOOK, SPDockNotebookClass))
#define SP_IS_DOCK_NOTEBOOK(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_DOCK_NOTEBOOK))
#define SP_IS_DOCK_NOTEBOOK_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DOCK_NOTEBOOK))
#define SP_DOCK_NOTEBOOK_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GTK_TYPE_DOCK_NOTEBOOK, SPDockNotebookClass))

/* data types & structures */
typedef struct _SPDockNotebook        SPDockNotebook;
typedef struct _SPDockNotebookClass   SPDockNotebookClass;
typedef struct _SPDockNotebookPrivate SPDockNotebookPrivate;

struct _SPDockNotebook {
    SPDockItem  item;
	SPDockNotebookPrivate *_priv;
};

struct _SPDockNotebookClass {
    SPDockItemClass  parent_class;
	gpointer unused[2];
};


/* public interface */
 
GtkWidget     *sp_dock_notebook_new               (void);

GType          sp_dock_notebook_get_type          (void);

G_END_DECLS

#endif

/*
 * sp-dock-placeholder.h - Placeholders for docking items
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

#ifndef __SP_DOCK_PLACEHOLDER_H__
#define __SP_DOCK_PLACEHOLDER_H__

#include <widgets/sp-dock-object.h>

G_BEGIN_DECLS

/* standard macros */
#define SP_TYPE_DOCK_PLACEHOLDER             (sp_dock_placeholder_get_type ())
#define SP_DOCK_PLACEHOLDER(obj)             (GTK_CHECK_CAST ((obj), SP_TYPE_DOCK_PLACEHOLDER, SPDockPlaceholder))
#define SP_DOCK_PLACEHOLDER_CLASS(klass)     (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DOCK_PLACEHOLDER, SPDockPlaceholderClass))
#define SP_IS_DOCK_PLACEHOLDER(obj)          (GTK_CHECK_TYPE ((obj), SP_TYPE_DOCK_PLACEHOLDER))
#define SP_IS_DOCK_PLACEHOLDER_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DOCK_PLACEHOLDER))
#define SP_DOCK_PLACEHOLDER_GET_CLASS(obj)   (GTK_CHECK_GET_CLASS ((obj), GTK_TYPE_DOCK_PLACEHOLDER, SPDockPlaceholderClass))

/* data types & structures */
typedef struct _SPDockPlaceholder        SPDockPlaceholder;
typedef struct _SPDockPlaceholderClass   SPDockPlaceholderClass;
typedef struct _SPDockPlaceholderPrivate SPDockPlaceholderPrivate;

struct _SPDockPlaceholder {
    SPDockObject              object;

    SPDockPlaceholderPrivate *_priv;
};

struct _SPDockPlaceholderClass {
    SPDockObjectClass parent_class;
	gpointer unused[2];
};

/* public interface */

GType       sp_dock_placeholder_get_type (void);

GtkWidget  *sp_dock_placeholder_new      (gchar              *name,
                                           SPDockObject      *object,
                                           SPDockPlacement    position,
                                           gboolean            sticky);

void        sp_dock_placeholder_attach   (SPDockPlaceholder *ph,
                                           SPDockObject      *object);


G_END_DECLS

#endif /* __SP_DOCK_PLACEHOLDER_H__ */

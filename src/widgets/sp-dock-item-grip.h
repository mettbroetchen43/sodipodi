/*
 * sp-dock-item-grip.h
 * Copyright (C) 2002 Gustavo Giráldez <gustavo.giraldez@gmx.net>
 * Copyright (C) 2003 Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
 * 
 * Based on bonobo-dock-item-grip.  Original copyright notice follows.
 *
 * Author:
 *    Michael Meeks
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
 */

#ifndef _SP_DOCK_ITEM_GRIP_H_
#define _SP_DOCK_ITEM_GRIP_H_

#include <gtk/gtkwidget.h>
#include <widgets/sp-dock-item.h>

G_BEGIN_DECLS

#define SP_TYPE_DOCK_ITEM_GRIP            (sp_dock_item_grip_get_type())
#define SP_DOCK_ITEM_GRIP(obj)            \
    (GTK_CHECK_CAST ((obj), SP_TYPE_DOCK_ITEM_GRIP, SPDockItemGrip))
#define SP_DOCK_ITEM_GRIP_CLASS(klass)    \
    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DOCK_ITEM_GRIP, SPDockItemGripClass))
#define SP_IS_DOCK_ITEM_GRIP(obj)         \
    (GTK_CHECK_TYPE ((obj), SP_TYPE_DOCK_ITEM_GRIP))
#define SP_IS_DOCK_ITEM_GRIP_CLASS(klass) \
    (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DOCK_ITEM_GRIP))
#define SP_DOCK_ITEM_GRIP_GET_CLASS(obj)  \
    (GTK_CHECK_GET_CLASS ((obj), SP_TYPE_DOCK_ITEM_GRIP, SPDockItemGripClass))

typedef struct _SPDockItemGripPrivate SPDockItemGripPrivate;

typedef struct {
    GtkWidget parent;
    SPDockItem *item;
	/* private */
	SPDockItemGripPrivate *_priv;
} SPDockItemGrip;

typedef struct {
    GtkWidgetClass parent_class;

    void (*activate) (SPDockItemGrip *grip);
	gpointer unused[2]; /* For future expansion without breaking ABI */
} SPDockItemGripClass;

GType      sp_dock_item_grip_get_type (void);
GtkWidget *sp_dock_item_grip_new      (SPDockItem *item);

G_END_DECLS

#endif /* _SP_DOCK_ITEM_GRIP_H_ */

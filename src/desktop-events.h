#ifndef __DESKTOP_EVENTS_H__
#define __DESKTOP_EVENTS_H__

/*
 * Entry points for event distribution
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libgnomeui/gnome-canvas.h>
#include "forward.h"

/* Item handlers */

void sp_desktop_root_handler (GnomeCanvasItem *item, GdkEvent *event, SPDesktop *desktop);
void sp_desktop_item_handler (GnomeCanvasItem *item, GdkEvent *event, gpointer data);

/* Default handlers */

gint sp_desktop_enter_notify (GtkWidget *widget, GdkEventCrossing *event);
gint sp_canvas_enter_notify (GtkWidget *widget, GdkEventCrossing *event, SPDesktop *desktop);
gint sp_canvas_leave_notify (GtkWidget *widget, GdkEventCrossing *event, SPDesktop *desktop);
gint sp_canvas_motion_notify (GtkWidget *widget,GdkEventMotion *motion, SPDesktop *desktop);

/* Rulers */

gint sp_dt_hruler_event (GtkWidget *widget, GdkEvent *event, gpointer data);
gint sp_dt_vruler_event (GtkWidget *widget, GdkEvent *event, gpointer data);

/* Guides */

gint sp_dt_guide_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data);

#endif


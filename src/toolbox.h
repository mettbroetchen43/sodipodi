#ifndef __SP_TOOLBOX_H__
#define __SP_TOOLBOX_H__

/*
 * Main toolbox
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "desktop.h"

void sp_maintoolbox_create (void);
gint sp_maintoolbox_close (GtkWidget * widget, GdkEventAny * event, gpointer data);
void sp_maintoolbox_drag_data_received (GtkWidget * widget,
					GdkDragContext * drag_context,
					gint x, gint y,
					GtkSelectionData * data,
					guint info,
					guint event_time,
					gpointer user_data);
#endif

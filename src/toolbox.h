#ifndef SP_TOOLBOX_H
#define SP_TOOLBOX_H

/*
 * Toolbox
 *
 * Authors:
 *   Frank Felfe  <innerspace@iname.com>
 *   Lauris Kaplinski  <lauris@helixcode.com>
 *
 * Copyright (C) 2000-2001 Helix Code, Inc. and authors
 */

#include "desktop.h"

void sp_maintoolbox_create (void);
gint sp_maintoolbox_close (GtkWidget * widget, GdkEventAny * event, gpointer data);

#endif

#ifndef __SP_FILL_STYLE_H__
#define __SP_FILL_STYLE_H__

/*
 * Fill style dialog
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#include <gtk/gtkwidget.h>
#include "../forward.h"

GtkWidget *sp_fill_style_widget_new (void);

void sp_fill_style_dialog (void);

END_GNOME_DECLS

#endif

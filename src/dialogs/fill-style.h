#ifndef __SP_FILL_STYLE_H__
#define __SP_FILL_STYLE_H__

/*
 * Fill style configuration
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib.h>

G_BEGIN_DECLS

#include <gtk/gtkwidget.h>
#include "../forward.h"

GtkWidget *sp_fill_style_widget_new (void);

void sp_fill_style_dialog (void);

G_END_DECLS

#endif

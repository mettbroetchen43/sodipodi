#ifndef __SP_ITEM_PROPERTIES_H__
#define __SP_ITEM_PROPERTIES_H__

/*
 * Display settings dialog
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

void sp_item_dialog (SPItem *item);
void sp_item_dialog_apply (GtkWidget * widget);
void sp_item_dialog_close (GtkWidget * widget);

END_GNOME_DECLS

#endif

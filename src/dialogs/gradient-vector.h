#ifndef __SP_GRADIENT_VECTOR_H__
#define __SP_GRADIENT_VECTOR_H__

/*
 * Gradient vector editing widget and dialog
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

GtkWidget *sp_gradient_vector_widget_new (SPGradient *gradient);

void sp_gradient_vector_dialog (SPGradient *gradient);

END_GNOME_DECLS

#endif

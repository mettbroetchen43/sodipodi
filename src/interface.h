#ifndef __SP_INTERFACE_H__
#define __SP_INTERFACE_H__

#include "view.h"

void sp_create_window (SPViewWidget *vw, gboolean editable);

void sp_ui_new_view (GtkWidget * widget);
void sp_ui_new_view_preview (GtkWidget * widget);
void sp_ui_close_view (GtkWidget * widget);

/* I am not sure, what is the right place for that (Lauris) */

GtkWidget *sp_ui_generic_menu (SPView *v, SPItem *item);

#endif

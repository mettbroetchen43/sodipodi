#ifndef SP_INTERFACE_H
#define SP_INTERFACE_H

#include "desktop.h"

void sp_create_window (SPViewWidget *vw, gboolean editable);

void sp_ui_new_view (GtkWidget * widget);
void sp_ui_new_view_preview (GtkWidget * widget);
void sp_ui_close_view (GtkWidget * widget);

#endif

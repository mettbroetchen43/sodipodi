#ifndef SP_TOOLBOX_H
#define SP_TOOLBOX_H

#include "desktop.h"

void sp_maintoolbox_create (void);
gint sp_maintoolbox_close (GtkWidget * widget, GdkEventAny * event, gpointer data);
#if 0
void sp_update_draw_toolbox (SPDesktop * desktop);
#endif
#endif

#ifndef SP_DESKTOP_PROPERTIES_H
#define SP_DESKTOP_PROPERTIES_H

#include <gtk/gtk.h>

#if 0
void sp_guides_dialog (void);
void sp_guides_dialog_close (void);
void sp_guides_dialog_apply (void);
#else
void sp_desktop_dialog (void);
void sp_desktop_dialog_apply (GtkWidget * widget);
void sp_desktop_dialog_close (GtkWidget * widget);
#endif

#endif

#define SP_MDI_DESKTOP_C

#include "mdi-desktop.h"
#include "desktop.h"
// ?
#include "toolbox.h"

static SPDesktop * active;

SPDesktop *
sp_active_desktop (GnomeMDI * mdi)
{
	GtkWidget * active_view;

	g_return_val_if_fail (mdi != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_MDI (mdi), NULL);

	if (active != NULL) return active;

	active_view = gnome_mdi_get_active_view (mdi);

	if (active_view == NULL) return NULL;
	g_return_val_if_fail (SP_IS_DESKTOP (active_view), NULL);

	return SP_DESKTOP (active_view);
}

void
sp_active_desktop_set (SPDesktop * desktop)
{
  SPDesktop * old_desk;

  old_desk = SP_ACTIVE_DESKTOP;
  if (old_desk) {
    gtk_widget_hide (old_desk->active);
    gtk_widget_show (old_desk->inactive);
  }

  g_assert (desktop != NULL);
  g_assert (SP_IS_DESKTOP (desktop));

  active = desktop;
  gtk_widget_hide (desktop->inactive);
  gtk_widget_show (desktop->active);
  sp_update_draw_toolbox (active);
}


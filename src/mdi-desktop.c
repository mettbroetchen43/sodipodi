#define SP_MDI_DESKTOP_C

#include "mdi-desktop.h"
#include "desktop.h"

static SPDesktop * active;

SPDesktop *
sp_active_desktop (GnomeMDI * mdi)
{
	GtkWidget * active_view;

	g_return_val_if_fail (mdi != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_MDI (mdi), NULL);

	if (active != NULL) return active;

	active_view = gnome_mdi_get_active_view (mdi);

	g_return_val_if_fail (active_view != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (active_view), NULL);

	return SP_DESKTOP (active_view);
}

void
sp_active_desktop_set (SPDesktop * desktop)
{
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));

	active = desktop;
}


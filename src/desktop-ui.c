#define SP_DESKTOP_UI_C

#include "desktop.h"

void
sp_desktop_new_view (GtkWidget * widget)
{
	SPDesktop * desktop;
	SPDocument * doc;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	desktop = SP_WIDGET_DESKTOP (widget);

	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	doc = SP_DT_DOCUMENT (desktop);

	desktop = sp_desktop_new (desktop->app, doc);
}


#define SP_DESKTOP_HANDLES_C

#include "desktop.h"
#include "desktop-handles.h"

SPEventContext *
sp_desktop_event_context (SPDesktop * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	return desktop->event_context;
}

SPSelection *
sp_desktop_selection (SPDesktop * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	return desktop->selection;
}

SPDocument *
sp_desktop_document (SPDesktop * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	return SP_VIEW_DOCUMENT (desktop);
}

GnomeCanvas *
sp_desktop_canvas (SPDesktop * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	return desktop->owner->canvas;
}

GnomeCanvasItem *
sp_desktop_acetate (SPDesktop * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	return desktop->acetate;
}

GnomeCanvasGroup *
sp_desktop_main (SPDesktop * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	return desktop->main;
}

GnomeCanvasGroup *
sp_desktop_grid (SPDesktop * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	return desktop->grid;
}

GnomeCanvasGroup *
sp_desktop_guides (SPDesktop * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	return desktop->guides;
}

GnomeCanvasGroup *
sp_desktop_drawing (SPDesktop * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	return desktop->drawing;
}

GnomeCanvasGroup *
sp_desktop_sketch (SPDesktop * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	return desktop->sketch;
}

GnomeCanvasGroup *
sp_desktop_controls (SPDesktop * desktop)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	return desktop->controls;
}


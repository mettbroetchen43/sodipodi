#ifndef SP_DRAW_CONTEXT_H
#define SP_DRAW_CONTEXT_H

#include <libgnomeui/gnome-canvas.h>
#include "desktop-handles.h"

gint sp_draw_handler (SPDesktop * desktop, SPItem * item, GdkEvent * event);
gint sp_draw_root_handler (SPDesktop * desktop, GdkEvent * event);

void sp_draw_context_set (SPDesktop * desktop);

void sp_draw_context_release (SPDesktop * desktop);

#endif

#ifndef TEXT_CONTEXT_H
#define TEXT_CONTEXT_H

#include <libgnomeui/gnome-canvas.h>
#include "desktop.h"

gint sp_text_handler (SPDesktop * desktop, SPItem * item, GdkEvent * event);
gint sp_text_root_handler (SPDesktop * desktop, GdkEvent * event);

void sp_text_release (SPDesktop * desktop);

void sp_text_context_set (SPDesktop * desktop);

#endif

#ifndef SP_SELECT_CONTEXT_H
#define SP_SELECT_CONTEXT_H

#include "desktop.h"

gint sp_select_handler (SPDesktop * desktop, SPItem * item, GdkEvent * event);
gint sp_select_root_handler (SPDesktop * desktop, GdkEvent * event);

void sp_select_context_release (SPDesktop * desktop);
void sp_select_context_set (SPDesktop * desktop);

void
sp_select_context_selection_changed (SPDesktop * desktop);

#endif

#ifndef SP_NODE_CONTEXT_H
#define SP_NODE_CONTEXT_H

#include <libgnomeui/gnome-canvas.h>

gint
sp_node_handler (SPDesktop * desktop, SPItem * item, GdkEvent * event);

gint
sp_node_root_handler (SPDesktop * desktop, GdkEvent * event);

void sp_node_context_selection_changed (SPDesktop * desktop);

void sp_node_context_set (SPDesktop * desktop);

void sp_node_context_release (SPDesktop * desktop);

#endif

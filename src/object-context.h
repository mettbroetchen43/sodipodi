#ifndef OBJECT_CONTEXT_H
#define OBJECT_CONTEXT_H

#include <libgnomeui/gnome-canvas.h>

typedef enum {
	SP_OBJECT_NONE,
	SP_OBJECT_RECT,
	SP_OBJECT_ELLIPSE
} SPObjectType;

gint
sp_object_handler (SPDesktop * desktop, SPItem * item, GdkEvent * event);

gint
sp_object_root_handler (SPDesktop * desktop, GdkEvent * event);

void sp_object_context_set (SPDesktop * desktop, SPObjectType type);

void sp_object_release (SPDesktop * desktop);

#endif

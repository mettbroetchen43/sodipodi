#ifndef SP_DRAW_CONTEXT_H
#define SP_DRAW_CONTEXT_H

#include "display/canvas-shape.h"
#include "event-context.h"

#define SP_TYPE_DRAW_CONTEXT            (sp_draw_context_get_type ())
#define SP_DRAW_CONTEXT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_DRAW_CONTEXT, SPDrawContext))
#define SP_DRAW_CONTEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DRAW_CONTEXT, SPDrawContextClass))
#define SP_IS_DRAW_CONTEXT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_DRAW_CONTEXT))
#define SP_IS_DRAW_CONTEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DRAW_CONTEXT))

typedef struct _SPDrawContext SPDrawContext;
typedef struct _SPDrawContextClass SPDrawContextClass;

typedef struct _SPDrawCtrl SPDrawCtrl;

struct _SPDrawContext {
	SPEventContext event_context;
	SPCanvasShape * shape;
	ArtBpath * bpath;
	gint pos;
	gint length;
	gboolean moving_end;
	SPDrawCtrl * ctrl;
};

struct _SPDrawContextClass {
	SPEventContextClass parent_class;
};

/* Standard Gtk function */

GtkType sp_draw_context_get_type (void);

#endif

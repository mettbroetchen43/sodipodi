#ifndef SP_DYNA_DRAW_CONTEXT_H
#define SP_DYNA_DRAW_CONTEXT_H

#include "helper/curve.h"
#include "display/canvas-shape.h"
#include "event-context.h"

#define SP_TYPE_DYNA_DRAW_CONTEXT            (sp_dyna_draw_context_get_type ())
#define SP_DYNA_DRAW_CONTEXT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_DYNA_DRAW_CONTEXT, SPDynaDrawContext))
#define SP_DYNA_DRAW_CONTEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DYNA_DRAW_CONTEXT, SPDynaDrawContextClass))
#define SP_IS_DYNA_DRAW_CONTEXT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_DYNA_DRAW_CONTEXT))
#define SP_IS_DYNA_DRAW_CONTEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DYNA_DRAW_CONTEXT))

typedef struct _SPDynaDrawContext SPDynaDrawContext;
typedef struct _SPDynaDrawContextClass SPDynaDrawContextClass;

typedef struct _SPDynaDrawCtrl SPDynaDrawCtrl;

struct _SPDynaDrawContext {
	SPEventContext event_context;
	SPCurve * accumulated;
	ArtPoint p[16];
	gint npoints;
	GSList * segments;
	SPCurve * currentcurve;
	SPCanvasShape * currentshape;

	SPRepr * repr;
	guint destroyid;

	GnomeCanvasItem * citem;
	ArtPoint cpos;
	guint32 ccolor;
	gboolean cinside;
};

struct _SPDynaDrawContextClass {
	SPEventContextClass parent_class;
};

/* Standard Gtk function */

GtkType sp_dyna_draw_context_get_type (void);

#endif

#ifndef SP_ELLIPSE_CONTEXT_H
#define SP_ELLIPSE_CONTEXT_H

#include "event-context.h"

#define SP_TYPE_ELLIPSE_CONTEXT            (sp_ellipse_context_get_type ())
#define SP_ELLIPSE_CONTEXT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_ELLIPSE_CONTEXT, SPEllipseContext))
#define SP_ELLIPSE_CONTEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_ELLIPSE_CONTEXT, SPEllipseContextClass))
#define SP_IS_ELLIPSE_CONTEXT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_ELLIPSE_CONTEXT))
#define SP_IS_ELLIPSE_CONTEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_ELLIPSE_CONTEXT))

typedef struct _SPEllipseContext SPEllipseContext;
typedef struct _SPEllipseContextClass SPEllipseContextClass;

struct _SPEllipseContext {
	SPEventContext event_context;
};

struct _SPEllipseContextClass {
	SPEventContextClass parent_class;
};

/* Standard Gtk function */

GtkType sp_ellipse_context_get_type (void);

#endif

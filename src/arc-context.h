#ifndef SP_ARC_CONTEXT_H
#define SP_ARC_CONTEXT_H

#include "event-context.h"

#define SP_TYPE_ARC_CONTEXT            (sp_arc_context_get_type ())
#define SP_ARC_CONTEXT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_ARC_CONTEXT, SPArcContext))
#define SP_ARC_CONTEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_ARC_CONTEXT, SPArcContextClass))
#define SP_IS_ARC_CONTEXT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_ARC_CONTEXT))
#define SP_IS_ARC_CONTEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_ARC_CONTEXT))

typedef struct _SPArcContext SPArcContext;
typedef struct _SPArcContextClass SPArcContextClass;

struct _SPArcContext {
	SPEventContext event_context;
	SPItem * item;
	ArtPoint center;
};

struct _SPArcContextClass {
	SPEventContextClass parent_class;
};

/* Standard Gtk function */

GtkType sp_arc_context_get_type (void);

#endif

#ifndef SP_RECT_CONTEXT_H
#define SP_RECT_CONTEXT_H

#include "knot.h"
#include "event-context.h"

#define SP_TYPE_RECT_CONTEXT            (sp_rect_context_get_type ())
#define SP_RECT_CONTEXT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_RECT_CONTEXT, SPRectContext))
#define SP_RECT_CONTEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_RECT_CONTEXT, SPRectContextClass))
#define SP_IS_RECT_CONTEXT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_RECT_CONTEXT))
#define SP_IS_RECT_CONTEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_RECT_CONTEXT))

typedef struct _SPRectContext SPRectContext;
typedef struct _SPRectContextClass SPRectContextClass;

struct _SPRectContext {
	SPEventContext event_context;
	SPItem * item;
	ArtPoint center;
};

struct _SPRectContextClass {
	SPEventContextClass parent_class;
};

/* Standard Gtk function */

GtkType sp_rect_context_get_type (void);

#endif

#ifndef SP_SLIDE_CONTEXT_H
#define SP_SLIDE_CONTEXT_H

#include "event-context.h"

#define SP_TYPE_SLIDE_CONTEXT            (sp_slide_context_get_type ())
#define SP_SLIDE_CONTEXT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_SLIDE_CONTEXT, SPSlideContext))
#define SP_SLIDE_CONTEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_SLIDE_CONTEXT, SPSlideContextClass))
#define SP_IS_SLIDE_CONTEXT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_SLIDE_CONTEXT))
#define SP_IS_SLIDE_CONTEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_SLIDE_CONTEXT))

typedef struct _SPSlideContext SPSlideContext;
typedef struct _SPSlideContextClass SPSlideContextClass;

struct _SPSlideContext {
	SPEventContext event_context;
	GSList *forward;
};

struct _SPSlideContextClass {
	SPEventContextClass parent_class;
};

/* Standard Gtk function */

GtkType sp_slide_context_get_type (void);

#endif

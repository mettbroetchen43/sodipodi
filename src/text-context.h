#ifndef SP_TEXT_CONTEXT_H
#define SP_TEXT_CONTEXT_H

#include "event-context.h"

#define SP_TYPE_TEXT_CONTEXT            (sp_text_context_get_type ())
#define SP_TEXT_CONTEXT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_TEXT_CONTEXT, SPTextContext))
#define SP_TEXT_CONTEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_TEXT_CONTEXT, SPTextContextClass))
#define SP_IS_TEXT_CONTEXT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_TEXT_CONTEXT))
#define SP_IS_TEXT_CONTEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_TEXT_CONTEXT))

typedef struct _SPTextContext SPTextContext;
typedef struct _SPTextContextClass SPTextContextClass;

struct _SPTextContext {
	SPEventContext event_context;
	SPItem *item;
	ArtPoint pdoc;
};

struct _SPTextContextClass {
	SPEventContextClass parent_class;
};

/* Standard Gtk function */

GtkType sp_text_context_get_type (void);

#endif

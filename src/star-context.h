#ifndef SP_STAR_CONTEXT_H
#define SP_STAR_CONTEXT_H

#include "knot.h"
#include "event-context.h"

#define SP_TYPE_STAR_CONTEXT            (sp_star_context_get_type ())
#define SP_STAR_CONTEXT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_STAR_CONTEXT, SPStarContext))
#define SP_STAR_CONTEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_STAR_CONTEXT, SPStarContextClass))
#define SP_IS_STAR_CONTEXT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_STAR_CONTEXT))
#define SP_IS_STAR_CONTEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_STAR_CONTEXT))

typedef struct _SPStarContext SPStarContext;
typedef struct _SPStarContextClass SPStarContextClass;

struct _SPStarContext {
	SPEventContext event_context;
	SPItem * item;
	ArtPoint center;
};

struct _SPStarContextClass {
	SPEventContextClass parent_class;
};

/* Standard Gtk function */

GtkType sp_star_context_get_type (void);

#endif

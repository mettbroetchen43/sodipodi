#ifndef SP_ZOOM_CONTEXT_H
#define SP_ZOOM_CONTEXT_H

#include "event-context.h"

#define SP_TYPE_ZOOM_CONTEXT            (sp_zoom_context_get_type ())
#define SP_ZOOM_CONTEXT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_ZOOM_CONTEXT, SPZoomContext))
#define SP_ZOOM_CONTEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_ZOOM_CONTEXT, SPZoomContextClass))
#define SP_IS_ZOOM_CONTEXT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_ZOOM_CONTEXT))
#define SP_IS_ZOOM_CONTEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_ZOOM_CONTEXT))

typedef struct _SPZoomContext SPZoomContext;
typedef struct _SPZoomContextClass SPZoomContextClass;

struct _SPZoomContext {
	SPEventContext event_context;
};

struct _SPZoomContextClass {
	SPEventContextClass parent_class;
};

/* Standard Gtk function */

GtkType sp_zoom_context_get_type (void);

void sp_zoom_selection (GtkWidget * widget);
void sp_zoom_drawing (GtkWidget * widget);
void sp_zoom_page (GtkWidget * widget);
void sp_zoom_1_to_2 (GtkWidget * widget);
void sp_zoom_1_to_1 (GtkWidget * widget);
void sp_zoom_2_to_1 (GtkWidget * widget);
void sp_zoom_in (GtkWidget * widget);
void sp_zoom_out (GtkWidget * widget);

#endif

#ifndef SP_CANVAS_BGROUP_H
#define SP_CANVAS_BGROUP_H

#include <libgnomeui/gnome-canvas.h>

#define SP_TYPE_CANVAS_BGROUP            (sp_canvas_bgroup_get_type ())
#define SP_CANVAS_BGROUP(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_CANVAS_BGROUP, SPCanvasBgroup))
#define SP_CANVAS_BGROUP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CANVAS_BGROUP, SPCanvasBgroupClass))
#define SP_IS_CANVAS_BGROUP(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_CANVAS_BGROUP))
#define SP_IS_CANVAS_BGROUP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CANVAS_BGROUP))

typedef struct _SPCanvasBgroup SPCanvasBgroup;
typedef struct _SPCanvasBgroupClass SPCanvasBgroupClass;

struct _SPCanvasBgroup {
	GnomeCanvasGroup group;
	/* Controls, whether events belong to canvas_bgroup or childrens */
	gboolean transparent;
};

struct _SPCanvasBgroupClass {
	GnomeCanvasGroupClass parent_class;
};


/* Standard Gtk function */

GtkType sp_canvas_bgroup_get_type (void);

#endif

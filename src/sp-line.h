#ifndef SP_LINE_H
#define SP_LINE_H

#include "sp-shape.h"

BEGIN_GNOME_DECLS

#define SP_TYPE_LINE            (sp_line_get_type ())
#define SP_LINE(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_LINE, SPLine))
#define SP_LINE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_LINE, SPLineClass))
#define SP_IS_LINE(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_LINE))
#define SP_IS_LINE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_LINE))

typedef struct _SPLine SPLine;
typedef struct _SPLineClass SPLineClass;

struct _SPLine {
	SPShape shape;

	gdouble x1, y1;
	gdouble x2, y2;
};

struct _SPLineClass {
	SPShapeClass parent_class;
};

GtkType sp_line_get_type (void);

END_GNOME_DECLS

#endif

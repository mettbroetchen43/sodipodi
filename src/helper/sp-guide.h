#ifndef __SODIPODI_GUIDELINE_H__
#define __SODIPODI_GUIDELINE_H__

/*
 * SPGuideLine
 *
 * This is axis-aligned dotted line, usable for guidelinelines etc.
 *
 * Copyright (C) Lauris Kaplinski 2000
 *
 */

#include "sp-canvas.h"

G_BEGIN_DECLS

typedef enum {
	SP_GUIDELINE_ORIENTATION_HORIZONTAL,
	SP_GUIDELINE_ORIENTATION_VERTICAL
} SPGuideLineOrientationType;

#define SP_TYPE_GUIDELINE            (sp_guideline_get_type ())
#define SP_GUIDELINE(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_GUIDELINE, SPGuideLine))
#define SP_GUIDELINE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_GUIDELINE, SPGuideLineClass))
#define SP_IS_GUIDELINE(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_GUIDELINE))
#define SP_IS_GUIDELINE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_GUIDELINE))


typedef struct _SPGuideLine SPGuideLine;
typedef struct _SPGuideLineClass SPGuideLineClass;

struct _SPGuideLine {
	SPCanvasItem item;

	SPGuideLineOrientationType orientation;
	guint32 color;

	gint position;
	gboolean shown;
	gboolean sensitive;
};

struct _SPGuideLineClass {
	SPCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType sp_guideline_get_type (void);

void sp_guideline_moveto (SPGuideLine * guideline, double x, double y);
void sp_guideline_sensitize (SPGuideLine * guideline, gboolean sensitive);

G_END_DECLS

#endif

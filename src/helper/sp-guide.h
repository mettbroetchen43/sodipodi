#ifndef SODIPODI_GUIDE_H
#define SODIPODI_GUIDE_H

/*
 * SPGuide
 *
 * This is axis-aligned dotted line, usable for guidelines etc.
 *
 * Copyright (C) Lauris Kaplinski 2000
 *
 */

#include <libgnomeui/gnome-canvas.h>

BEGIN_GNOME_DECLS

typedef enum {
	SP_GUIDE_ORIENTATION_HORIZONTAL,
	SP_GUIDE_ORIENTATION_VERTICAL
} SPGuideOrientationType;

#define SP_TYPE_GUIDE            (sp_guide_get_type ())
#define SP_GUIDE(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_GUIDE, SPGuide))
#define SP_GUIDE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_GUIDE, SPGuideClass))
#define SP_IS_GUIDE(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_GUIDE))
#define SP_IS_GUIDE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_GUIDE))


typedef struct _SPGuide SPGuide;
typedef struct _SPGuideClass SPGuideClass;

struct _SPGuide {
	GnomeCanvasItem item;

	SPGuideOrientationType orientation;
	guint32 color;

	gint position;
	gboolean shown;
};

struct _SPGuideClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType sp_guide_get_type (void);

void sp_guide_moveto (SPGuide * guide, double x, double y);

END_GNOME_DECLS

#endif

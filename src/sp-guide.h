#ifndef SP_GUIDE_H
#define SP_GUIDE_H

/*
 * SPGuide
 *
 * A guideline
 *
 * Copyright (C) Lauris Kaplinski 2000
 *
 */

#include <libgnomeui/gnome-canvas.h>
#include "sp-object.h"

typedef enum {
	SP_GUIDE_HORIZONTAL,
	SP_GUIDE_VERTICAL
} SPGuideOrientation;

typedef struct _SPGuide SPGuide;
typedef struct _SPGuideClass SPGuideClass;

#define SP_TYPE_GUIDE            (sp_guide_get_type ())
#define SP_GUIDE(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_GUIDE, SPGuide))
#define SP_GUIDE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_GUIDE, SPGuideClass))
#define SP_IS_GUIDE(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_GUIDE))
#define SP_IS_GUIDE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_GUIDE))

struct _SPGuide {
	SPObject object;
	SPGuideOrientation orientation;
	gdouble position;
	GSList * views;
};

struct _SPGuideClass {
	SPObjectClass parent_class;
};

GtkType sp_guide_get_type (void);

void sp_guide_show (SPGuide * guide, GnomeCanvasGroup * group);

gint sp_guide_compare (gconstpointer a, gconstpointer b);

#endif

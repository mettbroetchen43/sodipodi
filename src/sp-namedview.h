#ifndef SP_NAMEDVIEW_H
#define SP_NAMEDVIEW_H

/*
 * SPNamedView
 *
 * A view into document, from which desktop can be constructed.
 * It holds per-desktop data, such as guides, grids etc.
 *
 * Copyright (C) Lauris Kaplinski 2000
 *
 */

typedef struct _SPNamedView SPNamedView;
typedef struct _SPNamedViewClass SPNamedViewClass;

#define SP_TYPE_NAMEDVIEW            (sp_namedview_get_type ())
#define SP_NAMEDVIEW(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_NAMEDVIEW, SPNamedView))
#define SP_NAMEDVIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_NAMEDVIEW, SPNamedViewClass))
#define SP_IS_NAMEDVIEW(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_NAMEDVIEW))
#define SP_IS_NAMEDVIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_NAMEDVIEW))

#include <libgnomeui/gnome-canvas.h>
#include "sp-object-group.h"

struct _SPNamedView {
	SPObjectGroup objectgroup;
	gboolean editable;
	GSList * hguides;
	GSList * vguides;
	GSList * views;
};

struct _SPNamedViewClass {
	SPObjectGroupClass parent_class;
};

GtkType sp_namedview_get_type (void);

void sp_namedview_show (SPNamedView * namedview, GnomeCanvasGroup * group);

#endif

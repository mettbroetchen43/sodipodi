#ifndef __SP_CANVAS_BPATH_H__
#define __SP_CANVAS_BPATH_H__

/*
 * Simple bezier bpath CanvasItem for sodipodi
 *
 * Authors:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#include <glib.h>

G_BEGIN_DECLS

#define SP_TYPE_CANVAS_BPATH (sp_canvas_bpath_get_type ())
#define SP_CANVAS_BPATH(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_CANVAS_BPATH, SPCanvasBPath))
#define SP_CANVAS_BPATH_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CANVAS_BPATH, SPCanvasBPathClass))
#define SP_IS_CANVAS_BPATH(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_CANVAS_BPATH))
#define SP_IS_CANVAS_BPATH_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CANVAS_BPATH))

typedef struct _SPCanvasBPath SPCanvasBPath;
typedef struct _SPCanvasBPathClass SPCanvasBPathClass;

#include <libnr/nr-svp.h>

#include "sp-canvas.h"
#include "curve.h"

struct _SPCanvasBPath {
	SPCanvasItem item;

	/* Line def */
	SPCurve *curve;

	/* Fill attributes */
	guint32 fill_rgba;
	unsigned int fill_rule;

	/* Line attributes */
	guint32 stroke_rgba;
	gdouble stroke_width;
	unsigned int stroke_linejoin;
	unsigned int stroke_linecap;
	gdouble stroke_miterlimit;

	/* State */
	NRSVP *fill_svp;
	NRSVP *stroke_svp;
};

struct _SPCanvasBPathClass {
	SPCanvasItemClass parent_class;
};

GtkType sp_canvas_bpath_get_type (void);

SPCanvasItem *sp_canvas_bpath_new (SPCanvasGroup *parent, SPCurve *curve);

void sp_canvas_bpath_set_bpath (SPCanvasBPath *cbp, SPCurve *curve);
void sp_canvas_bpath_set_fill (SPCanvasBPath *cbp, guint32 rgba, unsigned int rule);
void sp_canvas_bpath_set_stroke (SPCanvasBPath *cbp, guint32 rgba, gdouble width, unsigned int join, unsigned int cap);

G_END_DECLS

#endif


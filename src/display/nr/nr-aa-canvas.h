#ifndef _NR_AA_CANVAS_H_
#define _NR_AA_CANVAS_H_

/*
 * NRAACanvas
 *
 * RGB Buffer based canvas implementation
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

typedef struct _NRAACanvas NRAACanvas;
typedef struct _NRAACanvasClass NRAACanvasClass;
typedef struct _NRAAGraphicCtx NRAAGraphicCtx;
typedef struct _NRAADrawingArea NRAADrawingArea;

#define NR_TYPE_AA_CANVAS (nr_aa_canvas_get_type ())
#define NR_AA_CANVAS(o) (GTK_CHECK_CAST ((o), NR_TYPE_AA_CANVAS, NRAACanvas))
#define NR_AA_CANVAS_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), NR_TYPE_AA_CANVAS, NRAACanvasClass))
#define NR_IS_AA_CANVAS(o) (GTK_CHECK_TYPE ((o), NR_TYPE_AA_CANVAS))
#define NR_IS_AA_CANVAS_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), NR_TYPE_AA_CANVAS))

#include "nr-canvas.h"

struct _NRAACanvas {
	NRCanvas canvas;
};

struct _NRAACanvasClass {
	NRCanvasClass parent_class;
};

struct _NRAAGraphicCtx {
	NRGraphicCtx ctx;
	float opacity;
};

struct _NRAADrawingArea {
	NRDrawingArea area;
	gint rowstride;
	guchar * pixels;
};

GtkType nr_aa_canvas_get_type (void);

NRAACanvas * nr_aa_canvas_new (void);

#endif

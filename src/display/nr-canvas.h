#ifndef _NR_CANVAS_H_
#define _NR_CANVAS_H_

/*
 * NRCanvas
 *
 * Abstract base class for display caches
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

typedef struct _NRCanvas NRCanvas;
typedef struct _NRCanvasClass NRCanvasClass;

#define NR_TYPE_CANVAS (nr_canvas_get_type ())
#define NR_CANVAS(o) (GTK_CHECK_CAST ((o), NR_TYPE_CANVAS, NRCanvas))
#define NR_CANVAS_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), NR_TYPE_CANVAS, NRCanvasClass))
#define NR_IS_CANVAS(o) (GTK_CHECK_TYPE ((o), NR_TYPE_CANVAS))
#define NR_IS_CANVAS_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), NR_TYPE_CANVAS))

#define SP_OBJECT_CLONED_FLAG (1 << 4)
#define SP_OBJECT_IS_CLONED(o) (GTK_OBJECT_FLAGS (o) & SP_OBJECT_CLONED_FLAG)

#include <gtk/gtktypeutils.h>
#include <gtk/gtkobject.h>

struct _NRCanvas {
	GtkObject object;
};

struct _NRCanvasClass {
	GtkObjectClass parent_class;
};

GtkType nr_canvas_get_type (void);

#endif

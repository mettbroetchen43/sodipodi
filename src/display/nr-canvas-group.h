#ifndef _NR_CANVAS_GROUP_H_
#define _NR_CANVAS_GROUP_H_

/*
 * NRCanvasGroup
 *
 * Base class for NRCanvas groups
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

typedef struct _NRCanvasGroup NRCanvasGroup;
typedef struct _NRCanvasGroupClass NRCanvasGroupClass;

#define NR_TYPE_CANVAS_GROUP (nr_canvas_group_get_type ())
#define NR_CANVAS_GROUP(o) (GTK_CHECK_CAST ((o), NR_TYPE_CANVAS_GROUP, NRCanvasGroup))
#define NR_CANVAS_GROUP_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), NR_TYPE_CANVAS_GROUP, NRCanvasGroupClass))
#define NR_IS_CANVAS_GROUP(o) (GTK_CHECK_TYPE ((o), NR_TYPE_CANVAS_GROUP))
#define NR_IS_CANVAS_GROUP_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), NR_TYPE_CANVAS_GROUP))

#include "nr-canvas-item.h"

struct _NRCanvasGroup {
	NRCanvasItem item;
	GSList * children;
	GSList * last;
};

struct _NRCanvasGroupClass {
	NRCanvasItemClass parent_class;
};

GtkType nr_canvas_group_get_type (void);

#endif

#ifndef _SP_CANVAS_GROUP_H_
#define _SP_CANVAS_GROUP_H_

/*
 * SPCanvasGroup
 *
 * Group of SPCanvasItems
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

typedef struct _SPCanvasGroup SPCanvasGroup;
typedef struct _SPCanvasGroupClass SPCanvasGroupClass;

#define SP_TYPE_CANVAS_GROUP (sp_canvas_group_get_type ())
#define SP_CANVAS_GROUP(o) (GTK_CHECK_CAST ((o), SP_TYPE_CANVAS_GROUP, SPCanvasGroup))
#define SP_CANVAS_GROUP_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_CANVAS_GROUP, SPCanvasGroupClass))
#define SP_IS_CANVAS_GROUP(o) (GTK_CHECK_TYPE ((o), SP_TYPE_CANVAS_GROUP))
#define SP_IS_CANVAS_GROUP_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_CANVAS_GROUP))

#include "sp-canvas-item.h"

struct _SPCanvasGroup {
	SPCanvasItem spitem;
	GList * children;
	GList * last;
};

struct _SPCanvasGroupClass {
	SPCanvasItemClass parent_class;
};

GtkType sp_canvas_group_get_type (void);

#endif

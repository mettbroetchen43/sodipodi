#ifndef _NR_CANVAS_ITEM_H_
#define _NR_CANVAS_ITEM_H_

/*
 * NRCanvasItem
 *
 * Base class for NRCanvas items
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

typedef struct _NRCanvasItem NRCanvasItem;
typedef struct _NRCanvasItemClass NRCanvasItemClass;

#define NR_TYPE_CANVAS_ITEM (nr_canvas_item_get_type ())
#define NR_CANVAS_ITEM(o) (GTK_CHECK_CAST ((o), NR_TYPE_CANVAS_ITEM, NRCanvasItem))
#define NR_CANVAS_ITEM_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), NR_TYPE_CANVAS_ITEM, NRCanvasItemClass))
#define NR_IS_CANVAS_ITEM(o) (GTK_CHECK_TYPE ((o), NR_TYPE_CANVAS_ITEM))
#define NR_IS_CANVAS_ITEM_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), NR_TYPE_CANVAS_ITEM))

/*
 * State
 *
 * We use GtkObject flags bits 8-15
 */

typedef enum {
	NR_CANVAS_ITEM_STATE_INITIAL = 0,
	NR_CANVAS_ITEM_STATE_BBOX = 0x40,
	NR_CANVAS_ITEM_STATE_COVERAGE = 0x80,
	NR_CANVAS_ITEM_STATE_RENDER = 0xc0,
	NR_CANVAS_ITEM_STATE_COMPLETE = 0xff
} NRCanvasItemState;

#define NR_CI_GET_STATE(i) ((GTK_OBJECT_FLAGS (i) & 0xff00) >> 8)
#define NR_CI_SET_STATE(i,s) G_STMT_START{ GTK_OBJECT_FLAGS (i) = ((GTK_OBJECT_FLAGS (i) & 0xffff00ff) | ((guint32) (s) << 8)); }G_STMT_END

/*
 * Flags
 */

enum {
	NR_CANVAS_ITEM_MULTITHREAD = (1 << 4),
	NR_CANVAS_ITEM_FORCED_UPDATE = (1 << 5)
};

#define NR_CI_FLAG_MASK 0x30

#include <gtk/gtktypeutils.h>
#include <gtk/gtkobject.h>
#include "nr-primitives.h"
#include "nr-canvas.h"

struct _NRCanvasItem {
	GtkObject object;
	NRCanvas * canvas;
	NRCanvasItem * parent;
	NRIRect bbox;
	NRAffine * transform;
};

struct _NRCanvasItemClass {
	GtkObjectClass parent_class;
	/* To be used by NRCanvas */
	void (* add_child) (NRCanvasItem * item, NRCanvasItem * child, gint position);
	void (* remove_child) (NRCanvasItem * item, NRCanvasItem * child);
	/* Basic virtual methods */
	/* Returns ne state and cleans flags, if requested update is completed */
	NRCanvasItemState (* update) (NRCanvasItem * item, NRGraphicCtx * ctx, NRCanvasItemState state, guint32 flags);
	void (* render) (NRCanvasItem * item, NRDrawingArea * area);
	NRCanvasItem * (* pick) (NRCanvasItem * item, NRPoint * point);
};

GtkType nr_canvas_item_get_type (void);

/* To be used only by implementations */

NRCanvasItemState nr_canvas_item_invoke_update (NRCanvasItem * item, NRGraphicCtx * ctx, NRCanvasItemState state, guint32 flags);
void nr_canvas_item_invoke_render (NRCanvasItem * item, NRDrawingArea * area);
NRCanvasItem * nr_canvas_item_invoke_pick (NRCanvasItem * item, NRPoint * point);

/* Public methods */

void nr_canvas_item_request_update (NRCanvasItem * item, NRCanvasItemState state, guint32 flags);
void nr_canvas_item_set_transform (NRCanvasItem * item, NRAffine * transform);

#endif

#ifndef __SP_CANVAS_UTILS_H__
#define __SP_CANVAS_UTILS_H__

/*
 * Helper stuff for SPCanvas
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-canvas.h"

/* Miscellaneous utility & convenience functions for general canvas objects */

void sp_canvas_update_bbox (SPCanvasItem *item, int x1, int y1, int x2, int y2);
void sp_canvas_item_reset_bounds (SPCanvasItem *item);
void sp_canvas_buf_ensure_buf (SPCanvasBuf *buf);

void sp_canvas_item_move_to_z (SPCanvasItem * item, gint z);

gint sp_canvas_item_compare_z (SPCanvasItem * a, SPCanvasItem * b);

void sp_render_hline (NRPixBlock *pb, gint y, gint xs, gint xe, guint32 rgba);
void sp_render_vline (NRPixBlock *pb, gint x, gint ys, gint ye, guint32 rgba);
void sp_render_area (NRPixBlock *pb, gint xs, gint ys, gint xe, gint ye, guint32 rgba);

#endif

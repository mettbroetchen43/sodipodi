#ifndef __SP_CANVAS_UTILS_H__
#define __SP_CANVAS_UTILS_H__

/*
 * Helper stuff for GnomeCanvas
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

void gnome_canvas_update_bbox (GnomeCanvasItem *item, int x1, int y1, int x2, int y2);
void gnome_canvas_item_reset_bounds (GnomeCanvasItem *item);
void gnome_canvas_buf_ensure_buf (GnomeCanvasBuf *buf);
/*
 * Grabs canvas item, but unlike the original method does not pass
 * illegal key event mask to canvas, who passes it ahead to Gdk, but
 * instead sets event mask in canvas struct by hand
 */
int sp_canvas_item_grab (GnomeCanvasItem *item, unsigned int event_mask, GdkCursor *cursor, guint32 etime);



/* fill buffer with background color */

void
gnome_canvas_clear_buffer (GnomeCanvasBuf * buf);

/* render svp, translated to x, y */

void
gnome_canvas_render_svp_translated (GnomeCanvasBuf * buf, ArtSVP * svp,
	guint32 rgba, gint x, gint y);

/* get i2p (item to parent) affine transformation as general 6-element array */

void gnome_canvas_item_i2p_affine (GnomeCanvasItem * item, double affine[]);

/* get i2i (item to item) affine transformation as general 6-element array */

void gnome_canvas_item_i2i_affine (GnomeCanvasItem * from, GnomeCanvasItem * to, double affine[]);

/* set item affine matrix to achieve given i2w matrix */

void gnome_canvas_item_set_i2w_affine (GnomeCanvasItem * item, double i2w[]);

/* get item z-order in parent group */

gint gnome_canvas_item_order (GnomeCanvasItem * item);

void gnome_canvas_item_move_to_z (GnomeCanvasItem * item, gint z);

gint gnome_canvas_item_compare_z (GnomeCanvasItem * a, GnomeCanvasItem * b);

#endif

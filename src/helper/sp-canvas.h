#ifndef __SP_CANVAS_H__
#define __SP_CANVAS_H__

/*
 * Port of GnomeCanvas for sodiopodi needs
 *
 * Author:
 *   Federico Mena <federico@nuclecu.unam.mx>
 *   Raph Levien <raph@gimp.org>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1998 The Free Software Foundation
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#include <glib.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_rect.h>
#include <libart_lgpl/art_uta.h>
#include <gdk/gdk.h>
#include <gtk/gtklayout.h>

#include "helper-forward.h"

typedef struct {
	guchar *buf;
	int buf_rowstride;
	ArtIRect rect;
	/* Background color, given as 0xrrggbb */
	guint32 bg_color;
	/* Invariant: at least one of the following flags is true. */
	/* Set when the render rectangle area is the solid color bg_color */
	unsigned int is_bg : 1;
	/* Set when the render rectangle area is represented by the buf */
	unsigned int is_buf : 1;
} SPCanvasBuf;

struct _SPCanvasItem {
	GtkObject object;

	SPCanvas *canvas;
	SPCanvasItem *parent;

	double x1, y1, x2, y2;
	double *xform;
};

struct _SPCanvasItemClass {
	GtkObjectClass parent_class;

	void (* update) (SPCanvasItem *item, double *affine, unsigned int flags);

	void (* render) (SPCanvasItem *item, SPCanvasBuf *buf);
	double (* point) (SPCanvasItem *item, double x, double y, int cx, int cy, SPCanvasItem **actual_item);

	int (* event) (SPCanvasItem *item, GdkEvent *event);
};

SPCanvasItem *sp_canvas_item_new (SPCanvasGroup *parent, GtkType type, const gchar *first_arg_name, ...);
SPCanvasItem *sp_canvas_item_newv (SPCanvasGroup *parent, GtkType type, guint nargs, GtkArg *args);
void sp_canvas_item_construct (SPCanvasItem *item, SPCanvasGroup *parent, const gchar *first_arg_name, va_list args);
void sp_canvas_item_constructv (SPCanvasItem *item, SPCanvasGroup *parent, guint nargs, GtkArg *args);

#define sp_canvas_item_set gtk_object_set

void sp_canvas_item_affine_absolute (SPCanvasItem *item, const double affine[6]);

void sp_canvas_item_raise (SPCanvasItem *item, int positions);
void sp_canvas_item_lower (SPCanvasItem *item, int positions);
void sp_canvas_item_show (SPCanvasItem *item);
void sp_canvas_item_hide (SPCanvasItem *item);
int sp_canvas_item_grab (SPCanvasItem *item, unsigned int event_mask, GdkCursor *cursor, guint32 etime);
void sp_canvas_item_ungrab (SPCanvasItem *item, guint32 etime);

void sp_canvas_item_w2i (SPCanvasItem *item, double *x, double *y);
void sp_canvas_item_i2w (SPCanvasItem *item, double *x, double *y);
void sp_canvas_item_i2w_affine (SPCanvasItem *item, double affine[6]);

void sp_canvas_item_grab_focus (SPCanvasItem *item);

void sp_canvas_item_request_update (SPCanvasItem *item);

/* get item z-order in parent group */

gint sp_canvas_item_order (SPCanvasItem * item);

/* SPCanvas */

struct _SPCanvas {
	GtkLayout layout;

	guint idle_id;

	SPCanvasItem *root;

	guint root_destroy_id;

	/* Scrolling region */
	double scroll_x1, scroll_y1;
	double scroll_x2, scroll_y2;

	/* Area that needs redrawing, stored as a microtile array */
	ArtUta *redraw_area;

	/* Last known modifier state, for deferred repick when a button is down */
	int state;

	/* The item containing the mouse pointer, or NULL if none */
	SPCanvasItem *current_item;

	/* Item that is about to become current (used to track deletions and such) */
	SPCanvasItem *new_current_item;

	/* Item that holds a pointer grab, or NULL if none */
	SPCanvasItem *grabbed_item;

	/* Event mask specified when grabbing an item */
	guint grabbed_event_mask;

	/* If non-NULL, the currently focused item */
	SPCanvasItem *focused_item;

	/* Event on which selection of current item is based */
	GdkEvent pick_event;

	int close_enough;

	/* GC for temporary draw pixmap */
	GdkGC *pixmap_gc;

	unsigned int need_update : 1;
	unsigned int need_redraw : 1;
	unsigned int need_repick : 1;

	/* For use by internal pick_current_item() function */
	unsigned int left_grabbed_item : 1;
	/* For use by internal pick_current_item() function */
	unsigned int in_repick : 1;

	unsigned int aa : 1;
	unsigned int dither : 2;
};

GtkWidget *sp_canvas_new_aa (void);

SPCanvasGroup *sp_canvas_root (SPCanvas *canvas);
void sp_canvas_set_scroll_region (SPCanvas *canvas, double x1, double y1, double x2, double y2);

void sp_canvas_scroll_to (SPCanvas *canvas, int cx, int cy);
void sp_canvas_get_scroll_offsets (SPCanvas *canvas, int *cx, int *cy);
void sp_canvas_update_now (SPCanvas *canvas);

void sp_canvas_request_redraw_uta (SPCanvas *canvas, ArtUta *uta);
void sp_canvas_request_redraw (SPCanvas *canvas, int x1, int y1, int x2, int y2);

void sp_canvas_window_to_world (SPCanvas *canvas, double winx, double winy, double *worldx, double *worldy);
void sp_canvas_world_to_window (SPCanvas *canvas, double worldx, double worldy, double *winx, double *winy);

END_GNOME_DECLS

#endif

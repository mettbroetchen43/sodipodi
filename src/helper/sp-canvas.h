#ifndef __SP_CANVAS_H__
#define __SP_CANVAS_H__

/*
 * SVG <text> and <tspan> implementation
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

typedef struct _GnomeCanvas           GnomeCanvas;
typedef struct _GnomeCanvasClass      GnomeCanvasClass;
typedef struct _GnomeCanvasItem       GnomeCanvasItem;
typedef struct _GnomeCanvasItemClass  GnomeCanvasItemClass;
typedef struct _GnomeCanvasGroup      GnomeCanvasGroup;
typedef struct _GnomeCanvasGroupClass GnomeCanvasGroupClass;


#include <gtk/gtklayout.h>
#include <stdarg.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_rect.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_uta.h>
#include <libart_lgpl/art_affine.h>

enum {
	GNOME_CANVAS_ITEM_VISIBLE       = 1 << 7,
	GNOME_CANVAS_ITEM_NEED_UPDATE	= 1 << 8,
	GNOME_CANVAS_ITEM_NEED_AFFINE	= 1 << 9,
#if 0
	GNOME_CANVAS_ITEM_AFFINE_FULL	= 1 << 12
#endif
};

enum {
	GNOME_CANVAS_UPDATE_REQUESTED  = 1 << 0,
	GNOME_CANVAS_UPDATE_AFFINE     = 1 << 1,
};

/* Data for rendering in antialiased mode */
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
} GnomeCanvasBuf;


#define GNOME_TYPE_CANVAS_ITEM            (gnome_canvas_item_get_type ())
#define GNOME_CANVAS_ITEM(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_ITEM, GnomeCanvasItem))
#define GNOME_CANVAS_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_ITEM, GnomeCanvasItemClass))
#define GNOME_IS_CANVAS_ITEM(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_ITEM))
#define GNOME_IS_CANVAS_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_ITEM))

struct _GnomeCanvasItem {
	GtkObject object;

	GnomeCanvas *canvas;
	GnomeCanvasItem *parent;

	double x1, y1, x2, y2;
	double *xform;
};

struct _GnomeCanvasItemClass {
	GtkObjectClass parent_class;

	void (* update) (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);

	/* Binary compatibility with GnomeCanvas */
        void (* doop_1) (GnomeCanvasItem *item);
        void (* doop_2) (GnomeCanvasItem *item);
        void (* doop_3) (GnomeCanvasItem *item);
        void (* doop_4) (GnomeCanvasItem *item);
	void *(* doop_5) (GnomeCanvasItem *item);
	void (* doop_6) (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height);

	void (* render) (GnomeCanvasItem *item, GnomeCanvasBuf *buf);
	double (* point) (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item);

	void (* doop_7) (GnomeCanvasItem *item, double dx, double dy);
	void (* doop_8) (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2);

	int (* event) (GnomeCanvasItem *item, GdkEvent *event);
};

GtkType gnome_canvas_item_get_type (void);

GnomeCanvasItem *gnome_canvas_item_new (GnomeCanvasGroup *parent, GtkType type, const gchar *first_arg_name, ...);
GnomeCanvasItem *gnome_canvas_item_newv (GnomeCanvasGroup *parent, GtkType type, guint nargs, GtkArg *args);
void gnome_canvas_item_construct (GnomeCanvasItem *item, GnomeCanvasGroup *parent, const gchar *first_arg_name, va_list args);
void gnome_canvas_item_constructv (GnomeCanvasItem *item, GnomeCanvasGroup *parent, guint nargs, GtkArg *args);

void gnome_canvas_item_set (GnomeCanvasItem *item, const gchar *first_arg_name, ...);
void gnome_canvas_item_setv (GnomeCanvasItem *item, guint nargs, GtkArg *args);
void gnome_canvas_item_set_valist (GnomeCanvasItem *item, const gchar *first_arg_name, va_list args);

void gnome_canvas_item_affine_absolute (GnomeCanvasItem *item, const double affine[6]);

void gnome_canvas_item_raise (GnomeCanvasItem *item, int positions);
void gnome_canvas_item_lower (GnomeCanvasItem *item, int positions);
void gnome_canvas_item_show (GnomeCanvasItem *item);
void gnome_canvas_item_hide (GnomeCanvasItem *item);
int gnome_canvas_item_grab (GnomeCanvasItem *item, unsigned int event_mask, GdkCursor *cursor, guint32 etime);
void gnome_canvas_item_ungrab (GnomeCanvasItem *item, guint32 etime);

void gnome_canvas_item_w2i (GnomeCanvasItem *item, double *x, double *y);
void gnome_canvas_item_i2w (GnomeCanvasItem *item, double *x, double *y);
void gnome_canvas_item_i2w_affine (GnomeCanvasItem *item, double affine[6]);

void gnome_canvas_item_grab_focus (GnomeCanvasItem *item);

void gnome_canvas_item_request_update (GnomeCanvasItem *item);

#define GNOME_TYPE_CANVAS_GROUP            (gnome_canvas_group_get_type ())
#define GNOME_CANVAS_GROUP(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_GROUP, GnomeCanvasGroup))
#define GNOME_CANVAS_GROUP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_GROUP, GnomeCanvasGroupClass))
#define GNOME_IS_CANVAS_GROUP(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_GROUP))
#define GNOME_IS_CANVAS_GROUP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_GROUP))


struct _GnomeCanvasGroup {
	GnomeCanvasItem item;

	GList *items, *last;
};

struct _GnomeCanvasGroupClass {
	GnomeCanvasItemClass parent_class;
};

GtkType gnome_canvas_group_get_type (void);

/* GnomeCanvas */

#define GNOME_TYPE_CANVAS (gnome_canvas_get_type ())
#define GNOME_CANVAS(obj) (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS, GnomeCanvas))
#define GNOME_CANVAS_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS, GnomeCanvasClass))
#define GNOME_IS_CANVAS(obj) (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS))
#define GNOME_IS_CANVAS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS))

struct _GnomeCanvas {
	GtkLayout layout;

	guint idle_id;

	GnomeCanvasItem *root;

	guint root_destroy_id;

	/* Scrolling region */
	double scroll_x1, scroll_y1;
	double scroll_x2, scroll_y2;

	/* Binary compatibility */
	double bret_0;
	int bret_1, bret_2;
	int bret_3, bret_4;

	/* Area that needs redrawing, stored as a microtile array */
	ArtUta *redraw_area;

	int bret_5, bret_6;
	int bret_7, bret_8;

	/* Last known modifier state, for deferred repick when a button is down */
	int state;

	/* The item containing the mouse pointer, or NULL if none */
	GnomeCanvasItem *current_item;

	/* Item that is about to become current (used to track deletions and such) */
	GnomeCanvasItem *new_current_item;

	/* Item that holds a pointer grab, or NULL if none */
	GnomeCanvasItem *grabbed_item;

	/* Event mask specified when grabbing an item */
	guint grabbed_event_mask;

	/* If non-NULL, the currently focused item */
	GnomeCanvasItem *focused_item;

	/* Event on which selection of current item is based */
	GdkEvent pick_event;

	int close_enough;

	GdkColorContext *bret_x;

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

struct _GnomeCanvasClass {
	GtkLayoutClass parent_class;
};

GtkType gnome_canvas_get_type (void);

GtkWidget *gnome_canvas_new_aa (void);

GnomeCanvasGroup *gnome_canvas_root (GnomeCanvas *canvas);
void gnome_canvas_set_scroll_region (GnomeCanvas *canvas, double x1, double y1, double x2, double y2);

void gnome_canvas_scroll_to (GnomeCanvas *canvas, int cx, int cy);
void gnome_canvas_get_scroll_offsets (GnomeCanvas *canvas, int *cx, int *cy);
void gnome_canvas_update_now (GnomeCanvas *canvas);

void gnome_canvas_request_redraw_uta (GnomeCanvas *canvas, ArtUta *uta);
void gnome_canvas_request_redraw (GnomeCanvas *canvas, int x1, int y1, int x2, int y2);

void gnome_canvas_window_to_world (GnomeCanvas *canvas, double winx, double winy, double *worldx, double *worldy);
void gnome_canvas_world_to_window (GnomeCanvas *canvas, double worldx, double worldy, double *winx, double *winy);

END_GNOME_DECLS

#endif

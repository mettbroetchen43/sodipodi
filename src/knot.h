#ifndef SP_KNOT_H
#define SP_KNOT_H

/*
 * SPKnot
 *
 * A flexible handle for most transforming contexts. It can:
 * - display rectangles, circles, diamonds, bitmaps and pixmaps
 * - change color and/or curson on mouseover
 * - test snapping and suggest actaul distance
 *
 * Copyright (C) Lauris Kaplinski <lauris@ariman.ee> 2000
 *
 */

#include <gtk/gtktypeutils.h>
#include <gtk/gtkpacker.h>
#include <libgnomeui/gnome-canvas.h>
#include "forward.h"

typedef enum {
	SP_KNOT_SHAPE_SQUARE,
	SP_KNOT_SHAPE_DIAMOND,
	SP_KNOT_SHAPE_CIRCLE,
	SP_KNOT_SHAPE_BITMAP,
	SP_KNOT_SHAPE_IMAGE
} SPKnotShapeType;

typedef enum {
	SP_KNOT_STATE_NORMAL,
	SP_KNOT_STATE_MOUSEOVER,
	SP_KNOT_STATE_DRAGGING,
	SP_KNOT_STATE_HIDDEN
} SPKnotStateType;

#define SP_KNOT_VISIBLE_STATES 3

enum {
	SP_KNOT_VISIBLE = 1 << 0,
	SP_KNOT_MOUSEOVER = 1 << 1,
	SP_KNOT_DRAGGING = 1 << 2,
	SP_KNOT_GRABBED = 1 << 3
};

typedef struct _SPKnot SPKnot;
typedef struct _SPKnotClass SPKnotClass;

#define SP_TYPE_KNOT            (sp_knot_get_type ())
#define SP_KNOT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_KNOT, SPKnot))
#define SP_KNOT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_KNOT, SPKnotClass))
#define SP_IS_KNOT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_KNOT))
#define SP_IS_KNOT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_KNOT))

struct _SPKnot {
	GtkObject object;
	SPDesktop * desktop;		/* Desktop we are on */
	GnomeCanvasItem * item;		/* Our CanvasItem */
	guint flags;

	guint size;			/* Always square */
	gdouble x, y;			/* Our desktop coordinates */
	gdouble hx, hy;			/* Grabbed relative position */
	GtkAnchorType anchor;		/* Anchor */

	SPKnotShapeType shape;		/* Shape type */

	guint32 fill [SP_KNOT_VISIBLE_STATES];
	guint32 stroke [SP_KNOT_VISIBLE_STATES];
	guchar * image [SP_KNOT_VISIBLE_STATES];

	GdkCursor * cursor [SP_KNOT_VISIBLE_STATES];

	GdkCursor * saved_cursor;
};

struct _SPKnotClass {
	GtkObjectClass parent_class;

	gint (* event) (SPKnot * knot, GdkEvent * event);

	/*
	 * These are unconditional
	 */

	void (* clicked) (SPKnot * knot, guint state);
	void (* grabbed) (SPKnot * knot, guint state);
	void (* ungrabbed) (SPKnot * knot, guint state);
	void (* moved) (SPKnot * knot, ArtPoint * position, guint state);

	/*
	 * Request knot to move to absolute position
	 */

	gboolean (* request) (SPKnot * knot, ArtPoint * position, guint state);

	/*
	 * Find complex distance from knot to point
	 */

	gdouble (* distance) (SPKnot * knot, ArtPoint * position, guint state);
};

GtkType sp_knot_get_type (void);

SPKnot * sp_knot_new (SPDesktop * desktop);

#define SP_KNOT_IS_VISIBLE(k) ((k->flags & SP_KNOT_VISIBLE) != 0)
#define SP_KNOT_IS_MOSEOVER(k) ((k->flags & SP_KNOT_MOUSEOVER) != 0)
#define SP_KNOT_IS_DRAGGING(k) ((k->flags & SP_KNOT_DRAGGING) != 0)
#define SP_KNOT_IS_GRABBED(k) ((k->flags & SP_KNOT_GRABBED) != 0)

void sp_knot_show (SPKnot * knot);
void sp_knot_hide (SPKnot * knot);

void sp_knot_request_position (SPKnot * knot, ArtPoint * position, guint state);
gdouble sp_knot_distance (SPKnot * knot, ArtPoint * p, guint state);

/* Unconditional */

void sp_knot_set_position (SPKnot * knot, ArtPoint * p, guint state);

ArtPoint * sp_knot_position (SPKnot * knot, ArtPoint * p);

#endif

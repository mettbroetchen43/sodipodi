#ifndef __SP_ITEM_H__
#define __SP_ITEM_H__

/*
 * Abstract base class for all nodes
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtk/gtkmenu.h>
#include <libgnomeprint/gnome-print.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_pixbuf.h>
#include "helper/units.h"
#include "display/nr-arena-forward.h"
#include "forward.h"
#include "sp-object.h"
#include "knotholder.h"

#define SP_TYPE_ITEM (sp_item_get_type ())
#define SP_ITEM(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_ITEM, SPItem))
#define SP_ITEM_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_ITEM, SPItemClass))
#define SP_IS_ITEM(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_ITEM))
#define SP_IS_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_ITEM))

/* fixme: This is just placeholder */
/*
 * Plan:
 * We do extensible event structure, that hold applicable (ui, non-ui)
 * data pointers. So it is up to given object/arena implementation
 * to process correct ones in meaningful way.
 * Also, this probably goes to SPObject base class
 *
 */

typedef enum {
	SP_EVENT_INVALID,
	SP_EVENT_NONE,
	SP_EVENT_ACTIVATE,
	SP_EVENT_MOUSEOVER,
	SP_EVENT_MOUSEOUT
} SPEventType;

struct _SPEvent {
	SPEventType type;
	gpointer data;
};

typedef struct _SPItemView SPItemView;

struct _SPItemView {
	SPItemView *next;
	SPItemView *prev;
	SPItem *item;
	NRArena *arena;
	NRArenaItem *arenaitem;
};

struct _SPItem {
	SPObject object;
	guint sensitive : 1;
	guint stop_paint: 1;	/* If set, ::paint returns TRUE */
	gdouble affine[6];
	SPItemView *display;
	SPClipPath *clip;
};

struct _SPItemClass {
	SPObjectClass parent_class;

	/* BBox in given coordinate system */
	void (* bbox) (SPItem *item, ArtDRect *bbox, const gdouble *transform);

	/* Give list of points for item to be controled */
	SPKnotHolder *(* knot_holder) (SPItem *item, SPDesktop *desktop);

	/* Printing method. Assumes ctm is set to item affine matrix */
	void (* print) (SPItem * item, GnomePrintContext * gpc);

	/* Give short description of item (for status display) */
	gchar * (* description) (SPItem * item);

	/* Silly, silly. We should assign handlers a more intelligent way */
	NRArenaItem * (* show) (SPItem *item, NRArena *arena);
	void (* hide) (SPItem * item, NRArena *arena);

	/* Append to context menu */
	void (* menu) (SPItem * item, SPDesktop *desktop, GtkMenu * menu);
	/* give list of points for item to be considered for snapping */ 
	GSList * (* snappoints) (SPItem * item, GSList * points);

	/* Write item transform to repr optimally */
	void (* write_transform) (SPItem *item, SPRepr *repr, gdouble *transform);

	/* Emit event, if applicable */
	gint (* event) (SPItem *item, SPEvent *event);
};

/* Flag testing macros */

#define SP_ITEM_STOP_PAINT(i) (SP_ITEM (i)->stop_paint)

/* Standard Gtk function */

GtkType sp_item_get_type (void);

/* Methods */

void sp_item_invoke_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform);
SPKnotHolder *sp_item_knot_holder (SPItem *item, SPDesktop *desktop);
gchar * sp_item_description (SPItem * item);
void sp_item_print (SPItem * item, GnomePrintContext * gpc);

/* Shows/Hides item on given arena display list */
NRArenaItem *sp_item_show (SPItem *item, NRArena *arena);
void sp_item_hide (SPItem *item, NRArena *arena);

gboolean sp_item_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[]);

GSList * sp_item_snappoints (SPItem * item);

void sp_item_write_transform (SPItem *item, SPRepr *repr, gdouble *transform);

gint sp_item_event (SPItem *item, SPEvent *event);

void sp_item_set_item_transform (SPItem *item, const gdouble *transform);

/* Utility */

void sp_item_bbox_desktop (SPItem *item, ArtDRect *bbox);
gdouble *sp_item_i2doc_affine (SPItem *item, gdouble affine[]);
/* Transformation to normalized (0,0-1,1) viewport */
gdouble *sp_item_i2vp_affine (SPItem *item, gdouble affine[]);

/* fixme: - these are evil, but OK */

gdouble *sp_item_i2d_affine (SPItem *item, gdouble affine[]);
void sp_item_set_i2d_affine (SPItem *item, gdouble affine[]);

gdouble *sp_item_dt2i_affine (SPItem *item, SPDesktop *dt, gdouble affine[]);

/* Context menu stuff */

void sp_item_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);

SPItemView *sp_item_view_new_prepend (SPItemView *list, SPItem *item, NRArena *arena, NRArenaItem *arenaitem);
SPItemView *sp_item_view_list_remove (SPItemView *list, SPItemView *view);

/* Convert distances into SVG units */

gdouble sp_item_distance_to_svg_viewport (SPItem *item, gdouble distance, const SPUnit *unit);
gdouble sp_item_distance_to_svg_bbox (SPItem *item, gdouble distance, const SPUnit *unit);

#endif

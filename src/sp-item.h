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

#include <libnr/nr-types.h>
#include <gtk/gtkmenu.h>
#include "helper/units.h"
#include "display/nr-arena-forward.h"
#include "forward.h"
#include "sp-object.h"
#include "knotholder.h"

/* fixme: This is just placeholder */
/*
 * Plan:
 * We do extensible event structure, that hold applicable (ui, non-ui)
 * data pointers. So it is up to given object/arena implementation
 * to process correct ones in meaningful way.
 * Also, this probably goes to SPObject base class
 *
 */

enum {
	SP_EVENT_INVALID,
	SP_EVENT_NONE,
	SP_EVENT_ACTIVATE,
	SP_EVENT_MOUSEOVER,
	SP_EVENT_MOUSEOUT
};

struct _SPEvent {
	unsigned int type;
	gpointer data;
};

typedef struct _SPItemView SPItemView;

struct _SPItemView {
	SPItemView *next;
	SPItem *item;
	NRArena *arena;
	NRArenaItem *arenaitem;
};

enum {
	SP_ITEM_BBOX_LOGICAL,
	SP_ITEM_BBOX_VISUAL
};

typedef struct _SPItemCtx SPItemCtx;

struct _SPItemCtx {
	SPCtx ctx;
	NRMatrixD ctm;
};

struct _SPItem {
	SPObject object;

	guint sensitive : 1;
	guint stop_paint: 1;

	NRMatrixF transform;

	SPObject *clip;

	SPItemView *display;
};

struct _SPItemClass {
	SPObjectClass parent_class;

	/* BBox union in given coordinate system */
	void (* bbox) (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags);

	/* Give list of points for item to be controled */
	SPKnotHolder *(* knot_holder) (SPItem *item, SPDesktop *desktop);

	/* Printing method. Assumes ctm is set to item affine matrix */
	/* fixme: Think about it, and maybe implement generic export method instead (Lauris) */
	void (* print) (SPItem *item, SPPrintContext *ctx);

	/* Give short description of item (for status display) */
	gchar * (* description) (SPItem * item);

	NRArenaItem * (* show) (SPItem *item, NRArena *arena);
	void (* hide) (SPItem *item, NRArena *arena);

	/* give list of points for item to be considered for snapping */ 
	GSList * (* snappoints) (SPItem * item, GSList * points);

	/* Write item transform to repr optimally */
	void (* write_transform) (SPItem *item, SPRepr *repr, NRMatrixF *transform);

	/* Emit event, if applicable */
	gint (* event) (SPItem *item, SPEvent *event);

	/* Append to context menu */
	/* fixme: i do not want this, so move to some other place (Lauris) */
	void (* menu) (SPItem * item, SPDesktop *desktop, GtkMenu * menu);
};

/* Flag testing macros */

#define SP_ITEM_STOP_PAINT(i) (SP_ITEM (i)->stop_paint)

/* Methods */

void sp_item_invoke_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int clear);
void sp_item_invoke_bbox_full (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags, unsigned int clear);

SPKnotHolder *sp_item_knot_holder (SPItem *item, SPDesktop *desktop);
gchar * sp_item_description (SPItem * item);
void sp_item_invoke_print (SPItem *item, SPPrintContext *ctx);

/* Shows/Hides item on given arena display list */
NRArenaItem *sp_item_show (SPItem *item, NRArena *arena);
void sp_item_hide (SPItem *item, NRArena *arena);

GSList * sp_item_snappoints (SPItem * item);

void sp_item_write_transform (SPItem *item, SPRepr *repr, NRMatrixF *transform);

gint sp_item_event (SPItem *item, SPEvent *event);

void sp_item_set_item_transform (SPItem *item, const NRMatrixF *transform);

/* Utility */

void sp_item_bbox_desktop (SPItem *item, NRRectF *bbox);
NRMatrixF *sp_item_i2doc_affine (SPItem *item, NRMatrixF *transform);
NRMatrixF *sp_item_i2root_affine (SPItem *item, NRMatrixF *transform);
/* Transformation to normalized (0,0-1,1) viewport */
NRMatrixF *sp_item_i2vp_affine (SPItem *item, NRMatrixF *transform);

/* fixme: - these are evil, but OK */

NRMatrixF *sp_item_i2d_affine (SPItem *item, NRMatrixF *transform);
void sp_item_set_i2d_affine (SPItem *item, const NRMatrixF *transform);

NRMatrixF *sp_item_dt2i_affine (SPItem *item, SPDesktop *dt, NRMatrixF *transform);

/* Context menu stuff */

void sp_item_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);

SPItemView *sp_item_view_new_prepend (SPItemView *list, SPItem *item, NRArena *arena, NRArenaItem *arenaitem);
SPItemView *sp_item_view_list_remove (SPItemView *list, SPItemView *view);

/* Convert distances into SVG units */

gdouble sp_item_distance_to_svg_viewport (SPItem *item, gdouble distance, const SPUnit *unit);
gdouble sp_item_distance_to_svg_bbox (SPItem *item, gdouble distance, const SPUnit *unit);

#endif

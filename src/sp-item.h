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

#include "helper/units.h"
#include "display/nr-arena-forward.h"

#include "forward.h"

#include "sp-object.h"

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
	unsigned int flags;
	unsigned int key;
	/* SPItem *item; */
	NRArenaItem *arenaitem;
};

/* flags */

#define SP_ITEM_BBOX_VISUAL 1

#define SP_ITEM_SHOW_DISPLAY (1 << 0)
#define SP_ITEM_SHOW_PRINT (1 << 1)

/*
 * We do not differentiate referenced views (i.e. clippaths, maks and patterns)
 * by display/print targets (they are non-editable anyways)
 */
#define SP_ITEM_REFERENCE_FLAGS SP_ITEM_SHOW_PRINT

typedef struct _SPItemCtx SPItemCtx;

struct _SPItemCtx {
	SPCtx ctx;
	/* Item to document transformation */
	NRMatrixD i2doc;
	/* Viewport size */
	NRRectD vp;
	/* Item to viewport transformation */
	NRMatrixD i2vp;
};

struct _SPItem {
	SPObject object;

	unsigned int sensitive : 1;
	unsigned int printable : 1;
	/* unsigned int stop_paint: 1; */

	/* True if item establishes new viewport */
	unsigned int has_viewport : 1;
	/* True if item has additional transform (viewBox) */
	unsigned int has_extra_transform : 1;

	NRMatrixF transform;

	SPObject *clip;
	SPObject *mask;

	SPItemView *display;
};

struct _SPItemClass {
	SPObjectClass parent_class;

	/* BBox union in given coordinate system */
	void (* bbox) (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags);

	/* Give short description of item (for status display) */
	gchar * (* description) (SPItem * item);

	NRArenaItem * (* show) (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags);
	void (* hide) (SPItem *item, unsigned int key);

	/* Printing method. Assumes ctm is set to item affine matrix */
	/* fixme: Think about it, and maybe implement generic export method instead (Lauris) */
	void (* print) (SPItem *item, SPPrintContext *ctx);

	/* Returns a number of points used */ 
	int (* snappoints) (SPItem *item, NRPointF *points, int size, const NRMatrixF *transform);

	/* Write item transform to repr optimally */
	void (* write_transform) (SPItem *item, SPRepr *repr, NRMatrixF *transform);

	/* Emit event, if applicable */
	gint (* event) (SPItem *item, SPEvent *event);

	/* Item master is coordinate system, where item properties are defined */
	/* Viewport master is where viewport dimensions are defined */
	/* In case of viewbox, viewport (and item) master differ from propagated coordinates */
	/* Appends item extra transform from child to item master coordinate system) */
	unsigned int (* get_extra_transform) (SPItem *item, NRMatrixD *ch2m);
	/* Get item viewport in master coordinates */
	unsigned int (* get_viewport) (SPItem *item, NRRectF *vieport);
};

/* Flag testing macros */

#define SP_ITEM_STOP_PAINT(i) (SP_ITEM (i)->stop_paint)

/* Methods */

void sp_item_invoke_bbox (SPItem *item, NRRectF *bb, const NRMatrixD *t, unsigned int clear);
void sp_item_invoke_bbox_full (SPItem *item, NRRectF *bb, const NRMatrixD *t, unsigned int flags, unsigned int clear);

unsigned int sp_item_get_extra_transform (SPItem *item, NRMatrixD *transform);
SPItem *sp_item_get_viewport (SPItem *item, NRRectF *viewport, NRMatrixD *i2vp);

void sp_item_invoke_print (SPItem *item, SPPrintContext *ctx);
unsigned char * sp_item_description (SPItem * item);

/* Shows/Hides item on given arena display list */
unsigned int sp_item_display_key_new (unsigned int numkeys);
NRArenaItem *sp_item_invoke_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags);
void sp_item_invoke_hide (SPItem *item, unsigned int key);

int sp_item_snappoints (SPItem *item, NRPointF *points, int size, const NRMatrixF *transform);

void sp_item_write_transform (SPItem *item, SPRepr *repr, NRMatrixF *transform);

gint sp_item_event (SPItem *item, SPEvent *event);

void sp_item_set_item_transform (SPItem *item, const NRMatrixF *transform);

/* Utility */

NRMatrixF *sp_item_i2doc_affine (SPItem *item, NRMatrixF *transform);
NRMatrixF *sp_item_i2root_affine (SPItem *item, NRMatrixF *transform);

void sp_item_get_bbox_document (SPItem *item, NRRectF *bb, unsigned int flags, unsigned int clear);

/* Transformation to normalized (0,0-1,1) viewport */
NRMatrixF *sp_item_i2vp_affine (SPItem *item, NRMatrixF *transform);

#if 0
/* fixme: - these are evil, but OK */
void sp_item_set_i2d_affine (SPItem *item, const NRMatrixF *transform);
NRMatrixF *sp_item_dt2i_affine (SPItem *item, SPDesktop *dt, NRMatrixF *transform);
#endif

/* Convert distances into SVG units */

gdouble sp_item_distance_to_svg_viewport (SPItem *item, gdouble distance, const SPUnit *unit);
gdouble sp_item_distance_to_svg_bbox (SPItem *item, gdouble distance, const SPUnit *unit);

/* Utility */
int sp_corner_snappoints (NRPointF *points, int size, const NRMatrixF *transform, float x0, float y0, float x1, float y1);

#endif

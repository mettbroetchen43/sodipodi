#ifndef __NR_ARENA_ITEM_H__
#define __NR_ARENA_ITEM_H__

/*
 * RGBA display list system for sodipodi
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define NR_TYPE_ARENA_ITEM (nr_arena_item_get_type ())
#define NR_ARENA_ITEM(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), NR_TYPE_ARENA_ITEM, NRArenaItem))
#define NR_ARENA_ITEM_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), NR_TYPE_ARENA_ITEM, NRArenaItemClass))
#define NR_IS_ARENA_ITEM(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), NR_TYPE_ARENA_ITEM))
#define NR_IS_ARENA_ITEM_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), NR_TYPE_ARENA_ITEM))

#define NR_ARENA_ITEM_VIRTUAL(i,m) (((NRArenaItemClass *) G_OBJECT_GET_CLASS (i))->m)

typedef struct _NRGC NRGC;

/* Warning: This is UNDEFINED in NR, implementations should do that */
typedef struct _NREvent NREvent;

/* State flags */

#define NR_ARENA_ITEM_STATE_INVALID  (1 << 0)
#define NR_ARENA_ITEM_STATE_BBOX     (1 << 1)
#define NR_ARENA_ITEM_STATE_COVERAGE (1 << 2)
#define NR_ARENA_ITEM_STATE_DRAFT    (1 << 3)
#define NR_ARENA_ITEM_STATE_RENDER   (1 << 4)
#define NR_ARENA_ITEM_STATE_CLIP     (1 << 5)
#define NR_ARENA_ITEM_STATE_MASK     (1 << 6)
#define NR_ARENA_ITEM_STATE_PICK     (1 << 7)
#define NR_ARENA_ITEM_STATE_IMAGE    (1 << 8)

#define NR_ARENA_ITEM_STATE_NONE  0x0000
#define NR_ARENA_ITEM_STATE_ALL   0x01fe

#define NR_ARENA_ITEM_STATE(i,s) (NR_ARENA_ITEM (i)->state & (s))
#define NR_ARENA_ITEM_SET_STATE(i,s) (NR_ARENA_ITEM (i)->state |= (s))
#define NR_ARENA_ITEM_UNSET_STATE(i,s) (NR_ARENA_ITEM (i)->state &= ^(s))

#define NR_ARENA_ITEM_RENDER_NO_CACHE (1 << 0)

#include <glib-object.h>
#include <libnr/nr-types.h>
#include <libnr/nr-pixblock.h>
#include "nr-arena-forward.h"

struct _NRGC {
	NRMatrixD transform;
};

struct _NRArenaItem {
	GObject object;
	NRArena *arena;
	NRArenaItem *parent;
	NRArenaItem *next;
	NRArenaItem *prev;

	/* Item state */
	guint state : 16;
	guint propagate : 1;
	guint sensitive : 1;
	/* BBox in grid coordinates */
	NRRectL bbox;
	/* Our affine */
	NRMatrixF *transform;
	/* Our opacity */
	float opacity;
	/* Clip item */
	NRArenaItem *clip;
	/* Rendered buffer */
	unsigned char *px;
};

struct _NRArenaItemClass {
	GObjectClass parent_class;

	NRArenaItem * (* children) (NRArenaItem *item);
	void (* add_child) (NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref);
	void (* remove_child) (NRArenaItem *item, NRArenaItem *child);
	void (* set_child_position) (NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref);

	guint (* update) (NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset);
	unsigned int (* render) (NRArenaItem *item, NRRectL *area, NRPixBlock *pb, unsigned int flags);
	guint (* clip) (NRArenaItem *item, NRRectL *area, NRPixBlock *pb);
	NRArenaItem * (* pick) (NRArenaItem *item, gdouble x, gdouble y, gdouble delta, gboolean sticky);

	gint (* event) (NRArenaItem *item, NREvent *event);
};

#define NR_ARENA_ITEM_ARENA(ai) (NR_ARENA_ITEM (ai)->arena)

GType nr_arena_item_get_type (void);

NRArenaItem *nr_arena_item_ref (NRArenaItem *item);
NRArenaItem *nr_arena_item_unref(NRArenaItem *item);

NRArenaItem *nr_arena_item_children (NRArenaItem *item);
void nr_arena_item_add_child (NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref);
void nr_arena_item_remove_child (NRArenaItem *item, NRArenaItem *child);
void nr_arena_item_set_child_position (NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref);

/*
 * Invoke update to given state, if item is inside area
 *
 * area == NULL is infinite
 * gc is PARENT gc for invoke, CHILD gc in corresponding virtual method
 * state - requested to state
 * reset - reset to state
 * reset == 0 means no reset
 */

guint nr_arena_item_invoke_update (NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset);
unsigned int nr_arena_item_invoke_render (NRArenaItem *item, NRRectL *area, NRPixBlock *pb, unsigned int flags);
guint nr_arena_item_invoke_clip (NRArenaItem *item, NRRectL *area, NRPixBlock *pb);
NRArenaItem *nr_arena_item_invoke_pick (NRArenaItem *item, gdouble x, gdouble y, gdouble delta, gboolean sticky);

gint nr_arena_item_emit_event (NRArenaItem *item, NREvent *event);

void nr_arena_item_request_update (NRArenaItem *item, guint reset, gboolean propagate);
void nr_arena_item_request_render (NRArenaItem *item);

/* Public */

NRArenaItem *nr_arena_item_new (NRArena *arena, GType type);
NRArenaItem *nr_arena_item_destroy (NRArenaItem *item);

void nr_arena_item_append_child (NRArenaItem *parent, NRArenaItem *child);

void nr_arena_item_set_transform (NRArenaItem *item, const NRMatrixF *transform);
void nr_arena_item_set_opacity (NRArenaItem *item, gdouble opacity);
void nr_arena_item_set_sensitive (NRArenaItem *item, gboolean sensitive);
void nr_arena_item_set_clip (NRArenaItem *item, NRArenaItem *clip);
void nr_arena_item_set_order (NRArenaItem *item, gint order);

/* Helpers */

NRArenaItem *nr_arena_item_attach_ref (NRArenaItem *parent, NRArenaItem *child, NRArenaItem *prev, NRArenaItem *next);
NRArenaItem *nr_arena_item_detach_unref (NRArenaItem *parent, NRArenaItem *child);

#endif

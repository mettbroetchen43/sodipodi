#define _SP_CANVAS_GROUP_C_

/*
 * SPCanvasGroup
 *
 * Group of SPCanvasItems
 *
 * Author: Lauris Kaplinski <lauris@helixcode.com>
 * Copyright (C) 2000 Helix Code, Inc.
 *
 */

#include "nr-aa-canvas.h" /* For AA stuff */
#include "sp-canvas-group.h"

static void sp_canvas_group_class_init (SPCanvasGroupClass * klass);
static void sp_canvas_group_init (SPCanvasGroup * group);
static void sp_canvas_group_destroy (GtkObject * object);

static void sp_canvas_group_add_child (NRCanvasItem * item, NRCanvasItem * child, gint position);
static void sp_canvas_group_remove_child (NRCanvasItem * item, NRCanvasItem * child);

static NRCanvasItemState sp_canvas_group_update (NRCanvasItem * item, NRGraphicCtx * ctx, NRCanvasItemState state, guint32 flags);
static void sp_canvas_group_render (NRCanvasItem * item, NRDrawingArea * area);
static NRCanvasItem * sp_canvas_group_pick (NRCanvasItem * item, NRPoint * point);

static SPCanvasItemClass * parent_class;

GtkType
sp_canvas_group_get_type (void)
{
	static GtkType group_type = 0;
	if (!group_type) {
		GtkTypeInfo group_info = {
			"SPCanvasGroup",
			sizeof (SPCanvasGroup),
			sizeof (SPCanvasGroupClass),
			(GtkClassInitFunc) sp_canvas_group_class_init,
			(GtkObjectInitFunc) sp_canvas_group_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		group_type = gtk_type_unique (sp_canvas_item_get_type (), &group_info);
	}
	return group_type;
}

static void
sp_canvas_group_class_init (SPCanvasGroupClass * klass)
{
	GtkObjectClass * object_class;
	NRCanvasItemClass * item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (NRCanvasItemClass *) klass;

	parent_class = gtk_type_class (sp_canvas_item_get_type ());

	object_class->destroy = sp_canvas_group_destroy;

	item_class->add_child = sp_canvas_group_add_child;
	item_class->remove_child = sp_canvas_group_remove_child;
	item_class->update = sp_canvas_group_update;
	item_class->render = sp_canvas_group_render;
	item_class->pick = sp_canvas_group_pick;
}

static void
sp_canvas_group_init (SPCanvasGroup * group)
{
	group->children = NULL;
	group->last = NULL;
}

static void
sp_canvas_group_destroy (GtkObject * object)
{
	SPCanvasGroup * group;

	group = (SPCanvasGroup *) object;

	/* fixme: use unparent? */

	while (group->children) {
		NRCanvasItem * child;
		child = (NRCanvasItem *) group->children->data;
		child->parent = NULL;
		group->children = g_list_remove (group->children, child);
		gtk_object_unref ((GtkObject *) child);
	}
	group->last = NULL;

	if (((GtkObjectClass *) (parent_class))->destroy) (* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_canvas_group_add_child (NRCanvasItem * item, NRCanvasItem * child, gint position)
{
	SPCanvasGroup * group;

	group = (SPCanvasGroup *) item;

	gtk_object_ref ((GtkObject *) child);

	if (!group->children) {
		g_assert (!group->last);
		group->last = group->children = g_list_prepend (NULL, child);
	} else {
		g_assert (group->last);
		if (position == 0) {
			group->children = g_list_prepend (group->children, child);
		} else if (position < 0) {
			g_list_append (group->last, child);
			group->last = group->last->next;
		} else {
			GList * l;
			l = g_list_nth (group->children, position - 1);
			l->next = g_list_prepend (l->next, child);
			if (l == group->last) group->last = l->next;
		}
	}

	child->parent = item;
}

static void
sp_canvas_group_remove_child (NRCanvasItem * item, NRCanvasItem * child)
{
	SPCanvasGroup * group;
	GList * p, * l;

	group = (SPCanvasGroup *) item;

	p = NULL;
	for (l = group->children; l != NULL; l = l->next) {
		if (l->data == child) {
			child->parent = NULL;
			if (l == group->last) {
				group->last = p;
			}
			if (p != NULL) {
				p->next = g_list_remove (l, child);
			} else {
				group->children = g_list_remove (group->children, child);
			}
			gtk_object_unref ((GtkObject *) child);
			return;
		}
		p = l;
	}
}

static NRCanvasItemState
sp_canvas_group_update (NRCanvasItem * item, NRGraphicCtx * ctx, NRCanvasItemState state, guint32 flags)
{
	SPCanvasGroup * group;
	NRCanvasItemState beststate;
	NRAAGraphicCtx * aactx;
	GList * l;

	/* Our parent class saves ctx */
	if (((NRCanvasItemClass *) parent_class)->update) (* ((NRCanvasItemClass *) parent_class)->update) (item, ctx, state, flags);

	group = (SPCanvasGroup *) item;
	aactx = (NRAAGraphicCtx *) ctx;

	beststate = NR_CANVAS_ITEM_STATE_COMPLETE;

	for (l = group->children; l != NULL; l = l->next) {
		NRCanvasItem * child;
		NRCanvasItemState childstate;
		child = (NRCanvasItem *) l->data;
		childstate = NR_CI_GET_STATE (child);
		if ((childstate < state) || (flags & NR_CANVAS_ITEM_FORCED_UPDATE)) {
			NRAAGraphicCtx childctx;
			childctx = *aactx;
			if (child->transform) {
				nr_affine_multiply (&childctx.ctx.transform, child->transform, &ctx->transform);
			}
			childstate = nr_canvas_item_invoke_update (child, (NRGraphicCtx *) &childctx, state, flags);
		}
		if (childstate < beststate) beststate = childstate;
	}

	return beststate;
}

static void
sp_canvas_group_render (NRCanvasItem * item, NRDrawingArea * area)
{
	SPCanvasGroup * group;
	GList * l;

	group = (SPCanvasGroup *) item;

	for (l = group->children; l != NULL; l = l->next) {
		NRCanvasItem * child;
		child = (NRCanvasItem *) l->data;
		if (nr_irect_do_intersect (&child->bbox, &area->rect)) {
			NRCanvasItemState childstate;
			childstate = NR_CI_GET_STATE (child);
			if (childstate < NR_CANVAS_ITEM_STATE_RENDER) {
				NRAAGraphicCtx childctx;
				childctx = *((NRAAGraphicCtx *) item->ctx);
				if (child->transform) {
					nr_affine_multiply (&childctx.ctx.transform, child->transform, &item->ctx->transform);
				}
				childstate = nr_canvas_item_invoke_update (child, (NRGraphicCtx *) &childctx,
									   NR_CANVAS_ITEM_STATE_RENDER, 0);
			}
			nr_canvas_item_invoke_render (child, area);
		}
	}
}

static NRCanvasItem *
sp_canvas_group_pick (NRCanvasItem * item, NRPoint * point)
{
	SPCanvasGroup * group;
	NRCanvasItem * picked;
	GList * l;

	group = (SPCanvasGroup *) item;

	for (l = group->last; l != NULL; l = l->prev) {
		NRCanvasItem * child;
		child = (NRCanvasItem *) l->data;
		if (nr_irect_point_is_inside (&child->bbox, point)) {
			NRCanvasItemState childstate;
			childstate = NR_CI_GET_STATE (child);
			if (childstate < NR_CANVAS_ITEM_STATE_RENDER) {
				NRAAGraphicCtx childctx;
				childctx = *((NRAAGraphicCtx *) item->ctx);
				if (child->transform) {
					nr_affine_multiply (&childctx.ctx.transform, child->transform, &item->ctx->transform);
				}
				childstate = nr_canvas_item_invoke_update (child, (NRGraphicCtx *) &childctx,
									   NR_CANVAS_ITEM_STATE_RENDER, 0);
			}
			picked = nr_canvas_item_invoke_pick (child, point);
			if (picked) return picked;
		}
	}

	return NULL;
}


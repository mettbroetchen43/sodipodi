#define __SP_GROUP_C__

/*
 * SVG <g> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "display/nr-arena-group.h"
#include "xml/repr-private.h"
#include "sp-object-repr.h"
#include "svg/svg.h"
#include "document.h"
#include "style.h"

#include "enums.h"
#include "attributes.h"
#include "sp-root.h"
#include "sp-item-group.h"
#include "helper/sp-intl.h"

static void sp_group_class_init (SPGroupClass *klass);
static void sp_group_init (SPGroup *group);

static void sp_group_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_group_release (SPObject *object);
static void sp_group_child_added (SPObject * object, SPRepr * child, SPRepr * ref);
static unsigned int sp_group_remove_child (SPObject *object, SPRepr *child);
static void sp_group_order_changed (SPObject * object, SPRepr * child, SPRepr * old, SPRepr * new);
static void sp_group_update (SPObject *object, SPCtx *ctx, guint flags);
static void sp_group_modified (SPObject *object, guint flags);
static unsigned int sp_group_sequence (SPObject *object, SPObject *target, unsigned int *seq);
static SPRepr *sp_group_write (SPObject *object, SPRepr *repr, guint flags);

static void sp_group_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags);
static void sp_group_print (SPItem * item, SPPrintContext *ctx);
static gchar * sp_group_description (SPItem * item);
static NRArenaItem *sp_group_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags);
static void sp_group_hide (SPItem * item, unsigned int key);

static SPItemClass * parent_class;

GType
sp_group_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPGroupClass),
			NULL, NULL,
			(GClassInitFunc) sp_group_class_init,
			NULL, NULL,
			sizeof (SPGroup),
			16,
			(GInstanceInitFunc) sp_group_init,
		};
		type = g_type_register_static (SP_TYPE_ITEM, "SPGroup", &info, 0);
	}
	return type;
}

static void
sp_group_class_init (SPGroupClass *klass)
{
	GObjectClass * object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	object_class = (GObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	sp_object_class->build = sp_group_build;
	sp_object_class->release = sp_group_release;
	sp_object_class->child_added = sp_group_child_added;
	sp_object_class->remove_child = sp_group_remove_child;
	sp_object_class->order_changed = sp_group_order_changed;
	sp_object_class->update = sp_group_update;
	sp_object_class->modified = sp_group_modified;
	sp_object_class->sequence = sp_group_sequence;
	sp_object_class->write = sp_group_write;

	item_class->bbox = sp_group_bbox;
	item_class->print = sp_group_print;
	item_class->description = sp_group_description;
	item_class->show = sp_group_show;
	item_class->hide = sp_group_hide;
}

static void
sp_group_init (SPGroup *group)
{
	/* Nothing here */
}

static void sp_group_build (SPObject *object, SPDocument * document, SPRepr * repr)
{
	SPGroup * group;
	SPObject * last;
	SPRepr * rchild;

	group = SP_GROUP (object);

	if (((SPObjectClass *) (parent_class))->build)
		(* ((SPObjectClass *) (parent_class))->build) (object, document, repr);

	last = NULL;
	for (rchild = repr->children; rchild != NULL; rchild = rchild->next) {
		GType type;
		SPObject * child;
		type = sp_repr_type_lookup (rchild);
		child = g_object_new (type, 0);
		if (last) {
			last->next = sp_object_attach_reref (object, child, NULL);
		} else {
			object->children = sp_object_attach_reref (object, child, NULL);
		}
		sp_object_invoke_build (child, document, rchild, SP_OBJECT_IS_CLONED (object));
		last = child;
	}
}

static void
sp_group_release (SPObject *object)
{
	SPGroup * group;

	group = SP_GROUP (object);

	while (object->children) {
		object->children = sp_object_detach_unref (object, object->children);
	}

	if (((SPObjectClass *) parent_class)->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_group_child_added (SPObject *object, SPRepr *child, SPRepr *ref)
{
	SPGroup *group;
	SPItem *item;
	GType type;
	SPObject *ochild, *prev;
	gint position;

	item = SP_ITEM (object);
	group = SP_GROUP (object);

	if (((SPObjectClass *) (parent_class))->child_added)
		(* ((SPObjectClass *) (parent_class))->child_added) (object, child, ref);

	/* Search for position reference */
	prev = NULL;
	position = 0;
	if (ref != NULL) {
		prev = object->children;
		while (prev && (prev->repr != ref)) {
			if (SP_IS_ITEM (prev)) position += 1;
			prev = prev->next;
		}
		if (SP_IS_ITEM (prev)) position += 1;
	}

	type = sp_repr_type_lookup (child);
	ochild = g_object_new (type, 0);
	if (prev) {
		prev->next = sp_object_attach_reref (object, ochild, prev->next);
	} else {
		object->children = sp_object_attach_reref (object, ochild, object->children);
	}

	sp_object_invoke_build (ochild, object->document, child, SP_OBJECT_IS_CLONED (object));

	if (SP_IS_ITEM (ochild)) {
		SPItemView *v;
		NRArenaItem *ac;
		for (v = item->display; v != NULL; v = v->next) {
			ac = sp_item_invoke_show (SP_ITEM (ochild), NR_ARENA_ITEM_ARENA (v->arenaitem), v->key, v->flags);
			if (ac) {
				nr_arena_item_add_child (v->arenaitem, ac, NULL);
				nr_arena_item_set_order (ac, position);
				nr_arena_item_unref (ac);
			}
		}
	}

	sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
}

/* fixme: hide (Lauris) */

static unsigned int
sp_group_remove_child (SPObject * object, SPRepr * child)
{
	SPGroup *group;
	SPObject *prev, *ochild;
	unsigned int ret;

	group = SP_GROUP (object);

	if (((SPObjectClass *) (parent_class))->remove_child) {
		ret = (* ((SPObjectClass *) (parent_class))->remove_child) (object, child);
		if (!ret) return ret;
	}

	prev = NULL;
	ochild = object->children;
	while (ochild->repr != child) {
		prev = ochild;
		ochild = ochild->next;
	}

	if (prev) {
		prev->next = sp_object_detach_unref (object, ochild);
	} else {
		object->children = sp_object_detach_unref (object, ochild);
	}
	sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);

	return TRUE;
}

static void
sp_group_order_changed (SPObject *object, SPRepr *child, SPRepr *old, SPRepr *new)
{
	SPGroup *group;
	SPObject *childobj, *oldobj, *newobj, *o;
	gint childpos, oldpos, newpos;

	group = SP_GROUP (object);

	if (((SPObjectClass *) (parent_class))->order_changed)
		(* ((SPObjectClass *) (parent_class))->order_changed) (object, child, old, new);

	childobj = oldobj = newobj = NULL;
	oldpos = newpos = 0;

	/* Scan children list */
	childpos = 0;
	for (o = object->children; !childobj || (old && !oldobj) || (new && !newobj); o = o->next) {
		if (o->repr == child) {
			childobj = o;
		} else {
			if (SP_IS_ITEM (o)) childpos += 1;
		}
		if (old && o->repr == old) {
			oldobj = o;
			oldpos = childpos;
		}
		if (new && o->repr == new) {
			newobj = o;
			newpos = childpos;
		}
	}

	if (oldobj) {
		oldobj->next = childobj->next;
	} else {
		object->children = childobj->next;
	}
	if (newobj) {
		childobj->next = newobj->next;
		newobj->next = childobj;
	} else {
		childobj->next = object->children;
		object->children = childobj;
	}

	if (SP_IS_ITEM (childobj)) {
		SPItemView *v;
		for (v = SP_ITEM (childobj)->display; v != NULL; v = v->next) {
			nr_arena_item_set_order (v->arenaitem, newpos);
		}
	}

	sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
}

static void
sp_group_update (SPObject *object, SPCtx *ctx, unsigned int flags)
{
	SPGroup *group;
	SPObject *child;
	SPItemCtx *ictx, cctx;
	GSList *l;

	group = SP_GROUP (object);
	ictx = (SPItemCtx *) ctx;
	cctx = *ictx;

	if (((SPObjectClass *) (parent_class))->update)
		((SPObjectClass *) (parent_class))->update (object, ctx, flags);

	if (flags & SP_OBJECT_MODIFIED_FLAG) flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	flags &= SP_OBJECT_MODIFIED_CASCADE;

	l = NULL;
	for (child = object->children; child != NULL; child = child->next) {
		g_object_ref (G_OBJECT (child));
		l = g_slist_prepend (l, child);
	}
	l = g_slist_reverse (l);
	while (l) {
		child = SP_OBJECT (l->data);
		l = g_slist_remove (l, child);
		if (flags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
			if (SP_IS_ITEM (child)) {
				SPItem *chi;
				chi = SP_ITEM (child);
				nr_matrix_multiply_dfd (&cctx.i2doc, &chi->transform, &ictx->i2doc);
				nr_matrix_multiply_dfd (&cctx.i2vp, &chi->transform, &ictx->i2vp);
				sp_object_invoke_update (child, (SPCtx *) &cctx, flags);
			} else {
				sp_object_invoke_update (child, ctx, flags);
			}
		}
		g_object_unref (G_OBJECT (child));
	}
}

static void
sp_group_modified (SPObject *object, guint flags)
{
	SPGroup *group;
	SPObject *child;
	GSList *l;

	group = SP_GROUP (object);

	if (flags & SP_OBJECT_MODIFIED_FLAG) flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	flags &= SP_OBJECT_MODIFIED_CASCADE;

	l = NULL;
	for (child = object->children; child != NULL; child = child->next) {
		g_object_ref (G_OBJECT (child));
		l = g_slist_prepend (l, child);
	}
	l = g_slist_reverse (l);
	while (l) {
		child = SP_OBJECT (l->data);
		l = g_slist_remove (l, child);
		if (flags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
			sp_object_invoke_modified (child, flags);
		}
		g_object_unref (G_OBJECT (child));
	}
}

static unsigned int
sp_group_sequence (SPObject *object, SPObject *target, unsigned int *seq)
{
	SPGroup *group;
	SPObject *child;

	group = (SPGroup *) object;

	for (child = object->children; child != NULL; child = child->next) {
		*seq += 1;
		if (sp_object_invoke_sequence (child, target, seq)) return TRUE;
	}

	return FALSE;
}

static SPRepr *
sp_group_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPGroup *group;
	SPObject *child;
	SPRepr *crepr;

	group = SP_GROUP (object);

	if (flags & SP_OBJECT_WRITE_BUILD) {
		GSList *l;
		if (!repr) repr = sp_repr_new ("g");
		l = NULL;
		for (child = object->children; child != NULL; child = child->next) {
			crepr = sp_object_invoke_write (child, NULL, flags);
			if (crepr) l = g_slist_prepend (l, crepr);
		}
		while (l) {
			sp_repr_add_child (repr, (SPRepr *) l->data, NULL);
			sp_repr_unref ((SPRepr *) l->data);
			l = g_slist_remove (l, l->data);
		}
	} else {
		for (child = object->children; child != NULL; child = child->next) {
			sp_object_invoke_write (child, SP_OBJECT_REPR (child), flags);
		}
	}

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

static void
sp_group_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags)
{
	SPGroup * group;
	SPItem * child;
	SPObject * o;

	group = SP_GROUP (item);

	for (o = ((SPObject *) item)->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			NRMatrixD ct;
			child = SP_ITEM (o);
			nr_matrix_multiply_dfd (&ct, &child->transform, transform);
			sp_item_invoke_bbox_full (child, bbox, &ct, flags, FALSE);
		}
	}
}

static void
sp_group_print (SPItem * item, SPPrintContext *ctx)
{
	SPGroup * group;
	SPItem * child;
	SPObject * o;

	group = SP_GROUP (item);

	for (o = ((SPObject *) item)->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			sp_item_invoke_print (SP_ITEM (o), ctx);
		}
	}
}

static gchar * sp_group_description (SPItem * item)
{
	SPGroup * group;
	SPObject * o;
	gint len;
	static char c[128];

	group = SP_GROUP (item);

	len = 0;
	for (o = ((SPObject *) item)->children; o != NULL; o = o->next) len += 1;

	g_snprintf (c, 128, _("Group of %d objects"), len);

	return g_strdup (c);
}

static NRArenaItem *
sp_group_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags)
{
	SPGroup *group;
	NRArenaItem *ai, *ac, *ar;
	SPItem * child;
	SPObject * o;

	group = (SPGroup *) item;

	ai = nr_arena_item_new (arena, NR_TYPE_ARENA_GROUP);
	nr_arena_group_set_transparent (NR_ARENA_GROUP (ai), group->transparent);

	ar = NULL;
	for (o = ((SPObject *) item)->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			ac = sp_item_invoke_show (child, arena, key, flags);
			if (ac) {
				nr_arena_item_add_child (ai, ac, ar);
				ar = ac;
				nr_arena_item_unref (ac);
			}
		}
	}

	return ai;
}

static void
sp_group_hide (SPItem *item, unsigned int key)
{
	SPGroup * group;
	SPItem * child;
	SPObject * o;

	group = (SPGroup *) item;

	for (o = ((SPObject *) item)->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			sp_item_invoke_hide (child, key);
		}
	}

	if (((SPItemClass *) parent_class)->hide)
		((SPItemClass *) parent_class)->hide (item, key);
}

void
sp_item_group_ungroup (SPGroup *group, GSList **children)
{
	SPDocument *doc;
	SPItem *gitem, *pitem;
	SPRepr *grepr, *prepr, *lrepr;
	SPObject *root, *defs, *child;
	GSList *items, *objects;

	g_return_if_fail (group != NULL);
	g_return_if_fail (SP_IS_GROUP (group));

	doc = SP_OBJECT_DOCUMENT (group);
	root = SP_DOCUMENT_ROOT (doc);
	defs = SP_OBJECT (SP_ROOT (root)->defs);

	gitem = SP_ITEM (group);
	grepr = SP_OBJECT_REPR (gitem);
	pitem = SP_ITEM (SP_OBJECT_PARENT (gitem));
	prepr = SP_OBJECT_REPR (pitem);

	g_return_if_fail (!strcmp (sp_repr_name (grepr), "g") || !strcmp (sp_repr_name (grepr), "a"));

	/* Step 1 - generate lists of children objects */
	items = NULL;
	objects = NULL;
	for (child = ((SPObject *) group)->children; child != NULL; child = child->next) {
		SPRepr *nrepr;
		nrepr = sp_repr_duplicate (SP_OBJECT_REPR (child));
		if (SP_IS_ITEM (child)) {
			SPItem *citem;
			NRMatrixF ctrans;
			gchar affinestr[80];
			guchar *ss;

			citem = SP_ITEM (child);

			nr_matrix_multiply_fff (&ctrans, &citem->transform, &gitem->transform);
			if (sp_svg_transform_write (affinestr, 79, &ctrans)) {
				sp_repr_set_attr (nrepr, "transform", affinestr);
			} else {
				sp_repr_set_attr (nrepr, "transform", NULL);
			}

			/* Merging of style */
			/* fixme: We really should respect presentation attributes too */
			ss = sp_style_write_difference (SP_OBJECT_STYLE (citem), SP_OBJECT_STYLE (pitem));
			sp_repr_set_attr (nrepr, "style", ss);
			g_free (ss);

			items = g_slist_prepend (items, nrepr);
		} else {
			objects = g_slist_prepend (objects, nrepr);
		}
	}

	items = g_slist_reverse (items);
	objects = g_slist_reverse (objects);

	/* Step 2 - clear group */
	while (((SPObject *) group)->children) {
		/* Now it is time to remove original */
		sp_repr_remove_child (grepr, SP_OBJECT_REPR (((SPObject *) group)->children));
	}

	/* Step 3 - add nonitems */
	while (objects) {
		sp_repr_append_child (SP_OBJECT_REPR (defs), (SPRepr *) objects->data);
		sp_repr_unref ((SPRepr *) objects->data);
		objects = g_slist_remove (objects, objects->data);
	}

	/* Step 4 - add items */
	lrepr = grepr;
	while (items) {
		SPItem *nitem;
		sp_repr_add_child (prepr, (SPRepr *) items->data, lrepr);
		lrepr = (SPRepr *) items->data;
		nitem = (SPItem *) sp_document_lookup_id (doc, sp_repr_attr ((SPRepr *) items->data, "id"));
		sp_repr_unref ((SPRepr *) items->data);
		if (children && SP_IS_ITEM (nitem)) *children = g_slist_prepend (*children, nitem);
		items = g_slist_remove (items, items->data);
	}

	sp_repr_unparent (grepr);
	sp_document_done (doc);
}

/*
 * some API for list aspect of SPGroup
 */

GSList * 
sp_item_group_item_list (SPGroup * group)
{
        GSList *s;
	SPObject *o;

	g_return_val_if_fail (group != NULL, NULL);
	g_return_val_if_fail (SP_IS_GROUP (group), NULL);

	s = NULL;

	for (o = ((SPObject *) group)->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) s = g_slist_prepend (s, o);
	}

	return g_slist_reverse (s);
}

/* fixme: Make SPObject method (Lauris) */

SPObject *
sp_item_group_get_child_by_name (SPGroup *group, SPObject *ref, const unsigned char *name)
{
	SPObject *child;
	child = (ref) ? ref->next : ((SPObject *) group)->children;
	while (child && strcmp (sp_repr_name (child->repr), name)) child = child->next;
	return child;
}

/* SPVPGroup */

static void sp_vpgroup_class_init (SPVPGroupClass *klass);
static void sp_vpgroup_init (SPVPGroup *vpgroup);

static void sp_vpgroup_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_vpgroup_set (SPObject *object, unsigned int key, const unsigned char *value);
static void sp_vpgroup_update (SPObject *object, SPCtx *ctx, guint flags);

static SPGroupClass *vpgroup_parent_class;

GType
sp_vpgroup_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPVPGroupClass),
			NULL, NULL,
			(GClassInitFunc) sp_vpgroup_class_init,
			NULL, NULL,
			sizeof (SPVPGroup),
			16,
			(GInstanceInitFunc) sp_vpgroup_init,
		};
		type = g_type_register_static (SP_TYPE_GROUP, "SPVPGroup", &info, 0);
	}
	return type;
}

static void
sp_vpgroup_class_init (SPVPGroupClass *klass)
{
	SPObjectClass *sp_object_class;
	SPItemClass *item_class;

	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;

	vpgroup_parent_class = g_type_class_peek_parent (klass);

	sp_object_class->build = sp_vpgroup_build;
	sp_object_class->set = sp_vpgroup_set;
	sp_object_class->update = sp_vpgroup_update;
}

static void
sp_vpgroup_init (SPVPGroup *vpgroup)
{
	SPItem *item;

	item = (SPItem *) vpgroup;

	item->has_viewport = 1;
	item->has_extra_transform = 1;

	sp_svg_length_unset (&vpgroup->x, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&vpgroup->height, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&vpgroup->width, SP_SVG_UNIT_PERCENT, 1.0, 1.0);
	sp_svg_length_unset (&vpgroup->height, SP_SVG_UNIT_PERCENT, 1.0, 1.0);

	nr_matrix_d_set_identity (&vpgroup->c2p);
}

static void
sp_vpgroup_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	/* It is important to parse these here, so objects will have viewport build-time */
	sp_object_read_attr (object, "x");
	sp_object_read_attr (object, "y");
	sp_object_read_attr (object, "width");
	sp_object_read_attr (object, "height");
	sp_object_read_attr (object, "viewBox");
	sp_object_read_attr (object, "preserveAspectRatio");

	if (((SPObjectClass *) vpgroup_parent_class)->build)
		(* ((SPObjectClass *) vpgroup_parent_class)->build) (object, document, repr);
}

static void
sp_vpgroup_set (SPObject *object, unsigned int key, const unsigned char *value)
{
	SPVPGroup *vpgroup;
	unsigned long unit;

	vpgroup = (SPVPGroup *) object;

	switch (key) {
	case SP_ATTR_X:
		if (sp_svg_length_read_lff (value, &unit, &vpgroup->x.value, &vpgroup->x.computed) &&
		    /* fixme: These are probably valid, but require special treatment (Lauris) */
		    (unit != SP_SVG_UNIT_EM) &&
		    (unit != SP_SVG_UNIT_EX)) {
			vpgroup->x.set = TRUE;
			vpgroup->x.unit = unit;
		} else {
			sp_svg_length_unset (&vpgroup->x, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		/* fixme: I am almost sure these do not require viewport flag (Lauris) */
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		break;
	case SP_ATTR_Y:
		if (sp_svg_length_read_lff (value, &unit, &vpgroup->y.value, &vpgroup->y.computed) &&
		    /* fixme: These are probably valid, but require special treatment (Lauris) */
		    (unit != SP_SVG_UNIT_EM) &&
		    (unit != SP_SVG_UNIT_EX)) {
			vpgroup->y.set = TRUE;
			vpgroup->y.unit = unit;
		} else {
			sp_svg_length_unset (&vpgroup->y, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		/* fixme: I am almost sure these do not require viewport flag (Lauris) */
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		break;
	case SP_ATTR_WIDTH:
		if (sp_svg_length_read_lff (value, &unit, &vpgroup->width.value, &vpgroup->width.computed) &&
		    /* fixme: These are probably valid, but require special treatment (Lauris) */
		    (unit != SP_SVG_UNIT_EM) &&
		    (unit != SP_SVG_UNIT_EX) &&
		    (vpgroup->width.computed > 0.0)) {
			vpgroup->width.set = TRUE;
			vpgroup->width.unit = unit;
		} else {
			sp_svg_length_unset (&vpgroup->width, SP_SVG_UNIT_PERCENT, 1.0, 1.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		break;
	case SP_ATTR_HEIGHT:
		if (sp_svg_length_read_lff (value, &unit, &vpgroup->height.value, &vpgroup->height.computed) &&
		    /* fixme: These are probably valid, but require special treatment (Lauris) */
		    (unit != SP_SVG_UNIT_EM) &&
		    (unit != SP_SVG_UNIT_EX) &&
		    (vpgroup->height.computed >= 0.0)) {
			vpgroup->height.set = TRUE;
			vpgroup->height.unit = unit;
		} else {
			sp_svg_length_unset (&vpgroup->height, SP_SVG_UNIT_PERCENT, 1.0, 1.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		break;
	case SP_ATTR_VIEWBOX:
		if (sp_svg_viewbox_read (value, &vpgroup->viewBox)) {
			vpgroup->viewBox.set = 1;
		} else {
			vpgroup->viewBox.set = 0;
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		break;
	case SP_ATTR_PRESERVEASPECTRATIO:
		/* Do setup before, so we can use break to escape */
		vpgroup->aspect_set = FALSE;
		vpgroup->aspect_align = SP_ASPECT_XMID_YMID;
		vpgroup->aspect_clip = SP_ASPECT_MEET;
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		if (value) {
			int len;
			unsigned char c[256];
			const unsigned char *p, *e;
			unsigned int align, clip;
			p = value;
			while (*p && *p == 32) p += 1;
			if (!*p) break;
			e = p;
			while (*e && *e != 32) e += 1;
			len = e - p;
			if (len > 8) break;
			memcpy (c, value, len);
			c[len] = 0;
			/* Now the actual part */
			if (!strcmp (c, "none")) {
				align = SP_ASPECT_NONE;
			} else if (!strcmp (c, "xMinYMin")) {
				align = SP_ASPECT_XMIN_YMIN;
			} else if (!strcmp (c, "xMidYMin")) {
				align = SP_ASPECT_XMID_YMIN;
			} else if (!strcmp (c, "xMaxYMin")) {
				align = SP_ASPECT_XMAX_YMIN;
			} else if (!strcmp (c, "xMinYMid")) {
				align = SP_ASPECT_XMIN_YMID;
			} else if (!strcmp (c, "xMidYMid")) {
				align = SP_ASPECT_XMID_YMID;
			} else if (!strcmp (c, "xMaxYMin")) {
				align = SP_ASPECT_XMAX_YMID;
			} else if (!strcmp (c, "xMinYMax")) {
				align = SP_ASPECT_XMIN_YMAX;
			} else if (!strcmp (c, "xMidYMax")) {
				align = SP_ASPECT_XMID_YMAX;
			} else if (!strcmp (c, "xMaxYMax")) {
				align = SP_ASPECT_XMAX_YMAX;
			} else {
				break;
			}
			clip = SP_ASPECT_MEET;
			while (*e && *e == 32) e += 1;
			if (*e) {
				if (!strcmp (e, "meet")) {
					clip = SP_ASPECT_MEET;
				} else if (!strcmp (e, "slice")) {
					clip = SP_ASPECT_SLICE;
				} else {
					break;
				}
			}
			vpgroup->aspect_set = TRUE;
			vpgroup->aspect_align = align;
			vpgroup->aspect_clip = clip;
		}
		break;
	default:
		if (((SPObjectClass *) vpgroup_parent_class)->set)
			((SPObjectClass *) vpgroup_parent_class)->set (object, key, value);
		break;
	}
}

static void
sp_vpgroup_update (SPObject *object, SPCtx *ctx, guint flags)
{
	SPItem *item;
	SPVPGroup *vpgroup;
	SPItemCtx *ictx, rctx;
	SPItemView *v;

	item = (SPItem *) object;
	vpgroup = (SPVPGroup *) object;
	ictx = (SPItemCtx *) ctx;

	/* fixme: This will be invoked too often (Lauris) */
	/* fixme: We should calculate only if parent viewport has changed (Lauris) */
	/* If position is specified as percentage, calculate actual values */
	if (vpgroup->x.unit == SP_SVG_UNIT_PERCENT) {
		vpgroup->x.computed = vpgroup->x.value * (ictx->vp.x1 - ictx->vp.x0);
	}
	if (vpgroup->y.unit == SP_SVG_UNIT_PERCENT) {
		vpgroup->y.computed = vpgroup->y.value * (ictx->vp.y1 - ictx->vp.y0);
	}
	if (vpgroup->width.unit == SP_SVG_UNIT_PERCENT) {
		vpgroup->width.computed = vpgroup->width.value * (ictx->vp.x1 - ictx->vp.x0);
	}
	if (vpgroup->height.unit == SP_SVG_UNIT_PERCENT) {
		vpgroup->height.computed = vpgroup->height.value * (ictx->vp.y1 - ictx->vp.y0);
	}

#if 0
	g_print ("<svg> raw %g %g %g %g\n",
		 vpgroup->x.value, vpgroup->y.value,
		 vpgroup->width.value, vpgroup->height.value);

	g_print ("<svg> computed %g %g %g %g\n",
		 vpgroup->x.computed, vpgroup->y.computed,
		 vpgroup->width.computed, vpgroup->height.computed);
#endif

	/* Create copy of item context */
	rctx = *ictx;

	/* Calculate child to parent transformation */
	nr_matrix_d_set_identity (&vpgroup->c2p);

	if (object->parent) {
		/*
		 * fixme: I am not sure whether setting x and y does or does not
		 * fixme: translate the content of inner SVG.
		 * fixme: Still applying translation and setting viewport to width and
		 * fixme: height seems natural, as this makes the inner svg element
		 * fixme: self-contained. The spec is vague here.
		 */
		nr_matrix_d_set_translate (&vpgroup->c2p, vpgroup->x.computed, vpgroup->y.computed);
	}

	if (vpgroup->viewBox.set) {
		double x, y, width, height;
		NRMatrixD q;
		/* Determine actual viewbox in viewport coordinates */
		if (vpgroup->aspect_align == SP_ASPECT_NONE) {
			x = 0.0;
			y = 0.0;
			width = vpgroup->width.computed;
			height = vpgroup->height.computed;
		} else {
			double scalex, scaley, scale;
			/* Things are getting interesting */
			scalex = vpgroup->width.computed / (vpgroup->viewBox.x1 - vpgroup->viewBox.x0);
			scaley = vpgroup->height.computed / (vpgroup->viewBox.y1 - vpgroup->viewBox.y0);
			scale = (vpgroup->aspect_clip == SP_ASPECT_MEET) ? MIN (scalex, scaley) : MAX (scalex, scaley);
			width = (vpgroup->viewBox.x1 - vpgroup->viewBox.x0) * scale;
			height = (vpgroup->viewBox.y1 - vpgroup->viewBox.y0) * scale;
			/* Now place viewbox to requested position */
			switch (vpgroup->aspect_align) {
			case SP_ASPECT_XMIN_YMIN:
				x = 0.0;
				y = 0.0;
				break;
			case SP_ASPECT_XMID_YMIN:
				x = 0.5 * (vpgroup->width.computed - width);
				y = 0.0;
				break;
			case SP_ASPECT_XMAX_YMIN:
				x = 1.0 * (vpgroup->width.computed - width);
				y = 0.0;
				break;
			case SP_ASPECT_XMIN_YMID:
				x = 0.0;
				y = 0.5 * (vpgroup->height.computed - height);
				break;
			case SP_ASPECT_XMID_YMID:
				x = 0.5 * (vpgroup->width.computed - width);
				y = 0.5 * (vpgroup->height.computed - height);
				break;
			case SP_ASPECT_XMAX_YMID:
				x = 1.0 * (vpgroup->width.computed - width);
				y = 0.5 * (vpgroup->height.computed - height);
				break;
			case SP_ASPECT_XMIN_YMAX:
				x = 0.0;
				y = 1.0 * (vpgroup->height.computed - height);
				break;
			case SP_ASPECT_XMID_YMAX:
				x = 0.5 * (vpgroup->width.computed - width);
				y = 1.0 * (vpgroup->height.computed - height);
				break;
			case SP_ASPECT_XMAX_YMAX:
				x = 1.0 * (vpgroup->width.computed - width);
				y = 1.0 * (vpgroup->height.computed - height);
				break;
			default:
				x = 0.0;
				y = 0.0;
				break;
			}
		}
		/* Compose additional transformation from scale and position */
		q.c[0] = width / (vpgroup->viewBox.x1 - vpgroup->viewBox.x0);
		q.c[1] = 0.0;
		q.c[2] = 0.0;
		q.c[3] = height / (vpgroup->viewBox.y1 - vpgroup->viewBox.y0);
		q.c[4] = -vpgroup->viewBox.x0 * q.c[0] + x;
		q.c[5] = -vpgroup->viewBox.y0 * q.c[3] + y;
		/* Append viewbox transformation */
		nr_matrix_multiply_ddd (&vpgroup->c2p, &q, &vpgroup->c2p);
	}

	nr_matrix_multiply_ddd (&rctx.i2doc, &vpgroup->c2p, &rctx.i2doc);

	/* Initialize child viewport */
	if (vpgroup->viewBox.set) {
		rctx.vp.x0 = vpgroup->viewBox.x0;
		rctx.vp.y0 = vpgroup->viewBox.y0;
		rctx.vp.x1 = vpgroup->viewBox.x1;
		rctx.vp.y1 = vpgroup->viewBox.y1;
	} else {
		/* fixme: I wonder whether this logic is correct (Lauris) */
		if (object->parent) {
			rctx.vp.x0 = vpgroup->x.computed;
			rctx.vp.y0 = vpgroup->y.computed;
		} else {
			rctx.vp.x0 = 0.0;
			rctx.vp.y0 = 0.0;
		}
		rctx.vp.x1 = vpgroup->width.computed;
		rctx.vp.y1 = vpgroup->height.computed;
	}

	nr_matrix_d_set_identity (&rctx.i2vp);

	/* And invoke parent method */
	if (((SPObjectClass *) (vpgroup_parent_class))->update)
		((SPObjectClass *) (vpgroup_parent_class))->update (object, (SPCtx *) &rctx, flags);

	/* As last step set additional transform of arena group */
	for (v = item->display; v != NULL; v = v->next) {
		NRMatrixF vbf;
		nr_matrix_f_from_d (&vbf, &vpgroup->c2p);
		nr_arena_group_set_child_transform (NR_ARENA_GROUP (v->arenaitem), &vbf);
	}
}

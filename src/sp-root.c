#define __SP_ROOT_C__

/*
 * SVG <svg> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <string.h>

#include <libarikkei/arikkei-strlib.h>

#include "svg/svg.h"
#include "display/nr-arena-group.h"
#include "attributes.h"
#include "print.h"
#include "document.h"
#include "desktop.h"
#include "sp-defs.h"
/* #include "sp-namedview.h" */
#include "sp-root.h"

static void sp_root_class_init (SPRootClass *klass);
static void sp_root_init (SPRoot *root);

static void sp_root_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_root_release (SPObject *object);
static void sp_root_set (SPObject *object, unsigned int key, const unsigned char *value);
static void sp_root_child_added (SPObject *object, SPRepr *child, SPRepr *ref);
static unsigned int sp_root_remove_child (SPObject *object, SPRepr *child);
static void sp_root_update (SPObject *object, SPCtx *ctx, guint flags);
static void sp_root_modified (SPObject *object, guint flags);
static SPRepr *sp_root_write (SPObject *object, SPRepr *repr, guint flags);

static NRArenaItem *sp_root_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags);
static void sp_root_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags);
static unsigned int sp_root_get_extra_transform (SPItem *item, NRMatrixD *transform);
static unsigned int sp_root_get_viewport (SPItem *item, NRRectF *viewport);
static void sp_root_print (SPItem *item, SPPrintContext *ctx);

static SPVPGroupClass *parent_class;

GType
sp_root_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPRootClass),
			NULL, NULL,
			(GClassInitFunc) sp_root_class_init,
			NULL, NULL,
			sizeof (SPRoot),
			16,
			(GInstanceInitFunc) sp_root_init,
		};
		type = g_type_register_static (SP_TYPE_VPGROUP, "SPRoot", &info, 0);
	}
	return type;
}

static void
sp_root_class_init (SPRootClass *klass)
{
	GObjectClass *object_class;
	SPObjectClass *sp_object_class;
	SPItemClass *sp_item_class;

	object_class = G_OBJECT_CLASS (klass);
	sp_object_class = (SPObjectClass *) klass;
	sp_item_class = (SPItemClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	sp_object_class->build = sp_root_build;
	sp_object_class->release = sp_root_release;
	sp_object_class->set = sp_root_set;
	sp_object_class->child_added = sp_root_child_added;
	sp_object_class->remove_child = sp_root_remove_child;
	sp_object_class->update = sp_root_update;
	sp_object_class->modified = sp_root_modified;
	sp_object_class->write = sp_root_write;

	sp_item_class->show = sp_root_show;
	sp_item_class->bbox = sp_root_bbox;
	sp_item_class->get_extra_transform = sp_root_get_extra_transform;
	sp_item_class->get_viewport = sp_root_get_viewport;
	sp_item_class->print = sp_root_print;
}

static void
sp_root_init (SPRoot *root)
{
	SPItem *item;
	SPGroup *group;

	item = (SPItem *) root;
	group = (SPGroup *) root;

	item->has_extra_transform = 1;
	group->transparent = 1;

	root->version = 1.0;

	root->svg = 100;
	root->sodipodi = 0;
	root->original = 0;

#if 0
	sp_svg_length_unset (&root->x, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&root->height, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&root->width, SP_SVG_UNIT_PERCENT, 1.0, 1.0);
	sp_svg_length_unset (&root->height, SP_SVG_UNIT_PERCENT, 1.0, 1.0);

	nr_matrix_d_set_identity (&root->c2p);
#endif

	/* root->namedviews = NULL; */
	root->defs = NULL;
}

static void
sp_root_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	SPGroup *group;
	SPRoot *root;
	SPObject *o;

	group = (SPGroup *) object;
	root = (SPRoot *) object;

	if (sp_repr_get_attr (repr, "sodipodi:docname") || sp_repr_get_attr (repr, "SP-DOCNAME")) {
		/* This is ugly, but works */
		root->original = 1;
	}
	sp_object_read_attr (object, "version");
	sp_object_read_attr (object, "sodipodi:version");
#if 0
	/* It is important to parse these here, so objects will have viewport build-time */
	sp_object_read_attr (object, "x");
	sp_object_read_attr (object, "y");
	sp_object_read_attr (object, "width");
	sp_object_read_attr (object, "height");
	sp_object_read_attr (object, "viewBox");
	sp_object_read_attr (object, "preserveAspectRatio");
#endif

	if (((SPObjectClass *) parent_class)->build)
		(* ((SPObjectClass *) parent_class)->build) (object, document, repr);

	/* Search for first <defs> node */
	for (o = object->children; o != NULL; o = o->next) {
		if (SP_IS_DEFS (o)) {
			root->defs = SP_DEFS (o);
			sp_object_set_blocked (o, TRUE);
			break;
		}
	}
}

static void
sp_root_release (SPObject *object)
{
	SPRoot * root;

	root = (SPRoot *) object;

	if (root->defs) {
		sp_object_set_blocked (root->defs, FALSE);
		root->defs = NULL;
	}

	if (((SPObjectClass *) parent_class)->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_root_set (SPObject *object, unsigned int key, const unsigned char *value)
{
	SPItem *item;
	SPRoot *root;
#if 0
	gulong unit;
#endif

	item = SP_ITEM (object);
	root = SP_ROOT (object);

	switch (key) {
	case SP_ATTR_VERSION:
		if (value) {
			root->version = sp_svg_atof (value);
		} else {
			root->version = 1.0;
		}
		break;
	case SP_ATTR_SODIPODI_VERSION:
		if (value) {
			root->sodipodi = (guint) (sp_svg_atof (value) * 100.0 + 0.5);
		} else {
			root->sodipodi = root->original;
		}
		break;
#if 0
	case SP_ATTR_X:
		if (sp_svg_length_read_lff (value, &unit, &root->x.value, &root->x.computed) &&
		    /* fixme: These are probably valid, but require special treatment (Lauris) */
		    (unit != SP_SVG_UNIT_EM) &&
		    (unit != SP_SVG_UNIT_EX)) {
			root->x.set = TRUE;
			root->x.unit = unit;
		} else {
			sp_svg_length_unset (&root->x, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		/* fixme: I am almost sure these do not require viewport flag (Lauris) */
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		break;
	case SP_ATTR_Y:
		if (sp_svg_length_read_lff (value, &unit, &root->y.value, &root->y.computed) &&
		    /* fixme: These are probably valid, but require special treatment (Lauris) */
		    (unit != SP_SVG_UNIT_EM) &&
		    (unit != SP_SVG_UNIT_EX)) {
			root->y.set = TRUE;
			root->y.unit = unit;
		} else {
			sp_svg_length_unset (&root->y, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		/* fixme: I am almost sure these do not require viewport flag (Lauris) */
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		break;
	case SP_ATTR_WIDTH:
		if (sp_svg_length_read_lff (value, &unit, &root->width.value, &root->width.computed) &&
		    /* fixme: These are probably valid, but require special treatment (Lauris) */
		    (unit != SP_SVG_UNIT_EM) &&
		    (unit != SP_SVG_UNIT_EX) &&
		    (root->width.computed > 0.0)) {
			root->width.set = TRUE;
			root->width.unit = unit;
		} else {
			sp_svg_length_unset (&root->width, SP_SVG_UNIT_PERCENT, 1.0, 1.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		break;
	case SP_ATTR_HEIGHT:
		if (sp_svg_length_read_lff (value, &unit, &root->height.value, &root->height.computed) &&
		    /* fixme: These are probably valid, but require special treatment (Lauris) */
		    (unit != SP_SVG_UNIT_EM) &&
		    (unit != SP_SVG_UNIT_EX) &&
		    (root->height.computed >= 0.0)) {
			root->height.set = TRUE;
			root->height.unit = unit;
		} else {
			sp_svg_length_unset (&root->height, SP_SVG_UNIT_PERCENT, 1.0, 1.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		break;
	case SP_ATTR_VIEWBOX:
		if (sp_svg_viewbox_read (value, &root->viewBox)) {
			root->viewBox.set = 1;
		} else {
			root->viewBox.set = 0;
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		break;
	case SP_ATTR_PRESERVEASPECTRATIO:
		/* Do setup before, so we can use break to escape */
		root->aspect_set = FALSE;
		root->aspect_align = SP_ASPECT_XMID_YMID;
		root->aspect_clip = SP_ASPECT_MEET;
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
			root->aspect_set = TRUE;
			root->aspect_align = align;
			root->aspect_clip = clip;
		}
		break;
#endif
	default:
		if (((SPObjectClass *) parent_class)->set)
			((SPObjectClass *) parent_class)->set (object, key, value);
		break;
	}
}

static void
sp_root_child_added (SPObject *object, SPRepr *child, SPRepr *ref)
{
	SPRoot *root;
	SPGroup *group;
	SPObject *co;
	const gchar * id;

	root = (SPRoot *) object;
	group = (SPGroup *) object;

	if (((SPObjectClass *) (parent_class))->child_added)
		(* ((SPObjectClass *) (parent_class))->child_added) (object, child, ref);

	id = sp_repr_get_attr (child, "id");
	co = sp_document_lookup_id (object->document, id);
	g_assert (co != NULL);

	if (SP_IS_DEFS (co)) {
		SPObject *c;
		/* We search for first <defs> node - it is not beautiful, but works */
		if (root->defs) {
			sp_object_set_blocked (root->defs, FALSE);
			root->defs = NULL;
		}
		for (c = object->children; c != NULL; c = c->next) {
			if (SP_IS_DEFS (c)) {
				root->defs = SP_DEFS (c);
				sp_object_set_blocked (root->defs, TRUE);
				break;
			}
		}
	}
}

static unsigned int
sp_root_remove_child (SPObject * object, SPRepr * child)
{
	SPRoot * root;
	SPObject * co;
	const gchar * id;

	root = (SPRoot *) object;

	id = sp_repr_get_attr (child, "id");
	co = sp_document_lookup_id (object->document, id);
	g_assert (co != NULL);

	/* fixme: test whether it is shown */
	/* if (SP_IS_NAMEDVIEW (co)) return FALSE; */

	/* fixme: test whether it is empty */
	if (SP_IS_DEFS (co) && (root->defs == (SPDefs *) co)) return FALSE;

	if (((SPObjectClass *) (parent_class))->remove_child)
		return (* ((SPObjectClass *) (parent_class))->remove_child) (object, child);

	return TRUE;
}

static void
sp_root_update (SPObject *object, SPCtx *ctx, guint flags)
{
	SPItem *item;
	SPRoot *root;
	SPItemCtx *ictx, rctx;
#if 0
	SPItemView *v;
#endif

	item = SP_ITEM (object);
	root = SP_ROOT (object);
	ictx = (SPItemCtx *) ctx;

#if 0
	g_print ("viewPort %g %g %g %g\n", ictx->vp.x0, ictx->vp.y0, ictx->vp.x1, ictx->vp.y1);
#endif

#if 0
	/* fixme: This will be invoked too often (Lauris) */
	/* fixme: We should calculate only if parent viewport has changed (Lauris) */
	/* If position is specified as percentage, calculate actual values */
	if (root->x.unit == SP_SVG_UNIT_PERCENT) {
		root->x.computed = root->x.value * (ictx->vp.x1 - ictx->vp.x0);
	}
	if (root->y.unit == SP_SVG_UNIT_PERCENT) {
		root->y.computed = root->y.value * (ictx->vp.y1 - ictx->vp.y0);
	}
	if (root->width.unit == SP_SVG_UNIT_PERCENT) {
		root->width.computed = root->width.value * (ictx->vp.x1 - ictx->vp.x0);
	}
	if (root->height.unit == SP_SVG_UNIT_PERCENT) {
		root->height.computed = root->height.value * (ictx->vp.y1 - ictx->vp.y0);
	}

#if 0
	g_print ("<svg> raw %g %g %g %g\n",
		 root->x.value, root->y.value,
		 root->width.value, root->height.value);

	g_print ("<svg> computed %g %g %g %g\n",
		 root->x.computed, root->y.computed,
		 root->width.computed, root->height.computed);
#endif

	/* Create copy of item context */
	rctx = *ictx;

	/* Calculate child to parent transformation */
	nr_matrix_d_set_identity (&root->c2p);

	if (object->parent) {
		/*
		 * fixme: I am not sure whether setting x and y does or does not
		 * fixme: translate the content of inner SVG.
		 * fixme: Still applying translation and setting viewport to width and
		 * fixme: height seems natural, as this makes the inner svg element
		 * fixme: self-contained. The spec is vague here.
		 */
		nr_matrix_d_set_translate (&root->c2p, root->x.computed, root->y.computed);
	}

	if (root->viewBox.set) {
		double x, y, width, height;
		NRMatrixD q;
		/* Determine actual viewbox in viewport coordinates */
		if (root->aspect_align == SP_ASPECT_NONE) {
			x = 0.0;
			y = 0.0;
			width = root->width.computed;
			height = root->height.computed;
		} else {
			double scalex, scaley, scale;
			/* Things are getting interesting */
			scalex = root->width.computed / (root->viewBox.x1 - root->viewBox.x0);
			scaley = root->height.computed / (root->viewBox.y1 - root->viewBox.y0);
			scale = (root->aspect_clip == SP_ASPECT_MEET) ? MIN (scalex, scaley) : MAX (scalex, scaley);
			width = (root->viewBox.x1 - root->viewBox.x0) * scale;
			height = (root->viewBox.y1 - root->viewBox.y0) * scale;
			/* Now place viewbox to requested position */
			switch (root->aspect_align) {
			case SP_ASPECT_XMIN_YMIN:
				x = 0.0;
				y = 0.0;
				break;
			case SP_ASPECT_XMID_YMIN:
				x = 0.5 * (root->width.computed - width);
				y = 0.0;
				break;
			case SP_ASPECT_XMAX_YMIN:
				x = 1.0 * (root->width.computed - width);
				y = 0.0;
				break;
			case SP_ASPECT_XMIN_YMID:
				x = 0.0;
				y = 0.5 * (root->height.computed - height);
				break;
			case SP_ASPECT_XMID_YMID:
				x = 0.5 * (root->width.computed - width);
				y = 0.5 * (root->height.computed - height);
				break;
			case SP_ASPECT_XMAX_YMID:
				x = 1.0 * (root->width.computed - width);
				y = 0.5 * (root->height.computed - height);
				break;
			case SP_ASPECT_XMIN_YMAX:
				x = 0.0;
				y = 1.0 * (root->height.computed - height);
				break;
			case SP_ASPECT_XMID_YMAX:
				x = 0.5 * (root->width.computed - width);
				y = 1.0 * (root->height.computed - height);
				break;
			case SP_ASPECT_XMAX_YMAX:
				x = 1.0 * (root->width.computed - width);
				y = 1.0 * (root->height.computed - height);
				break;
			default:
				x = 0.0;
				y = 0.0;
				break;
			}
		}
		/* Compose additional transformation from scale and position */
		q.c[0] = width / (root->viewBox.x1 - root->viewBox.x0);
		q.c[1] = 0.0;
		q.c[2] = 0.0;
		q.c[3] = height / (root->viewBox.y1 - root->viewBox.y0);
		q.c[4] = -root->viewBox.x0 * q.c[0] + x;
		q.c[5] = -root->viewBox.y0 * q.c[3] + y;
		/* Append viewbox transformation */
		nr_matrix_multiply_ddd (&root->c2p, &q, &root->c2p);
	}

	nr_matrix_multiply_ddd (&rctx.i2doc, &root->c2p, &rctx.i2doc);

	/* Initialize child viewport */
	if (root->viewBox.set) {
		rctx.vp.x0 = root->viewBox.x0;
		rctx.vp.y0 = root->viewBox.y0;
		rctx.vp.x1 = root->viewBox.x1;
		rctx.vp.y1 = root->viewBox.y1;
	} else {
		/* fixme: I wonder whether this logic is correct (Lauris) */
		if (object->parent) {
			rctx.vp.x0 = root->x.computed;
			rctx.vp.y0 = root->y.computed;
		} else {
			rctx.vp.x0 = 0.0;
			rctx.vp.y0 = 0.0;
		}
		rctx.vp.x1 = root->width.computed;
		rctx.vp.y1 = root->height.computed;
	}

	nr_matrix_d_set_identity (&rctx.i2vp);
#endif

	/* And invoke parent method */
	if (((SPObjectClass *) (parent_class))->update)
		((SPObjectClass *) (parent_class))->update (object, (SPCtx *) &rctx, flags);

#if 0
	/* As last step set additional transform of arena group */
	for (v = item->display; v != NULL; v = v->next) {
		NRMatrixF vbf;
		nr_matrix_f_from_d (&vbf, &root->c2p);
		nr_arena_group_set_child_transform (NR_ARENA_GROUP (v->arenaitem), &vbf);
	}
#endif
}

static void
sp_root_modified (SPObject *object, guint flags)
{
	SPRoot *root;
	SPVPGroup *vpgroup;

	root = (SPRoot *) object;
	vpgroup = (SPVPGroup *) object;

	if (((SPObjectClass *) (parent_class))->modified)
		(* ((SPObjectClass *) (parent_class))->modified) (object, flags);

	/* fixme: (Lauris) */
	if (!object->parent && (flags & SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
		sp_document_set_size_px (SP_OBJECT_DOCUMENT (root), vpgroup->width.computed, vpgroup->height.computed);
	}
}

static SPRepr *
sp_root_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPRoot *root;
	SPVPGroup *vpgroup;

	root = (SPRoot *) object;
	vpgroup = (SPVPGroup *) object;

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("svg");
	}

	sp_repr_set_attr (repr, "xmlns", "http://www.w3.org/2000/svg");
	sp_repr_set_attr (repr, "xmlns:xlink", "http://www.w3.org/1999/xlink");

	if (flags & SP_OBJECT_WRITE_SODIPODI) {
		sp_repr_set_attr (repr, "xmlns:sodipodi", "http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd");
		sp_repr_set_double (repr, "sodipodi:version", (double) root->sodipodi / 100.0);
	}

	sp_repr_set_attr (repr, "version", "1.0");
	sp_repr_set_double (repr, "x", vpgroup->x.computed);
	sp_repr_set_double (repr, "y", vpgroup->y.computed);
	sp_repr_set_double (repr, "width", vpgroup->width.computed);
	sp_repr_set_double (repr, "height", vpgroup->height.computed);
	sp_repr_set_attr (repr, "viewBox", sp_repr_get_attr (object->repr, "viewBox"));

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

static NRArenaItem *
sp_root_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags)
{
	SPRoot *root;
	SPVPGroup *vpgroup;
	NRArenaItem *ai;

	root = (SPRoot *) item;
	vpgroup = (SPVPGroup *) item;

	if (((SPItemClass *) (parent_class))->show) {
		ai = ((SPItemClass *) (parent_class))->show (item, arena, key, flags);
		if (ai) {
			NRMatrixF vbf;
			nr_matrix_f_from_d (&vbf, &vpgroup->c2p);
			nr_arena_group_set_child_transform (NR_ARENA_GROUP (ai), &vbf);
		}
	} else {
		ai = NULL;
	}

	return ai;
}

static void
sp_root_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags)
{
	SPRoot *root;
	SPVPGroup *vpgroup;
	NRMatrixD a[6];

	root = (SPRoot *) item;
	vpgroup = (SPVPGroup *) item;

	nr_matrix_multiply_ddd (a, &vpgroup->c2p, transform);

	if (((SPItemClass *) (parent_class))->bbox) {
		((SPItemClass *) (parent_class))->bbox (item, bbox, a, flags);
	}
}

static unsigned int
sp_root_get_extra_transform (SPItem *item, NRMatrixD *transform)
{
	SPRoot *root;
	root = (SPRoot *) item;
	nr_matrix_multiply_ddd (transform, transform, &root->group.c2p);
	return 1;
}

static unsigned int
sp_root_get_viewport (SPItem *item, NRRectF *viewport)
{
	SPRoot *root;

	root = (SPRoot *) item;

	/* lala - this is not correct */
	/* if dt->root == root, i2vp has to be identity */

	/* fixme: I think this is correct (Lauris) */
	/* viewBox is represented as c2p and viewport is size */
	viewport->x0 = root->group.x.computed;
	viewport->y0 = root->group.y.computed;
	viewport->x1 = viewport->x0 + root->group.width.computed;
	viewport->y1 = viewport->y0 + root->group.height.computed;

	return 1;
}

static void
sp_root_print (SPItem *item, SPPrintContext *ctx)
{
	SPRoot *root;
	NRMatrixF t;

	root = SP_ROOT (item);

	nr_matrix_f_from_d (&t, &root->group.c2p);
	sp_print_bind (ctx, &t, 1.0);

	if (((SPItemClass *) (parent_class))->print) {
		((SPItemClass *) (parent_class))->print (item, ctx);
	}

	sp_print_release (ctx);
}

#define __SP_PATH_C__

/*
 * SVG <path> implementation
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
#include <libnr/nr-path.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_vpath_bpath.h>
#include "helper/art-utils.h"
#include "svg/svg.h"
#include "attributes.h"
#include "sp-root.h"
#include "sp-path.h"

#define noPATH_VERBOSE

static void sp_path_class_init (SPPathClass *class);
static void sp_path_init (SPPath *path);

static void sp_path_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_path_release (SPObject *object);
static void sp_path_set (SPObject *object, unsigned int key, const unsigned char *value);

static void sp_path_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags);

static void sp_path_private_remove_comp (SPPath * path, SPPathComp * comp);
static void sp_path_private_add_comp (SPPath * path, SPPathComp * comp);
static void sp_path_private_change_bpath (SPPath * path, SPPathComp * comp, SPCurve * curve);

static SPItemClass * parent_class;

GType
sp_path_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (SPPathClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_path_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPPath),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_path_init,
		};
		type = g_type_register_static (SP_TYPE_ITEM, "SPPath", &info, 0);
	}
	return type;
}

static void
sp_path_class_init (SPPathClass * klass)
{
	GObjectClass * object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	object_class = (GObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;

	parent_class = g_type_class_ref (SP_TYPE_ITEM);

	sp_object_class->build = sp_path_build;
	sp_object_class->release = sp_path_release;
	sp_object_class->set = sp_path_set;

	item_class->bbox = sp_path_bbox;

	klass->remove_comp = sp_path_private_remove_comp;
	klass->add_comp = sp_path_private_add_comp;
	klass->change_bpath = sp_path_private_change_bpath;
}

static void
sp_path_init (SPPath *path)
{
	path->independent = TRUE;
	path->comp = NULL;
}

/* fixme: Better place (Lauris) */

static guint
sp_path_find_version (SPObject *object)
{
	while (object) {
		if (SP_IS_ROOT (object)) {
			return SP_ROOT (object)->sodipodi;
		}
		object = SP_OBJECT_PARENT (object);
	}

	return 0;
}

static void
sp_path_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	SPPath *path;
	guint version;

	path = SP_PATH (object);

	version = sp_path_find_version (object);

	if ((version > 0) && (version < 25)) {
		const guchar *str;
		str = sp_repr_attr (repr, "SODIPODI-PATH-NODE-TYPES");
		sp_repr_set_attr (repr, "sodipodi:nodetypes", str);
		sp_repr_set_attr (repr, "SODIPODI-PATH-NODE-TYPES", NULL);
	}

	sp_object_read_attr (object, "d");

	if (path->independent && (version > 0) && (version < 25)) {
		SPCSSAttr *css;
		const guchar *val;
		gboolean changed;
		GSList *l;
		gboolean open;
		css = sp_repr_css_attr (repr, "style");
		/* We foce style rewrite at moment (Lauris) */
		changed = TRUE;
		open = FALSE;
		for (l = path->comp; l != NULL; l = l->next) {
			SPPathComp *comp;
			comp = (SPPathComp *) l->data;
			if (comp->curve && comp->curve->bpath) {
				ArtBpath *bp;
				for (bp = comp->curve->bpath; bp->code != ART_END; bp++) {
					if (bp->code == ART_MOVETO_OPEN) {
						open = TRUE;
						break;
					}
				}
			}
		}
		if (open) {
			val = sp_repr_css_property (css, "fill", NULL);
			if (val && strcmp (val, "none")) {
				sp_repr_css_set_property (css, "fill", "none");
				changed = TRUE;
			}
		} else {
			val = sp_repr_css_property (css, "fill-rule", NULL);
			if (!val) {
				sp_repr_css_set_property (css, "fill-rule", "evenodd");
				changed = TRUE;
			}
		}
		if (changed) sp_repr_css_set (repr, css, "style");
		sp_repr_css_attr_unref (css);
	}

	if (((SPObjectClass *) parent_class)->build)
		((SPObjectClass *) parent_class)->build (object, document, repr);
}

static void
sp_path_release (SPObject *object)
{
	SPPath *path;

	path = SP_PATH (object);

	while (path->comp) {
		sp_path_comp_destroy ((SPPathComp *) path->comp->data);
		path->comp = g_slist_remove (path->comp, path->comp->data);
	}

	if (((SPObjectClass *) parent_class)->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_path_set (SPObject *object, unsigned int key, const unsigned char *value)
{
	SPPath * path;
	ArtBpath * bpath;
	SPCurve * curve;
	SPPathComp * comp;
	double affine[6];

	path = SP_PATH (object);

	art_affine_identity (affine);

	switch (key) {
	case SP_ATTR_D:
		sp_path_clear (path);
		if (value) {
			/* fixme: */
			bpath = sp_svg_read_path (value);
			curve = sp_curve_new_from_bpath (bpath);
			comp = sp_path_comp_new (curve, TRUE, affine);
			sp_curve_unref (curve);
			sp_path_add_comp (path, comp);
		}
		sp_object_request_modified (SP_OBJECT (path), SP_OBJECT_MODIFIED_FLAG);
		break;
	default:
		if (((SPObjectClass *) parent_class)->set)
			((SPObjectClass *) parent_class)->set (object, key, value);
		break;
	}
}

static void
sp_path_private_remove_comp (SPPath * path, SPPathComp * comp)
{
	SPPathComp * c;
	GSList * l;

	g_assert (SP_IS_PATH (path));

	l = g_slist_find (path->comp, comp);
	g_return_if_fail (l != NULL);

	c = (SPPathComp *) l->data;
	path->comp = g_slist_remove (path->comp, c);
	sp_path_comp_destroy (c);
}

static void
sp_path_private_add_comp (SPPath * path, SPPathComp * comp)
{
	g_assert (SP_IS_PATH (path));
	g_assert (comp != NULL);

	path->comp = g_slist_prepend (path->comp, comp);
}

static void
sp_path_private_change_bpath (SPPath * path, SPPathComp * comp, SPCurve * curve)
{
	SPPathComp * c;
	GSList * l;

	g_assert (SP_IS_PATH (path));

	l = g_slist_find (path->comp, comp);
	g_return_if_fail (l != NULL);

	c = (SPPathComp *) l->data;

	sp_curve_ref (curve);
	sp_curve_unref (c->curve);
	c->curve = curve;
}

static void
sp_path_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags)
{
	SPPath *path;
	GSList *l;

	path = SP_PATH (item);

	for (l = path->comp; l != NULL; l = l->next) {
		SPPathComp *comp;
		NRMatrixF a;
		NRBPath bp;
		comp = (SPPathComp *) l->data;
		nr_matrix_multiply_fdd (&a, (NRMatrixD *) comp->affine, transform);
		bp.path = SP_CURVE_BPATH (comp->curve);
		nr_path_matrix_f_bbox_f_union (&bp, &a, bbox, 0.25);
	}
}

void
sp_path_remove_comp (SPPath * path, SPPathComp * comp)
{
	g_return_if_fail (path != NULL);
	g_return_if_fail (SP_IS_PATH (path));
	g_return_if_fail (comp != NULL);
	g_return_if_fail (g_slist_find (path->comp, comp) != NULL);

	(* SP_PATH_CLASS (G_OBJECT_GET_CLASS(path))->remove_comp) (path, comp);
}

void
sp_path_add_comp (SPPath * path, SPPathComp * comp)
{
	g_return_if_fail (path != NULL);
	g_return_if_fail (SP_IS_PATH (path));
	g_return_if_fail (comp != NULL);
	g_return_if_fail (g_slist_find (path->comp, comp) == NULL);

	(* SP_PATH_CLASS (G_OBJECT_GET_CLASS(path))->add_comp) (path, comp);
}

void
sp_path_change_bpath (SPPath * path, SPPathComp * comp, SPCurve * curve)
{
	g_return_if_fail (path != NULL);
	g_return_if_fail (SP_IS_PATH (path));
	g_return_if_fail (comp != NULL);
	g_return_if_fail (g_slist_find (path->comp, comp) != NULL);
	g_return_if_fail (curve != NULL);

	(* SP_PATH_CLASS (G_OBJECT_GET_CLASS(path))->change_bpath) (path, comp, curve);
}

void
sp_path_clear (SPPath * path)
{
	g_return_if_fail (path != NULL);
	g_return_if_fail (SP_IS_PATH (path));

	while (path->comp) {
		sp_path_remove_comp (path, (SPPathComp *) path->comp->data);
	}
}

void
sp_path_add_bpath (SPPath * path, SPCurve * curve, gboolean private, gdouble affine[])
{
	SPPathComp * comp;

	g_return_if_fail (path != NULL);
	g_return_if_fail (SP_IS_PATH (path));
	g_return_if_fail (curve != NULL);

	comp = sp_path_comp_new (curve, private, affine);

	sp_path_add_comp (path, comp);
}

SPCurve *
sp_path_normalized_bpath (SPPath * path)
{
	SPPathComp * comp;
	ArtBpath * bp, * bpath;
	ArtPoint p;
	gint n_nodes;
	GSList * l;
	gint i;

	n_nodes = 0;

	for (l = path->comp; l != NULL; l = l->next) {
		comp = (SPPathComp *) l->data;
		if (comp->curve != NULL) {
			for (i = 0; (comp->curve->bpath + i)->code != ART_END; i++)
				n_nodes++;
		}
	}

	if (n_nodes < 2)
		return NULL;

	bpath = art_new (ArtBpath, n_nodes + 1);

	n_nodes = 0;

	for (l = path->comp; l != NULL; l = l->next) {
		comp = (SPPathComp *) l->data;
		if (comp->curve != NULL) {
			for (i = 0; (comp->curve->bpath + i)->code != ART_END; i++) {
				bp = comp->curve->bpath + i;
				bpath[n_nodes].code = bp->code;
				if (bp->code == ART_CURVETO) {
					p.x = bp->x1;
					p.y = bp->y1;
					art_affine_point (&p, &p, comp->affine);
					bpath[n_nodes].x1 = p.x;
					bpath[n_nodes].y1 = p.y;
					p.x = bp->x2;
					p.y = bp->y2;
					art_affine_point (&p, &p, comp->affine);
					bpath[n_nodes].x2 = p.x;
					bpath[n_nodes].y2 = p.y;
				}
				p.x = bp->x3;
				p.y = bp->y3;
				art_affine_point (&p, &p, comp->affine);
				bpath[n_nodes].x3 = p.x;
				bpath[n_nodes].y3 = p.y;
				n_nodes++;
			}
		}
	}

	bpath[n_nodes].code = ART_END;

	return sp_curve_new_from_bpath (bpath);
}

SPCurve *
sp_path_normalize (SPPath * path)
{
	SPPathComp * comp;
	SPCurve * curve;

	g_assert (path->independent);

	curve = sp_path_normalized_bpath (path);

	if (curve == NULL) return NULL;

	sp_path_clear (path);
	comp = sp_path_comp_new (curve, TRUE, NULL);
	sp_path_add_comp (path, comp);

	return curve;
}

void
sp_path_bpath_modified (SPPath * path, SPCurve * curve)
{
	SPPathComp * comp;

	g_return_if_fail (path != NULL);
	g_return_if_fail (SP_IS_PATH (path));
	g_return_if_fail (curve != NULL);

	g_return_if_fail (path->independent);
	g_return_if_fail (path->comp != NULL);
	g_return_if_fail (path->comp->next == NULL);

	comp = (SPPathComp *) path->comp->data;

	g_return_if_fail (comp->private);

	sp_path_change_bpath (path, comp, curve);
}

/* Old SPPathComp methods */

SPPathComp *
sp_path_comp_new (SPCurve * curve, gboolean private, double affine[])
{
	SPPathComp * comp;
	gint i;

	g_return_val_if_fail (curve != NULL, NULL);

	comp = g_new (SPPathComp, 1);
	comp->curve = curve;
	sp_curve_ref (curve);
	comp->private = private;
	if (affine != NULL) {
		for (i = 0; i < 6; i++) comp->affine[i] = affine[i];
	} else {
		art_affine_identity (comp->affine);
	}
	return comp;
}

void
sp_path_comp_destroy (SPPathComp * comp)
{
	g_assert (comp != NULL);

	sp_curve_unref (comp->curve);

	g_free (comp);
}

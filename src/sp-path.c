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
#include <libart_lgpl/art_bpath.h>
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

static SPRepr *sp_path_write (SPObject *object, SPRepr *repr, guint flags);

static SPShapeClass *parent_class;

unsigned int
sp_path_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (SPPathClass),
			NULL, NULL,
			(GClassInitFunc) sp_path_class_init,
			NULL, NULL,
			sizeof (SPPath),
			16,
			(GInstanceInitFunc) sp_path_init,
		};
		type = g_type_register_static (SP_TYPE_SHAPE, "SPPath", &info, 0);
	}
	return type;
}

static void
sp_path_class_init (SPPathClass * klass)
{
	SPObjectClass *sp_object_class;
	SPItemClass *item_class;

	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	sp_object_class->build = sp_path_build;
	sp_object_class->release = sp_path_release;
	sp_object_class->set = sp_path_set;
	sp_object_class->write = sp_path_write;
}

static void
sp_path_init (SPPath *path)
{
	/* Nothing here */
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

	if ((version > 0) && (version < 25)) {
		SPShape *shape;
		SPCSSAttr *css;
		const guchar *val;
		gboolean changed;
		gboolean open;
		shape = SP_SHAPE (path);
		css = sp_repr_css_attr (repr, "style");
		/* We foce style rewrite at moment (Lauris) */
		changed = TRUE;
		open = FALSE;
		if (shape->curve && shape->curve->bpath) {
			ArtBpath *bp;
			for (bp = shape->curve->bpath; bp->code != ART_END; bp++) {
				if (bp->code == ART_MOVETO_OPEN) {
					open = TRUE;
					break;
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

	if (((SPObjectClass *) parent_class)->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_path_set (SPObject *object, unsigned int key, const unsigned char *value)
{
	SPPath *path;
	ArtBpath *bpath;
	SPCurve *curve;

	path = SP_PATH (object);

	switch (key) {
	case SP_ATTR_D:
		if (value) {
			bpath = sp_svg_read_path (value);
			curve = sp_curve_new_from_bpath (bpath);
			sp_shape_set_curve (SP_SHAPE (path), curve, TRUE);
			sp_curve_unref (curve);
		} else {
			sp_shape_set_curve (SP_SHAPE (path), NULL, TRUE);
		}
		break;
	default:
		if (((SPObjectClass *) parent_class)->set)
			((SPObjectClass *) parent_class)->set (object, key, value);
		break;
	}
}

static SPRepr *
sp_path_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPShape *shape;
	SPPath *path;
	ArtBpath *abp;
	gchar *str;

	shape = SP_SHAPE (object);
	path = SP_PATH (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("path");
	}

	abp = sp_curve_first_bpath (shape->curve);
	str = sp_svg_write_path (abp);
	sp_repr_set_attr (repr, "d", str);
	g_free (str);

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

#if 0
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
		nr_matrix_d_set_identity (NR_MATRIX_D_FROM_DOUBLE (comp->affine));
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
#endif

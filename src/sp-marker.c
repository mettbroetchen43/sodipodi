#define __SP_MARKER_C__

/*
 * SVG <marker> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2003 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#include <string.h>

#include <libarikkei/arikkei-strlib.h>

#include "svg/svg.h"
#include "display/nr-arena-group.h"
#include "attributes.h"
#include "print.h"
#include "document.h"
#include "desktop.h"
#include "sp-defs.h"
#include "sp-namedview.h"
#include "sp-marker.h"

struct _SPMarkerView {
	SPMarkerView *next;
	unsigned int key;
	unsigned int size;
	NRArenaItem *items[1];
};

static void sp_marker_class_init (SPMarkerClass *klass);
static void sp_marker_init (SPMarker *marker);

static void sp_marker_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_marker_release (SPObject *object);
static void sp_marker_set (SPObject *object, unsigned int key, const unsigned char *value);
static void sp_marker_update (SPObject *object, SPCtx *ctx, guint flags);
static SPRepr *sp_marker_write (SPObject *object, SPRepr *repr, guint flags);

static NRArenaItem *sp_marker_private_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags);
static void sp_marker_private_hide (SPItem *item, unsigned int key);
static void sp_marker_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags);
static void sp_marker_print (SPItem *item, SPPrintContext *ctx);

static void sp_marker_view_remove (SPMarker *marker, SPMarkerView *view, unsigned int destroyitems);

static SPGroupClass *parent_class;

GType
sp_marker_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPMarkerClass),
			NULL, NULL,
			(GClassInitFunc) sp_marker_class_init,
			NULL, NULL,
			sizeof (SPMarker),
			16,
			(GInstanceInitFunc) sp_marker_init,
		};
		type = g_type_register_static (SP_TYPE_GROUP, "SPMarker", &info, 0);
	}
	return type;
}

static void
sp_marker_class_init (SPMarkerClass *klass)
{
	GObjectClass *object_class;
	SPObjectClass *sp_object_class;
	SPItemClass *sp_item_class;

	object_class = G_OBJECT_CLASS (klass);
	sp_object_class = (SPObjectClass *) klass;
	sp_item_class = (SPItemClass *) klass;

	parent_class = g_type_class_ref (SP_TYPE_GROUP);

	sp_object_class->build = sp_marker_build;
	sp_object_class->release = sp_marker_release;
	sp_object_class->set = sp_marker_set;
	sp_object_class->update = sp_marker_update;
	sp_object_class->write = sp_marker_write;

	sp_item_class->show = sp_marker_private_show;
	sp_item_class->hide = sp_marker_private_hide;
	sp_item_class->bbox = sp_marker_bbox;
	sp_item_class->print = sp_marker_print;
}

static void
sp_marker_init (SPMarker *marker)
{
	marker->viewBox_set = FALSE;

	nr_matrix_d_set_identity (&marker->c2p);
}

static void
sp_marker_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	SPGroup *group;
	SPMarker *marker;

	group = (SPGroup *) object;
	marker = (SPMarker *) object;

	sp_object_read_attr (object, "markerUnits");
	sp_object_read_attr (object, "refX");
	sp_object_read_attr (object, "refY");
	sp_object_read_attr (object, "markerWidth");
	sp_object_read_attr (object, "markerHeight");
	sp_object_read_attr (object, "orient");
	sp_object_read_attr (object, "viewBox");
	sp_object_read_attr (object, "preserveAspectRatio");

	if (((SPObjectClass *) parent_class)->build)
		((SPObjectClass *) parent_class)->build (object, document, repr);
}

static void
sp_marker_release (SPObject *object)
{
	SPMarker *marker;

	marker = (SPMarker *) object;

	while (marker->views) {
		/* Destroy all NRArenaitems etc. */
		/* Parent class ::hide method */
		((SPItemClass *) parent_class)->hide ((SPItem *) marker, marker->views->key);
		sp_marker_view_remove (marker, marker->views, TRUE);
	}

	if (((SPObjectClass *) parent_class)->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_marker_set (SPObject *object, unsigned int key, const unsigned char *value)
{
	SPItem *item;
	SPMarker *marker;

	item = SP_ITEM (object);
	marker = SP_MARKER (object);

	switch (key) {
	case SP_ATTR_MARKERUNITS:
		marker->markerUnits_set = FALSE;
		marker->markerUnits = SP_MARKER_UNITS_STROKEWIDTH;
		if (value) {
			if (!strcmp (value, "strokeWidth")) {
				marker->markerUnits_set = TRUE;
			} else if (!strcmp (value, "userSpaceOnUse")) {
				marker->markerUnits = SP_MARKER_UNITS_USERSPACEONUSE;
				marker->markerUnits_set = TRUE;
			}
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		break;
	case SP_ATTR_REFX:
		if (!sp_svg_length_read (value, &marker->refX)) {
			sp_svg_length_unset (&marker->refX, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_REFY:
		if (!sp_svg_length_read (value, &marker->refY)) {
			sp_svg_length_unset (&marker->refX, SP_SVG_UNIT_NONE, 0.0, 0.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_MARKERWIDTH:
		if (!sp_svg_length_read (value, &marker->markerWidth)) {
			sp_svg_length_unset (&marker->markerWidth, SP_SVG_UNIT_NONE, 3.0, 3.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_MARKERHEIGHT:
		if (!sp_svg_length_read (value, &marker->markerHeight)) {
			sp_svg_length_unset (&marker->markerHeight, SP_SVG_UNIT_NONE, 3.0, 3.0);
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_ORIENT:
		marker->orient_set = FALSE;
		marker->orient_auto = FALSE;
		marker->orient = 0.0;
		if (value) {
			if (!strcmp (value, "auto")) {
				marker->orient_auto = TRUE;
				marker->orient_set = TRUE;
			} else if (sp_svg_number_read_f (value, &marker->orient)) {
				marker->orient_set = TRUE;
			}
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG);
		break;
	case SP_ATTR_VIEWBOX:
		marker->viewBox_set = FALSE;
		if (value) {
			double x, y, width, height;
			char *eptr;
			/* fixme: We have to take original item affine into account */
			/* fixme: Think (Lauris) */
			eptr = (gchar *) value;
			eptr += arikkei_strtod_exp (eptr, 1024, &x);
			while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
			eptr += arikkei_strtod_exp (eptr, 1024, &y);
			while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
			eptr += arikkei_strtod_exp (eptr, 1024, &width);
			while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
			eptr += arikkei_strtod_exp (eptr, 1024, &height);
			while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
			if ((width > 0) && (height > 0)) {
				/* Set viewbox */
				marker->viewBox.x0 = x;
				marker->viewBox.y0 = y;
				marker->viewBox.x1 = x + width;
				marker->viewBox.y1 = y + height;
				marker->viewBox_set = TRUE;
			}
		}
		sp_object_request_update (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
		break;
	case SP_ATTR_PRESERVEASPECTRATIO:
		/* Do setup before, so we can use break to escape */
		marker->aspect_set = FALSE;
		marker->aspect_align = SP_ASPECT_NONE;
		marker->aspect_clip = SP_ASPECT_MEET;
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
			if (e) {
				if (!strcmp (e, "meet")) {
					clip = SP_ASPECT_MEET;
				} else if (!strcmp (e, "slice")) {
					clip = SP_ASPECT_SLICE;
				} else {
					break;
				}
			}
			marker->aspect_set = TRUE;
			marker->aspect_align = align;
			marker->aspect_clip = clip;
		}
		break;
	default:
		if (((SPObjectClass *) parent_class)->set)
			((SPObjectClass *) parent_class)->set (object, key, value);
		break;
	}
}

/*
 * Updating <marker> - we are not renderable anyways, so
 * we as well cascade with identity transforms
 */

static void
sp_marker_update (SPObject *object, SPCtx *ctx, guint flags)
{
	SPItem *item;
	SPMarker *marker;
	SPItemCtx rctx;
	NRRectD *vb;
	double x, y, width, height;
	NRMatrixD q;
	SPMarkerView *v;

	item = SP_ITEM (object);
	marker = SP_MARKER (object);

	/* fixme: We have to set up clip here too */

	/* Copy parent context */
	rctx.ctx = *ctx;
	/* Initialize tranformations */
	nr_matrix_d_set_identity (&rctx.i2doc);
	nr_matrix_d_set_identity (&rctx.i2vp);
	/* Set up viewport */
	rctx.vp.x0 = 0.0;
	rctx.vp.y0 = 0.0;
	rctx.vp.x1 = marker->markerWidth.computed;
	rctx.vp.y1 = marker->markerHeight.computed;

	/* Start with identity transform */
	nr_matrix_d_set_identity (&marker->c2p);

	/* Viewbox is always present, either implicitly or explicitly */
	if (marker->viewBox_set) {
		vb = &marker->viewBox;
	} else {
		vb = &rctx.vp;
	}
	/* Now set up viewbox transformation */
	/* Determine actual viewbox in viewport coordinates */
	if (marker->aspect_align == SP_ASPECT_NONE) {
		x = 0.0;
		y = 0.0;
		width = rctx.vp.x1 - rctx.vp.x0;
		height = rctx.vp.y1 - rctx.vp.y0;
	} else {
		double scalex, scaley, scale;
		/* Things are getting interesting */
		scalex = (rctx.vp.x1 - rctx.vp.x0) / (vb->x1 - vb->x0);
		scaley = (rctx.vp.y1 - rctx.vp.y0) / (vb->y1 - vb->y0);
		scale = (marker->aspect_clip == SP_ASPECT_MEET) ? MIN (scalex, scaley) : MAX (scalex, scaley);
		width = (vb->x1 - vb->x0) * scale;
		height = (vb->y1 - vb->y0) * scale;
		/* Now place viewbox to requested position */
		switch (marker->aspect_align) {
		case SP_ASPECT_XMIN_YMIN:
			x = 0.0;
			y = 0.0;
			break;
		case SP_ASPECT_XMID_YMIN:
			x = 0.5 * ((rctx.vp.x1 - rctx.vp.x0) - width);
			y = 0.0;
			break;
		case SP_ASPECT_XMAX_YMIN:
			x = 1.0 * ((rctx.vp.x1 - rctx.vp.x0) - width);
			y = 0.0;
			break;
		case SP_ASPECT_XMIN_YMID:
			x = 0.0;
			y = 0.5 * ((rctx.vp.y1 - rctx.vp.y0) - height);
			break;
		case SP_ASPECT_XMID_YMID:
			x = 0.5 * ((rctx.vp.x1 - rctx.vp.x0) - width);
			y = 0.5 * ((rctx.vp.y1 - rctx.vp.y0) - height);
			break;
		case SP_ASPECT_XMAX_YMID:
			x = 1.0 * ((rctx.vp.x1 - rctx.vp.x0) - width);
			y = 0.5 * ((rctx.vp.y1 - rctx.vp.y0) - height);
			break;
		case SP_ASPECT_XMIN_YMAX:
			x = 0.0;
			y = 1.0 * ((rctx.vp.y1 - rctx.vp.y0) - height);
			break;
		case SP_ASPECT_XMID_YMAX:
			x = 0.5 * ((rctx.vp.x1 - rctx.vp.x0) - width);
			y = 1.0 * ((rctx.vp.y1 - rctx.vp.y0) - height);
			break;
		case SP_ASPECT_XMAX_YMAX:
			x = 1.0 * ((rctx.vp.x1 - rctx.vp.x0) - width);
			y = 1.0 * ((rctx.vp.y1 - rctx.vp.y0) - height);
			break;
		default:
			x = 0.0;
			y = 0.0;
			break;
		}
	}
	/* Compose additional transformation from scale and position */
	q.c[0] = width / (vb->x1 - vb->x0);
	q.c[1] = 0.0;
	q.c[2] = 0.0;
	q.c[3] = height / (vb->y1 - vb->y0);
	q.c[4] = -vb->x0 * q.c[0] + x;
	q.c[5] = -vb->y0 * q.c[3] + y;
	/* Append viewbox transformation */
	nr_matrix_multiply_ddd (&marker->c2p, &q, &marker->c2p);


	/* Append reference translation */
	/* fixme: lala (Lauris) */
	nr_matrix_d_set_translate (&q, -marker->refX.computed, -marker->refY.computed);
	nr_matrix_multiply_ddd (&marker->c2p, &q, &marker->c2p);


	nr_matrix_multiply_ddd (&rctx.i2doc, &marker->c2p, &rctx.i2doc);

	/* If viewBox is set reinitialize child viewport */
	/* Otherwise it already correct */
	if (marker->viewBox_set) {
		rctx.vp.x0 = marker->viewBox.x0;
		rctx.vp.y0 = marker->viewBox.y0;
		rctx.vp.x1 = marker->viewBox.x1;
		rctx.vp.y1 = marker->viewBox.y1;
		nr_matrix_d_set_identity (&rctx.i2vp);
	}

	/* And invoke parent method */
	if (((SPObjectClass *) (parent_class))->update)
		((SPObjectClass *) (parent_class))->update (object, (SPCtx *) &rctx, flags);

	/* As last step set additional transform of arena group */
	for (v = marker->views; v != NULL; v = v->next) {
		NRMatrixF vbf;
		int i;
		nr_matrix_f_from_d (&vbf, &marker->c2p);
		for (i = 0; i < v->size; i++) {
			if (v->items[i]) nr_arena_group_set_child_transform (NR_ARENA_GROUP (v->items[i]), &vbf);
		}
	}
}

static SPRepr *
sp_marker_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPMarker *marker;

	marker = SP_MARKER (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("marker");
	}

	if (marker->markerUnits_set) {
		if (marker->markerUnits == SP_MARKER_UNITS_STROKEWIDTH) {
			sp_repr_set_attr (repr, "markerUnits", "strokeWidth");
		} else {
			sp_repr_set_attr (repr, "markerUnits", "userSpaceOnUse");
		}
	} else {
		sp_repr_set_attr (repr, "markerUnits", NULL);
	}
	if (marker->refX.set) {
		sp_repr_set_double (repr, "refX", marker->refX.computed);
	} else {
		sp_repr_set_attr (repr, "refX", NULL);
	}
	if (marker->refY.set) {
		sp_repr_set_double (repr, "refY", marker->refY.computed);
	} else {
		sp_repr_set_attr (repr, "refY", NULL);
	}
	if (marker->markerWidth.set) {
		sp_repr_set_double (repr, "markerWidth", marker->markerWidth.computed);
	} else {
		sp_repr_set_attr (repr, "markerWidth", NULL);
	}
	if (marker->markerHeight.set) {
		sp_repr_set_double (repr, "markerHeight", marker->markerHeight.computed);
	} else {
		sp_repr_set_attr (repr, "markerHeight", NULL);
	}
	if (marker->orient_set) {
		if (marker->orient_auto) {
			sp_repr_set_attr (repr, "orient", "auto");
		} else {
			sp_repr_set_double (repr, "orient", marker->orient);
		}
	} else {
		sp_repr_set_attr (repr, "orient", NULL);
	}
	/* fixme: */
	sp_repr_set_attr (repr, "viewBox", sp_repr_attr (object->repr, "viewBox"));
	sp_repr_set_attr (repr, "preserveAspectRatio", sp_repr_attr (object->repr, "preserveAspectRatio"));

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

static NRArenaItem *
sp_marker_private_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags)
{
	/* Break propagation */
	return NULL;
}

static void
sp_marker_private_hide (SPItem *item, unsigned int key)
{
	/* Break propagation */
}

static void
sp_marker_bbox (SPItem *item, NRRectF *bbox, const NRMatrixD *transform, unsigned int flags)
{
	/* Break propagation */
}

static void
sp_marker_print (SPItem *item, SPPrintContext *ctx)
{
	/* Break propagation */
}

/* fixme: Remove link if zero-sized (Lauris) */

void
sp_marker_show_dimension (SPMarker *marker, unsigned int key, unsigned int size)
{
	SPMarkerView *view;
	int i;

	for (view = marker->views; view != NULL; view = view->next) {
		if (view->key == key) break;
	}
	if (view && (view->size != size)) {
		/* Free old view and allocate new */
		/* Parent class ::hide method */
		((SPItemClass *) parent_class)->hide ((SPItem *) marker, key);
		sp_marker_view_remove (marker, view, TRUE);
		view = NULL;
	}
	if (!view) {
		view = malloc (sizeof (SPMarkerView) + (size - 1) * sizeof (NRArenaItem *));
		for (i = 0; i < size; i++) view->items[i] = NULL;
		view->next = marker->views;
		marker->views = view;
		view->key = key;
		view->size = size;
	}
}

NRArenaItem *
sp_marker_show_instance (SPMarker *marker, NRArenaItem *parent,
			 unsigned int key, unsigned int pos,
			 NRMatrixF *base, float linewidth)
{
	SPMarkerView *v;

	for (v = marker->views; v != NULL; v = v->next) {
		if (v->key == key) {
			if (pos >= v->size) return NULL;
			if (!v->items[pos]) {
				/* Parent class ::show method */
				v->items[pos] = ((SPItemClass *) parent_class)->show ((SPItem *) marker, parent->arena, key,
										      SP_ITEM_REFERENCE_FLAGS);
				if (v->items[pos]) {
					NRMatrixF vbf;
					/* fixme: Position (Lauris) */
					nr_arena_item_add_child (parent, v->items[pos], NULL);
					/* nr_arena_item_unref (v->items[pos]); */
					nr_matrix_f_from_d (&vbf, &marker->c2p);
					nr_arena_group_set_child_transform ((NRArenaGroup *) v->items[pos], &vbf);
				}
			}
			if (v->items[pos]) {
				NRMatrixF m;
				if (marker->orient_auto) {
					m = *base;
				} else {
					/* fixme: Orient units (Lauris) */
					nr_matrix_f_set_rotate (&m, marker->orient * M_PI / 180.0);
					m.c[4] = base->c[4];
					m.c[5] = base->c[5];
				}
				if (marker->markerUnits == SP_MARKER_UNITS_STROKEWIDTH) {
					m.c[0] *= linewidth;
					m.c[1] *= linewidth;
					m.c[2] *= linewidth;
					m.c[3] *= linewidth;
				}
				nr_arena_item_set_transform (v->items[pos], &m);
			}
			return v->items[pos];
		}
	}

	return NULL;
}

void
sp_marker_union_bbox (SPMarker *marker,
                      unsigned int key, unsigned int pos,
                      const NRMatrixD *base, float linewidth,
                      NRRectF *bbox)
{
	SPMarkerView *v;

	printf("sp_marker_union_bbox(key: %u)\n",key);

	for (v = marker->views; v != NULL; v = v->next) {
		printf("\tv->key: %u\n",v->key);
		if (v->key == key) {
			NRMatrixD m;
			float x, y;
			if (pos >= v->size) return;
			if (marker->orient_auto) {
				m = *base;
			} else {
				/* fixme: Orient units (Lauris) */
				nr_matrix_d_set_rotate (&m, marker->orient * M_PI / 180.0);
				m.c[4] = base->c[4];
				m.c[5] = base->c[5];
			}
			if (marker->markerUnits == SP_MARKER_UNITS_STROKEWIDTH) {
				m.c[0] *= linewidth;
				m.c[1] *= linewidth;
				m.c[2] *= linewidth;
				m.c[3] *= linewidth;
			}

			x = fabs (NR_MATRIX_DF_TRANSFORM_X(&m,1,1));
			y = fabs (NR_MATRIX_DF_TRANSFORM_Y(&m,1,1));

			bbox->x0=MIN(bbox->x0,-x);
			bbox->y0=MIN(bbox->y0,-y);
			bbox->x1=MAX(bbox->x1,x);
			bbox->y1=MAX(bbox->y1,y);

			break;
		}
	}
}

/* This replaces SPItem implementation because we have own views */

void
sp_marker_hide (SPMarker *marker, unsigned int key)
{
	SPMarkerView *v;

	v = marker->views;
	while (v != NULL) {
		SPMarkerView *next;
		next = v->next;
		if (v->key == key) {
			/* Parent class ::hide method */
			((SPItemClass *) parent_class)->hide ((SPItem *) marker, key);
			sp_marker_view_remove (marker, v, TRUE);
			return;
		}
		v = next;
	}
}

static void
sp_marker_view_remove (SPMarker *marker, SPMarkerView *view, unsigned int destroyitems)
{
	int i;
	if (view == marker->views) {
		marker->views = view->next;
	} else {
		SPMarkerView *v;
		for (v = marker->views; v->next != view; v = v->next) if (!v->next) return;
		v->next = view->next;
	}
	if (destroyitems) {
		for (i = 0; i < view->size; i++) {
			/* We have to walk through the whole array because there may be hidden items */
			if (view->items[i]) nr_arena_item_unref (view->items[i]);
		}
	}
	g_free (view);
}


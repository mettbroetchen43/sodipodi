#define __SP_GRADIENT_C__

/*
 * SVG <stop> <linearGradient> and <radialGradient> implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <libnr/nr-gradient.h>

#include <libart_lgpl/art_affine.h>
#include <gtk/gtksignal.h>

#include "svg/svg.h"
#include "xml/repr-private.h"
#include "helper/nr-plain-stuff.h"
#include "document-private.h"
#include "sp-object-repr.h"
#include "sp-gradient.h"

#define SP_MACROS_SILENT
#include "macros.h"

/* Has to be power of 2 */
#define NCOLORS NR_GRADIENT_VECTOR_LENGTH

static void sp_stop_class_init (SPStopClass * klass);
static void sp_stop_init (SPStop * stop);

static void sp_stop_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_stop_read_attr (SPObject * object, const gchar * key);
static SPRepr *sp_stop_write (SPObject *object, SPRepr *repr, guint flags);

static SPObjectClass * stop_parent_class;

GtkType
sp_stop_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPStop",
			sizeof (SPStop),
			sizeof (SPStopClass),
			(GtkClassInitFunc) sp_stop_class_init,
			(GtkObjectInitFunc) sp_stop_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_OBJECT, &info);
	}
	return type;
}

static void
sp_stop_class_init (SPStopClass * klass)
{
	SPObjectClass * sp_object_class;

	sp_object_class = (SPObjectClass *) klass;

	stop_parent_class = gtk_type_class (SP_TYPE_OBJECT);

	sp_object_class->build = sp_stop_build;
	sp_object_class->read_attr = sp_stop_read_attr;
	sp_object_class->write = sp_stop_write;
}

static void
sp_stop_init (SPStop *stop)
{
	stop->offset = 0.0;
	sp_color_set_rgb_rgba32 (&stop->color, 0x000000ff);
}

static void
sp_stop_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	if (((SPObjectClass *) stop_parent_class)->build)
		(* ((SPObjectClass *) stop_parent_class)->build) (object, document, repr);

	sp_stop_read_attr (object, "offset");
	sp_stop_read_attr (object, "stop-color");
	sp_stop_read_attr (object, "stop-opacity");
	sp_stop_read_attr (object, "style");
}

static void
sp_stop_read_attr (SPObject * object, const gchar * key)
{
	SPStop * stop;
	guint32 color;
	gdouble opacity;

	stop = SP_STOP (object);

	if (!strcmp (key, "style")) {
		const guchar *p;
		/* fixme: We are reading simple values 3 times during build (Lauris) */
		/* fixme: We need presentation attributes etc. */
		p = sp_object_get_style_property (object, "stop-color", "black");
		color = sp_svg_read_color (p, sp_color_get_rgba32_ualpha (&stop->color, 0x00));
		sp_color_set_rgb_rgba32 (&stop->color, color);
		p = sp_object_get_style_property (object, "stop-opacity", "1");
		opacity = sp_svg_read_percentage (p, stop->opacity);
		stop->opacity = opacity;
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
	} else if (!strcmp (key, "stop-color")) {
		const guchar *p;
		/* fixme: We need presentation attributes etc. */
		p = sp_object_get_style_property (object, "stop-color", "black");
		color = sp_svg_read_color (p, sp_color_get_rgba32_ualpha (&stop->color, 0x00));
		sp_color_set_rgb_rgba32 (&stop->color, color);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
	} else if (!strcmp (key, "stop-opacity")) {
		const guchar *p;
		/* fixme: We need presentation attributes etc. */
		p = sp_object_get_style_property (object, "stop-opacity", "1");
		opacity = sp_svg_read_percentage (p, stop->opacity);
		stop->opacity = opacity;
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
	} else if (!strcmp (key, "offset")) {
		const guchar *str;
		str = sp_repr_attr (object->repr, key);
		stop->offset = sp_svg_read_percentage (str, 0.0);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else {
		if (((SPObjectClass *) stop_parent_class)->read_attr)
			(* ((SPObjectClass *) stop_parent_class)->read_attr) (object, key);
	}
}

static SPRepr *
sp_stop_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPStop *stop;

	stop = SP_STOP (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) & !repr) {
		repr = sp_repr_new ("stop");
	}

	if (repr != SP_OBJECT_REPR (object)) {
		sp_repr_set_attr (repr, "stop-color", sp_repr_attr (object->repr, "stop-color"));
		sp_repr_set_attr (repr, "stop-opacity", sp_repr_attr (object->repr, "stop-opacity"));
		sp_repr_set_attr (repr, "offset", sp_repr_attr (object->repr, "offset"));
	}

	if (((SPObjectClass *) stop_parent_class)->write)
		(* ((SPObjectClass *) stop_parent_class)->write) (object, repr, flags);

	return repr;
}

/*
 * Gradient
 */

static void sp_gradient_class_init (SPGradientClass *klass);
static void sp_gradient_init (SPGradient *gr);

static void sp_gradient_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_gradient_release (SPObject *object);
static void sp_gradient_read_attr (SPObject *object, const gchar *key);
static void sp_gradient_child_added (SPObject *object, SPRepr *child, SPRepr *ref);
static void sp_gradient_remove_child (SPObject *object, SPRepr *child);
static void sp_gradient_modified (SPObject *object, guint flags);
static SPRepr *sp_gradient_write (SPObject *object, SPRepr *repr, guint flags);

static void sp_gradient_flatten_attributes (SPGradient *gradient, SPRepr *repr, gboolean set_missing);

static void sp_gradient_href_destroy (SPObject *href, SPGradient *gradient);
static void sp_gradient_href_modified (SPObject *href, guint flags, SPGradient *gradient);

static void sp_gradient_invalidate_vector (SPGradient *gr);
static void sp_gradient_rebuild_vector (SPGradient *gr);

static SPPaintServerClass * gradient_parent_class;

GtkType
sp_gradient_get_type (void)
{
	static GtkType gradient_type = 0;
	if (!gradient_type) {
		GtkTypeInfo gradient_info = {
			"SPGradient",
			sizeof (SPGradient),
			sizeof (SPGradientClass),
			(GtkClassInitFunc) sp_gradient_class_init,
			(GtkObjectInitFunc) sp_gradient_init,
			NULL, NULL, NULL
		};
		gradient_type = gtk_type_unique (SP_TYPE_PAINT_SERVER, &gradient_info);
	}
	return gradient_type;
}

static void
sp_gradient_class_init (SPGradientClass *klass)
{
	GtkObjectClass *gtk_object_class;
	SPObjectClass *sp_object_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	gradient_parent_class = gtk_type_class (SP_TYPE_PAINT_SERVER);

	sp_object_class->build = sp_gradient_build;
	sp_object_class->release = sp_gradient_release;
	sp_object_class->read_attr = sp_gradient_read_attr;
	sp_object_class->child_added = sp_gradient_child_added;
	sp_object_class->remove_child = sp_gradient_remove_child;
	sp_object_class->modified = sp_gradient_modified;
	sp_object_class->write = sp_gradient_write;

	klass->flatten_attributes = sp_gradient_flatten_attributes;
}

static void
sp_gradient_init (SPGradient *gr)
{
	/* fixme: There is one problem - if reprs are rearranged, state has to be cleared somehow */
	/* fixme: Maybe that is not problem at all, as no force can rearrange childrens of <defs> */
	/* fixme: But keep that in mind, if messing with XML tree (Lauris) */
	gr->state = SP_GRADIENT_STATE_UNKNOWN;

	gr->units = SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX;
	art_affine_identity (gr->transform);

	gr->spread = SP_GRADIENT_SPREAD_PAD;
	gr->spread_set = FALSE;

	gr->stops = NULL;
	gr->has_stops = FALSE;

	gr->vector = NULL;
	gr->color = NULL;

	gr->len = 0.0;
}

static void
sp_gradient_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	SPGradient *gradient;
	SPObject *last;
	SPRepr *rchild;

	gradient = SP_GRADIENT (object);

	if (((SPObjectClass *) gradient_parent_class)->build)
		(* ((SPObjectClass *) gradient_parent_class)->build) (object, document, repr);

	/* fixme: Add all children, not only stops? */
	last = NULL;
	for (rchild = repr->children; rchild != NULL; rchild = rchild->next) {
		GtkType type;
		SPObject *child;
		type = sp_repr_type_lookup (rchild);
		if (gtk_type_is_a (type, SP_TYPE_OBJECT)) {
			child = gtk_type_new (type);
			(last) ? last->next : gradient->stops = sp_object_attach_reref (object, child, NULL);
			sp_object_invoke_build (child, document, rchild, SP_OBJECT_IS_CLONED (object));
			/* Set has_stops flag */
			if (SP_IS_STOP (child)) gradient->has_stops = TRUE;
			last = child;
		}
	}

	sp_gradient_read_attr (object, "gradientUnits");
	sp_gradient_read_attr (object, "gradientTransform");
	sp_gradient_read_attr (object, "spreadMethod");
	sp_gradient_read_attr (object, "xlink:href");

	/* Register ourselves */
	sp_document_add_resource (document, "gradient", object);
}

static void
sp_gradient_release (SPObject *object)
{
	SPGradient *gradient;

	gradient = (SPGradient *) object;

	if (SP_OBJECT_DOCUMENT (object)) {
		/* Unregister ourselves */
		sp_document_remove_resource (SP_OBJECT_DOCUMENT (object), "gradient", SP_OBJECT (object));
	}

	if (gradient->href) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (gradient->href), gradient);
		gradient->href = (SPGradient *) sp_object_hunref (SP_OBJECT (gradient->href), object);
	}

	if (gradient->color) {
		g_free (gradient->color);
		gradient->color = NULL;
	}

	if (gradient->vector) {
		g_free (gradient->vector);
		gradient->vector = NULL;
	}

	while (gradient->stops) {
		gradient->stops = sp_object_detach_unref (object, gradient->stops);
	}

	if (((SPObjectClass *) gradient_parent_class)->release)
		((SPObjectClass *) gradient_parent_class)->release (object);
}

static void
sp_gradient_read_attr (SPObject *object, const gchar *key)
{
	SPGradient *gr;
	const gchar *val;

	gr = SP_GRADIENT (object);

	val = sp_repr_attr (object->repr, key);

	/* fixme: We should unset properties, if val == NULL */
	if (!strcmp (key, "gradientUnits")) {
		if (val) {
			if (!strcmp (val, "userSpaceOnUse")) {
				gr->units = SP_GRADIENT_UNITS_USERSPACEONUSE;
			} else {
				gr->units = SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX;
			}
			gr->units_set = TRUE;
		} else {
			gr->units_set = FALSE;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	} else if (!strcmp (key, "gradientTransform")) {
		if (val) {
			art_affine_identity (gr->transform);
			sp_svg_read_affine (gr->transform, val);
			gr->transform_set = TRUE;
		} else {
			gr->transform_set = FALSE;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	} else if (!strcmp (key, "spreadMethod")) {
		if (val) {
			if (!strcmp (val, "reflect")) {
				gr->spread = SP_GRADIENT_SPREAD_REFLECT;
			} else if (!strcmp (val, "repeat")) {
				gr->spread = SP_GRADIENT_SPREAD_REPEAT;
			} else {
				gr->spread = SP_GRADIENT_SPREAD_PAD;
			}
			gr->spread_set = TRUE;
		} else {
			gr->spread_set = FALSE;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	} else if (!strcmp (key, "xlink:href")) {
		if (gr->href) {
			gtk_signal_disconnect_by_data (GTK_OBJECT (gr->href), gr);
			gr->href = (SPGradient *) sp_object_hunref (SP_OBJECT (gr->href), object);
		}
		if (val && *val == '#') {
			SPObject *href;
			href = sp_document_lookup_id (object->document, val + 1);
			if (SP_IS_GRADIENT (href)) {
				gr->href = (SPGradient *) sp_object_href (href, object);
				gtk_signal_connect (GTK_OBJECT (href), "destroy",
						    GTK_SIGNAL_FUNC (sp_gradient_href_destroy), gr);
				gtk_signal_connect (GTK_OBJECT (href), "modified",
						    GTK_SIGNAL_FUNC (sp_gradient_href_modified), gr);
			}
		}
		sp_gradient_invalidate_vector (gr);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	}

	if (((SPObjectClass *) gradient_parent_class)->read_attr)
		(* ((SPObjectClass *) gradient_parent_class)->read_attr) (object, key);
}

static void
sp_gradient_child_added (SPObject *object, SPRepr *child, SPRepr *ref)
{
	SPGradient *gr;
	GtkType type;
	SPObject * ochild, * prev;

	gr = SP_GRADIENT (object);

	if (((SPObjectClass *) gradient_parent_class)->child_added)
		(* ((SPObjectClass *) gradient_parent_class)->child_added) (object, child, ref);

	sp_gradient_invalidate_vector (gr);

	type = sp_repr_type_lookup (child);
	ochild = gtk_type_new (type);
	ochild->parent = object;

	prev = NULL;
	if (ref) {
		prev = gr->stops;
		while (prev->repr != ref) prev = prev->next;
	}

	if (!prev) {
		ochild->next = gr->stops;
		gr->stops = ochild;
	} else {
		ochild->next = prev->next;
		prev->next = ochild;
	}

	sp_object_invoke_build (ochild, object->document, child, SP_OBJECT_IS_CLONED (object));

	/* fixme: (Lauris) */
	if (SP_IS_STOP (ochild)) gr->has_stops = TRUE;

	/* fixme: should we schedule "modified" here? */
}

static void
sp_gradient_remove_child (SPObject *object, SPRepr *child)
{
	SPGradient *gr;
	SPObject *prev, *ochild;

	gr = SP_GRADIENT (object);

	if (((SPObjectClass *) gradient_parent_class)->remove_child)
		(* ((SPObjectClass *) gradient_parent_class)->remove_child) (object, child);

	sp_gradient_invalidate_vector (gr);

	prev = NULL;
	ochild = gr->stops;
	while (ochild->repr != child) {
		prev = ochild;
		ochild = ochild->next;
	}

	if (prev) {
		prev->next = ochild->next;
	} else {
		gr->stops = ochild->next;
	}

	ochild->parent = NULL;
	ochild->next = NULL;
	gtk_object_unref (GTK_OBJECT (ochild));

	/* fixme: (Lauris) */
	gr->has_stops = FALSE;
	for (ochild = gr->stops; ochild != NULL; ochild = ochild->next) {
		if (SP_IS_STOP (ochild)) {
			gr->has_stops = TRUE;
			break;
		}
	}

	/* fixme: should we schedule "modified" here? */
}

static void
sp_gradient_modified (SPObject *object, guint flags)
{
	SPGradient *gr;
	SPObject *child;
	GSList *l;

	gr = SP_GRADIENT (object);

	if (flags & SP_OBJECT_CHILD_MODIFIED_FLAG) {
		sp_gradient_invalidate_vector (gr);
	}

	if (flags & SP_OBJECT_MODIFIED_FLAG) flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
	flags &= SP_OBJECT_MODIFIED_CASCADE;

	l = NULL;
	for (child = gr->stops; child != NULL; child = child->next) {
		gtk_object_ref (GTK_OBJECT (child));
		l = g_slist_prepend (l, child);
	}
	l = g_slist_reverse (l);
	while (l) {
		child = SP_OBJECT (l->data);
		l = g_slist_remove (l, child);
		if (flags || (GTK_OBJECT_FLAGS (child) & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
			sp_object_modified (child, flags);
		}
		gtk_object_unref (GTK_OBJECT (child));
	}
}

static SPRepr *
sp_gradient_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPGradient *gr;

	gr = SP_GRADIENT (object);

	if (((SPObjectClass *) gradient_parent_class)->write)
		(* ((SPObjectClass *) gradient_parent_class)->write) (object, repr, flags);

	if (flags & SP_OBJECT_WRITE_BUILD) {
		SPObject *child;
		GSList *l;
		l = NULL;
		for (child = gr->stops; child != NULL; child = child->next) {
			SPRepr *crepr;
			crepr = sp_object_invoke_write (child, NULL, flags);
			if (crepr) l = g_slist_prepend (l, crepr);
		}
		while (l) {
			sp_repr_add_child (repr, (SPRepr *) l->data, NULL);
			sp_repr_unref ((SPRepr *) l->data);
			l = g_slist_remove (l, l->data);
		}
	}

	/* fixme: (Lauris) */
	sp_gradient_flatten_attributes (SP_GRADIENT (object), repr, TRUE);
}

static void
sp_gradient_flatten_attributes (SPGradient *gr, SPRepr *repr, gboolean set_missing)
{
	if (set_missing || gr->units_set) {
		switch (gr->units) {
		case SP_GRADIENT_UNITS_USERSPACEONUSE:
			sp_repr_set_attr (repr, "gradientUnits", "userSpaceOnUse");
			break;
		default:
			sp_repr_set_attr (repr, "gradientUnits", "objectBoundingBox");
			break;
		}
	}

	if (set_missing || gr->transform_set) {
		gchar c[256];
		if (sp_svg_write_affine (c, 256, gr->transform)) {
			sp_repr_set_attr (repr, "gradientTransform", c);
		} else {
			sp_repr_set_attr (repr, "gradientTransform", NULL);
		}
	}

	if (set_missing || gr->spread_set) {
		switch (gr->spread) {
		case SP_GRADIENT_SPREAD_REFLECT:
			sp_repr_set_attr (repr, "spreadMethod", "reflect");
			break;
		case SP_GRADIENT_SPREAD_REPEAT:
			sp_repr_set_attr (repr, "spreadMethod", "repeat");
			break;
		default:
			sp_repr_set_attr (repr, "spreadMethod", "pad");
			break;
		}
	}
}

/* Forces vector to be built, if not present (i.e. changed) */

void
sp_gradient_ensure_vector (SPGradient *gradient)
{
	g_return_if_fail (gradient != NULL);
	g_return_if_fail (SP_IS_GRADIENT (gradient));

	if (!gradient->vector) {
		sp_gradient_rebuild_vector (gradient);
	}
}

void
sp_gradient_set_vector (SPGradient *gradient, SPGradientVector *vector)
{
	g_return_if_fail (gradient != NULL);
	g_return_if_fail (SP_IS_GRADIENT (gradient));
	g_return_if_fail (vector != NULL);

	if (gradient->color) {
		g_free (gradient->color);
		gradient->color = NULL;
	}

	if (gradient->vector && (gradient->vector->nstops != vector->nstops)) {
		g_free (gradient->vector);
		gradient->vector = NULL;
	}
	if (!gradient->vector) {
		gradient->vector = g_malloc (sizeof (SPGradientVector) + (vector->nstops - 1) * sizeof (SPGradientStop));
	}
	memcpy (gradient->vector, vector, sizeof (SPGradientVector) + (vector->nstops - 1) * sizeof (SPGradientStop));

	sp_object_request_modified (SP_OBJECT (gradient), SP_OBJECT_MODIFIED_FLAG);
}

/* Gradient repr methods */
void
sp_gradient_repr_flatten_attributes (SPGradient *gr, SPRepr *repr, gboolean set_missing)
{
	g_return_if_fail (gr != NULL);
	g_return_if_fail (SP_IS_GRADIENT (gr));
	g_return_if_fail (repr != NULL);

	if (((SPGradientClass *) ((GtkObject *) gr)->klass)->flatten_attributes)
		((SPGradientClass *) ((GtkObject *) gr)->klass)->flatten_attributes (gr, repr, set_missing);
}

void
sp_gradient_repr_set_vector (SPGradient *gr, SPRepr *repr, SPGradientVector *vector)
{
	SPRepr *child;
	GSList *sl, *cl;
	gint i;

	g_return_if_fail (gr != NULL);
	g_return_if_fail (SP_IS_GRADIENT (gr));
	g_return_if_fail (repr != NULL);

	/* We have to be careful, as vector may be our own, so construct repr list at first */
	cl = NULL;
	if (vector) {
		for (i = 0; i < vector->nstops; i++) {
			SPRepr *child;
			guchar c[64], s[256];
			child = sp_repr_new ("stop");
			sp_repr_set_double_attribute (child, "offset",
						      vector->stops[i].offset * (vector->end - vector->start) + vector->start);
			sp_svg_write_color (c, 64, sp_color_get_rgba32_ualpha (&vector->stops[i].color, 0x00));
			g_snprintf (s, 256, "stop-color:%s;stop-opacity:%g;", c, vector->stops[i].opacity);
			sp_repr_set_attr (child, "style", s);
			/* Order will be reversed here */
			cl = g_slist_prepend (cl, child);
		}
	}

	/* Now collect stops from original repr */
	sl = NULL;
	for (child = repr->children; child != NULL; child = child->next) {
		if (!strcmp (sp_repr_name (child), "stop")) sl = g_slist_prepend (sl, child);
	}
	/* Remove all stops */
	while (sl) {
		/* fixme: This should work, unless we make gradient into generic group */
		sp_repr_unparent (sl->data);
		sl = g_slist_remove (sl, sl->data);
	}

	/* And insert new children from list */
	while (cl) {
		sp_repr_add_child (repr, (SPRepr *) cl->data, NULL);
		sp_repr_unref (child);
		cl = g_slist_remove (cl, cl->data);
	}
}

static void
sp_gradient_href_destroy (SPObject *href, SPGradient *gradient)
{
	gradient->href = (SPGradient *) sp_object_hunref (href, gradient);
	sp_gradient_invalidate_vector (gradient);
	sp_object_request_modified (SP_OBJECT (gradient), SP_OBJECT_MODIFIED_FLAG);
}

static void
sp_gradient_href_modified (SPObject *href, guint flags, SPGradient *gradient)
{
	sp_gradient_invalidate_vector (gradient);
	sp_object_request_modified (SP_OBJECT (gradient), SP_OBJECT_MODIFIED_FLAG);
}

/* Creates normalized color vector */

static void
sp_gradient_invalidate_vector (SPGradient *gr)
{
	if (gr->color) {
		g_free (gr->color);
		gr->color = NULL;
	}

	if (gr->vector) {
		g_free (gr->vector);
		gr->vector = NULL;
	}
}

static void
sp_gradient_rebuild_vector (SPGradient *gr)
{
	SPObject *child;
	SPColor color;
	gfloat opacity;
	gdouble offsets, offsete, offset;
	gboolean oset;
	gint len, vlen, pos;

	len = 0;
	sp_color_set_rgb_rgba32 (&color, 0x00000000);
	opacity = 0.0;
	offsets = offsete = 0.0;
	oset = FALSE;
	for (child = gr->stops; child != NULL; child = child->next) {
		if (SP_IS_STOP (child)) {
			SPStop *stop;
			stop = SP_STOP (child);
			if (!oset) {
				oset = TRUE;
				offsets = offsete = stop->offset;
				len += 1;
			} else if (stop->offset > (offsete + 1e-9)) {
				offsete = stop->offset;
				len += 1;
			}
			sp_color_copy (&color, &stop->color);
			opacity = stop->opacity;
		}
	}

	gr->has_stops = (len != 0);

	if ((len == 0) && (gr->href)) {
		/* Copy vector from parent */
		sp_gradient_ensure_vector (gr->href);
		if (!gr->vector || (gr->vector->nstops != gr->href->vector->nstops)) {
			if (gr->vector) g_free (gr->vector);
			gr->vector = g_malloc (sizeof (SPGradientVector) + (gr->href->vector->nstops - 1) * sizeof (SPGradientStop));
			gr->vector->nstops = gr->href->vector->nstops;
		}
		memcpy (gr->vector, gr->href->vector, sizeof (SPGradientVector) + (gr->vector->nstops - 1) * sizeof (SPGradientStop));
		return;
	}

	vlen = MAX (len, 2);

	if (!gr->vector || gr->vector->nstops != vlen) {
		if (gr->vector) g_free (gr->vector);
		gr->vector = g_malloc (sizeof (SPGradientVector) + (vlen - 1) * sizeof (SPGradientStop));
		gr->vector->nstops = vlen;
	}

	if (len < 2) {
		gr->vector->start = 0.0;
		gr->vector->end = 1.0;
		gr->vector->stops[0].offset = 0.0;
		sp_color_copy (&gr->vector->stops[0].color, &color);
		gr->vector->stops[0].opacity = opacity;
		gr->vector->stops[1].offset = 1.0;
		sp_color_copy (&gr->vector->stops[1].color, &color);
		gr->vector->stops[1].opacity = opacity;
		return;
	}

	/* o' = (o - oS) / (oE - oS) */
	gr->vector->start = offsets;
	gr->vector->end = offsete;

	pos = 0;
	offset = offsets;
	gr->vector->stops[0].offset = 0.0;
	for (child = gr->stops; child != NULL; child = child->next) {
		if (SP_IS_STOP (child)) {
			SPStop *stop;
			stop = SP_STOP (child);
			if (stop->offset > (offset + 1e-9)) {
				pos += 1;
				gr->vector->stops[pos].offset = (stop->offset - offsets) / (offsete - offsets);
			}
			sp_color_copy (&gr->vector->stops[pos].color, &stop->color);
			gr->vector->stops[pos].opacity = stop->opacity;
		}
	}
}

void
sp_gradient_ensure_colors (SPGradient *gr)
{
	gint i;

	if (!gr->vector) {
		sp_gradient_rebuild_vector (gr);
	}

	if (!gr->color) {
		gr->color = g_new (guchar, 4 * NCOLORS);
	}

	for (i = 0; i < gr->vector->nstops - 1; i++) {
		guint32 color;
		gint r0, g0, b0, a0;
		gint r1, g1, b1, a1;
		gint dr, dg, db, da;
		gint r, g, b, a;
		gint o0, o1;
		gint j;
		color = sp_color_get_rgba32_falpha (&gr->vector->stops[i].color, gr->vector->stops[i].opacity);
		r0 = (color >> 24) & 0xff;
		g0 = (color >> 16) & 0xff;
		b0 = (color >> 8) & 0xff;
		a0 = color & 0xff;
		color = sp_color_get_rgba32_falpha (&gr->vector->stops[i + 1].color, gr->vector->stops[i + 1].opacity);
		r1 = (color >> 24) & 0xff;
		g1 = (color >> 16) & 0xff;
		b1 = (color >> 8) & 0xff;
		a1 = color & 0xff;
		o0 = (gint) floor (gr->vector->stops[i].offset * (NCOLORS + 0.9999));
		o1 = (gint) floor (gr->vector->stops[i + 1].offset * (NCOLORS + 0.9999));
		if (o1 > o0) {
			dr = ((r1 - r0) << 16) / (o1 - o0);
			dg = ((g1 - g0) << 16) / (o1 - o0);
			db = ((b1 - b0) << 16) / (o1 - o0);
			da = ((a1 - a0) << 16) / (o1 - o0);
			r = r0 << 16;
			g = g0 << 16;
			b = b0 << 16;
			a = a0 << 16;
#if 0
			g_print ("from %d to %d: %x %x %x %x\n", o0, o1, dr, dg, db, da);
#endif
			for (j = o0; j < o1 + 1; j++) {
				gr->color[4 * j] = r >> 16;
				gr->color[4 * j + 1] = g >> 16;
				gr->color[4 * j + 2] = b >> 16;
				gr->color[4 * j + 3] = a >> 16;
#if 0
				g_print ("%x\n", gr->color[j]);
#endif
				r += dr;
				g += dg;
				b += db;
				a += da;
			}
		}
	}

	gr->len = gr->vector->end - gr->vector->start;
}

/*
 * Renders gradient vector to buffer
 *
 * len, width, height, rowstride - buffer parameters (1 or 2 dimensional)
 * span - full integer width of requested gradient
 * pos - buffer starting position in span
 *
 * RGB buffer background should be set up before
 */

void
sp_gradient_render_vector_line_rgba (SPGradient *gradient, guchar *buf, gint len, gint pos, gint span)
{
	gint x, idx, didx;

	g_return_if_fail (gradient != NULL);
	g_return_if_fail (SP_IS_GRADIENT (gradient));
	g_return_if_fail (buf != NULL);
	g_return_if_fail (len > 0);
	g_return_if_fail (pos >= 0);
	g_return_if_fail (pos + len <= span);
	g_return_if_fail (span > 0);

	if (!gradient->color) {
		sp_gradient_ensure_colors (gradient);
	}

	idx = (pos * 1024 << 8) / span;
	didx = (1024 << 8) / span;

	for (x = 0; x < len; x++) {
		*buf++ = gradient->color[4 * (idx >> 8)];
		*buf++ = gradient->color[4 * (idx >> 8) + 1];
		*buf++ = gradient->color[4 * (idx >> 8) + 2];
		*buf++ = gradient->color[4 * (idx >> 8) + 3];
		idx += didx;
	}
}

void
sp_gradient_render_vector_line_rgb (SPGradient *gradient, guchar *buf, gint len, gint pos, gint span)
{
	gint x, idx, didx;

	g_return_if_fail (gradient != NULL);
	g_return_if_fail (SP_IS_GRADIENT (gradient));
	g_return_if_fail (buf != NULL);
	g_return_if_fail (len > 0);
	g_return_if_fail (pos >= 0);
	g_return_if_fail (pos + len <= span);
	g_return_if_fail (span > 0);

	if (!gradient->color) {
		sp_gradient_ensure_colors (gradient);
	}

	idx = (pos * 1024 << 8) / span;
	didx = (1024 << 8) / span;

	for (x = 0; x < len; x++) {
		gint r, g, b, a, fc;
		r = gradient->color[4 * (idx >> 8)];
		g = gradient->color[4 * (idx >> 8) + 1];
		b = gradient->color[4 * (idx >> 8) + 2];
		a = gradient->color[4 * (idx >> 8) + 3];
		fc = (r - *buf) * a;
		buf[0] = *buf + ((fc + (fc >> 8) + 0x80) >> 8);
		fc = (g - *buf) * a;
		buf[1] = *buf + ((fc + (fc >> 8) + 0x80) >> 8);
		fc = (b - *buf) * a;
		buf[2] = *buf + ((fc + (fc >> 8) + 0x80) >> 8);
		buf += 3;
		idx += didx;
	}
}

void
sp_gradient_render_vector_block_rgba (SPGradient *gradient, guchar *buf, gint width, gint height, gint rowstride,
				      gint pos, gint span, gboolean horizontal)
{
	g_return_if_fail (gradient != NULL);
	g_return_if_fail (SP_IS_GRADIENT (gradient));
	g_return_if_fail (buf != NULL);
	g_return_if_fail (width > 0);
	g_return_if_fail (height > 0);
	g_return_if_fail (pos >= 0);
	g_return_if_fail ((horizontal && (pos + width <= span)) || (!horizontal && (pos + height <= span)));
	g_return_if_fail (span > 0);

	if (horizontal) {
		gint y;
		sp_gradient_render_vector_line_rgba (gradient, buf, width, pos, span);
		for (y = 1; y < height; y++) {
			memcpy (buf + y * rowstride, buf, 4 * width);
		}
	} else {
		guchar *tmp;
		gint x, y;
		tmp = alloca (4 * height);
		sp_gradient_render_vector_line_rgba (gradient, tmp, height, pos, span);
		for (y = 0; y < height; y++) {
			guchar *b;
			b = buf + y * rowstride;
			for (x = 0; x < width; x++) {
				*b++ = tmp[0];
				*b++ = tmp[1];
				*b++ = tmp[2];
				*b++ = tmp[3];
			}
			tmp += 4;
		}
	}
}

void
sp_gradient_render_vector_block_rgb (SPGradient *gradient, guchar *buf, gint width, gint height, gint rowstride,
				     gint pos, gint span, gboolean horizontal)
{
	g_return_if_fail (gradient != NULL);
	g_return_if_fail (SP_IS_GRADIENT (gradient));
	g_return_if_fail (buf != NULL);
	g_return_if_fail (width > 0);
	g_return_if_fail (height > 0);
	g_return_if_fail (pos >= 0);
	g_return_if_fail ((horizontal && (pos + width <= span)) || (!horizontal && (pos + height <= span)));
	g_return_if_fail (span > 0);

	if (horizontal) {
		guchar *tmp;
		gint x, y;
		tmp = alloca (4 * width);
		sp_gradient_render_vector_line_rgba (gradient, tmp, width, pos, span);
		for (y = 0; y < height; y++) {
			guchar *b, *t;
			b = buf + y * rowstride;
			t = tmp;
			for (x = 0; x < width; x++) {
				gint a, fc;
				a = t[3];
				fc = (t[0] - buf[0]) * a;
				buf[0] = buf[0] + ((fc + (fc >> 8) + 0x80) >> 8);
				fc = (t[1] - buf[1]) * a;
				buf[1] = buf[1] + ((fc + (fc >> 8) + 0x80) >> 8);
				fc = (t[2] - buf[2]) * a;
				buf[2] = buf[2] + ((fc + (fc >> 8) + 0x80) >> 8);
				buf += 3;
				t += 4;
			}
		}
	} else {
		guchar *tmp;
		gint x, y;
		tmp = alloca (4 * height);
		sp_gradient_render_vector_line_rgba (gradient, tmp, height, pos, span);
		for (y = 0; y < height; y++) {
			guchar *b, *t;
			b = buf + y * rowstride;
			t = tmp + 4 * y;
			for (x = 0; x < width; x++) {
				gint a, fc;
				a = t[3];
				fc = (t[0] - buf[0]) * a;
				buf[0] = buf[0] + ((fc + (fc >> 8) + 0x80) >> 8);
				fc = (t[1] - buf[1]) * a;
				buf[1] = buf[1] + ((fc + (fc >> 8) + 0x80) >> 8);
				fc = (t[2] - buf[2]) * a;
				buf[2] = buf[2] + ((fc + (fc >> 8) + 0x80) >> 8);
			}
		}
	}
}

NRMatrixF *
sp_gradient_get_gs2d_matrix_f (SPGradient *gr, NRMatrixF *ctm, NRRectF *bbox, NRMatrixF *gs2d)
{
	if (gr->units == SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX) {
		NRMatrixF bb2u, gs2u;

		bb2u.c[0] = bbox->x1 - bbox->x0;
		bb2u.c[1] = 0.0;
		bb2u.c[2] = 0.0;
		bb2u.c[3] = bbox->y1 - bbox->y0;
		bb2u.c[4] = bbox->x0;
		bb2u.c[5] = bbox->y0;

		nr_matrix_multiply_fdf (&gs2u, (NRMatrixD *) gr->transform, &bb2u);
		nr_matrix_multiply_fff (gs2d, &gs2u, ctm);
	} else {
		nr_matrix_multiply_fdf (gs2d, (NRMatrixD *) gr->transform, ctm);
	}

	return gs2d;
}

void
sp_gradient_set_gs2d_matrix_f (SPGradient *gr, NRMatrixF *ctm, NRRectF *bbox, NRMatrixF *gs2d)
{
	NRMatrixF g2d, d2g, gs2g;

	SP_PRINT_MATRIX ("* GS2D:", gs2d);

	if (gr->units == SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX) {
		NRMatrixF bb2u;

		bb2u.c[0] = bbox->x1 - bbox->x0;
		bb2u.c[1] = 0.0;
		bb2u.c[2] = 0.0;
		bb2u.c[3] = bbox->y1 - bbox->y0;
		bb2u.c[4] = bbox->x0;
		bb2u.c[5] = bbox->y0;

		SP_PRINT_MATRIX ("* BB2U:", &bb2u);

		nr_matrix_multiply_fff (&g2d, &bb2u, ctm);
	} else {
		g2d = *ctm;
	}

	SP_PRINT_MATRIX ("* G2D:", &g2d);

	nr_matrix_f_invert (&d2g, &g2d);
	SP_PRINT_MATRIX ("* D2G:", &d2g);
	nr_matrix_multiply_fff (&gs2g, gs2d, &d2g);
	SP_PRINT_MATRIX ("* GS2G:", &gs2g);

	gr->transform[0] = gs2g.c[0];
	gr->transform[1] = gs2g.c[1];
	gr->transform[2] = gs2g.c[2];
	gr->transform[3] = gs2g.c[3];
	gr->transform[4] = gs2g.c[4];
	gr->transform[5] = gs2g.c[5];

	sp_object_request_modified (SP_OBJECT (gr), SP_OBJECT_MODIFIED_FLAG);
}

void
sp_gradient_from_position_xy (SPGradient *gr, gdouble *ctm, ArtDRect *bbox, NRPointF *p, float x, float y)
{
	NRMatrixF gs2d;

	g_return_if_fail (gr != NULL);
	g_return_if_fail (SP_IS_GRADIENT (gr));
	g_return_if_fail (p != NULL);

	if (gr->units == SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX) {
		NRMatrixF bb2u, gs2u;

		bb2u.c[0] = bbox->x1 - bbox->x0;
		bb2u.c[1] = 0.0;
		bb2u.c[2] = 0.0;
		bb2u.c[3] = bbox->y1 - bbox->y0;
		bb2u.c[4] = bbox->x0;
		bb2u.c[5] = bbox->y0;

		nr_matrix_multiply_fdf (&gs2u, (NRMatrixD *) gr->transform, &bb2u);
		nr_matrix_multiply_ffd (&gs2d, &gs2u, (NRMatrixD *) ctm);
	} else {
		nr_matrix_multiply_fdd (&gs2d, (NRMatrixD *) gr->transform, (NRMatrixD *) ctm);
	}

	p->x = gs2d.c[0] * x + gs2d.c[2] * y + gs2d.c[4];
	p->y = gs2d.c[1] * x + gs2d.c[3] * y + gs2d.c[5];
}

void
sp_gradient_to_position_xy (SPGradient *gr, gdouble *ctm, ArtDRect *bbox, NRPointF *p, float x, float y)
{
	NRMatrixF gs2d, d2gs;

	g_return_if_fail (gr != NULL);
	g_return_if_fail (SP_IS_GRADIENT (gr));
	g_return_if_fail (p != NULL);

	if (gr->units == SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX) {
		NRMatrixF bb2u, gs2u;

		bb2u.c[0] = bbox->x1 - bbox->x0;
		bb2u.c[1] = 0.0;
		bb2u.c[2] = 0.0;
		bb2u.c[3] = bbox->y1 - bbox->y0;
		bb2u.c[4] = bbox->x0;
		bb2u.c[5] = bbox->y0;

		nr_matrix_multiply_fdf (&gs2u, (NRMatrixD *) gr->transform, &bb2u);
		nr_matrix_multiply_ffd (&gs2d, &gs2u, (NRMatrixD *) ctm);
	} else {
		nr_matrix_multiply_fdd (&gs2d, (NRMatrixD *) gr->transform, (NRMatrixD *) ctm);
	}

	nr_matrix_f_invert (&d2gs, &gs2d);

	p->x = d2gs.c[0] * x + d2gs.c[2] * y + d2gs.c[4];
	p->y = d2gs.c[1] * x + d2gs.c[3] * y + d2gs.c[5];
}

/*
 * Linear Gradient
 */

typedef struct _SPLGPainter SPLGPainter;

struct _SPLGPainter {
	SPPainter painter;
	SPLinearGradient *lg;

	NRLGradientRenderer lgr;
};

static void sp_lineargradient_class_init (SPLinearGradientClass * klass);
static void sp_lineargradient_init (SPLinearGradient * lg);

static void sp_lineargradient_build (SPObject *object, SPDocument * document, SPRepr * repr);
static void sp_lineargradient_read_attr (SPObject * object, const gchar * key);
static SPRepr *sp_lineargradient_write (SPObject *object, SPRepr *repr, guint flags);

static SPPainter *sp_lineargradient_painter_new (SPPaintServer *ps, const gdouble *affine, const ArtDRect *bbox);
static void sp_lineargradient_painter_free (SPPaintServer *ps, SPPainter *painter);

static void sp_lineargradient_flatten_attributes (SPGradient *gradient, SPRepr *repr, gboolean set_missing);

static void sp_lg_fill (SPPainter *painter, guchar *px, gint x0, gint y0, gint width, gint height, gint rowstride);

static SPGradientClass *lg_parent_class;

GtkType
sp_lineargradient_get_type (void)
{
	static GtkType lineargradient_type = 0;
	if (!lineargradient_type) {
		GtkTypeInfo lineargradient_info = {
			"SPLinearGradient",
			sizeof (SPLinearGradient),
			sizeof (SPLinearGradientClass),
			(GtkClassInitFunc) sp_lineargradient_class_init,
			(GtkObjectInitFunc) sp_lineargradient_init,
			NULL, NULL, NULL
		};
		lineargradient_type = gtk_type_unique (SP_TYPE_GRADIENT, &lineargradient_info);
	}
	return lineargradient_type;
}

static void
sp_lineargradient_class_init (SPLinearGradientClass * klass)
{
	GtkObjectClass *gtk_object_class;
	SPObjectClass *sp_object_class;
	SPPaintServerClass *ps_class;
	SPGradientClass *gr_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	ps_class = (SPPaintServerClass *) klass;
	gr_class = SP_GRADIENT_CLASS (klass);

	lg_parent_class = gtk_type_class (SP_TYPE_GRADIENT);

	sp_object_class->build = sp_lineargradient_build;
	sp_object_class->read_attr = sp_lineargradient_read_attr;
	sp_object_class->write = sp_lineargradient_write;

	ps_class->painter_new = sp_lineargradient_painter_new;
	ps_class->painter_free = sp_lineargradient_painter_free;

	gr_class->flatten_attributes = sp_lineargradient_flatten_attributes;
}

static void
sp_lineargradient_init (SPLinearGradient * lg)
{
	sp_svg_length_unset (&lg->x1, SP_SVG_UNIT_PERCENT, 0.0, 0.0);
	sp_svg_length_unset (&lg->y1, SP_SVG_UNIT_PERCENT, 0.0, 0.0);
	sp_svg_length_unset (&lg->x2, SP_SVG_UNIT_PERCENT, 1.0, 1.0);
	sp_svg_length_unset (&lg->y2, SP_SVG_UNIT_PERCENT, 0.0, 0.0);
}

static void
sp_lineargradient_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPLinearGradient * lg;

	lg = SP_LINEARGRADIENT (object);

	if (((SPObjectClass *) lg_parent_class)->build)
		(* ((SPObjectClass *) lg_parent_class)->build) (object, document, repr);

	sp_lineargradient_read_attr (object, "x1");
	sp_lineargradient_read_attr (object, "y1");
	sp_lineargradient_read_attr (object, "x2");
	sp_lineargradient_read_attr (object, "y2");
}

static void
sp_lineargradient_read_attr (SPObject * object, const gchar * key)
{
	SPLinearGradient * lg;
	const guchar *str;

	lg = SP_LINEARGRADIENT (object);

	str = sp_repr_attr (object->repr, key);

	if (!strcmp (key, "x1")) {
		if (!sp_svg_length_read (str, &lg->x1)) {
			sp_svg_length_unset (&lg->x1, SP_SVG_UNIT_PERCENT, 0.0, 0.0);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "y1")) {
		if (!sp_svg_length_read (str, &lg->y1)) {
			sp_svg_length_unset (&lg->y1, SP_SVG_UNIT_PERCENT, 0.0, 0.0);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "x2")) {
		if (!sp_svg_length_read (str, &lg->x2)) {
			sp_svg_length_unset (&lg->x2, SP_SVG_UNIT_PERCENT, 1.0, 1.0);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "y2")) {
		if (!sp_svg_length_read (str, &lg->y2)) {
			sp_svg_length_unset (&lg->y2, SP_SVG_UNIT_PERCENT, 0.0, 0.0);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else {
		if (((SPObjectClass *) lg_parent_class)->read_attr)
			(* ((SPObjectClass *) lg_parent_class)->read_attr) (object, key);
	}
}

static SPRepr *
sp_lineargradient_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPLinearGradient * lg;

	lg = SP_LINEARGRADIENT (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("linearGradient");
	}

	sp_repr_set_double (repr, "x1", lg->x1.computed);
	sp_repr_set_double (repr, "y1", lg->y1.computed);
	sp_repr_set_double (repr, "x2", lg->x2.computed);
	sp_repr_set_double (repr, "y2", lg->y2.computed);

	if (((SPObjectClass *) lg_parent_class)->write)
		(* ((SPObjectClass *) lg_parent_class)->write) (object, repr, flags);

	return repr;
}

/*
 * Basically we have to deal with transformations
 *
 * 1) color2norm - maps point in (0,NCOLORS) vector to (0,1) vector
 *    fixme: I do not know, how to deal with start > 0 and end < 1
 * 2) norm2pos - maps (0,1) vector to x1,y1 - x2,y2
 * 2) gradientTransform
 * 3) bbox2user
 * 4) ctm == userspace to pixel grid
 */

static SPPainter *
sp_lineargradient_painter_new (SPPaintServer *ps, const gdouble *ctm, const ArtDRect *bbox)
{
	SPLinearGradient *lg;
	SPGradient *gr;
	SPLGPainter *lgp;
	gdouble color2norm[6], color2px[6];
	NRMatrixF v2px;

	lg = SP_LINEARGRADIENT (ps);
	gr = SP_GRADIENT (ps);

	if (!gr->color) sp_gradient_ensure_colors (gr);

	lgp = g_new (SPLGPainter, 1);

	lgp->painter.type = SP_PAINTER_IND;
	lgp->painter.fill = sp_lg_fill;

	lgp->lg = lg;

	/* fixme: Technically speaking, we map NCOLORS on line [start,end] onto line [0,1] (Lauris) */
	/* fixme: I almost think, we should fill color array start and end in that case (Lauris) */
	/* fixme: The alternative would be to leave these just empty garbage or something similar (Lauris) */
	/* fixme: Originally I had 1023.9999 here - not sure, whether we have really to cut out ceil int (Lauris) */
#if 0
	art_affine_scale (color2norm, gr->len / (gdouble) NCOLORS, gr->len / (gdouble) NCOLORS);
	SP_PRINT_TRANSFORM ("color2norm", color2norm);
	/* Now we have normalized vector */
#else
	art_affine_identity (color2norm);
#endif

	if (gr->units == SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX) {
		gdouble norm2pos[6], bbox2user[6];
		gdouble color2pos[6], color2tpos[6], color2user[6];

#if 0
		/* This is easy case, as we can just ignore percenting here */
		/* fixme: Still somewhat tricky, but I think I got it correct (lauris) */
		norm2pos[0] = lg->x2.computed - lg->x1.computed;
		norm2pos[1] = lg->y2.computed - lg->y1.computed;
		norm2pos[2] = lg->y2.computed - lg->y1.computed;
		norm2pos[3] = lg->x1.computed - lg->x2.computed;
		norm2pos[4] = lg->x1.computed;
		norm2pos[5] = lg->y1.computed;
		SP_PRINT_TRANSFORM ("norm2pos", norm2pos);
#else
		art_affine_identity (norm2pos);
#endif

		/* gradientTransform goes here (Lauris) */
		SP_PRINT_TRANSFORM ("gradientTransform", gr->transform);

		/* BBox to user coordinate system */
		bbox2user[0] = bbox->x1 - bbox->x0;
		bbox2user[1] = 0.0;
		bbox2user[2] = 0.0;
		bbox2user[3] = bbox->y1 - bbox->y0;
		bbox2user[4] = bbox->x0;
		bbox2user[5] = bbox->y0;
		SP_PRINT_TRANSFORM ("bbox2user", bbox2user);

		/* CTM goes here */
		SP_PRINT_TRANSFORM ("ctm", ctm);

		art_affine_multiply (color2pos, color2norm, norm2pos);
		SP_PRINT_TRANSFORM ("color2pos", color2pos);
		art_affine_multiply (color2tpos, color2pos, gr->transform);
		SP_PRINT_TRANSFORM ("color2tpos", color2tpos);
		art_affine_multiply (color2user, color2tpos, bbox2user);
		SP_PRINT_TRANSFORM ("color2user", color2user);
		art_affine_multiply (color2px, color2user, ctm);
		SP_PRINT_TRANSFORM ("color2px", color2px);
	} else {
		gdouble norm2pos[6];
		gdouble color2pos[6], color2tpos[6];
		/* Problem: What to do, if we have mixed lengths and percentages? */
		/* Currently we do ignore percentages at all, but that is not good (lauris) */

#if 0
		/* fixme: Do percentages (Lauris) */
		norm2pos[0] = lg->x2.computed - lg->x1.computed;
		norm2pos[1] = lg->y2.computed - lg->y1.computed;
		norm2pos[2] = lg->y2.computed - lg->y1.computed;
		norm2pos[3] = lg->x1.computed - lg->x2.computed;
		norm2pos[4] = lg->x1.computed;
		norm2pos[5] = lg->y1.computed;
		SP_PRINT_TRANSFORM ("norm2pos", norm2pos);
#else
		art_affine_identity (norm2pos);
#endif

		/* gradientTransform goes here (Lauris) */
		SP_PRINT_TRANSFORM ("gradientTransform", gr->transform);

		/* CTM goes here */
		SP_PRINT_TRANSFORM ("ctm", ctm);

		art_affine_multiply (color2pos, color2norm, norm2pos);
		SP_PRINT_TRANSFORM ("color2pos", color2pos);
		art_affine_multiply (color2tpos, color2pos, gr->transform);
		SP_PRINT_TRANSFORM ("color2tpos", color2tpos);
		art_affine_multiply (color2px, color2tpos, ctm);
		SP_PRINT_TRANSFORM ("color2px", color2px);
	}

	v2px.c[0] = color2px[0];
	v2px.c[1] = color2px[1];
	v2px.c[2] = color2px[2];
	v2px.c[3] = color2px[3];
	v2px.c[4] = color2px[4];
	v2px.c[5] = color2px[5];

	nr_lgradient_renderer_setup (&lgp->lgr, gr->color, gr->spread, &v2px,
				     lg->x1.computed, lg->y1.computed, lg->x2.computed, lg->y2.computed);

	return (SPPainter *) lgp;
}

static void
sp_lineargradient_painter_free (SPPaintServer *ps, SPPainter *painter)
{
	SPLGPainter *lgp;

	lgp = (SPLGPainter *) painter;

	g_free (lgp);
}

static void
sp_lineargradient_flatten_attributes (SPGradient *gr, SPRepr *repr, gboolean set_missing)
{
	SPLinearGradient *lg;

	lg = SP_LINEARGRADIENT (gr);

	/* fixme: Write real units etc. (Lauris) */
	if (set_missing || lg->x1.set) sp_repr_set_double_attribute (repr, "x1", lg->x1.computed);
	if (set_missing || lg->y1.set) sp_repr_set_double_attribute (repr, "y1", lg->y1.computed);
	if (set_missing || lg->x2.set) sp_repr_set_double_attribute (repr, "x2", lg->x2.computed);
	if (set_missing || lg->y2.set) sp_repr_set_double_attribute (repr, "y2", lg->y2.computed);
}

void
sp_lineargradient_set_position (SPLinearGradient *lg, gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
	g_return_if_fail (lg != NULL);
	g_return_if_fail (SP_IS_LINEARGRADIENT (lg));

	/* fixme: units? (Lauris)  */
	sp_svg_length_unset (&lg->x1, SP_SVG_UNIT_NONE, x1, x1);
	sp_svg_length_unset (&lg->y1, SP_SVG_UNIT_NONE, y1, y1);
	sp_svg_length_unset (&lg->x2, SP_SVG_UNIT_NONE, x2, x2);
	sp_svg_length_unset (&lg->y2, SP_SVG_UNIT_NONE, y2, y2);

	sp_object_request_modified (SP_OBJECT (lg), SP_OBJECT_MODIFIED_FLAG);
}

/* Builds flattened repr tree of gradient - i.e. no href */

SPRepr *
sp_lineargradient_build_repr (SPLinearGradient *lg, gboolean vector)
{
	SPRepr *repr;

	g_return_val_if_fail (lg != NULL, NULL);
	g_return_val_if_fail (SP_IS_LINEARGRADIENT (lg), NULL);

	repr = sp_repr_new ("linearGradient");

	sp_gradient_repr_flatten_attributes (SP_GRADIENT (lg), repr, TRUE);

	if (vector) {
		sp_gradient_ensure_vector (SP_GRADIENT (lg));
		sp_gradient_repr_set_vector (SP_GRADIENT (lg), repr, SP_GRADIENT (lg)->vector);
	}

	return repr;
}

static void
sp_lg_fill (SPPainter *painter, guchar *px, gint x0, gint y0, gint width, gint height, gint rowstride)
{
	SPLGPainter *lgp;
	NRPixBlock pb;

	lgp = (SPLGPainter *) painter;

	nr_pixblock_setup_extern (&pb, NR_PIXBLOCK_MODE_R8G8B8A8N, x0, y0, x0 + width, y0 + height, px, rowstride, FALSE, FALSE);

	nr_render ((NRRenderer *) &lgp->lgr, &pb, NULL);

	nr_pixblock_release (&pb);
}

/*
 * Radial Gradient
 */

typedef struct _SPRGPainter SPRGPainter;

struct _SPRGPainter {
	SPPainter painter;
	SPRadialGradient *rg;
	NRRGradientRenderer rgr;
};

static void sp_radialgradient_class_init (SPRadialGradientClass *klass);
static void sp_radialgradient_init (SPRadialGradient *rg);

static void sp_radialgradient_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_radialgradient_read_attr (SPObject *object, const gchar *key);
static SPRepr *sp_radialgradient_write (SPObject *object, SPRepr *repr, guint flags);

static SPPainter *sp_radialgradient_painter_new (SPPaintServer *ps, const gdouble *affine, const ArtDRect *bbox);
static void sp_radialgradient_painter_free (SPPaintServer *ps, SPPainter *painter);

static void sp_radialgradient_flatten_attributes (SPGradient *gradient, SPRepr *repr, gboolean set_missing);

static void sp_rg_fill (SPPainter *painter, guchar *px, gint x0, gint y0, gint width, gint height, gint rowstride);

static SPGradientClass *rg_parent_class;

GtkType
sp_radialgradient_get_type (void)
{
	static GtkType radialgradient_type = 0;
	if (!radialgradient_type) {
		GtkTypeInfo radialgradient_info = {
			"SPRadialGradient",
			sizeof (SPRadialGradient),
			sizeof (SPRadialGradientClass),
			(GtkClassInitFunc) sp_radialgradient_class_init,
			(GtkObjectInitFunc) sp_radialgradient_init,
			NULL, NULL, NULL
		};
		radialgradient_type = gtk_type_unique (SP_TYPE_GRADIENT, &radialgradient_info);
	}
	return radialgradient_type;
}

static void
sp_radialgradient_class_init (SPRadialGradientClass * klass)
{
	GtkObjectClass *gtk_object_class;
	SPObjectClass *sp_object_class;
	SPPaintServerClass *ps_class;
	SPGradientClass *gr_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	ps_class = (SPPaintServerClass *) klass;
	gr_class = SP_GRADIENT_CLASS (klass);

	rg_parent_class = gtk_type_class (SP_TYPE_GRADIENT);

	sp_object_class->build = sp_radialgradient_build;
	sp_object_class->read_attr = sp_radialgradient_read_attr;
	sp_object_class->write = sp_radialgradient_write;

	ps_class->painter_new = sp_radialgradient_painter_new;
	ps_class->painter_free = sp_radialgradient_painter_free;

	gr_class->flatten_attributes = sp_radialgradient_flatten_attributes;
}

static void
sp_radialgradient_init (SPRadialGradient *rg)
{
	sp_svg_length_unset (&rg->cx, SP_SVG_UNIT_PERCENT, 0.5, 0.5);
	sp_svg_length_unset (&rg->cy, SP_SVG_UNIT_PERCENT, 0.5, 0.5);
	sp_svg_length_unset (&rg->r, SP_SVG_UNIT_PERCENT, 0.5, 0.5);
	sp_svg_length_unset (&rg->fx, SP_SVG_UNIT_PERCENT, 0.5, 0.5);
	sp_svg_length_unset (&rg->fy, SP_SVG_UNIT_PERCENT, 0.5, 0.5);
}

static void
sp_radialgradient_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	SPRadialGradient *rg;

	rg = SP_RADIALGRADIENT (object);

	if (((SPObjectClass *) rg_parent_class)->build)
		(* ((SPObjectClass *) rg_parent_class)->build) (object, document, repr);

	sp_radialgradient_read_attr (object, "cx");
	sp_radialgradient_read_attr (object, "cy");
	sp_radialgradient_read_attr (object, "r");
	sp_radialgradient_read_attr (object, "fx");
	sp_radialgradient_read_attr (object, "fy");
}

static void
sp_radialgradient_read_attr (SPObject *object, const gchar *key)
{
	SPRadialGradient *rg;
	const guchar *str;

	rg = SP_RADIALGRADIENT (object);

	str = sp_repr_attr (object->repr, key);

	if (!strcmp (key, "cx")) {
		if (!sp_svg_length_read (str, &rg->cx)) {
			sp_svg_length_unset (&rg->cx, SP_SVG_UNIT_PERCENT, 0.5, 0.5);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "cy")) {
		if (!sp_svg_length_read (str, &rg->cy)) {
			sp_svg_length_unset (&rg->cy, SP_SVG_UNIT_PERCENT, 0.5, 0.5);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "r")) {
		if (!sp_svg_length_read (str, &rg->r)) {
			sp_svg_length_unset (&rg->r, SP_SVG_UNIT_PERCENT, 0.5, 0.5);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "fx")) {
		if (!sp_svg_length_read (str, &rg->fx)) {
			sp_svg_length_unset (&rg->fx, SP_SVG_UNIT_PERCENT, 0.5, 0.5);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "fy")) {
		if (!sp_svg_length_read (str, &rg->fy)) {
			sp_svg_length_unset (&rg->fy, SP_SVG_UNIT_PERCENT, 0.5, 0.5);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else {
		if (((SPObjectClass *) rg_parent_class)->read_attr)
			(* ((SPObjectClass *) rg_parent_class)->read_attr) (object, key);
	}
}

static SPRepr *
sp_radialgradient_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPRadialGradient *rg;

	rg = SP_RADIALGRADIENT (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("radialGradient");
	}

	sp_repr_set_double (repr, "cx", rg->cx.computed);
	sp_repr_set_double (repr, "cy", rg->cy.computed);
	sp_repr_set_double (repr, "fx", rg->fx.computed);
	sp_repr_set_double (repr, "fy", rg->fy.computed);
	sp_repr_set_double (repr, "r", rg->r.computed);

	if (((SPObjectClass *) rg_parent_class)->write)
		(* ((SPObjectClass *) rg_parent_class)->write) (object, repr, flags);

	return repr;
}

static SPPainter *
sp_radialgradient_painter_new (SPPaintServer *ps, const gdouble *ctm, const ArtDRect *bbox)
{
	SPRadialGradient *rg;
	SPGradient *gr;
	SPRGPainter *rgp;
	NRMatrixF gs2px;

	rg = SP_RADIALGRADIENT (ps);
	gr = SP_GRADIENT (ps);

	if (!gr->color) sp_gradient_ensure_colors (gr);

	rgp = g_new (SPRGPainter, 1);

	rgp->painter.type = SP_PAINTER_IND;
	rgp->painter.fill = sp_rg_fill;

	rgp->rg = rg;

	/* fixme: We may try to normalize here too, look at linearGradient (Lauris) */

	if (gr->units == SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX) {
		NRMatrixF bbox2user;
		NRMatrixF gs2user;

		/* fixme: We may try to normalize here too, look at linearGradient (Lauris) */

		/* gradientTransform goes here (Lauris) */

		/* BBox to user coordinate system */
		bbox2user.c[0] = bbox->x1 - bbox->x0;
		bbox2user.c[1] = 0.0;
		bbox2user.c[2] = 0.0;
		bbox2user.c[3] = bbox->y1 - bbox->y0;
		bbox2user.c[4] = bbox->x0;
		bbox2user.c[5] = bbox->y0;

		/* fixme: (Lauris) */
		nr_matrix_multiply_fdf (&gs2user, (NRMatrixD *) gr->transform, &bbox2user);
		nr_matrix_multiply_ffd (&gs2px, &gs2user, (NRMatrixD *) ctm);
	} else {
		/* Problem: What to do, if we have mixed lengths and percentages? */
		/* Currently we do ignore percentages at all, but that is not good (lauris) */

		/* fixme: We may try to normalize here too, look at linearGradient (Lauris) */

		/* fixme: (Lauris) */
		nr_matrix_multiply_fdd (&gs2px, (NRMatrixD *) gr->transform, (NRMatrixD *) ctm);
	}

	nr_rgradient_renderer_setup (&rgp->rgr, gr->color, gr->spread,
				     &gs2px,
				     rg->cx.computed, rg->cy.computed,
				     rg->fx.computed, rg->fy.computed,
				     rg->r.computed);

	return (SPPainter *) rgp;
}

static void
sp_radialgradient_painter_free (SPPaintServer *ps, SPPainter *painter)
{
	SPRGPainter *rgp;

	rgp = (SPRGPainter *) painter;

	g_free (rgp);
}

static void
sp_radialgradient_flatten_attributes (SPGradient *gr, SPRepr *repr, gboolean set_missing)
{
	SPRadialGradient *rg;

	rg = SP_RADIALGRADIENT (gr);

	if (set_missing || rg->cx.set) sp_repr_set_double_attribute (repr, "cx", rg->cx.computed);
	if (set_missing || rg->cy.set) sp_repr_set_double_attribute (repr, "cy", rg->cy.computed);
	if (set_missing || rg->r.set) sp_repr_set_double_attribute (repr, "r", rg->r.computed);
	if (set_missing || rg->fx.set) sp_repr_set_double_attribute (repr, "fx", rg->fx.computed);
	if (set_missing || rg->fy.set) sp_repr_set_double_attribute (repr, "fy", rg->fy.computed);
}

void
sp_radialgradient_set_position (SPRadialGradient *rg, gdouble cx, gdouble cy, gdouble fx, gdouble fy, gdouble r)
{
	g_return_if_fail (rg != NULL);
	g_return_if_fail (SP_IS_RADIALGRADIENT (rg));

	/* fixme: units? (Lauris)  */
	sp_svg_length_unset (&rg->cx, SP_SVG_UNIT_PERCENT, cx, cx);
	sp_svg_length_unset (&rg->cy, SP_SVG_UNIT_PERCENT, cy, cy);
	sp_svg_length_unset (&rg->fx, SP_SVG_UNIT_PERCENT, fx, fx);
	sp_svg_length_unset (&rg->fy, SP_SVG_UNIT_PERCENT, fy, fy);
	sp_svg_length_unset (&rg->r, SP_SVG_UNIT_PERCENT, r, r);

	sp_object_request_modified (SP_OBJECT (rg), SP_OBJECT_MODIFIED_FLAG);
}


/* Builds flattened repr tree of gradient - i.e. no href */

SPRepr *
sp_radialgradient_build_repr (SPRadialGradient *rg, gboolean vector)
{
	SPRepr *repr;

	g_return_val_if_fail (rg != NULL, NULL);
	g_return_val_if_fail (SP_IS_RADIALGRADIENT (rg), NULL);

	repr = sp_repr_new ("radialGradient");

	sp_gradient_repr_flatten_attributes (SP_GRADIENT (rg), repr, TRUE);

	if (vector) {
		sp_gradient_ensure_vector (SP_GRADIENT (rg));
		sp_gradient_repr_set_vector (SP_GRADIENT (rg), repr, SP_GRADIENT (rg)->vector);
	}

	return repr;
}

static void
sp_rg_fill (SPPainter *painter, guchar *px, gint x0, gint y0, gint width, gint height, gint rowstride)
{
	SPRGPainter *rgp;
	NRPixBlock pb;

	rgp = (SPRGPainter *) painter;

	nr_pixblock_setup_extern (&pb, NR_PIXBLOCK_MODE_R8G8B8A8N, x0, y0, x0 + width, y0 + height, px, rowstride, FALSE, FALSE);

	nr_render ((NRRenderer *) &rgp->rgr, &pb, NULL);

	nr_pixblock_release (&pb);
}


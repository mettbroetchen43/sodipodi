#define __SP_GRADIENT_C__

/*
 * SPGradient
 *
 * TODO: Implement radial & other fancy gradients
 * TODO: Implement linking attributes
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <libart_lgpl/art_affine.h>
#include <gtk/gtksignal.h>
#include "svg/svg.h"
#include "xml/repr-private.h"
#include "document-private.h"
#include "sp-object-repr.h"
#include "sp-gradient.h"

#define NCOLORS 1024

static void sp_stop_class_init (SPStopClass * klass);
static void sp_stop_init (SPStop * stop);

static void sp_stop_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_stop_read_attr (SPObject * object, const gchar * key);

static SPObjectClass * stop_parent_class;

GtkType
sp_stop_get_type (void)
{
	static GtkType stop_type = 0;
	if (!stop_type) {
		GtkTypeInfo stop_info = {
			"SPStop",
			sizeof (SPStop),
			sizeof (SPStopClass),
			(GtkClassInitFunc) sp_stop_class_init,
			(GtkObjectInitFunc) sp_stop_init,
			NULL, NULL, NULL
		};
		stop_type = gtk_type_unique (sp_object_get_type (), &stop_info);
	}
	return stop_type;
}

static void
sp_stop_class_init (SPStopClass * klass)
{
	SPObjectClass * sp_object_class;

	sp_object_class = (SPObjectClass *) klass;

	stop_parent_class = gtk_type_class (sp_object_get_type ());

	sp_object_class->build = sp_stop_build;
	sp_object_class->read_attr = sp_stop_read_attr;
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
	if (SP_OBJECT_CLASS (stop_parent_class)->build)
		(* SP_OBJECT_CLASS (stop_parent_class)->build) (object, document, repr);

	sp_stop_read_attr (object, "offset");
	sp_stop_read_attr (object, "style");
}

static void
sp_stop_read_attr (SPObject * object, const gchar * key)
{
	SPStop * stop;
	guint32 color;
	gdouble opacity;

	stop = SP_STOP (object);

	if (strcmp (key, "style") == 0) {
		const guchar *p;
		p = sp_object_get_style_property (object, "stop-color", "black");
		color = sp_svg_read_color (p, 0x00000000);
		sp_color_set_rgb_rgba32 (&stop->color, color);
		p = sp_object_get_style_property (object, "stop-opacity", "1");
		opacity = sp_svg_read_percentage (p, 1.0);
		stop->opacity = opacity;
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	}
	if (strcmp (key, "offset") == 0) {
		SPUnit unit;
		const guchar *val;
		val = sp_repr_attr (object->repr, key);
		stop->offset = sp_svg_read_length (&unit, val, 0.0);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	}

	if (SP_OBJECT_CLASS (stop_parent_class)->read_attr)
		(* SP_OBJECT_CLASS (stop_parent_class)->read_attr) (object, key);
}

/*
 * Gradient
 */

static void sp_gradient_class_init (SPGradientClass *klass);
static void sp_gradient_init (SPGradient *gr);
static void sp_gradient_destroy (GtkObject *object);

static void sp_gradient_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_gradient_read_attr (SPObject *object, const gchar *key);
static void sp_gradient_child_added (SPObject *object, SPRepr *child, SPRepr *ref);
static void sp_gradient_remove_child (SPObject *object, SPRepr *child);
static void sp_gradient_modified (SPObject *object, guint flags);

static void sp_gradient_href_destroy (SPObject *href, SPGradient *gradient);
static void sp_gradient_href_modified (SPObject *href, guint flags, SPGradient *gradient);

static void sp_gradient_invalidate_vector (SPGradient *gr);
static void sp_gradient_rebuild_vector (SPGradient *gr);
static void sp_gradient_write_colors (SPGradient *gradient);

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

	gtk_object_class->destroy = sp_gradient_destroy;

	sp_object_class->build = sp_gradient_build;
	sp_object_class->read_attr = sp_gradient_read_attr;
	sp_object_class->child_added = sp_gradient_child_added;
	sp_object_class->remove_child = sp_gradient_remove_child;
	sp_object_class->modified = sp_gradient_modified;
}

static void
sp_gradient_init (SPGradient *gr)
{
	/* fixme: There is one problem - if reprs are rearranged, state has to be cleared somehow */
	/* fixme: Maybe that is not problem at all, as no force can rearrange childrens of <defs> */
	/* fixme: But keep that in mind, if messing with XML tree (Lauris) */
	gr->state = SP_GRADIENT_STATE_UNKNOWN;

	gr->spread = SP_GRADIENT_SPREAD_PAD;
	gr->spread_set = FALSE;

	gr->stops = NULL;
	gr->has_stops = FALSE;

	gr->vector = NULL;
	gr->color = NULL;

	gr->len = 0.0;
}

static void
sp_gradient_destroy (GtkObject *object)
{
	SPGradient * gradient;

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
		SPObject *o;
		o = gradient->stops;
		gradient->stops = o->next;
		o->parent = NULL;
		o->next = NULL;
		gtk_object_unref (GTK_OBJECT (o));
	}

	if (GTK_OBJECT_CLASS (gradient_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (gradient_parent_class)->destroy) (object);
}

static void
sp_gradient_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	SPGradient *gradient;
	SPObject *last;
	SPRepr *rchild;

	gradient = SP_GRADIENT (object);

	if (SP_OBJECT_CLASS (gradient_parent_class)->build)
		(* SP_OBJECT_CLASS (gradient_parent_class)->build) (object, document, repr);

	/* fixme: Add all children, not only stops? */
	last = NULL;
	for (rchild = repr->children; rchild != NULL; rchild = rchild->next) {
		GtkType type;
		SPObject * child;
		type = sp_repr_type_lookup (rchild);
		if (gtk_type_is_a (type, SP_TYPE_STOP)) {
			child = gtk_type_new (type);
			child->parent = object;
			if (last) {
				last->next = child;
			} else {
				gradient->stops = child;
			}
			sp_object_invoke_build (child, document, rchild, SP_OBJECT_IS_CLONED (object));
			last = child;
		}
	}

	sp_gradient_read_attr (object, "xlink:href");
	sp_gradient_read_attr (object, "spreadMethod");

	/* Register ourselves */
	sp_document_add_resource (document, "gradient", object);
}

static void
sp_gradient_read_attr (SPObject *object, const gchar *key)
{
	SPGradient *gradient;
	const gchar *val;

	gradient = SP_GRADIENT (object);

	val = sp_repr_attr (object->repr, key);

	/* fixme: We should unset properties, if val == NULL */
	if (!strcmp (key, "xlink:href")) {
		if (gradient->href) {
			gtk_signal_disconnect_by_data (GTK_OBJECT (gradient->href), gradient);
			gradient->href = (SPGradient *) sp_object_hunref (SP_OBJECT (gradient->href), object);
		}
		if (val && *val == '#') {
			SPObject *href;
			href = sp_document_lookup_id (object->document, val + 1);
			if (SP_IS_GRADIENT (href)) {
				gradient->href = (SPGradient *) sp_object_href (href, object);
				gtk_signal_connect (GTK_OBJECT (href), "destroy",
						    GTK_SIGNAL_FUNC (sp_gradient_href_destroy), gradient);
				gtk_signal_connect (GTK_OBJECT (href), "modified",
						    GTK_SIGNAL_FUNC (sp_gradient_href_modified), gradient);
			}
		}
		sp_gradient_invalidate_vector (gradient);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	} else if (!strcmp (key, "spreadMethod")) {
		if (val) {
			if (!strcmp (val, "reflect")) {
				gradient->spread = SP_GRADIENT_SPREAD_REFLECT;
			} else if (!strcmp (val, "repeat")) {
				gradient->spread = SP_GRADIENT_SPREAD_REPEAT;
			} else {
				gradient->spread = SP_GRADIENT_SPREAD_PAD;
			}
			gradient->spread_set = TRUE;
		} else {
			gradient->spread_set = FALSE;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	}

	if (SP_OBJECT_CLASS (gradient_parent_class)->read_attr)
		(* SP_OBJECT_CLASS (gradient_parent_class)->read_attr) (object, key);
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
	flags &= SP_OBJECT_PARENT_MODIFIED_FLAG;

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

static void
sp_gradient_write_colors (SPGradient *gr)
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
		sp_gradient_write_colors (gradient);
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
		sp_gradient_write_colors (gradient);
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
				fc = (t[0] - *buf) * a;
				buf[0] = *buf + ((fc + (fc >> 8) + 0x80) >> 8);
				fc = (t[1] - *buf) * a;
				buf[1] = *buf + ((fc + (fc >> 8) + 0x80) >> 8);
				fc = (t[2] - *buf) * a;
				buf[2] = *buf + ((fc + (fc >> 8) + 0x80) >> 8);
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
				fc = (t[0] - *buf) * a;
				buf[0] = *buf + ((fc + (fc >> 8) + 0x80) >> 8);
				fc = (t[1] - *buf) * a;
				buf[1] = *buf + ((fc + (fc >> 8) + 0x80) >> 8);
				fc = (t[2] - *buf) * a;
				buf[2] = *buf + ((fc + (fc >> 8) + 0x80) >> 8);
			}
		}
	}
}

/*
 * Linear Gradient
 */

typedef struct _SPLGPainter SPLGPainter;

struct _SPLGPainter {
	SPPainter painter;
	SPLinearGradient *lg;
	gdouble len;
	gdouble x0, y0;
	gdouble dx, dy;
	guint32 opacity;
};

static void sp_lineargradient_class_init (SPLinearGradientClass * klass);
static void sp_lineargradient_init (SPLinearGradient * lg);
static void sp_lineargradient_destroy (GtkObject * object);

static void sp_lineargradient_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_lineargradient_read_attr (SPObject * object, const gchar * key);

static SPPainter *sp_lineargradient_painter_new (SPPaintServer *ps, gdouble *affine, gdouble opacity, ArtDRect *bbox);
static void sp_lineargradient_painter_free (SPPaintServer *ps, SPPainter *painter);

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

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	ps_class = (SPPaintServerClass *) klass;

	lg_parent_class = gtk_type_class (SP_TYPE_GRADIENT);

	gtk_object_class->destroy = sp_lineargradient_destroy;

	sp_object_class->build = sp_lineargradient_build;
	sp_object_class->read_attr = sp_lineargradient_read_attr;

	ps_class->painter_new = sp_lineargradient_painter_new;
	ps_class->painter_free = sp_lineargradient_painter_free;
}

static void
sp_lineargradient_init (SPLinearGradient * lg)
{
	lg->units = SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX;
	art_affine_identity (lg->transform);
	lg->x1.distance = 0.0;
	lg->x1.unit = SP_UNIT_PERCENT;
	lg->y1.distance = 0.0;
	lg->y1.unit = SP_UNIT_PERCENT;
	lg->x2.distance = 1.0;
	lg->x2.unit = SP_UNIT_PERCENT;
	lg->y2.distance = 0.0;
	lg->y2.unit = SP_UNIT_PERCENT;
}

static void
sp_lineargradient_destroy (GtkObject * object)
{
	SPLinearGradient * lg;

	lg = (SPLinearGradient *) object;

	if (GTK_OBJECT_CLASS (lg_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (lg_parent_class)->destroy) (object);
}

static void
sp_lineargradient_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPLinearGradient * lg;

	lg = SP_LINEARGRADIENT (object);

	if (SP_OBJECT_CLASS (lg_parent_class)->build)
		(* SP_OBJECT_CLASS (lg_parent_class)->build) (object, document, repr);

	sp_lineargradient_read_attr (object, "gradientUnits");
	sp_lineargradient_read_attr (object, "gradientTransform");
	sp_lineargradient_read_attr (object, "x1");
	sp_lineargradient_read_attr (object, "y1");
	sp_lineargradient_read_attr (object, "x2");
	sp_lineargradient_read_attr (object, "y2");
}

static void
sp_lineargradient_read_attr (SPObject * object, const gchar * key)
{
	SPLinearGradient * lg;
	const gchar *val;

	lg = SP_LINEARGRADIENT (object);

	val = sp_repr_attr (object->repr, key);

	/* fixme: We should unset properties, if val == NULL */
	if (!strcmp (key, "gradientUnits")) {
		if (val) {
			if (!strcmp (val, "userSpaceOnUse")) {
				lg->units = SP_GRADIENT_UNITS_USERSPACEONUSE;
			} else {
				lg->units = SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX;
			}
			lg->units_set = TRUE;
		} else {
			lg->units_set = FALSE;
		}
		return;
	} else if (!strcmp (key, "gradientTransform")) {
		if (val) {
			art_affine_identity (lg->transform);
			sp_svg_read_affine (lg->transform, val);
			lg->transform_set = TRUE;
		} else {
			lg->transform_set = FALSE;
		}
		return;
	} else if (!strcmp (key, "x1")) {
		lg->x1.distance = sp_svg_read_length (&lg->x1.unit, val, 0.0);
		lg->x1_set = (val != NULL);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	} else if (!strcmp (key, "y1")) {
		lg->y1.distance = sp_svg_read_length (&lg->y1.unit, val, 0.0);
		lg->y1_set = (val != NULL);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	} else if (!strcmp (key, "x2")) {
		lg->x2.distance = sp_svg_read_length (&lg->x2.unit, val, 1.0);
		lg->x2_set = (val != NULL);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	} else if (!strcmp (key, "y2")) {
		lg->y2.distance = sp_svg_read_length (&lg->y2.unit, val, 0.0);
		lg->y2_set = (val != NULL);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	}

	if (SP_OBJECT_CLASS (lg_parent_class)->read_attr)
		(* SP_OBJECT_CLASS (lg_parent_class)->read_attr) (object, key);
}

static SPPainter *
sp_lineargradient_painter_new (SPPaintServer *ps, gdouble *affine, gdouble opacity, ArtDRect *bbox)
{
	SPLinearGradient *lg;
	SPGradient *gr;
	SPLGPainter *lgp;

	lg = SP_LINEARGRADIENT (ps);
	gr = SP_GRADIENT (ps);

	if (!gr->color) {
		sp_gradient_write_colors (gr);
	}

	lgp = g_new (SPLGPainter, 1);

	lgp->painter.type = SP_PAINTER_IND;
	lgp->painter.fill = sp_lg_fill;

	lgp->lg = lg;

	lgp->opacity = (guint32) floor (opacity * 255.9999);

	if (lg->units == SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX) {
		gdouble b2c[6], vec2b[6], norm2vec[6], norm2c[6], c2norm[6];
		/* fixme: we should use start & end here */
		norm2vec[0] = gr->len / 1023.9999;
		norm2vec[1] = 0.0;
		norm2vec[2] = 0.0;
		norm2vec[3] = gr->len / 1023.9999;
		norm2vec[4] = 0.0;
		norm2vec[5] = 0.0;
#if 0
		g_print ("\nnorm2vec: ");for (i = 0; i < 6; i++) g_print ("%g ", norm2vec[i]);g_print ("\n");
#endif
		/* fixme: gradient transform somewhere here */
		vec2b[0] = lg->x2.distance - lg->x1.distance;
		vec2b[1] = lg->y2.distance - lg->y1.distance;
		vec2b[2] = lg->y2.distance - lg->y1.distance;
		vec2b[3] = lg->x1.distance - lg->x2.distance;
		vec2b[4] = lg->x1.distance;
		vec2b[5] = lg->y1.distance;
#if 0
		g_print ("vec2b: ");for (i = 0; i < 6; i++) g_print ("%g ", vec2b[i]);g_print ("\n");
#endif
		b2c[0] = bbox->x1 - bbox->x0;
		b2c[1] = 0.0;
		b2c[2] = 0.0;
		b2c[3] = bbox->y1 - bbox->y0;
		b2c[4] = bbox->x0;
		b2c[5] = bbox->y0;
#if 0
		g_print ("b2c: ");for (i = 0; i < 6; i++) g_print ("%g ", b2c[i]);g_print ("\n");
#endif
		art_affine_multiply (norm2c, norm2vec, vec2b);
		art_affine_multiply (norm2c, norm2c, b2c);
#if 0
		g_print ("norm2c: ");for (i = 0; i < 6; i++) g_print ("%g ", norm2c[i]);g_print ("\n");
#endif
		art_affine_invert (c2norm, norm2c);
#if 0
		g_print ("c2norm: ");for (i = 0; i < 6; i++) g_print ("%g ", c2norm[i]);g_print ("\n");
#endif
		lgp->x0 = norm2c[4];
		lgp->y0 = norm2c[5];
		lgp->dx = c2norm[0];
		lgp->dy = c2norm[2];
	} else {
		/* This is placeholder */
		lgp->x0 = affine[4];
		lgp->y0 = affine[5];
		lgp->dx = 127.0 / (bbox->x1 - bbox->x0);
		lgp->dy = 127.0 / (bbox->y1 - bbox->y0);
	}

	return (SPPainter *) lgp;
}

static void
sp_lineargradient_painter_free (SPPaintServer *ps, SPPainter *painter)
{
	SPLGPainter *lgp;

	lgp = (SPLGPainter *) painter;

	g_free (lgp);
}

void
sp_lineargradient_set_position (SPLinearGradient *lg, gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
	g_return_if_fail (lg != NULL);
	g_return_if_fail (SP_IS_LINEARGRADIENT (lg));

	/* fixme: units */
	lg->x1.distance = x1;
	lg->y1.distance = y1;
	lg->x2.distance = x2;
	lg->y2.distance = y2;

	sp_object_request_modified (SP_OBJECT (lg), SP_OBJECT_MODIFIED_FLAG);
}

/* Builds flattened repr tree of gradient - i.e. no href */

SPRepr *
sp_lineargradient_build_repr (SPLinearGradient *lg, gboolean vector)
{
	SPGradient *gr;
	SPRepr *repr;
	gchar c[256];
	gint i;

	gr = SP_GRADIENT (lg);

	repr = sp_repr_new ("linearGradient");
	sp_repr_set_attr (repr, "spreadMethod",
			  (gr->spread == SP_GRADIENT_SPREAD_PAD) ? "pad" :
			  (gr->spread == SP_GRADIENT_SPREAD_REFLECT) ? "reflect" : "repeat");
	sp_repr_set_attr (repr, "gradientUnits",
			  (lg->units == SP_GRADIENT_UNITS_USERSPACEONUSE) ? "userSpaceOnUse" : "objectBoundingBox");

	sp_svg_write_affine (c, 256, lg->transform);
	sp_repr_set_attr (repr, "gradientTransform", c);
	sp_repr_set_double_attribute (repr, "x1", lg->x1.distance);
	sp_repr_set_double_attribute (repr, "y1", lg->y1.distance);
	sp_repr_set_double_attribute (repr, "x2", lg->x2.distance);
	sp_repr_set_double_attribute (repr, "y2", lg->y2.distance);

	if (vector) {
		sp_gradient_ensure_vector (gr);
		for (i = gr->vector->nstops - 1; i >= 0; i--) {
			SPRepr *child;
			gchar *p;
			child = sp_repr_new ("stop");
			sp_repr_set_double_attribute (child, "offset",
						      gr->vector->stops[i].offset * (gr->vector->end - gr->vector->start) + gr->vector->start);
			sp_svg_write_color (c, 256, sp_color_get_rgba32_ualpha (&gr->vector->stops[i].color, 0x00));
			p = g_strdup_printf ("stop-color:%s;stop-opacity:%g;", c, gr->vector->stops[i].opacity);
			sp_repr_set_attr (child, "style", p);
			g_free (p);
			sp_repr_add_child (repr, child, NULL);
			sp_repr_unref (child);
		}
	}

	return repr;
}

static void
sp_lg_fill (SPPainter *painter, guchar *px, gint x0, gint y0, gint width, gint height, gint rowstride)
{
	SPLGPainter *lgp;
	SPLinearGradient *lg;
	SPGradient *g;
	gdouble pos;
	gint x, y;
	guchar *p;
	guint tmp;

	lgp = (SPLGPainter *) painter;
	lg = lgp->lg;
	g = (SPGradient *) lg;

	if (!g->color) {
		/* fixme: This is forbidden, so we should paint some mishmesh instead */
		sp_gradient_write_colors (g);
	}

	for (y = 0; y < height; y++) {
		p = px + y * rowstride;
		pos = (y + y0 - lgp->y0) * lgp->dy + (0 + x0 - lgp->x0) * lgp->dx;
		for (x = 0; x < width; x++) {
			gint ip, idx;
			ip = (gint) pos;
			switch (g->spread) {
			case SP_GRADIENT_SPREAD_PAD:
				idx = CLAMP (ip, 0, 1023);
				break;
			case SP_GRADIENT_SPREAD_REFLECT:
				idx = ip & 0x3ff;
				if (ip & 0x400) idx = 1023 - idx;
				break;
			case SP_GRADIENT_SPREAD_REPEAT:
				idx = ip & 0x3ff;
				break;
			default:
				g_assert_not_reached ();
				idx = 0;
				break;
			}
			/* a * b / 255 = a * b * (256 * 256 / 255) / 256 / 256 */
			tmp = g->color[4 * idx + 3] * lgp->opacity;
			tmp = (tmp << 16) + tmp + (tmp >> 8);
			*p++ = g->color[4 * idx];
			*p++ = g->color[4 * idx + 1];
			*p++ = g->color[4 * idx + 2];
			*p++ = tmp >> 24;
			pos += lgp->dx;
		}
	}
}

/*
 * Radial Gradient
 */

typedef struct _SPRGPainter SPRGPainter;

struct _SPRGPainter {
	SPPainter painter;
	SPRadialGradient *rg;
	gdouble len;
	gdouble x0, y0;
	gdouble dx, dy;
	guint32 opacity;
};

static void sp_radialgradient_class_init (SPRadialGradientClass *klass);
static void sp_radialgradient_init (SPRadialGradient *rg);
static void sp_radialgradient_destroy (GtkObject *object);

static void sp_radialgradient_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_radialgradient_read_attr (SPObject *object, const gchar *key);

static SPPainter *sp_radialgradient_painter_new (SPPaintServer *ps, gdouble *affine, gdouble opacity, ArtDRect *bbox);
static void sp_radialgradient_painter_free (SPPaintServer *ps, SPPainter *painter);

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

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	ps_class = (SPPaintServerClass *) klass;

	rg_parent_class = gtk_type_class (SP_TYPE_GRADIENT);

	gtk_object_class->destroy = sp_radialgradient_destroy;

	sp_object_class->build = sp_radialgradient_build;
	sp_object_class->read_attr = sp_radialgradient_read_attr;

	ps_class->painter_new = sp_radialgradient_painter_new;
	ps_class->painter_free = sp_radialgradient_painter_free;
}

static void
sp_radialgradient_init (SPRadialGradient *rg)
{
	rg->units = SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX;
	art_affine_identity (rg->transform);
	rg->cx.distance = 0.5;
	rg->cx.unit = SP_UNIT_PERCENT;
	rg->cy.distance = 0.5;
	rg->cy.unit = SP_UNIT_PERCENT;
	rg->r.distance = 0.5;
	rg->r.unit = SP_UNIT_PERCENT;
	rg->fx.distance = 0.5;
	rg->fx.unit = SP_UNIT_PERCENT;
	rg->fy.distance = 0.5;
	rg->fy.unit = SP_UNIT_PERCENT;
}

static void
sp_radialgradient_destroy (GtkObject *object)
{
	SPRadialGradient * rg;

	rg = (SPRadialGradient *) object;

	if (GTK_OBJECT_CLASS (rg_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (rg_parent_class)->destroy) (object);
}

static void
sp_radialgradient_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	SPRadialGradient *rg;

	rg = SP_RADIALGRADIENT (object);

	if (SP_OBJECT_CLASS (rg_parent_class)->build)
		(* SP_OBJECT_CLASS (rg_parent_class)->build) (object, document, repr);

	sp_radialgradient_read_attr (object, "gradientUnits");
	sp_radialgradient_read_attr (object, "gradientTransform");
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
	const gchar *val;

	rg = SP_RADIALGRADIENT (object);

	val = sp_repr_attr (object->repr, key);

	/* fixme: We should unset properties, if val == NULL */
	if (!strcmp (key, "gradientUnits")) {
		if (val) {
			if (!strcmp (val, "userSpaceOnUse")) {
				rg->units = SP_GRADIENT_UNITS_USERSPACEONUSE;
			} else {
				rg->units = SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX;
			}
			rg->units_set = TRUE;
		} else {
			rg->units_set = FALSE;
		}
		return;
	} else if (!strcmp (key, "gradientTransform")) {
		if (val) {
			art_affine_identity (rg->transform);
			sp_svg_read_affine (rg->transform, val);
			rg->transform_set = TRUE;
		} else {
			rg->transform_set = FALSE;
		}
		return;
	} else if (!strcmp (key, "cx")) {
		rg->cx.distance = sp_svg_read_length (&rg->cx.unit, val, 0.0);
		rg->cx_set = (val != NULL);
		return;
	} else if (!strcmp (key, "cy")) {
		rg->cy.distance = sp_svg_read_length (&rg->cy.unit, val, 0.0);
		rg->cy_set = (val != NULL);
		return;
	} else if (!strcmp (key, "r")) {
		rg->r.distance = sp_svg_read_length (&rg->r.unit, val, 0.0);
		rg->r_set = (val != NULL);
		return;
	} else if (!strcmp (key, "fx")) {
		rg->fx.distance = sp_svg_read_length (&rg->fx.unit, val, 1.0);
		rg->fx_set = (val != NULL);
		return;
	} else if (!strcmp (key, "fy")) {
		rg->fy.distance = sp_svg_read_length (&rg->fy.unit, val, 0.0);
		rg->fy_set = (val != NULL);
		return;
	}

	if (SP_OBJECT_CLASS (rg_parent_class)->read_attr)
		(* SP_OBJECT_CLASS (rg_parent_class)->read_attr) (object, key);
}

static SPPainter *
sp_radialgradient_painter_new (SPPaintServer *ps, gdouble *affine, gdouble opacity, ArtDRect *bbox)
{
	SPRadialGradient *rg;
	SPGradient *gr;
	SPRGPainter *rgp;

	rg = SP_RADIALGRADIENT (ps);
	gr = SP_GRADIENT (ps);

	if (!gr->color) {
		sp_gradient_write_colors (gr);
	}

	rgp = g_new (SPRGPainter, 1);

	rgp->painter.type = SP_PAINTER_IND;
	rgp->painter.fill = sp_rg_fill;

	rgp->rg = rg;

	rgp->opacity = (guint) floor (opacity * 255.9999);

#if 0
	if (rg->units == SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX) {
		gdouble b2c[6], vec2b[6], norm2vec[6], norm2c[6], c2norm[6];
		/* fixme: we should use start & end here */
		norm2vec[0] = gr->len / 1023.9999;
		norm2vec[1] = 0.0;
		norm2vec[2] = 0.0;
		norm2vec[3] = gr->len / 1023.9999;
		norm2vec[4] = 0.0;
		norm2vec[5] = 0.0;
#if 0
		g_print ("\nnorm2vec: ");for (i = 0; i < 6; i++) g_print ("%g ", norm2vec[i]);g_print ("\n");
#endif
		/* fixme: gradient transform somewhere here */
		vec2b[0] = lg->x2.distance - lg->x1.distance;
		vec2b[1] = lg->y2.distance - lg->y1.distance;
		vec2b[2] = lg->y2.distance - lg->y1.distance;
		vec2b[3] = lg->x1.distance - lg->x2.distance;
		vec2b[4] = lg->x1.distance;
		vec2b[5] = lg->y1.distance;
#if 0
		g_print ("vec2b: ");for (i = 0; i < 6; i++) g_print ("%g ", vec2b[i]);g_print ("\n");
#endif
		b2c[0] = bbox->x1 - bbox->x0;
		b2c[1] = 0.0;
		b2c[2] = 0.0;
		b2c[3] = bbox->y1 - bbox->y0;
		b2c[4] = bbox->x0;
		b2c[5] = bbox->y0;
#if 0
		g_print ("b2c: ");for (i = 0; i < 6; i++) g_print ("%g ", b2c[i]);g_print ("\n");
#endif
		art_affine_multiply (norm2c, norm2vec, vec2b);
		art_affine_multiply (norm2c, norm2c, b2c);
#if 0
		g_print ("norm2c: ");for (i = 0; i < 6; i++) g_print ("%g ", norm2c[i]);g_print ("\n");
#endif
		art_affine_invert (c2norm, norm2c);
#if 0
		g_print ("c2norm: ");for (i = 0; i < 6; i++) g_print ("%g ", c2norm[i]);g_print ("\n");
#endif
		lgp->x0 = norm2c[4];
		lgp->y0 = norm2c[5];
		lgp->dx = c2norm[0];
		lgp->dy = c2norm[2];
	} else {
		lgp->x0 = affine[4];
		lgp->y0 = affine[5];
		lgp->dx = 127.0 / (bbox->x1 - bbox->x0);
		lgp->dy = 127.0 / (bbox->y1 - bbox->y0);
	}
#endif

	return (SPPainter *) rgp;
}

static void
sp_radialgradient_painter_free (SPPaintServer *ps, SPPainter *painter)
{
	SPRGPainter *rgp;

	rgp = (SPRGPainter *) painter;

	g_free (rgp);
}

/* Builds flattened repr tree of gradient - i.e. no href */

SPRepr *
sp_radialgradient_build_repr (SPRadialGradient *rg, gboolean vector)
{
	SPGradient *gr;
	SPRepr *repr;
	gchar c[256];
	gint i;

	gr = SP_GRADIENT (rg);

	repr = sp_repr_new ("radialGradient");
	sp_repr_set_attr (repr, "spreadMethod",
			  (gr->spread == SP_GRADIENT_SPREAD_PAD) ? "pad" :
			  (gr->spread == SP_GRADIENT_SPREAD_REFLECT) ? "reflect" : "repeat");
	sp_repr_set_attr (repr, "gradientUnits",
			  (rg->units == SP_GRADIENT_UNITS_USERSPACEONUSE) ? "userSpaceOnUse" : "objectBoundingBox");

	sp_svg_write_affine (c, 256, rg->transform);
	sp_repr_set_attr (repr, "gradientTransform", c);
	sp_repr_set_double_attribute (repr, "cx", rg->cx.distance);
	sp_repr_set_double_attribute (repr, "cy", rg->cy.distance);
	sp_repr_set_double_attribute (repr, "r", rg->r.distance);
	sp_repr_set_double_attribute (repr, "fx", rg->fx.distance);
	sp_repr_set_double_attribute (repr, "fy", rg->fy.distance);

	if (vector) {
		sp_gradient_ensure_vector (gr);
		for (i = gr->vector->nstops - 1; i >= 0; i--) {
			SPRepr *child;
			gchar *p;
			child = sp_repr_new ("stop");
			sp_repr_set_double_attribute (child, "offset",
						      gr->vector->stops[i].offset * (gr->vector->end - gr->vector->start) + gr->vector->start);
			sp_svg_write_color (c, 256, sp_color_get_rgba32_ualpha (&gr->vector->stops[i].color, 0x00));
			p = g_strdup_printf ("stop-color:%s;stop-opacity:%g;", c, gr->vector->stops[i].opacity);
			sp_repr_set_attr (child, "style", p);
			g_free (p);
			sp_repr_add_child (repr, child, NULL);
			sp_repr_unref (child);
		}
	}

	return repr;
}

static void
sp_rg_fill (SPPainter *painter, guchar *px, gint x0, gint y0, gint width, gint height, gint rowstride)
{
#if 0
	SPLGPainter *lgp;
	SPRadialGradient *lg;
	SPGradient *g;
	gdouble pos;
	gint x, y;
	guint32 *p;
	guint32 tmp;

	lgp = (SPLGPainter *) painter;
	lg = lgp->lg;
	g = (SPGradient *) lg;

	if (!g->color) {
		/* fixme: This is forbidden, so we should paint some mishmesh instead */
		sp_gradient_write_colors (g);
	}

	for (y = 0; y < height; y++) {
		p = buf + y * rowstride;
		pos = (y + y0 - lgp->y0) * lgp->dy + (0 + x0 - lgp->x0) * lgp->dx;
		for (x = 0; x < width; x++) {
			gint ip, idx;
			ip = (gint) pos;
			switch (g->spread) {
			case SP_GRADIENT_SPREAD_PAD:
				idx = CLAMP (ip, 0, 1023);
				break;
			case SP_GRADIENT_SPREAD_REFLECT:
				idx = ip & 0x3ff;
				if (ip & 0x400) idx = 1023 - idx;
				break;
			case SP_GRADIENT_SPREAD_REPEAT:
				idx = ip & 0x3ff;
				break;
			default:
				g_assert_not_reached ();
				idx = 0;
				break;
			}
			/* a * b / 255 = a * b * (256 * 256 / 255) / 256 / 256 */
			tmp = (g->color[idx] & 0xff) * lgp->opacity;
			tmp = (tmp << 16) + tmp + (tmp >> 8);
			*p++ = (g->color[idx] & 0xffffff00) | (tmp >> 24);
			pos += lgp->dx;
		}
	}
#endif
}


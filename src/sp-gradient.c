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
 * Copyright (C) 200-2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <math.h>
#include <string.h>
#include <libart_lgpl/art_affine.h>
#include "svg/svg.h"
#include "xml/repr-private.h"
#include "sp-object-repr.h"
#include "sp-gradient.h"

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
			sizeof (SPStop),
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
sp_stop_init (SPStop * stop)
{
	stop->next = NULL;
	stop->offset = 0.0;
	stop->color = 0x000000ff;
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
		p = sp_object_get_style_property (object, "stop-opacity", "1");
		opacity = sp_svg_read_percentage (p, 1.0);

		color = (color & 0xffffff00) | ((guint32) (opacity * 255));
		stop->color = color;
		return;
	}
	if (strcmp (key, "offset") == 0) {
		SPUnit unit;
		const guchar *val;
		val = sp_repr_attr (object->repr, key);
		stop->offset = sp_svg_read_length (&unit, val, 0.0);
		return;
	}

	if (SP_OBJECT_CLASS (stop_parent_class)->read_attr)
		(* SP_OBJECT_CLASS (stop_parent_class)->read_attr) (object, key);
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

static void sp_lg_fill (SPPainter *painter, guint32 *buf, gint x0, gint y0, gint width, gint height, gint rowstride);
static void sp_lineargradient_write_colors (SPLinearGradient *lg);

static SPPaintServerClass * lg_parent_class;

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
		lineargradient_type = gtk_type_unique (SP_TYPE_PAINT_SERVER, &lineargradient_info);
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

	lg_parent_class = gtk_type_class (SP_TYPE_PAINT_SERVER);

	gtk_object_class->destroy = sp_lineargradient_destroy;

	sp_object_class->build = sp_lineargradient_build;
	sp_object_class->read_attr = sp_lineargradient_read_attr;

	ps_class->painter_new = sp_lineargradient_painter_new;
	ps_class->painter_free = sp_lineargradient_painter_free;
}

static void
sp_lineargradient_init (SPLinearGradient * lg)
{
	gint i;

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
	lg->spread = SP_GRADIENT_SPREAD_PAD;
	lg->stops = NULL;

	for (i = 0; i < 256; i++) {
		lg->color[i] = (i << 24) | ((255 - i) << 8) | 255;
	}

	lg->len = 0.0;
}

static void
sp_lineargradient_destroy (GtkObject * object)
{
	SPLinearGradient * lg;
	SPObject * spobject;

	lg = (SPLinearGradient *) object;

	while (lg->stops) {
		spobject = SP_OBJECT (lg->stops);
		lg->stops = (SPStop *) spobject->next;
		spobject->parent = NULL;
		spobject->next = NULL;
		gtk_object_unref (GTK_OBJECT (spobject));
	}

	if (GTK_OBJECT_CLASS (lg_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (lg_parent_class)->destroy) (object);
}

static void
sp_lineargradient_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPLinearGradient * lg;
	SPObject * last;
	SPRepr * rchild;

	lg = SP_LINEARGRADIENT (object);

	if (SP_OBJECT_CLASS (lg_parent_class)->build)
		(* SP_OBJECT_CLASS (lg_parent_class)->build) (object, document, repr);

	last = NULL;
	for (rchild = repr->children; rchild != NULL; rchild = rchild->next) {
		GtkType type;
		SPObject * child;
		type = sp_object_type_lookup (sp_repr_name (rchild));
		if (gtk_type_is_a (type, SP_TYPE_STOP)) {
			child = gtk_type_new (type);
			child->parent = object;
			if (last) {
				last->next = child;
			} else {
				lg->stops = SP_STOP (child);
			}
			sp_object_invoke_build (child, document, rchild, SP_OBJECT_IS_CLONED (object));
			last = child;
		}
	}

	sp_lineargradient_read_attr (object, "gradientUnits");
	sp_lineargradient_read_attr (object, "gradientTransform");
	sp_lineargradient_read_attr (object, "x1");
	sp_lineargradient_read_attr (object, "y1");
	sp_lineargradient_read_attr (object, "x2");
	sp_lineargradient_read_attr (object, "y2");
	sp_lineargradient_read_attr (object, "spreadMethod");

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
		return;
	} else if (!strcmp (key, "y1")) {
		lg->y1.distance = sp_svg_read_length (&lg->y1.unit, val, 0.0);
		lg->y1_set = (val != NULL);
		return;
	} else if (!strcmp (key, "x2")) {
		lg->x2.distance = sp_svg_read_length (&lg->x2.unit, val, 1.0);
		lg->x2_set = (val != NULL);
		return;
	} else if (!strcmp (key, "y2")) {
		lg->y2.distance = sp_svg_read_length (&lg->y2.unit, val, 0.0);
		lg->y2_set = (val != NULL);
		return;
	} else if (!strcmp (key, "spreadMethod")) {
		if (val) {
			if (!strcmp (val, "reflect")) {
				lg->spread = SP_GRADIENT_SPREAD_REFLECT;
			} else if (!strcmp (val, "repeat")) {
				lg->spread = SP_GRADIENT_SPREAD_REPEAT;
			} else {
				lg->spread = SP_GRADIENT_SPREAD_PAD;
			}
			lg->spread_set = TRUE;
		} else {
			lg->spread_set = FALSE;
		}
		return;
	}

	if (SP_OBJECT_CLASS (lg_parent_class)->read_attr)
		(* SP_OBJECT_CLASS (lg_parent_class)->read_attr) (object, key);
}

static SPPainter *
sp_lineargradient_painter_new (SPPaintServer *ps, gdouble *affine, gdouble opacity, ArtDRect *bbox)
{
	SPLinearGradient *lg;
	SPLGPainter *lgp;

	lg = SP_LINEARGRADIENT (ps);

	sp_lineargradient_write_colors (lg);

	lgp = g_new (SPLGPainter, 1);

	lgp->painter.type = SP_PAINTER_IND;
	lgp->painter.fill = sp_lg_fill;

	lgp->lg = lg;

	lgp->opacity = (guint32) floor (opacity * 255.9999);

	if (lg->units == SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX) {
		gdouble b2c[6], vec2b[6], norm2vec[6], norm2c[6], c2norm[6];
		gint i;
		/* fixme: we should use start & end here */
		norm2vec[0] = lg->len / 255.0;
		norm2vec[1] = 0.0;
		norm2vec[2] = 0.0;
		norm2vec[3] = lg->len / 255.0;
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
sp_lg_fill (SPPainter *painter, guint32 *buf, gint x0, gint y0, gint width, gint height, gint rowstride)
{
	SPLGPainter *lgp;
	SPLinearGradient *lg;
	gdouble pos;
	gint x, y;
	guint32 *p;
	guint32 tmp;

	lgp = (SPLGPainter *) painter;
	lg = lgp->lg;

	for (y = 0; y < height; y++) {
		p = buf + y * rowstride;
		pos = (y + y0 - lgp->y0) * lgp->dy + (0 + x0 - lgp->x0) * lgp->dx;
		for (x = 0; x < width; x++) {
			gint ip, idx;
			ip = (gint) pos;
			switch (lg->spread) {
			case SP_GRADIENT_SPREAD_PAD:
				idx = CLAMP (ip, 0, 255);
				break;
			case SP_GRADIENT_SPREAD_REFLECT:
				idx = ip & 0xff;
				if (ip & 0x100) idx = 255 - idx;
				break;
			case SP_GRADIENT_SPREAD_REPEAT:
				idx = ip & 0xff;
				break;
			default:
				g_assert_not_reached ();
				idx = 0;
				break;
			}
			/* a * b / 255 = a * b * (256 * 256 / 255) / 256 / 256 */
			tmp = (lg->color[idx] & 0xff) * lgp->opacity;
			tmp = (tmp << 16) + tmp + (tmp >> 8);
			*p++ = (lg->color[idx] & 0xffffff00) | (tmp >> 24);
			pos += lgp->dx;
		}
	}
}

static void
sp_lineargradient_write_colors (SPLinearGradient *lg)
{
	SPStop *stop;
	gdouble start, end, last;
	guint32 color;
	guint32 r0, g0, b0, a0;

	lg->len = 1.0;

	if (!lg->stops) {
		gint i;
		for (i = 0; i < 256; i++) lg->color[i] = 0x0;
		return;
	}

	if (!((SPObject *) lg->stops)->next) {
		gint i;
		for (i = 0; i < 256; i++) lg->color[i] = lg->stops->color;
		return;
	}

	start = lg->stops->offset;
	end = start;
	color = 0x0;
	for (stop = lg->stops; stop != NULL; stop = (SPStop *) ((SPObject *) stop)->next) {
		end = MAX (end, stop->offset);
		color = stop->color;
	}

	if ((end - start) < 1e-6) {
		gint i;
		for (i = 0; i < 256; i++) lg->color[i] = color;
		return;
	}

	lg->len = end - start;

	last = lg->stops->offset;
	color = lg->stops->color;
	r0 = (color >> 24) & 0xff;
	g0 = (color >> 16) & 0xff;
	b0 = (color >> 8) & 0xff;
	a0 = (color) & 0xff;
	for (stop = lg->stops; stop != NULL; stop = (SPStop *) ((SPObject *) stop)->next) {
		if (stop->offset > last) {
			gint s, e, l;
			guint32 r1, g1, b1, a1;
			s = (gint) floor ((last - start) * 255.9999 / lg->len);
			e = (gint) floor ((stop->offset - start) * 255.9999 / lg->len);
			l = e - s;
			r1 = (stop->color >> 24) & 0xff;
			g1 = (stop->color >> 16) & 0xff;
			b1 = (stop->color >> 8) & 0xff;
			a1 = (stop->color) & 0xff;
			if (l > 0) {
				gint i;
				for (i = 0; i <= l; i++) {
					guint32 r, g, b, a;
					r = ((l - i) * r0 + i * r1) / l;
					g = ((l - i) * g0 + i * g1) / l;
					b = ((l - i) * b0 + i * b1) / l;
					a = ((l - i) * a0 + i * a1) / l;
					lg->color[s + i] = (r << 24) | (g << 16) | (b << 8) | a;
				}
			} else {
				lg->color[e] = stop->color;
			}
			r0 = r1;
			g0 = g1;
			b0 = b1;
			a0 = a1;
		}
		last = stop->offset;
		color = stop->color;
	}

}


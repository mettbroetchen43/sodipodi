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

#include <string.h>
#include <libart_lgpl/art_affine.h>
#include "svg/svg.h"
#include "xml/repr-private.h"
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
		stop->offset = sp_repr_get_double_attribute (object->repr, key, 0.0);
		return;
	}

	if (SP_OBJECT_CLASS (stop_parent_class)->read_attr)
		(* SP_OBJECT_CLASS (stop_parent_class)->read_attr) (object, key);
}

/*
 * Linear Gradient
 */

static void sp_lineargradient_class_init (SPLinearGradientClass * klass);
static void sp_lineargradient_init (SPLinearGradient * lg);
static void sp_lineargradient_destroy (GtkObject * object);

static void sp_lineargradient_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_lineargradient_read_attr (SPObject * object, const gchar * key);

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
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	lg_parent_class = gtk_type_class (SP_TYPE_PAINT_SERVER);

	gtk_object_class->destroy = sp_lineargradient_destroy;

	sp_object_class->build = sp_lineargradient_build;
	sp_object_class->read_attr = sp_lineargradient_read_attr;
}

static void
sp_lineargradient_init (SPLinearGradient * lg)
{
	lg->units = SP_GRADIENT_UNITS_USERSPACEONUSE;
	art_affine_identity (lg->transform);
	lg->vector.x0 = lg->vector.y0 = lg->vector.y1 = 0.0;
	lg->vector.x1 = 1.0;
	lg->spread = SP_GRADIENT_SPREAD_PAD;
	lg->stops = NULL;
}

static void
sp_lineargradient_destroy (GtkObject * object)
{
	SPLinearGradient * lg;
	SPObject * spobject;

	lg = (SPLinearGradient *) object;

	while (lg->stops) {
		spobject = SP_OBJECT (lg->stops);
		lg->stops = lg->stops->next;
		spobject->parent = NULL;
		gtk_object_unref (GTK_OBJECT (spobject));
	}

	if (GTK_OBJECT_CLASS (lg_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (lg_parent_class)->destroy) (object);
}

static void
sp_lineargradient_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPLinearGradient * lg;
#if 0
	SPRepr * crepr;
	SPObject * child;
	const gchar * cname;
#endif

	lg = SP_LINEARGRADIENT (object);

	if (SP_OBJECT_CLASS (lg_parent_class)->build)
		(* SP_OBJECT_CLASS (lg_parent_class)->build) (object, document, repr);

	sp_lineargradient_read_attr (object, "gradientUnits");
	sp_lineargradient_read_attr (object, "gradientTransform");
	sp_lineargradient_read_attr (object, "x1");
	sp_lineargradient_read_attr (object, "y1");
	sp_lineargradient_read_attr (object, "x2");
	sp_lineargradient_read_attr (object, "y2");
	sp_lineargradient_read_attr (object, "spreadMethod");

#if 0
	/* fixme: build list */
	for (crepr = repr->children; crepr != NULL; crepr = crepr->next) {
		cname = sp_repr_name (crepr);
		if (strcmp (cname, "stop") == 0) {
			child = gtk_type_new (SP_TYPE_STOP);
			child->parent = object;
			lg->stops = g_slist_append (lg->stops, child);
			sp_object_invoke_build (child, document, crepr, SP_OBJECT_IS_CLONED (object));
		}
	}
#endif
}

static void
sp_lineargradient_read_attr (SPObject * object, const gchar * key)
{
	SPLinearGradient * lg;

	lg = SP_LINEARGRADIENT (object);

	/* fixme: implement this */

	if (SP_OBJECT_CLASS (lg_parent_class)->read_attr)
		(* SP_OBJECT_CLASS (lg_parent_class)->read_attr) (object, key);
}

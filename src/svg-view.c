#define __SP_SVG_VIEW_C__

/*
 * Generic SVG view and widget
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#include "svg-view.h"

static void sp_svg_view_class_init (SPSVGViewClass *klass);
static void sp_svg_view_init (SPSVGView *view);
static void sp_svg_view_destroy (GtkObject *object);

static SPViewClass *parent_class;

GtkType
sp_svg_view_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPSVGView",
			sizeof (SPSVGView),
			sizeof (SPSVGViewClass),
			(GtkClassInitFunc) sp_svg_view_class_init,
			(GtkObjectInitFunc) sp_svg_view_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_VIEW, &info);
	}
	return type;
}

static void
sp_svg_view_class_init (SPSVGViewClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);

	parent_class = gtk_type_class (SP_TYPE_VIEW);

	object_class->destroy = sp_svg_view_destroy;
}

static void
sp_svg_view_init (SPSVGView *view)
{
}

static void
sp_svg_view_destroy (GtkObject *object)
{
	SPSVGView *view;

	view = SP_SVG_VIEW (object);

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

/* SPSVGViewWidget */

static void sp_svg_view_widget_class_init (SPSVGViewWidgetClass *klass);
static void sp_svg_view_widget_init (SPSVGViewWidget *widget);
static void sp_svg_view_widget_destroy (GtkObject *object);

static SPViewWidgetClass *widget_parent_class;

GtkType
sp_svg_view_widget_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPSVGViewWidget",
			sizeof (SPSVGViewWidget),
			sizeof (SPSVGViewWidgetClass),
			(GtkClassInitFunc) sp_svg_view_widget_class_init,
			(GtkObjectInitFunc) sp_svg_view_widget_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_VIEW_WIDGET, &info);
	}
	return type;
}

static void
sp_svg_view_widget_class_init (SPSVGViewWidgetClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);

	widget_parent_class = gtk_type_class (SP_TYPE_SVG_VIEW);

	object_class->destroy = sp_svg_view_widget_destroy;
}

static void
sp_svg_view_widget_init (SPSVGViewWidget *vw)
{
}

static void
sp_svg_view_widget_destroy (GtkObject *object)
{
	SPSVGViewWidget *vw;

	vw = SP_SVG_VIEW_WIDGET (object);

	if (((GtkObjectClass *) (widget_parent_class))->destroy)
		(* ((GtkObjectClass *) (widget_parent_class))->destroy) (object);
}


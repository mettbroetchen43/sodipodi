#define __SP_GRADIENT_IMAGE_C__

/*
 * SPGradientImage
 *
 * A slider with colored background
 *
 * Copyright (C) Lauris Kaplinski <lauris@ximian.com> 2001
 *
 */

#include "gradient-image.h"

#define VBLOCK 8

static void sp_gradient_image_class_init (SPGradientImageClass *klass);
static void sp_gradient_image_init (SPGradientImage *image);
static void sp_gradient_image_destroy (GtkObject *object);

static void sp_gradient_image_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void sp_gradient_image_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint sp_gradient_image_expose (GtkWidget *widget, GdkEventExpose *event);

static GtkWidgetClass *parent_class;

GtkType
sp_gradient_image_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPGradientImage",
			sizeof (SPGradientImage),
			sizeof (SPGradientImageClass),
			(GtkClassInitFunc) sp_gradient_image_class_init,
			(GtkObjectInitFunc) sp_gradient_image_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_WIDGET, &info);
	}
	return type;
}

static void
sp_gradient_image_class_init (SPGradientImageClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	parent_class = gtk_type_class (GTK_TYPE_WIDGET);

	object_class->destroy = sp_gradient_image_destroy;

	widget_class->size_request = sp_gradient_image_size_request;
	widget_class->size_allocate = sp_gradient_image_size_allocate;
	widget_class->expose_event = sp_gradient_image_expose;
}

static void
sp_gradient_image_init (SPGradientImage *image)
{
	GTK_WIDGET_SET_FLAGS (image, GTK_NO_WINDOW);

	image->gradient = NULL;
	image->px = NULL;
}

static void
sp_gradient_image_destroy (GtkObject *object)
{
	SPGradientImage *image;

	image = SP_GRADIENT_IMAGE (object);

	if (image->px) {
		g_free (image->px);
		image->px = NULL;
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_gradient_image_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	SPGradientImage *slider;

	slider = SP_GRADIENT_IMAGE (widget);

	requisition->width = 64;
	requisition->height = 16;
}

static void
sp_gradient_image_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	SPGradientImage *image;

	image = SP_GRADIENT_IMAGE (widget);

	widget->allocation = *allocation;

	if (image->px) {
		g_free (image->px);
		image->px = NULL;
	}

	if (image->gradient) {
		image->px = g_new (guchar, 3 * VBLOCK * allocation->width);
		sp_gradient_render_vector_block_rgb (image->gradient,
						     image->px, allocation->width, VBLOCK, 3 * allocation->width,
						     0, allocation->width, TRUE);
	}
}

static gint
sp_gradient_image_expose (GtkWidget *widget, GdkEventExpose *event)
{
	SPGradientImage *image;

	image = SP_GRADIENT_IMAGE (widget);

	if (GTK_WIDGET_DRAWABLE (widget)) {
		gint x0, y0, x1, y1;
		x0 = MAX (event->area.x, widget->allocation.x);
		y0 = MAX (event->area.y, widget->allocation.y);
		x1 = MIN (event->area.x + event->area.width, widget->allocation.x + widget->allocation.width);
		y1 = MIN (event->area.y + event->area.height, widget->allocation.y + widget->allocation.height);
		if ((x1 > x0) && (y1 > y0)) {
			if (image->px) {
				gint y;
				guchar *p;
				p = image->px + 3 * (x0 - widget->allocation.x);
				for (y = y0; y < y1; y += VBLOCK) {
					gdk_draw_rgb_image (widget->window, widget->style->black_gc,
							    x0, y,
							    (x1 - x0), MIN (VBLOCK, y1 - y),
							    GDK_RGB_DITHER_MAX,
							    p, widget->allocation.width * 3);
				}
			} else {
				gdk_draw_rectangle (widget->window, widget->style->black_gc,
						    x0, y0,
						    (x1 - x0), (y1 - x0),
						    TRUE);
			}
		}
	}

	return TRUE;
}

GtkWidget *
sp_gradient_image_new (SPGradient *gradient)
{
	SPGradientImage *image;

	image = gtk_type_new (SP_TYPE_GRADIENT_IMAGE);

	image->gradient = gradient;

	return (GtkWidget *) image;
}


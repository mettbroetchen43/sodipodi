#ifndef __SP_GRADIENT_IMAGE_H__
#define __SP_GRADIENT_IMAGE_H__

/*
 * SPGradientImage
 *
 * A simple preview for gradient vector
 *
 * Copyright (C) Lauris Kaplinski <lauris@ximian.com> 2001
 *
 */

#include <gtk/gtkwidget.h>
#include "../sp-gradient.h"

typedef struct _SPGradientImage SPGradientImage;
typedef struct _SPGradientImageClass SPGradientImageClass;

#define SP_TYPE_GRADIENT_IMAGE (sp_gradient_image_get_type ())
#define SP_GRADIENT_IMAGE(o) (GTK_CHECK_CAST ((o), SP_TYPE_GRADIENT_IMAGE, SPGradientImage))
#define SP_GRADIENT_IMAGE_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_GRADIENT_IMAGE, SPGradientImageClass))
#define SP_IS_GRADIENT_IMAGE(o) (GTK_CHECK_TYPE ((o), SP_TYPE_GRADIENT_IMAGE))
#define SP_IS_GRADIENT_IMAGE_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_GRADIENT_IMAGE))

struct _SPGradientImage {
	GtkWidget widget;
	SPGradient *gradient;
	guchar *px;
};

struct _SPGradientImageClass {
	GtkWidgetClass parent_class;
};

GtkType sp_gradient_image_get_type (void);

GtkWidget *sp_gradient_image_new (SPGradient *gradient);

#endif

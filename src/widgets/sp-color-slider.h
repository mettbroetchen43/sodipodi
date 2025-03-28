#ifndef __SP_COLOR_SLIDER_H__
#define __SP_COLOR_SLIDER_H__

/*
 * A slider with colored background
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 *
 * This code is in public domain
 */

#include <gtk/gtkwidget.h>

typedef struct _SPColorSlider SPColorSlider;
typedef struct _SPColorSliderClass SPColorSliderClass;

#define SP_TYPE_COLOR_SLIDER (sp_color_slider_get_type ())
#define SP_COLOR_SLIDER(o) (GTK_CHECK_CAST ((o), SP_TYPE_COLOR_SLIDER, SPColorSlider))
#define SP_COLOR_SLIDER_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_COLOR_SLIDER, SPColorSliderClass))
#define SP_IS_COLOR_SLIDER(o) (GTK_CHECK_TYPE ((o), SP_TYPE_COLOR_SLIDER))
#define SP_IS_COLOR_SLIDER_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_COLOR_SLIDER))

struct _SPColorSlider {
	GtkWidget widget;

	guint dragging : 1;

	GtkAdjustment *adjustment;

	gfloat value;
	gfloat oldvalue;
	guchar c0[4], c1[4];
	guchar b0, b1;
	guchar bmask;

	gint mapsize;
	guchar *map;
};

struct _SPColorSliderClass {
	GtkWidgetClass parent_class;

	void (* grabbed) (SPColorSlider *slider);
	void (* dragged) (SPColorSlider *slider);
	void (* released) (SPColorSlider *slider);
	void (* changed) (SPColorSlider *slider);
};

GtkType sp_color_slider_get_type (void);

GtkWidget *sp_color_slider_new (GtkAdjustment *adjustment);

void sp_color_slider_set_adjustment (SPColorSlider *slider, GtkAdjustment *adjustment);
void sp_color_slider_set_colors (SPColorSlider *slider, guint32 start, guint32 end);
void sp_color_slider_set_map (SPColorSlider *slider, const guchar *map);
void sp_color_slider_set_background (SPColorSlider *slider, guint dark, guint light, guint size);

#endif

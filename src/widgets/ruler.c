#define __SP_RULER_C__

/*
 * Customized ruler class for sodipodi
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "ruler.h"

#define RULER_WIDTH 12
#define RULER_HEIGHT 12
#define MINIMUM_INCR 5
#define MAXIMUM_SUBDIVIDE 5
#define MAXIMUM_SCALES 10

#define ROUND(x) ((int) ((x) + 0.5))

static void sp_ruler_class_init (SPRulerClass *klass);
static void sp_ruler_init (SPRuler *hruler);
static gint sp_ruler_motion_notify (GtkWidget *widget, GdkEventMotion *event);
static void sp_ruler_draw_ticks (GtkRuler *ruler);
static void sp_ruler_draw_pos (GtkRuler *ruler);

GtkType
sp_ruler_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		static const GtkTypeInfo info = {
			"SPRuler",
			sizeof (SPRuler),
			sizeof (SPRulerClass),
			(GtkClassInitFunc) sp_ruler_class_init,
			(GtkObjectInitFunc) sp_ruler_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL,
		};
		type = gtk_type_unique (GTK_TYPE_RULER, &info);
	}
	return type;
}

static void
sp_ruler_class_init (SPRulerClass *klass)
{
	GtkWidgetClass *widget_class;
	GtkRulerClass *gtk_ruler_class;

	widget_class = (GtkWidgetClass*) klass;
	gtk_ruler_class = (GtkRulerClass*) klass;

	widget_class->motion_notify_event = sp_ruler_motion_notify;

	gtk_ruler_class->draw_ticks = sp_ruler_draw_ticks;
	gtk_ruler_class->draw_pos = sp_ruler_draw_pos;
}

static void
sp_ruler_init (SPRuler *hruler)
{
	/* Nothing here */
}


GtkWidget*
sp_ruler_new (unsigned int vertical)
{
	GtkWidget *widget;
	widget = (GtkWidget *) g_object_new (SP_TYPE_RULER, NULL);
	((SPRuler *) widget)->vertical = !(!vertical);
	if (vertical) {
		widget->requisition.width = widget->style->xthickness * 2 + RULER_WIDTH;
		widget->requisition.height = widget->style->ythickness * 2 + 1;
	} else {
		widget->requisition.width = widget->style->xthickness * 2 + 1;
		widget->requisition.height = widget->style->ythickness * 2 + RULER_HEIGHT;
	}
	return widget;
}

static gint
sp_ruler_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
	GtkRuler *ruler;
	gint x, y;

	ruler = GTK_RULER (widget);

	if (event->is_hint) {
		gdk_window_get_pointer (widget->window, &x, &y, NULL);
	} else {
		x = event->x;
		y = event->y;
	}
	if (((SPRuler *) ruler)->vertical) {
		ruler->position = ruler->lower + ((ruler->upper - ruler->lower) * y) / widget->allocation.height;
	} else {
		ruler->position = ruler->lower + ((ruler->upper - ruler->lower) * x) / widget->allocation.width;
	}

	if (ruler->backing_store != NULL) {
		/*  Make sure the ruler has been allocated already  */
		gtk_ruler_draw_pos (ruler);
	}

	return FALSE;
}

static void
sp_vruler_draw_ticks (GtkRuler *ruler)
{
  GtkWidget *widget;
  GdkGC *gc, *bg_gc;
  GdkFont *font;
  gint i, j;
  gint width, height;
  gint xthickness;
  gint ythickness;
  gint length, ideal_length;
  gfloat lower, upper;		/* Upper and lower limits, in ruler units */
  gfloat increment;		/* Number of pixels per unit */
  gint scale;			/* Number of units per major unit */
  gfloat subd_incr;
  gfloat start, end, cur;
  gchar unit_str[32];
  gchar digit_str[2] = { '\0', '\0' };
  gint digit_height;
  gint text_height;
  gint pos;

  if (!GTK_WIDGET_DRAWABLE (ruler)) 
    return;

  widget = GTK_WIDGET (ruler);

  gc = widget->style->fg_gc[GTK_STATE_NORMAL];
  bg_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
  font = gtk_style_get_font(widget->style);
  xthickness = widget->style->xthickness;
  ythickness = widget->style->ythickness;
  digit_height = font->ascent; /* assume descent == 0 ? */

  width = widget->allocation.height;
  height = widget->allocation.width;// - ythickness * 2;

   gtk_paint_box (widget->style, ruler->backing_store,
		  GTK_STATE_NORMAL, GTK_SHADOW_NONE, 
		  NULL, widget, "vruler",
		  0, 0, 
		  widget->allocation.width, widget->allocation.height);
   /*
   gdk_draw_line (ruler->backing_store, gc,
		 height + xthickness,
		 ythickness,
		 height + xthickness,
		 widget->allocation.height - ythickness);
   */
  upper = ruler->upper / ruler->metric->pixels_per_unit;
  lower = ruler->lower / ruler->metric->pixels_per_unit;

  if ((upper - lower) == 0)
    return;
  increment = (gfloat) width / (upper - lower);

  /* determine the scale
   *   use the maximum extents of the ruler to determine the largest
   *   possible number to be displayed.  Calculate the height in pixels
   *   of this displayed text. Use this height to find a scale which
   *   leaves sufficient room for drawing the ruler.  
   */
  scale = ceil (ruler->max_size / ruler->metric->pixels_per_unit);
  sprintf (unit_str, "%d", scale);
  text_height = strlen (unit_str) * digit_height + 1;

  for (scale = 0; scale < MAXIMUM_SCALES; scale++)
    if (ruler->metric->ruler_scale[scale] * fabs(increment) > 2 * text_height)
      break;

  if (scale == MAXIMUM_SCALES)
    scale = MAXIMUM_SCALES - 1;

  /* drawing starts here */
  length = 0;
  for (i = MAXIMUM_SUBDIVIDE - 1; i >= 0; i--)
    {
      subd_incr = (gfloat) ruler->metric->ruler_scale[scale] / 
	          (gfloat) ruler->metric->subdivide[i];
      if (subd_incr * fabs(increment) <= MINIMUM_INCR) 
	continue;

      /* Calculate the length of the tickmarks. Make sure that
       * this length increases for each set of ticks
       */
      ideal_length = height / (i + 1) - 1;
      if (ideal_length > ++length)
	length = ideal_length;

      if (lower < upper)
	{
	  start = floor (lower / subd_incr) * subd_incr;
	  end   = ceil  (upper / subd_incr) * subd_incr;
	}
      else
	{
	  start = floor (upper / subd_incr) * subd_incr;
	  end   = ceil  (lower / subd_incr) * subd_incr;
	}

      for (cur = start; cur <= end; cur += subd_incr)
	{
	  pos = ROUND ((cur - lower) * increment);

	  gdk_draw_line (ruler->backing_store, gc,
			 height + xthickness - length, pos,
			 height + xthickness, pos);

	  /* draw label */
	  if (i == 0)
	    {
	      sprintf (unit_str, "%d", (int) cur);
	      for (j = 0; j < (int) strlen (unit_str); j++)
		{
		  digit_str[0] = unit_str[j];
		  gdk_draw_string (ruler->backing_store, font, gc,
				   xthickness + 1,
				   pos + digit_height * (j + 1) + 1,
				   digit_str);
		}
	    }
	}
    }
}

static void
sp_ruler_draw_ticks (GtkRuler *ruler)
{
	GtkWidget *widget;
	GdkGC *gc, *bg_gc;
	GdkFont *font;
	gint i;
	gint width, height;
	gint xthickness;
	gint ythickness;
	gint length, ideal_length;
	gfloat lower, upper;		/* Upper and lower limits, in ruler units */
	gfloat increment;		/* Number of pixels per unit */
	gint scale;			/* Number of units per major unit */
	gfloat subd_incr;
	gfloat start, end, cur;
	gchar unit_str[32];
	gint digit_height;
	gint text_width;
	gint pos;

	if (((SPRuler *) ruler)->vertical) {
		sp_vruler_draw_ticks (ruler);
		return;
	}

	if (!GTK_WIDGET_DRAWABLE (ruler)) return;

	widget = GTK_WIDGET (ruler);

	gc = widget->style->fg_gc[GTK_STATE_NORMAL];
	bg_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
	font = gtk_style_get_font(widget->style);

	xthickness = widget->style->xthickness;
	ythickness = widget->style->ythickness;
	digit_height = font->ascent; /* assume descent == 0 ? */

	width = widget->allocation.width;
	height = widget->allocation.height;// - ythickness * 2;

	gtk_paint_box (widget->style, ruler->backing_store,
		       GTK_STATE_NORMAL, GTK_SHADOW_NONE, 
		       NULL, widget, "hruler",
		       0, 0, 
		       widget->allocation.width, widget->allocation.height);

	upper = ruler->upper / ruler->metric->pixels_per_unit;
	lower = ruler->lower / ruler->metric->pixels_per_unit;

	if ((upper - lower) == 0) return;
	increment = (gfloat) width / (upper - lower);

	/* determine the scale
	 *  We calculate the text size as for the vruler instead of using
	 *  text_width = gdk_string_width(font, unit_str), so that the result
	 *  for the scale looks consistent with an accompanying vruler
	 */
	scale = ceil (ruler->max_size / ruler->metric->pixels_per_unit);
	sprintf (unit_str, "%d", scale);
	text_width = strlen (unit_str) * digit_height + 1;

	for (scale = 0; scale < MAXIMUM_SCALES; scale++) {
		if (ruler->metric->ruler_scale[scale] * fabs(increment) > 2 * text_width) break;
	}
	if (scale == MAXIMUM_SCALES) scale = MAXIMUM_SCALES - 1;

	/* drawing starts here */
	length = 0;
	for (i = MAXIMUM_SUBDIVIDE - 1; i >= 0; i--) {
		subd_incr = (gfloat) ruler->metric->ruler_scale[scale] / (gfloat) ruler->metric->subdivide[i];
		if (subd_incr * fabs(increment) <= MINIMUM_INCR) continue;

		/* Calculate the length of the tickmarks. Make sure that
		 * this length increases for each set of ticks
		 */
		ideal_length = height / (i + 1) - 1;
		if (ideal_length > ++length) length = ideal_length;

		if (lower < upper) {
			start = floor (lower / subd_incr) * subd_incr;
			end   = ceil  (upper / subd_incr) * subd_incr;
		} else {
			start = floor (upper / subd_incr) * subd_incr;
			end   = ceil  (lower / subd_incr) * subd_incr;
		}

  
		for (cur = start; cur <= end; cur += subd_incr) {
			pos = ROUND ((cur - lower) * increment);
			gdk_draw_line (ruler->backing_store, gc,
				       pos, height + ythickness, 
				       pos, height - length + ythickness);
			/* draw label */
			if (i == 0) {
				sprintf (unit_str, "%d", (int) cur);
				gdk_draw_string (ruler->backing_store, font, gc,
						 pos + 2, ythickness + font->ascent - 1,
						 unit_str);
			}
		}
	}
}

static void
sp_vruler_draw_pos (GtkRuler *ruler)
{
  GtkWidget *widget;
  GdkGC *gc;
  int i;
  gint x, y;
  gint width, height;
  gint bs_width, bs_height;
  gint xthickness;
  gint ythickness;
  gfloat increment;

  if (GTK_WIDGET_DRAWABLE (ruler))
    {
      widget = GTK_WIDGET (ruler);

      gc = widget->style->fg_gc[GTK_STATE_NORMAL];
      xthickness = widget->style->xthickness;
      ythickness = widget->style->ythickness;
      width = widget->allocation.width - xthickness * 2;
      height = widget->allocation.height;

      bs_height = width / 2;
      bs_height |= 1;  /* make sure it's odd */
      bs_width = bs_height / 2 + 1;

      if ((bs_width > 0) && (bs_height > 0))
	{
	  /*  If a backing store exists, restore the ruler  */
	  if (ruler->backing_store && ruler->non_gr_exp_gc)
	    gdk_draw_pixmap (ruler->widget.window,
			     ruler->non_gr_exp_gc,
			     ruler->backing_store,
			     ruler->xsrc, ruler->ysrc,
			     ruler->xsrc, ruler->ysrc,
			     bs_width, bs_height);

	  increment = (gfloat) height / (ruler->upper - ruler->lower);

	  x = (width + bs_width) / 2 + xthickness;
	  y = ROUND ((ruler->position - ruler->lower) * increment) + (ythickness - bs_height) / 2 - 1;

	  for (i = 0; i < bs_width; i++)
	    gdk_draw_line (widget->window, gc,
			   x + i, y + i,
			   x + i, y + bs_height - 1 - i);

	  ruler->xsrc = x;
	  ruler->ysrc = y;
	}
    }
}

static void
sp_ruler_draw_pos (GtkRuler *ruler)
{
  GtkWidget *widget;
  GdkGC *gc;
  int i;
  gint x, y;
  gint width, height;
  gint bs_width, bs_height;
  gint xthickness;
  gint ythickness;
  gfloat increment;

  if (((SPRuler *) ruler)->vertical) {
	  sp_vruler_draw_pos (ruler);
	  return;
  }

  if (GTK_WIDGET_DRAWABLE (ruler))
    {
      widget = GTK_WIDGET (ruler);

      gc = widget->style->fg_gc[GTK_STATE_NORMAL];
      xthickness = widget->style->xthickness;
      ythickness = widget->style->ythickness;
      width = widget->allocation.width;
      height = widget->allocation.height - ythickness * 2;

      bs_width = height / 2;
      bs_width |= 1;  /* make sure it's odd */
      bs_height = bs_width / 2 + 1;

      if ((bs_width > 0) && (bs_height > 0))
	{
	  /*  If a backing store exists, restore the ruler  */
	  if (ruler->backing_store && ruler->non_gr_exp_gc)
	    gdk_draw_pixmap (ruler->widget.window,
			     ruler->non_gr_exp_gc,
			     ruler->backing_store,
			     ruler->xsrc, ruler->ysrc,
			     ruler->xsrc, ruler->ysrc,
			     bs_width, bs_height);

	  increment = (gfloat) width / (ruler->upper - ruler->lower);

	  x = ROUND ((ruler->position - ruler->lower) * increment) + (xthickness - bs_width) / 2 - 1;
	  y = (height + bs_height) / 2 + ythickness;

	  for (i = 0; i < bs_height; i++)
	    gdk_draw_line (widget->window, gc,
			   x + i, y + i,
			   x + bs_width - 1 - i, y + i);


	  ruler->xsrc = x;
	  ruler->ysrc = y;
	}
    }
}

void
sp_ruler_set_metric (GtkRuler *ruler,
		     SPMetric  metric)
{
  g_return_if_fail (ruler != NULL);
  g_return_if_fail (GTK_IS_RULER (ruler));

  ruler->metric = (GtkRulerMetric *) &sp_ruler_metrics[metric];

  if (GTK_WIDGET_DRAWABLE (ruler))
    gtk_widget_queue_draw (GTK_WIDGET (ruler));
}


#define __SP_GRADIENT_POSITION_C__

/*
 * SPGradientPosition
 *
 * Gradient positioning widget
 *
 * Copyright (C) Lauris Kaplinski <lauris@ximian.com> 2001
 *
 */

#include <math.h>
#include <string.h>
#include <libart_lgpl/art_affine.h>
#include <gtk/gtksignal.h>
#include "../helper/nr-plain-stuff.h"
#include "../helper/nr-plain-stuff-gdk.h"
#include "gradient-position.h"

#define RADIUS 3

enum {
	GRABBED,
	DRAGGED,
	RELEASED,
	CHANGED,
	LAST_SIGNAL
};

static void sp_gradient_position_class_init (SPGradientPositionClass *klass);
static void sp_gradient_position_init (SPGradientPosition *position);
static void sp_gradient_position_destroy (GtkObject *object);

static void sp_gradient_position_realize (GtkWidget *widget);
static void sp_gradient_position_unrealize (GtkWidget *widget);
static void sp_gradient_position_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void sp_gradient_position_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static void sp_gradient_position_draw (GtkWidget *widget, GdkRectangle *area);
static gint sp_gradient_position_expose (GtkWidget *widget, GdkEventExpose *event);

static gint sp_gradient_position_button_press (GtkWidget *widget, GdkEventButton *event);
static gint sp_gradient_position_button_release (GtkWidget *widget, GdkEventButton *event);
static gint sp_gradient_position_motion_notify (GtkWidget *widget, GdkEventMotion *event);

static void sp_gradient_position_gradient_destroy (SPGradient *gr, SPGradientPosition *im);
static void sp_gradient_position_gradient_modified (SPGradient *gr, guint flags, SPGradientPosition *im);
static void sp_gradient_position_update (SPGradientPosition *img);

static void sp_gradient_position_free_image_data (SPGradientPosition *pos);

static GtkWidgetClass *parent_class;
static guint position_signals[LAST_SIGNAL] = {0};

GtkType
sp_gradient_position_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPGradientPosition",
			sizeof (SPGradientPosition),
			sizeof (SPGradientPositionClass),
			(GtkClassInitFunc) sp_gradient_position_class_init,
			(GtkObjectInitFunc) sp_gradient_position_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_WIDGET, &info);
	}
	return type;
}

static void
sp_gradient_position_class_init (SPGradientPositionClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	parent_class = gtk_type_class (GTK_TYPE_WIDGET);

	position_signals[GRABBED] = gtk_signal_new ("grabbed",
						    GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						    object_class->type,
						    GTK_SIGNAL_OFFSET (SPGradientPositionClass, grabbed),
						    gtk_marshal_NONE__NONE,
						    GTK_TYPE_NONE, 0);
	position_signals[DRAGGED] = gtk_signal_new ("dragged",
						    GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						    object_class->type,
						    GTK_SIGNAL_OFFSET (SPGradientPositionClass, dragged),
						    gtk_marshal_NONE__NONE,
						    GTK_TYPE_NONE, 0);
	position_signals[RELEASED] = gtk_signal_new ("released",
						     GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						     object_class->type,
						     GTK_SIGNAL_OFFSET (SPGradientPositionClass, released),
						     gtk_marshal_NONE__NONE,
						     GTK_TYPE_NONE, 0);
	position_signals[CHANGED] = gtk_signal_new ("changed",
						    GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						    object_class->type,
						    GTK_SIGNAL_OFFSET (SPGradientPositionClass, changed),
						    gtk_marshal_NONE__NONE,
						    GTK_TYPE_NONE, 0);
	gtk_object_class_add_signals (object_class, position_signals, LAST_SIGNAL);

	object_class->destroy = sp_gradient_position_destroy;

	widget_class->realize = sp_gradient_position_realize;
	widget_class->unrealize = sp_gradient_position_unrealize;
	widget_class->size_request = sp_gradient_position_size_request;
	widget_class->size_allocate = sp_gradient_position_size_allocate;
	widget_class->draw = sp_gradient_position_draw;
	widget_class->expose_event = sp_gradient_position_expose;
	widget_class->button_press_event = sp_gradient_position_button_press;
	widget_class->button_release_event = sp_gradient_position_button_release;
	widget_class->motion_notify_event = sp_gradient_position_motion_notify;
}

static void
sp_gradient_position_init (SPGradientPosition *pos)
{
	/* We are widget with window */
	GTK_WIDGET_UNSET_FLAGS (pos, GTK_NO_WINDOW);

	pos->dragging = FALSE;
	pos->position = 0;

	pos->need_update = TRUE;

	pos->gradient = NULL;
	pos->painter = NULL;
	pos->bbox.x0 = pos->bbox.y0 = pos->bbox.x1 = pos->bbox.y1 = 0.0;
	pos->p0.x = pos->p0.y = 0.0;
	pos->p1.x = pos->p1.y = 1.0;
	art_affine_identity (pos->transform);
	pos->gc = NULL;
	pos->px = NULL;
	pos->rgb = NULL;
	pos->rgba = NULL;
}

static void
sp_gradient_position_destroy (GtkObject *object)
{
	SPGradientPosition *pos;

	pos = SP_GRADIENT_POSITION (object);

	sp_gradient_position_free_image_data (pos);

	if (pos->gradient) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (pos->gradient), pos);
		pos->gradient = NULL;
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_gradient_position_realize (GtkWidget *widget)
{
	SPGradientPosition *pos;
	GdkWindowAttr attributes;
	gint attributes_mask;

	pos = SP_GRADIENT_POSITION (widget);

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gdk_rgb_get_visual ();
	attributes.colormap = gdk_rgb_get_cmap ();
	attributes.event_mask = gtk_widget_get_events (widget);
	attributes.event_mask |= (GDK_EXPOSURE_MASK |
				  GDK_BUTTON_PRESS_MASK |
				  GDK_BUTTON_RELEASE_MASK |
				  GDK_POINTER_MOTION_MASK |
				  GDK_ENTER_NOTIFY_MASK |
				  GDK_LEAVE_NOTIFY_MASK);
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
	gdk_window_set_user_data (widget->window, widget);

	pos->need_update = TRUE;
}

static void
sp_gradient_position_unrealize (GtkWidget *widget)
{
	SPGradientPosition *pos;

	pos = SP_GRADIENT_POSITION (widget);

	if (((GtkWidgetClass *) parent_class)->unrealize)
		(* ((GtkWidgetClass *) parent_class)->unrealize) (widget);

	sp_gradient_position_free_image_data (pos);

	pos->need_update = TRUE;
}

static void
sp_gradient_position_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	SPGradientPosition *pos;

	pos = SP_GRADIENT_POSITION (widget);

	requisition->width = 128;
	requisition->height = 128;
}

static void
sp_gradient_position_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	SPGradientPosition *pos;

	pos = SP_GRADIENT_POSITION (widget);

	widget->allocation = *allocation;

	if (GTK_WIDGET_REALIZED (widget)) {
		sp_gradient_position_free_image_data (pos);
		pos->need_update = TRUE;
		gdk_window_move_resize (widget->window,
					widget->allocation.x, widget->allocation.y,
					widget->allocation.width, widget->allocation.height);
	}
}

static void
sp_gradient_position_draw (GtkWidget *widget, GdkRectangle *area)
{
	SPGradientPosition *pos;

	pos = SP_GRADIENT_POSITION (widget);

	if (GTK_WIDGET_DRAWABLE (widget)) {
		if (pos->need_update) {
			sp_gradient_position_update (pos);
		}
		gdk_gc_set_function (pos->gc, GDK_COPY);
		gdk_draw_pixmap (widget->window, pos->gc,
				 pos->px,
				 area->x, area->y,
				 area->x, area->y,
				 area->width, area->height);
	}
}

static gint
sp_gradient_position_expose (GtkWidget *widget, GdkEventExpose *event)
{
	SPGradientPosition *pos;

	pos = SP_GRADIENT_POSITION (widget);

	if (GTK_WIDGET_DRAWABLE (widget)) {
		sp_gradient_position_draw (widget, &event->area);
	}

	return TRUE;
}

static gint
sp_gradient_position_button_press (GtkWidget *widget, GdkEventButton *event)
{
	SPGradientPosition *pos;

	pos = SP_GRADIENT_POSITION (widget);

	if (!pos->gradient) return FALSE;

	if (event->button == 1) {
		gdouble x, y, dx, dy;
		/* Translate end to widget coords */
		x = pos->vbox.x0 + pos->p1.x * (pos->vbox.x1 - pos->vbox.x0);
		y = pos->vbox.y0 + pos->p1.y * (pos->vbox.y1 - pos->vbox.y0);
		dx = event->x - x;
		dy = event->y - y;
		if ((dx * dx + dy * dy) < ((RADIUS + 1) * (RADIUS + 1))) {
			/* Calculate mouse coordinates in bbox */
			x = (event->x - pos->vbox.x0) / (pos->vbox.x1 - pos->vbox.x0);
			y = (event->y - pos->vbox.y0) / (pos->vbox.y1 - pos->vbox.y0);
			/* Grab end */
			pos->dragging = TRUE;
			pos->position = 1;
			pos->pold = pos->p1;
			gtk_signal_emit (GTK_OBJECT (pos), position_signals[GRABBED]);
			if ((x != pos->p1.x) || (y != pos->p1.y)) {
				/* Write coords and emit "dragged" */
				pos->p1.x = x;
				pos->p1.y = y;
				pos->need_update = TRUE;
				if (GTK_WIDGET_DRAWABLE (pos)) {
					gtk_widget_queue_draw (GTK_WIDGET (pos));
				}
				gtk_signal_emit (GTK_OBJECT (pos), position_signals[DRAGGED]);
			}
			gdk_pointer_grab (widget->window, FALSE,
					  GDK_POINTER_MOTION_MASK |
					  GDK_BUTTON_RELEASE_MASK,
					  NULL, NULL, event->time);
			return FALSE;
		}
		/* Translate start to widget coords */
		x = pos->vbox.x0 + pos->p0.x * (pos->vbox.x1 - pos->vbox.x0);
		y = pos->vbox.y0 + pos->p0.y * (pos->vbox.y1 - pos->vbox.y0);
		dx = event->x - x;
		dy = event->y - y;
		if ((dx * dx + dy * dy) < ((RADIUS + 1) * (RADIUS + 1))) {
			/* Calculate mouse coordinates in bbox */
			x = (event->x - pos->vbox.x0) / (pos->vbox.x1 - pos->vbox.x0);
			y = (event->y - pos->vbox.y0) / (pos->vbox.y1 - pos->vbox.y0);
			/* Grab start */
			pos->dragging = TRUE;
			pos->position = 0;
			pos->pold = pos->p0;
			gtk_signal_emit (GTK_OBJECT (pos), position_signals[GRABBED]);
			if ((x != pos->p0.x) || (y != pos->p0.y)) {
				/* Write coords and emit "dragged" */
				pos->p0.x = x;
				pos->p0.y = y;
				pos->need_update = TRUE;
				if (GTK_WIDGET_DRAWABLE (pos)) {
					gtk_widget_queue_draw (GTK_WIDGET (pos));
				}
				gtk_signal_emit (GTK_OBJECT (pos), position_signals[DRAGGED]);
			}
			gdk_pointer_grab (widget->window, FALSE,
					  GDK_POINTER_MOTION_MASK |
					  GDK_BUTTON_RELEASE_MASK,
					  NULL, NULL, event->time);
			return FALSE;
		}
	}

	return FALSE;
}

static gint
sp_gradient_position_button_release (GtkWidget *widget, GdkEventButton *event)
{
	SPGradientPosition *pos;

	pos = SP_GRADIENT_POSITION (widget);

	if ((event->button == 1) && pos->dragging) {
		gdk_pointer_ungrab (event->time);
		pos->dragging = FALSE;
		gtk_signal_emit (GTK_OBJECT (pos), position_signals[RELEASED]);
		if (((pos->position == 0) && ((pos->pold.x != pos->p0.x) || (pos->pold.y != pos->p0.y))) ||
		    ((pos->position == 1) && ((pos->pold.x != pos->p1.x) || (pos->pold.y != pos->p1.y)))) {
			gtk_signal_emit (GTK_OBJECT (pos), position_signals[CHANGED]);
		}
	}

	return FALSE;
}

static gint
sp_gradient_position_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
	SPGradientPosition *pos;

	pos = SP_GRADIENT_POSITION (widget);

	if (pos->dragging) {
		if (pos->position == 0) {
			gdouble x, y;
			/* Dragging start */
			/* Calculate mouse coordinates in bbox */
			x = (event->x - pos->vbox.x0) / (pos->vbox.x1 - pos->vbox.x0);
			y = (event->y - pos->vbox.y0) / (pos->vbox.y1 - pos->vbox.y0);
			pos->p0.x = x;
			pos->p0.y = y;
			pos->need_update = TRUE;
			if (GTK_WIDGET_DRAWABLE (pos)) {
				gtk_widget_queue_draw (GTK_WIDGET (pos));
			}
			gtk_signal_emit (GTK_OBJECT (pos), position_signals[DRAGGED]);
		} else if (pos->position == 1) {
			gdouble x, y;
			/* Dragging start */
			/* Calculate mouse coordinates in bbox */
			x = (event->x - pos->vbox.x0) / (pos->vbox.x1 - pos->vbox.x0);
			y = (event->y - pos->vbox.y0) / (pos->vbox.y1 - pos->vbox.y0);
			pos->p1.x = x;
			pos->p1.y = y;
			pos->need_update = TRUE;
			if (GTK_WIDGET_DRAWABLE (pos)) {
				gtk_widget_queue_draw (GTK_WIDGET (pos));
			}
			gtk_signal_emit (GTK_OBJECT (pos), position_signals[DRAGGED]);
		}
	}

	return FALSE;
}

GtkWidget *
sp_gradient_position_new (SPGradient *gradient)
{
	SPGradientPosition *position;

	position = gtk_type_new (SP_TYPE_GRADIENT_POSITION);

	sp_gradient_position_set_gradient (position, gradient);

	return (GtkWidget *) position;
}

void
sp_gradient_position_set_gradient (SPGradientPosition *pos, SPGradient *gradient)
{
	if (pos->gradient) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (pos->gradient), pos);
	}

	pos->gradient = gradient;

	if (gradient) {
		gtk_signal_connect (GTK_OBJECT (gradient), "destroy",
				    GTK_SIGNAL_FUNC (sp_gradient_position_gradient_destroy), pos);
		gtk_signal_connect (GTK_OBJECT (gradient), "modified",
				    GTK_SIGNAL_FUNC (sp_gradient_position_gradient_modified), pos);
	}

	pos->need_update = TRUE;
	if (GTK_WIDGET_DRAWABLE (pos)) {
		gtk_widget_queue_draw (GTK_WIDGET (pos));
	}
}

static void
sp_gradient_position_gradient_destroy (SPGradient *gradient, SPGradientPosition *pos)
{
	sp_gradient_position_set_gradient (pos, NULL);
}

static void
sp_gradient_position_gradient_modified (SPGradient *gradient, guint flags, SPGradientPosition *pos)
{
	pos->need_update = TRUE;
	if (GTK_WIDGET_DRAWABLE (pos)) {
		gtk_widget_queue_draw (GTK_WIDGET (pos));
	}
}

void
sp_gradient_position_set_bbox (SPGradientPosition *pos, gdouble x0, gdouble y0, gdouble x1, gdouble y1)
{
	pos->bbox.x0 = x0;
	pos->bbox.y0 = y0;
	pos->bbox.x1 = x1;
	pos->bbox.y1 = y1;

	pos->need_update = TRUE;
	if (GTK_WIDGET_DRAWABLE (pos)) {
		gtk_widget_queue_draw (GTK_WIDGET (pos));
	}
}

void
sp_gradient_position_set_vector (SPGradientPosition *pos, gdouble x0, gdouble y0, gdouble x1, gdouble y1)
{
	pos->p0.x = x0;
	pos->p0.y = y0;
	pos->p1.x = x1;
	pos->p1.y = y1;

	pos->need_update = TRUE;
	if (GTK_WIDGET_DRAWABLE (pos)) {
		gtk_widget_queue_draw (GTK_WIDGET (pos));
	}
}

void
sp_gradient_position_set_transform (SPGradientPosition *pos, gdouble transform[])
{
	memcpy (pos->transform, transform, 6 * sizeof (gdouble));

	pos->need_update = TRUE;
	if (GTK_WIDGET_DRAWABLE (pos)) {
		gtk_widget_queue_draw (GTK_WIDGET (pos));
	}
}

static void
sp_gradient_position_update (SPGradientPosition *pos)
{
	GtkWidget *widget;
	gint x, y;
	guchar *p;
	gdouble xs, ys, xp, yp;
	ArtDRect bbox;
	gdouble id[6];
	gint x0, y0, x1, y1;
	guchar *t;

	widget = GTK_WIDGET (pos);

	pos->need_update = FALSE;

	/* Create image data */
	if (!pos->rgba) pos->rgba = g_new (guchar, 4 * widget->allocation.height * widget->allocation.width);
	if (!pos->rgb) pos->rgb = g_new (guchar, 3 * widget->allocation.height * widget->allocation.width);
	if (!pos->px) pos->px = gdk_pixmap_new (widget->window, widget->allocation.width, widget->allocation.height, -1);
	if (!pos->gc) pos->gc = gdk_gc_new (widget->window);
	g_assert (!pos->painter);

	if (!pos->gradient ||
	    (pos->bbox.x0 >= pos->bbox.x1) ||
	    (pos->bbox.y0 >= pos->bbox.y1) ||
	    (widget->allocation.width < 1) ||
	    (widget->allocation.height < 1)) {
		/* Draw empty thing */
		nr_gdk_draw_gray_garbage (pos->px, pos->gc, 0, 0, widget->allocation.width, widget->allocation.height);
		return;
	}

	/* Calculate bbox */
	xs = widget->allocation.width / (pos->bbox.x1 - pos->bbox.x0);
	ys = widget->allocation.height / (pos->bbox.y1 - pos->bbox.y0);
	if (xs > ys) {
		yp = 0.0;
		xp = (widget->allocation.width - widget->allocation.width * ys / xs) / 2;
		xs = ys;
	} else if (xs < ys) {
		xp = 0.0;
		yp = (widget->allocation.height - widget->allocation.height * xs / ys) / 2;
		ys = xs;
	} else {
		xp = 0.0;
		yp = 0.0;
	}
	pos->vbox.x0 = xp;
	pos->vbox.y0 = yp;
	pos->vbox.x1 = widget->allocation.width - pos->vbox.x0;
	pos->vbox.y1 = widget->allocation.height - pos->vbox.y0;
	/* Draw checkerboard */
	/* fixme: Get rid of uint32 stuff */
	nr_render_checkerboard_rgb (pos->rgb, widget->allocation.width, widget->allocation.height, 3 * widget->allocation.width, 0, 0);
	/* Create painter */
	art_affine_identity (id);
	bbox.x0 = xp;
	bbox.y0 = yp;
	bbox.x1 = widget->allocation.width - xp;
	bbox.y1 = widget->allocation.height - yp;
	pos->painter = sp_paint_server_painter_new (SP_PAINT_SERVER (pos->gradient), id, 1.0, &bbox);
	/* Paint rgb buffer */
	pos->painter->fill (pos->painter, pos->rgba, 0, 0, widget->allocation.width, widget->allocation.height, 4 * widget->allocation.width);
	pos->painter = sp_painter_free (pos->painter);
	for (y = 0; y < widget->allocation.height; y++) {
		t = pos->rgba + 4 * y * widget->allocation.width;
		p = pos->rgb + 3 * y * widget->allocation.width;
		for (x = 0; x < widget->allocation.width; x++) {
			guint a, fc;
			a = t[3];
			fc = (t[0] - *p) * a;
			p[0] = *p + ((fc + (fc >> 8) + 0x80) >> 8);
			fc = (t[1] - *p) * a;
			p[1] = *p + ((fc + (fc >> 8) + 0x80) >> 8);
			fc = (t[2] - *p) * a;
			p[2] = *p + ((fc + (fc >> 8) + 0x80) >> 8);
			p += 3;
			t += 4;
		}
	}
	/* Draw pixmap */
	gdk_gc_set_function (pos->gc, GDK_COPY);
	gdk_draw_rgb_image (pos->px, pos->gc,
			    0, 0,
			    widget->allocation.width, widget->allocation.height,
			    GDK_RGB_DITHER_MAX,
			    pos->rgb, widget->allocation.width * 3);
	/* Draw start */
	gdk_gc_set_function (pos->gc, GDK_INVERT);
	x0 = bbox.x0 + pos->p0.x * (bbox.x1 - bbox.x0);
	y0 = bbox.y0 + pos->p0.y * (bbox.y1 - bbox.y0);
	gdk_draw_arc (pos->px, pos->gc, FALSE, x0 - RADIUS, y0 - RADIUS, 2 * RADIUS + 1, 2 * RADIUS + 1, 0, 35999);
	/* Draw end */
	x1 = bbox.x0 + pos->p1.x * (bbox.x1 - bbox.x0);
	y1 = bbox.y0 + pos->p1.y * (bbox.y1 - bbox.y0);
	gdk_draw_arc (pos->px, pos->gc, FALSE, x1 - RADIUS, y1 - RADIUS, 2 * RADIUS + 1, 2 * RADIUS + 1, 0, 35999);
	/* Draw line */
	if (((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0)) > (RADIUS * RADIUS)) {
		gdouble dx, dy, len;
		dx = x1 - x0;
		dy = y1 - y0;
		len = sqrt (dx * dx + dy * dy);
		x0 += dx * RADIUS / len;
		y0 += dy * RADIUS / len;
		x1 -= dx * RADIUS / len;
		y1 -= dy * RADIUS / len;
		gdk_draw_line (pos->px, pos->gc, x0, y0, x1, y1);
	}
	/* Draw bbox */
	gdk_draw_rectangle (pos->px, pos->gc, FALSE,
			    floor (xp), floor (yp), floor (xs * (pos->bbox.x1 - pos->bbox.x0)), floor (ys * (pos->bbox.y1 - pos->bbox.y0)));
}

static void
sp_gradient_position_free_image_data (SPGradientPosition *pos)
{
	if (pos->rgba) {
		g_free (pos->rgba);
		pos->rgba = NULL;
	}
	if (pos->rgb) {
		g_free (pos->rgb);
		pos->rgb = NULL;
	}
	if (pos->px) {
		gdk_pixmap_unref (pos->px);
		pos->px = NULL;
	}
	if (pos->gc) {
		gdk_gc_unref (pos->gc);
		pos->gc = NULL;
	}
	if (pos->painter) pos->painter = sp_painter_free (pos->painter);
}


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

static void sp_gradient_position_paint (GtkWidget *widget, GdkRectangle *area);
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
	pos->bbox.x0 = pos->bbox.y0 = pos->bbox.x1 = pos->bbox.y1 = 0.0;
	pos->start.x = pos->start.y = 0.0;
	pos->end.x = pos->end.y = 1.0;
	pos->p0.x = pos->p0.y = 0.0;
	pos->p1.x = pos->p1.y = 1.0;
	art_affine_identity (pos->transform);
	pos->spread = NR_GRADIENT_SPREAD_PAD;

	pos->gc = NULL;
	pos->px = NULL;
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
		sp_gradient_position_paint (widget, area);
	}
}

static gint
sp_gradient_position_expose (GtkWidget *widget, GdkEventExpose *event)
{
	SPGradientPosition *pos;

	pos = SP_GRADIENT_POSITION (widget);

	if (GTK_WIDGET_DRAWABLE (widget)) {
		sp_gradient_position_paint (widget, &event->area);
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
				pos->end.x = pos->bbox.x0 + x * (pos->bbox.x1 - pos->bbox.x0);
				pos->end.y = pos->bbox.y0 + y * (pos->bbox.y1 - pos->bbox.y0);
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
			/* Grab start */
			pos->dragging = TRUE;
			pos->position = 0;
			pos->pold = pos->p0;
		} else {
			/* Grab end */
			pos->dragging = TRUE;
			pos->position = 1;
			pos->pold = pos->p1;
		}
		gtk_signal_emit (GTK_OBJECT (pos), position_signals[GRABBED]);
		/* Calculate mouse coordinates in bbox */
		x = (event->x - pos->vbox.x0) / (pos->vbox.x1 - pos->vbox.x0);
		y = (event->y - pos->vbox.y0) / (pos->vbox.y1 - pos->vbox.y0);
		if ((x != pos->p0.x) || (y != pos->p0.y)) {
			/* Write coords and emit "dragged" */
			pos->p0.x = x;
			pos->p0.y = y;
			pos->start.x = pos->bbox.x0 + x * (pos->bbox.x1 - pos->bbox.x0);
			pos->start.y = pos->bbox.y0 + y * (pos->bbox.y1 - pos->bbox.y0);
			pos->need_update = TRUE;
			if (GTK_WIDGET_DRAWABLE (pos)) {
				gtk_widget_queue_draw (GTK_WIDGET (pos));
			}
			gtk_signal_emit (GTK_OBJECT (pos), position_signals[DRAGGED]);
		}
		gtk_signal_emit (GTK_OBJECT (pos), position_signals[GRABBED]);
		gdk_pointer_grab (widget->window, FALSE,
				  GDK_POINTER_MOTION_MASK |
				  GDK_BUTTON_RELEASE_MASK,
				  NULL, NULL, event->time);
		return FALSE;
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
			pos->start.x = pos->bbox.x0 + x * (pos->bbox.x1 - pos->bbox.x0);
			pos->start.y = pos->bbox.y0 + y * (pos->bbox.y1 - pos->bbox.y0);
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
			pos->end.x = pos->bbox.x0 + x * (pos->bbox.x1 - pos->bbox.x0);
			pos->end.y = pos->bbox.y0 + y * (pos->bbox.y1 - pos->bbox.y0);
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
sp_gradient_position_set_mode (SPGradientPosition *pos, guint mode)
{
	pos->mode = mode;

	pos->need_update = TRUE;
	if (GTK_WIDGET_DRAWABLE (pos)) gtk_widget_queue_draw (GTK_WIDGET (pos));
}

void
sp_gradient_position_set_bbox (SPGradientPosition *pos, gdouble x0, gdouble y0, gdouble x1, gdouble y1)
{
	g_return_if_fail (x1 > x0);
	g_return_if_fail (y1 > y0);

	pos->bbox.x0 = x0;
	pos->bbox.y0 = y0;
	pos->bbox.x1 = x1;
	pos->bbox.y1 = y1;

	pos->p0.x = (pos->start.x - x0) / (x1 - x0);
	pos->p0.y = (pos->start.y - y0) / (y1 - y0);
	pos->p1.x = (pos->end.x - x0) / (x1 - x0);
	pos->p1.y = (pos->end.y - y0) / (y1 - y0);

	pos->need_update = TRUE;
	if (GTK_WIDGET_DRAWABLE (pos)) gtk_widget_queue_draw (GTK_WIDGET (pos));
}

void
sp_gradient_position_set_vector (SPGradientPosition *pos, gdouble x0, gdouble y0, gdouble x1, gdouble y1)
{
	pos->start.x = x0;
	pos->start.y = y0;
	pos->end.x = x1;
	pos->end.y = y1;

	pos->p0.x = (pos->start.x - pos->bbox.x0) / (pos->bbox.x1 - pos->bbox.x0);
	pos->p0.y = (pos->start.y - pos->bbox.y0) / (pos->bbox.y1 - pos->bbox.y0);
	pos->p1.x = (pos->end.x - pos->bbox.x0) / (pos->bbox.x1 - pos->bbox.x0);
	pos->p1.y = (pos->end.y - pos->bbox.y0) / (pos->bbox.y1 - pos->bbox.y0);

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

void
sp_gradient_position_set_spread (SPGradientPosition *pos, unsigned int spread)
{
	if (spread != pos->spread) {
		pos->spread = spread;
		pos->need_update = TRUE;
		if (GTK_WIDGET_DRAWABLE (pos)) {
			gtk_widget_queue_draw (GTK_WIDGET (pos));
		}
	}
}

void
sp_gradient_position_get_position_floatv (SPGradientPosition *gp, gfloat *pos)
{
	pos[0] = gp->start.x;
	pos[1] = gp->start.y;
	pos[2] = gp->end.x;
	pos[3] = gp->end.y;
}

static void
sp_gradient_position_update (SPGradientPosition *pos)
{
	GtkWidget *widget;
	gdouble xs, ys, xp, yp;
	gdouble bb2d[6], n2d[6];
	NRMatrixF v2px;

	widget = GTK_WIDGET (pos);

	pos->need_update = FALSE;

	/* Create image data */
	if (!pos->px) pos->px = gdk_pixmap_new (widget->window, 64, 64, -1);
	if (!pos->gc) pos->gc = gdk_gc_new (widget->window);

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

	if (!pos->cv) pos->cv = g_new (guchar, 4 * NR_GRADIENT_VECTOR_LENGTH);
	sp_gradient_render_vector_line_rgba (pos->gradient, pos->cv, NR_GRADIENT_VECTOR_LENGTH, 0 , NR_GRADIENT_VECTOR_LENGTH);

	/* BBox -> buffer */
	bb2d[0] = pos->vbox.x1 - pos->vbox.x0;
	bb2d[1] = 0.0;
	bb2d[2] = 0.0;
	bb2d[3] = pos->vbox.y1 - pos->vbox.y0;
	bb2d[4] = pos->vbox.x0;
	bb2d[5] = pos->vbox.y0;

	art_affine_multiply (n2d, pos->transform, bb2d);

	v2px.c[0] = n2d[0];
	v2px.c[1] = n2d[1];
	v2px.c[2] = n2d[2];
	v2px.c[3] = n2d[3];
	v2px.c[4] = n2d[4];
	v2px.c[5] = n2d[5];

	if (pos->mode == SP_GRADIENT_POSITION_MODE_LINEAR) {
		nr_lgradient_renderer_setup (&pos->renderer.lgr, pos->cv, pos->spread, &v2px, pos->p0.x, pos->p0.y, pos->p1.x, pos->p1.y);
	} else {
		/* fixme: This is radial renderer */
		nr_rgradient_renderer_setup (&pos->renderer.rgr, pos->cv, pos->spread, &v2px,
					     pos->p0.x, pos->p0.y, pos->p0.x, pos->p0.y,
					     hypot (pos->p1.x - pos->p0.x, pos->p1.y - pos->p0.y));
	}
}

static void
sp_gradient_position_free_image_data (SPGradientPosition *pos)
{
	if (pos->px) {
		gdk_pixmap_unref (pos->px);
		pos->px = NULL;
	}
	if (pos->gc) {
		gdk_gc_unref (pos->gc);
		pos->gc = NULL;
	}
	if (pos->cv) {
		g_free (pos->cv);
		pos->cv = NULL;
	}
}

static void
sp_gradient_position_paint (GtkWidget *widget, GdkRectangle *area)
{
	static guchar *rgb = NULL;
	SPGradientPosition *gp;
	gint x, y;

	gp = SP_GRADIENT_POSITION (widget);

	if (!gp->gradient ||
	    (gp->bbox.x0 >= gp->bbox.x1) ||
	    (gp->bbox.y0 >= gp->bbox.y1) ||
	    (widget->allocation.width < 1) ||
	    (widget->allocation.height < 1)) {
		/* Draw empty thing */
		if (!gp->gc) gp->gc = gdk_gc_new (widget->window);
		nr_gdk_draw_gray_garbage (widget->window, gp->gc, area->x, area->y, area->width, area->height);
		return;
	}

	if (gp->need_update) {
		sp_gradient_position_update (gp);
	}

	if (!rgb) rgb = g_new (guchar, 64 * 64 * 3);

	for (y = area->y; y < area->y + area->height; y += 64) {
		for (x = area->x; x < area->x + area->width; x += 64) {
			gint x0, y0, x1, y1, w, h;
			NRPixBlock pb;

			w = MIN (x + 64, area->x + area->width) - x;
			h = MIN (y + 64, area->y + area->height) - y;

			/* Draw checkerboard */
			nr_render_checkerboard_rgb (rgb, w, h, 3 * w, x, y);
			/* Set up pixblock */
			nr_pixblock_setup_extern (&pb, NR_PIXBLOCK_MODE_R8G8B8, x, y, x + w, y + h, rgb, 3 * w, FALSE, FALSE);

			/* fixme: fimxe: fixme: Use generic renderer */

			/* Render gradient */
			nr_lgradient_render (&gp->renderer.lgr, &pb);

			/* Draw pixmap */
			gdk_gc_set_function (gp->gc, GDK_COPY);
			gdk_draw_rgb_image (gp->px, gp->gc, 0, 0, w, h, GDK_RGB_DITHER_MAX, rgb, 3 * w);

			/* Release pixblock */
			nr_pixblock_release (&pb);

			/* Draw start */
			gdk_gc_set_function (gp->gc, GDK_INVERT);
			x0 = gp->vbox.x0 + gp->p0.x * (gp->vbox.x1 - gp->vbox.x0) - x;
			y0 = gp->vbox.y0 + gp->p0.y * (gp->vbox.y1 - gp->vbox.y0) - y;
			gdk_draw_arc (gp->px, gp->gc, FALSE, x0 - RADIUS, y0 - RADIUS, 2 * RADIUS + 1, 2 * RADIUS + 1, 0, 35999);
			/* Draw end */
			x1 = gp->vbox.x0 + gp->p1.x * (gp->vbox.x1 - gp->vbox.x0) - x;
			y1 = gp->vbox.y0 + gp->p1.y * (gp->vbox.y1 - gp->vbox.y0) - y;
			gdk_draw_arc (gp->px, gp->gc, FALSE, x1 - RADIUS, y1 - RADIUS, 2 * RADIUS + 1, 2 * RADIUS + 1, 0, 35999);
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
				gdk_draw_line (gp->px, gp->gc, x0, y0, x1, y1);
			}
			/* Draw bbox */
			gdk_draw_rectangle (gp->px, gp->gc, FALSE,
					    gp->vbox.x0 - x, gp->vbox.y0 - y,
					    gp->vbox.x1 - gp->vbox.x0, gp->vbox.y1 - gp->vbox.y0);
			/* Copy to window */
			gdk_gc_set_function (gp->gc, GDK_COPY);
			gdk_draw_pixmap (widget->window, gp->gc,
					 gp->px, 0, 0, x, y, w, h);
		}
	}
}

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
#include <libnr/nr-matrix.h>
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

	if (pos->mode == SP_GRADIENT_POSITION_MODE_LINEAR) {
		/* Linear mode */
		if (event->button == 1) {
			float x1, y1;
			pos->dragging = TRUE;
			pos->changed = FALSE;
			gtk_signal_emit (GTK_OBJECT (pos), position_signals[GRABBED]);
			x1 = NR_MATRIX_DF_TRANSFORM_X (&pos->w2gs, event->x, event->y);
			y1 = NR_MATRIX_DF_TRANSFORM_Y (&pos->w2gs, event->x, event->y);
			if (!NR_DF_TEST_CLOSE (x1, pos->gdata.linear.x1, NR_EPSILON_F) ||
			    !NR_DF_TEST_CLOSE (y1, pos->gdata.linear.y1, NR_EPSILON_F)) {
				pos->gdata.linear.x1 = x1;
				pos->gdata.linear.y1 = y1;
				gtk_signal_emit (GTK_OBJECT (pos), position_signals[DRAGGED]);
				pos->changed = TRUE;
				pos->need_update = TRUE;
				if (GTK_WIDGET_DRAWABLE (pos)) gtk_widget_queue_draw (GTK_WIDGET (pos));
			}
			gdk_pointer_grab (widget->window, FALSE, GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK, NULL, NULL, event->time);
		}
	} else {
		/* Radial mode */
		if (event->button == 1) {
			float cx, cy;
			pos->dragging = TRUE;
			pos->changed = FALSE;
			gtk_signal_emit (GTK_OBJECT (pos), position_signals[GRABBED]);
			cx = NR_MATRIX_DF_TRANSFORM_X (&pos->w2gs, event->x, event->y);
			cy = NR_MATRIX_DF_TRANSFORM_Y (&pos->w2gs, event->x, event->y);
			if (!NR_DF_TEST_CLOSE (cx, pos->gdata.radial.cx, NR_EPSILON_F) ||
			    !NR_DF_TEST_CLOSE (cy, pos->gdata.radial.cy, NR_EPSILON_F)) {
				pos->gdata.radial.cx = cx;
				pos->gdata.radial.cy = cy;
				pos->gdata.radial.fx = cx;
				pos->gdata.radial.fy = cy;
				gtk_signal_emit (GTK_OBJECT (pos), position_signals[DRAGGED]);
				pos->changed = TRUE;
				pos->need_update = TRUE;
				if (GTK_WIDGET_DRAWABLE (pos)) gtk_widget_queue_draw (GTK_WIDGET (pos));
			}
			gdk_pointer_grab (widget->window, FALSE, GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK, NULL, NULL, event->time);
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
		gtk_signal_emit (GTK_OBJECT (pos), position_signals[RELEASED]);
		if (pos->changed) {
			gtk_signal_emit (GTK_OBJECT (pos), position_signals[CHANGED]);
			pos->changed = FALSE;
		}
		gdk_pointer_ungrab (event->time);
		pos->dragging = FALSE;
	}

	return FALSE;
}

static gint
sp_gradient_position_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
	SPGradientPosition *pos;

	pos = SP_GRADIENT_POSITION (widget);

	if (pos->mode == SP_GRADIENT_POSITION_MODE_LINEAR) {
		if (pos->dragging) {
			pos->gdata.linear.x2 = NR_MATRIX_DF_TRANSFORM_X (&pos->w2gs, event->x, event->y);
			pos->gdata.linear.y2 = NR_MATRIX_DF_TRANSFORM_Y (&pos->w2gs, event->x, event->y);
			gtk_signal_emit (GTK_OBJECT (pos), position_signals[DRAGGED]);
			pos->changed = TRUE;
			pos->need_update = TRUE;
			if (GTK_WIDGET_DRAWABLE (pos)) gtk_widget_queue_draw (GTK_WIDGET (pos));
		}
	} else {
		if (pos->dragging) {
			float x, y;
			x = NR_MATRIX_DF_TRANSFORM_X (&pos->w2gs, event->x, event->y);
			y = NR_MATRIX_DF_TRANSFORM_Y (&pos->w2gs, event->x, event->y);
			pos->gdata.radial.r = hypot (x - pos->gdata.radial.cx, y - pos->gdata.radial.cy);
			gtk_signal_emit (GTK_OBJECT (pos), position_signals[DRAGGED]);
			pos->changed = TRUE;
			pos->need_update = TRUE;
			if (GTK_WIDGET_DRAWABLE (pos)) gtk_widget_queue_draw (GTK_WIDGET (pos));
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

	pos->need_update = TRUE;
	if (GTK_WIDGET_DRAWABLE (pos)) gtk_widget_queue_draw (GTK_WIDGET (pos));
}

void
sp_gradient_position_set_gs2d_matrix_f (SPGradientPosition *pos, NRMatrixF *gs2d)
{
	pos->gs2d = *gs2d;

	pos->need_update = TRUE;
	if (GTK_WIDGET_DRAWABLE (pos)) gtk_widget_queue_draw (GTK_WIDGET (pos));
}

void
sp_gradient_position_get_gs2d_matrix_f (SPGradientPosition *pos, NRMatrixF *gs2d)
{
	*gs2d = pos->gs2d;
}

void
sp_gradient_position_set_linear_position (SPGradientPosition *pos, float x1, float y1, float x2, float y2)
{
	pos->gdata.linear.x1 = x1;
	pos->gdata.linear.y1 = y1;
	pos->gdata.linear.x2 = x2;
	pos->gdata.linear.y2 = y2;

	pos->need_update = TRUE;
	if (GTK_WIDGET_DRAWABLE (pos)) gtk_widget_queue_draw (GTK_WIDGET (pos));
}

void
sp_gradient_position_set_radial_position (SPGradientPosition *pos, float cx, float cy, float fx, float fy, float r)
{
	pos->gdata.radial.cx = cx;
	pos->gdata.radial.cy = cy;
	pos->gdata.radial.fx = fx;
	pos->gdata.radial.fy = fy;
	pos->gdata.radial.r = r;

	pos->need_update = TRUE;
	if (GTK_WIDGET_DRAWABLE (pos)) gtk_widget_queue_draw (GTK_WIDGET (pos));
}

void
sp_gradient_position_set_spread (SPGradientPosition *pos, unsigned int spread)
{
	if (spread != pos->spread) {
		pos->spread = spread;
		pos->need_update = TRUE;
		if (GTK_WIDGET_DRAWABLE (pos)) gtk_widget_queue_draw (GTK_WIDGET (pos));
	}
}

void
sp_gradient_position_get_linear_position_floatv (SPGradientPosition *gp, float *pos)
{
	pos[0] = gp->gdata.linear.x1;
	pos[1] = gp->gdata.linear.y1;
	pos[2] = gp->gdata.linear.x2;
	pos[3] = gp->gdata.linear.y2;
}

void
sp_gradient_position_get_radial_position_floatv (SPGradientPosition *gp, float *pos)
{
	pos[0] = gp->gdata.radial.cx;
	pos[1] = gp->gdata.radial.cy;
	pos[2] = gp->gdata.radial.fx;
	pos[3] = gp->gdata.radial.fy;
	pos[4] = gp->gdata.radial.r;
}

static void
sp_gradient_position_update (SPGradientPosition *pos)
{
	GtkWidget *widget;
	int width, height;
	gdouble xs, ys;

	widget = GTK_WIDGET (pos);
	width = widget->allocation.width;
	height = widget->allocation.height;

	pos->need_update = FALSE;

	/* Create image data */
	if (!pos->px) pos->px = gdk_pixmap_new (widget->window, 64, 64, -1);
	if (!pos->gc) pos->gc = gdk_gc_new (widget->window);

	/* Calculate bbox */
	xs = width / (pos->bbox.x1 - pos->bbox.x0);
	ys = height / (pos->bbox.y1 - pos->bbox.y0);

	if (xs > ys) {
		pos->vbox.x0 = (short) floor (width * (1 - ys / xs) / 2.0);
		pos->vbox.y0 = 0;
	} else if (xs < ys) {
		pos->vbox.x0 = 0;
		pos->vbox.y0 = (short) floor (height * (1 - xs / ys) / 2.0);
	} else {
		pos->vbox.x0 = 0;
		pos->vbox.y0 = 0;
	}
	pos->vbox.x1 = widget->allocation.width - pos->vbox.x0;
	pos->vbox.y1 = widget->allocation.height - pos->vbox.y0;

	/* Calculate w2d */
	pos->w2d.c[0] = (pos->bbox.x1 - pos->bbox.x0) / (pos->vbox.x1 - pos->vbox.x0);
	pos->w2d.c[1] = 0.0;
	pos->w2d.c[2] = 0.0;
	pos->w2d.c[3] = (pos->bbox.y1 - pos->bbox.y0) / (pos->vbox.y1 - pos->vbox.y0);
	pos->w2d.c[4] = pos->bbox.x0 - (pos->vbox.x0 * pos->w2d.c[0]);
	pos->w2d.c[5] = pos->bbox.y0 - (pos->vbox.y0 * pos->w2d.c[3]);
	/* Calculate d2w */
	nr_matrix_f_invert (&pos->d2w, &pos->w2d);
	/* Calculate wbox */
	pos->wbox.x0 = pos->w2d.c[4];
	pos->wbox.x0 = pos->w2d.c[5];
	pos->wbox.x1 = pos->wbox.x0 + pos->w2d.c[0] * width;
	pos->wbox.y1 = pos->wbox.y0 + pos->w2d.c[1] * height;
	/* w2gs and gs2w */
	nr_matrix_multiply_fff (&pos->gs2w, &pos->gs2d, &pos->d2w);
	nr_matrix_f_invert (&pos->w2gs, &pos->gs2w);

	if (!pos->cv) pos->cv = g_new (guchar, 4 * NR_GRADIENT_VECTOR_LENGTH);
	sp_gradient_render_vector_line_rgba (pos->gradient, pos->cv, NR_GRADIENT_VECTOR_LENGTH, 0 , NR_GRADIENT_VECTOR_LENGTH);

	if (pos->mode == SP_GRADIENT_POSITION_MODE_LINEAR) {
		nr_lgradient_renderer_setup (&pos->renderer.lgr, pos->cv, pos->spread, &pos->gs2w,
					     pos->gdata.linear.x1, pos->gdata.linear.y1,
					     pos->gdata.linear.x2, pos->gdata.linear.y2);
	} else {
		nr_rgradient_renderer_setup (&pos->renderer.rgr, pos->cv, pos->spread, &pos->gs2w,
					     pos->gdata.radial.cx, pos->gdata.radial.cy,
					     pos->gdata.radial.fx, pos->gdata.radial.fy,
					     pos->gdata.radial.r);
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
			if (gp->mode == SP_GRADIENT_POSITION_MODE_LINEAR) {
				/* Render gradient */
				nr_lgradient_render (&gp->renderer.lgr, &pb);
			} else {
				/* Render gradient */
				nr_rgradient_render (&gp->renderer.rgr, &pb);
			}
			/* Draw pixmap */
			gdk_gc_set_function (gp->gc, GDK_COPY);
			gdk_draw_rgb_image (gp->px, gp->gc, 0, 0, w, h, GDK_RGB_DITHER_MAX, rgb, 3 * w);
			/* Release pixblock */
			nr_pixblock_release (&pb);

			if (gp->mode == SP_GRADIENT_POSITION_MODE_LINEAR) {
				/* Draw start */
				gdk_gc_set_function (gp->gc, GDK_INVERT);
				x0 = (short) floor (NR_MATRIX_DF_TRANSFORM_X (&gp->gs2w, gp->gdata.linear.x1, gp->gdata.linear.y1) + 0.5);
				y0 = (short) floor (NR_MATRIX_DF_TRANSFORM_Y (&gp->gs2w, gp->gdata.linear.x1, gp->gdata.linear.y1) + 0.5);
				gdk_draw_arc (gp->px, gp->gc, FALSE, x0 - x - RADIUS, y0 - y - RADIUS, 2 * RADIUS + 1, 2 * RADIUS + 1, 0, 35999);
				/* Draw end */
				x1 = (short) floor (NR_MATRIX_DF_TRANSFORM_X (&gp->gs2w, gp->gdata.linear.x2, gp->gdata.linear.y2) + 0.5);
				y1 = (short) floor (NR_MATRIX_DF_TRANSFORM_Y (&gp->gs2w, gp->gdata.linear.x2, gp->gdata.linear.y2) + 0.5);
				gdk_draw_arc (gp->px, gp->gc, FALSE, x1 - x - RADIUS, y1 - y - RADIUS, 2 * RADIUS + 1, 2 * RADIUS + 1, 0, 35999);
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
					gdk_draw_line (gp->px, gp->gc, x0 - x, y0 - y, x1 - x, y1 - y);
				}
			} else {
				short cx, cy, fx, fy;
				NRPointF p0, p1, p2, p3;
				/* Draw center */
				gdk_gc_set_function (gp->gc, GDK_INVERT);
				cx = (short) floor (NR_MATRIX_DF_TRANSFORM_X (&gp->gs2w, gp->gdata.radial.cx, gp->gdata.radial.cy) + 0.5);
				cy = (short) floor (NR_MATRIX_DF_TRANSFORM_Y (&gp->gs2w, gp->gdata.radial.cx, gp->gdata.radial.cy) + 0.5);
				gdk_draw_arc (gp->px, gp->gc, FALSE,
					      cx - x - RADIUS, cy - y - RADIUS,
					      2 * RADIUS + 1, 2 * RADIUS + 1, 0, 35999);
				fx = (short) floor (NR_MATRIX_DF_TRANSFORM_X (&gp->gs2w, gp->gdata.radial.fx, gp->gdata.radial.fy) + 0.5);
				fy = (short) floor (NR_MATRIX_DF_TRANSFORM_Y (&gp->gs2w, gp->gdata.radial.fx, gp->gdata.radial.fy) + 0.5);
				if ((fx != cx) || (fy != cy)) {
					gdk_draw_arc (gp->px, gp->gc, FALSE,
						      fx - x - RADIUS, fy - y - RADIUS,
						      2 * RADIUS + 1, 2 * RADIUS + 1, 0, 35999);
				}
				p0.x = (short) floor (NR_MATRIX_DF_TRANSFORM_X (&gp->gs2w,
										gp->gdata.radial.cx - gp->gdata.radial.r,
										gp->gdata.radial.cy - gp->gdata.radial.r) + 0.5);
				p0.y = (short) floor (NR_MATRIX_DF_TRANSFORM_Y (&gp->gs2w,
										gp->gdata.radial.cx - gp->gdata.radial.r,
										gp->gdata.radial.cy - gp->gdata.radial.r) + 0.5);
				p1.x = (short) floor (NR_MATRIX_DF_TRANSFORM_X (&gp->gs2w,
										gp->gdata.radial.cx + gp->gdata.radial.r,
										gp->gdata.radial.cy - gp->gdata.radial.r) + 0.5);
				p1.y = (short) floor (NR_MATRIX_DF_TRANSFORM_Y (&gp->gs2w,
										gp->gdata.radial.cx + gp->gdata.radial.r,
										gp->gdata.radial.cy - gp->gdata.radial.r) + 0.5);
				p2.x = (short) floor (NR_MATRIX_DF_TRANSFORM_X (&gp->gs2w,
										gp->gdata.radial.cx + gp->gdata.radial.r,
										gp->gdata.radial.cy + gp->gdata.radial.r) + 0.5);
				p2.y = (short) floor (NR_MATRIX_DF_TRANSFORM_Y (&gp->gs2w,
										gp->gdata.radial.cx + gp->gdata.radial.r,
										gp->gdata.radial.cy + gp->gdata.radial.r) + 0.5);
				p3.x = (short) floor (NR_MATRIX_DF_TRANSFORM_X (&gp->gs2w,
										gp->gdata.radial.cx - gp->gdata.radial.r,
										gp->gdata.radial.cy + gp->gdata.radial.r) + 0.5);
				p3.y = (short) floor (NR_MATRIX_DF_TRANSFORM_Y (&gp->gs2w,
										gp->gdata.radial.cx - gp->gdata.radial.r,
										gp->gdata.radial.cy + gp->gdata.radial.r) + 0.5);
				gdk_draw_line (gp->px, gp->gc, p0.x - x, p0.y - y, p1.x - x, p1.y - y);
				gdk_draw_line (gp->px, gp->gc, p1.x - x, p1.y - y, p2.x - x, p2.y - y);
				gdk_draw_line (gp->px, gp->gc, p2.x - x, p2.y - y, p3.x - x, p3.y - y);
				gdk_draw_line (gp->px, gp->gc, p3.x - x, p3.y - y, p0.x - x, p0.y - y);
			}
			/* Draw bbox */
			gdk_draw_rectangle (gp->px, gp->gc, FALSE,
					    gp->vbox.x0 - x, gp->vbox.y0 - y,
					    gp->vbox.x1 - gp->vbox.x0 - 1, gp->vbox.y1 - gp->vbox.y0 - 1);
			/* Copy to window */
			gdk_gc_set_function (gp->gc, GDK_COPY);
			gdk_draw_pixmap (widget->window, gp->gc, gp->px, 0, 0, x, y, w, h);
		}
	}
}

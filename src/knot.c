#define SP_KNOT_C

/*
 * SPKnot
 *
 * Copyright (C) Lauris Kaplinski <lauris@ariman.ee> 2000
 *
 */

#include <math.h>
#include "helper/sodipodi-ctrl.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-events.h"
#include "desktop-affine.h"
#include "knot.h"

#include "sp-guide.h"

#define hypot(a,b) sqrt ((a) * (a) + (b) * (b))

#define noKNOT_NOGRAB

enum {
	ARG_NONE,
	ARG_SIZE,
	ARG_ANCHOR,
	ARG_SHAPE,
	ARG_MODE,
	ARG_FILL, ARG_FILL_MOUSEOVER, ARG_FILL_DRAGGING,
	ARG_STROKE, ARG_STROKE_MOUSEOVER, ARG_STROKE_DRAGGING,
	ARG_IMAGE, ARG_IMAGE_MOUSEOVER, ARG_IMAGE_DRAGGING,
	ARG_CURSOR, ARG_CURSOR_MOUSEOVER, ARG_CURSOR_DRAGGING,
	ARG_PIXBUF
};

enum {
	EVENT,
	CLICKED,
	GRABBED,
	UNGRABBED,
	MOVED,
	REQUEST,
	DISTANCE,
	LAST_SIGNAL
};

static void sp_marshal_BOOL__POINTER_UINT (GtkObject * object, GtkSignalFunc func, gpointer func_data, GtkArg * args);
static void sp_marshal_DOUBLE__POINTER_UINT (GtkObject * object, GtkSignalFunc func, gpointer func_data, GtkArg * args);

static void sp_knot_class_init (SPKnotClass * klass);
static void sp_knot_init (SPKnot * knot);
static void sp_knot_destroy (GtkObject * object);
static void sp_knot_set_arg (GtkObject * object, GtkArg * arg, guint id);

static void sp_knot_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data);
static void sp_knot_set_flag (SPKnot * knot, guint flag, gboolean set);
static void sp_knot_update_ctrl (SPKnot * knot);
static void sp_knot_set_ctrl_state (SPKnot *knot);

static GtkObjectClass * parent_class;
static guint knot_signals[LAST_SIGNAL] = {0};

GtkType
sp_knot_get_type (void)
{
	static GtkType knot_type = 0;
	if (!knot_type) {
		GtkTypeInfo knot_info = {
			"SPKnot",
			sizeof (SPKnot),
			sizeof (SPKnotClass),
			(GtkClassInitFunc) sp_knot_class_init,
			(GtkObjectInitFunc) sp_knot_init,
			NULL, NULL, NULL
		};
		knot_type = gtk_type_unique (gtk_object_get_type (), &knot_info);
	}
	return knot_type;
}

static void
sp_knot_class_init (SPKnotClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (gtk_object_get_type ());

	/* Huh :) */

	gtk_object_add_arg_type ("SPKnot::size", GTK_TYPE_UINT, GTK_ARG_WRITABLE, ARG_SIZE);
	gtk_object_add_arg_type ("SPKnot::anchor", GTK_TYPE_ANCHOR_TYPE, GTK_ARG_WRITABLE, ARG_ANCHOR);
	gtk_object_add_arg_type ("SPKnot::shape", GTK_TYPE_ENUM, GTK_ARG_WRITABLE, ARG_SHAPE);
	gtk_object_add_arg_type ("SPKnot::mode", GTK_TYPE_ENUM, GTK_ARG_WRITABLE, ARG_MODE);
	gtk_object_add_arg_type ("SPKnot::fill", GTK_TYPE_UINT, GTK_ARG_WRITABLE, ARG_FILL);
	gtk_object_add_arg_type ("SPKnot::fill_mouseover", GTK_TYPE_UINT, GTK_ARG_WRITABLE, ARG_FILL_MOUSEOVER);
	gtk_object_add_arg_type ("SPKnot::fill_dragging", GTK_TYPE_UINT, GTK_ARG_WRITABLE, ARG_FILL_DRAGGING);
	gtk_object_add_arg_type ("SPKnot::stroke", GTK_TYPE_UINT, GTK_ARG_WRITABLE, ARG_STROKE);
	gtk_object_add_arg_type ("SPKnot::stroke_mouseover", GTK_TYPE_UINT, GTK_ARG_WRITABLE, ARG_STROKE_MOUSEOVER);
	gtk_object_add_arg_type ("SPKnot::stroke_dragging", GTK_TYPE_UINT, GTK_ARG_WRITABLE, ARG_STROKE_DRAGGING);
	gtk_object_add_arg_type ("SPKnot::image", GTK_TYPE_POINTER, GTK_ARG_WRITABLE, ARG_IMAGE);
	gtk_object_add_arg_type ("SPKnot::image_mouseover", GTK_TYPE_POINTER, GTK_ARG_WRITABLE, ARG_IMAGE_MOUSEOVER);
	gtk_object_add_arg_type ("SPKnot::image_dragging", GTK_TYPE_POINTER, GTK_ARG_WRITABLE, ARG_IMAGE_DRAGGING);
	gtk_object_add_arg_type ("SPKnot::cursor", GTK_TYPE_GDK_CURSOR_TYPE, GTK_ARG_WRITABLE, ARG_CURSOR);
	gtk_object_add_arg_type ("SPKnot::cursor_mouseover", GTK_TYPE_GDK_CURSOR_TYPE, GTK_ARG_WRITABLE, ARG_CURSOR_MOUSEOVER);
	gtk_object_add_arg_type ("SPKnot::cursor_dragging", GTK_TYPE_GDK_CURSOR_TYPE, GTK_ARG_WRITABLE, ARG_CURSOR_DRAGGING);
	gtk_object_add_arg_type ("SPKnot::pixbuf", GTK_TYPE_POINTER, GTK_ARG_WRITABLE, ARG_PIXBUF);

	knot_signals[EVENT] = gtk_signal_new ("event",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPKnotClass, event),
		gtk_marshal_BOOL__POINTER,
		GTK_TYPE_BOOL, 1,
		GTK_TYPE_GDK_EVENT);
	knot_signals[CLICKED] = gtk_signal_new ("clicked",
		GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPKnotClass, clicked),
		gtk_marshal_NONE__UINT,
		GTK_TYPE_NONE, 1, GTK_TYPE_UINT);
	knot_signals[GRABBED] = gtk_signal_new ("grabbed",
		GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPKnotClass, grabbed),
		gtk_marshal_NONE__UINT,
		GTK_TYPE_NONE, 1, GTK_TYPE_UINT);
	knot_signals[UNGRABBED] = gtk_signal_new ("ungrabbed",
		GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPKnotClass, ungrabbed),
		gtk_marshal_NONE__UINT,
		GTK_TYPE_NONE, 1, GTK_TYPE_UINT);
	knot_signals[MOVED] = gtk_signal_new ("moved",
		GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPKnotClass, moved),
		gtk_marshal_NONE__POINTER_UINT,
		GTK_TYPE_NONE, 2, GTK_TYPE_POINTER, GTK_TYPE_UINT);
	knot_signals[REQUEST] = gtk_signal_new ("request",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPKnotClass, request),
		sp_marshal_BOOL__POINTER_UINT,
		GTK_TYPE_BOOL, 2, GTK_TYPE_POINTER, GTK_TYPE_UINT);
	knot_signals[DISTANCE] = gtk_signal_new ("distance",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPKnotClass, distance),
		sp_marshal_DOUBLE__POINTER_UINT,
		GTK_TYPE_DOUBLE, 2, GTK_TYPE_POINTER, GTK_TYPE_UINT);

	gtk_object_class_add_signals (object_class, knot_signals, LAST_SIGNAL);

	object_class->destroy = sp_knot_destroy;
	object_class->set_arg = sp_knot_set_arg;
}

static void
sp_knot_init (SPKnot * knot)
{
	knot->desktop = NULL;
	knot->item = NULL;
	knot->flags = 0;

	knot->size = 8;
	knot->x = knot->y = 0.0;
	knot->hx = knot->hy = 0.0;
	knot->anchor = GTK_ANCHOR_CENTER;
	knot->shape = SP_KNOT_SHAPE_SQUARE;
	knot->mode = SP_KNOT_MODE_COLOR;

	knot->fill [SP_KNOT_STATE_NORMAL] = 0x000000ff;
	knot->fill [SP_KNOT_STATE_MOUSEOVER] = 0xff0000ff;
	knot->fill [SP_KNOT_STATE_DRAGGING] = 0x0000ffff;

	knot->stroke [SP_KNOT_STATE_NORMAL] = 0x000000ff;
	knot->stroke [SP_KNOT_STATE_MOUSEOVER] = 0x000000ff;
	knot->stroke [SP_KNOT_STATE_DRAGGING] = 0x000000ff;

	knot->image [SP_KNOT_STATE_NORMAL] = NULL;
	knot->image [SP_KNOT_STATE_MOUSEOVER] = NULL;
	knot->image [SP_KNOT_STATE_DRAGGING] = NULL;

	knot->cursor [SP_KNOT_STATE_NORMAL] = NULL;
	knot->cursor [SP_KNOT_STATE_MOUSEOVER] = NULL;
	knot->cursor [SP_KNOT_STATE_DRAGGING] = NULL;

	knot->saved_cursor = NULL;
	knot->pixbuf = NULL;
}

static void
sp_knot_destroy (GtkObject * object)
{
	SPKnot * knot;

	knot = (SPKnot *) object;

	/* ungrab pointer if still grabbed by mouseover, find a different way */
	if (gdk_pointer_is_grabbed) gdk_pointer_ungrab (0);

	if (knot->item) gtk_object_destroy (GTK_OBJECT (knot->item));
	/*
	  for (i = 0; i < SP_KNOT_VISIBLE_STATES; i++) {
	  
	  if (knot->cursor[i]) gdk_cursor_destroy (knot->cursor[i]);
	  }
	*/
	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_knot_set_arg (GtkObject * object, GtkArg * arg, guint id)
{
	SPKnot * knot;
	GdkCursor * cursor;
	gint i;

	knot = SP_KNOT (object);

	switch (id) {
	case ARG_SIZE:
		knot->size = GTK_VALUE_UINT (*arg);
		break;
	case ARG_ANCHOR:
		knot->anchor = GTK_VALUE_ENUM (*arg);
		break;
	case ARG_SHAPE:
		knot->shape = GTK_VALUE_ENUM (*arg);
		break;
	case ARG_MODE:
		knot->mode = GTK_VALUE_ENUM (*arg);
		break;
	case ARG_FILL:
		knot->fill[SP_KNOT_STATE_NORMAL] =
		knot->fill[SP_KNOT_STATE_MOUSEOVER] =
		knot->fill[SP_KNOT_STATE_DRAGGING] = GTK_VALUE_UINT (* arg);
		break;
	case ARG_FILL_MOUSEOVER:
		knot->fill[SP_KNOT_STATE_MOUSEOVER] = 
		knot->fill[SP_KNOT_STATE_DRAGGING] = GTK_VALUE_UINT (* arg);
		break;
	case ARG_FILL_DRAGGING:
		knot->fill[SP_KNOT_STATE_DRAGGING] = GTK_VALUE_UINT (* arg);
		break;
	case ARG_STROKE:
		knot->stroke[SP_KNOT_STATE_NORMAL] =
		knot->stroke[SP_KNOT_STATE_MOUSEOVER] =
		knot->stroke[SP_KNOT_STATE_DRAGGING] = GTK_VALUE_UINT (*arg);
		break;
	case ARG_STROKE_MOUSEOVER:
		knot->stroke[SP_KNOT_STATE_MOUSEOVER] = 
		knot->stroke[SP_KNOT_STATE_DRAGGING] = GTK_VALUE_UINT (*arg);
		break;
	case ARG_STROKE_DRAGGING:
		knot->stroke[SP_KNOT_STATE_DRAGGING] = GTK_VALUE_UINT (*arg);
		break;
	case ARG_IMAGE:
		knot->image[SP_KNOT_STATE_NORMAL] =
		knot->image[SP_KNOT_STATE_MOUSEOVER] =
		knot->image[SP_KNOT_STATE_DRAGGING] = GTK_VALUE_POINTER (*arg);
		break;
	case ARG_IMAGE_MOUSEOVER:
		knot->image[SP_KNOT_STATE_MOUSEOVER] = GTK_VALUE_POINTER (*arg);
		break;
	case ARG_IMAGE_DRAGGING:
		knot->image[SP_KNOT_STATE_DRAGGING] = GTK_VALUE_POINTER (*arg);
		break;
	case ARG_CURSOR:
		cursor = GTK_VALUE_BOXED (* arg);
		for (i = 0; i < SP_KNOT_VISIBLE_STATES; i++) {
			/* fixme: There should be ::unref somewhere */
			//if (knot->cursor[i]) gdk_cursor_destroy (knot->cursor[i]);
			knot->cursor[i] = cursor;
		}
		break;
	case ARG_CURSOR_MOUSEOVER:
		cursor = GTK_VALUE_BOXED (* arg);
		if (knot->cursor[SP_KNOT_STATE_MOUSEOVER]) {
			//gdk_cursor_destroy (knot->cursor[SP_KNOT_STATE_MOUSEOVER]);
		}
		knot->cursor[SP_KNOT_STATE_MOUSEOVER] = cursor;
		break;
	case ARG_CURSOR_DRAGGING:
		cursor = GTK_VALUE_BOXED (* arg);
		if (knot->cursor[SP_KNOT_STATE_DRAGGING]) {
			//gdk_cursor_destroy (knot->cursor[SP_KNOT_STATE_DRAGGING]);
		}
		knot->cursor[SP_KNOT_STATE_DRAGGING] = cursor;
		break;
	case ARG_PIXBUF:
	        knot->pixbuf = GTK_VALUE_POINTER (*arg);
	        break;
	default:
		g_assert_not_reached ();
		break;
	}

	sp_knot_update_ctrl (knot);
}

static void
sp_knot_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data)
{
	SPKnot * knot;
	ArtPoint p;
	gboolean consumed;
	static gboolean grabbed = FALSE;
	static gboolean moved = FALSE;

	g_assert (data != NULL);
	g_assert (SP_IS_KNOT (data));

	knot = SP_KNOT (data);

	consumed = FALSE;

	/* Run client universal event handler, if present */

	gtk_signal_emit (GTK_OBJECT (knot), knot_signals[EVENT], event, &consumed);

	if (consumed) return;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			sp_desktop_w2d_xy_point (knot->desktop,
				&p,
				event->button.x,
				event->button.y);
			knot->hx = p.x - knot->x;
			knot->hy = p.y - knot->y;
#ifndef KNOT_NOGRAB
			gdk_pointer_ungrab (event->button.time);
			gnome_canvas_item_grab (knot->item,
						GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
						knot->cursor[SP_KNOT_STATE_DRAGGING],
						event->button.time);
			
#endif
			sp_knot_set_flag (knot, SP_KNOT_GRABBED, TRUE);
			grabbed = TRUE;
			consumed = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (event->button.button == 1) {
			sp_knot_set_flag (knot, SP_KNOT_GRABBED, FALSE);
#ifndef KNOT_NOGRAB
			gnome_canvas_item_ungrab (knot->item, event->button.time);
			gdk_pointer_grab (knot->item->canvas->layout.bin_window,
					  FALSE,
					  GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK | 
					  GDK_BUTTON_PRESS_MASK | GDK_LEAVE_NOTIFY_MASK,
					  NULL,
					  knot->cursor[SP_KNOT_STATE_MOUSEOVER],
					  event->button.time);

#endif
			if (moved) {
				sp_knot_set_flag (knot,
					SP_KNOT_DRAGGING,
					FALSE);
				gtk_signal_emit (GTK_OBJECT (knot),
					knot_signals[UNGRABBED],
					event->button.state);
			} else {
				gtk_signal_emit (GTK_OBJECT (knot),
					knot_signals[CLICKED],
					event->button.state);
			}
			grabbed = FALSE;
			moved = FALSE;
			consumed = TRUE;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (grabbed) {
			if (!moved) {
				gtk_signal_emit (GTK_OBJECT (knot),
					knot_signals[GRABBED],
					event->motion.state);
				sp_knot_set_flag (knot,
					SP_KNOT_DRAGGING,
					TRUE);
			}
			sp_desktop_w2d_xy_point (knot->desktop,
				&p,
				event->motion.x,
				event->motion.y);
			p.x -= knot->hx;
			p.y -= knot->hy;
			sp_knot_request_position (knot, &p, event->motion.state);
			moved = TRUE;
			consumed = TRUE;
		}
		break;
	case GDK_ENTER_NOTIFY:
		gdk_pointer_grab (knot->item->canvas->layout.bin_window,
				  FALSE,
				  GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK | 
				  GDK_BUTTON_PRESS_MASK | GDK_LEAVE_NOTIFY_MASK,
				  NULL,
				  knot->cursor[SP_KNOT_STATE_MOUSEOVER],
				  event->button.time);

		sp_knot_set_flag (knot, SP_KNOT_MOUSEOVER, TRUE);
		consumed = TRUE;
		break;
	case GDK_LEAVE_NOTIFY:
		gdk_pointer_ungrab (event->button.time);
		sp_knot_set_flag (knot, SP_KNOT_MOUSEOVER, FALSE);
		consumed = TRUE;
		break;
	default:
		break;
	}

	if (!consumed) {
		sp_desktop_root_handler (item, event, knot->desktop);
	}
}

SPKnot *
sp_knot_new (SPDesktop * desktop)
{
	SPKnot * knot;

	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail (SP_DT_IS_EDITABLE (desktop), NULL);

	knot = gtk_type_new (SP_TYPE_KNOT);

	knot->desktop = desktop;
	knot->flags = SP_KNOT_VISIBLE;

	knot->item = gnome_canvas_item_new (SP_DT_CONTROLS (desktop),
		SP_TYPE_CTRL,
		"anchor", GTK_ANCHOR_CENTER,
		"size", 8.0,
		"filled", TRUE,
		"fill_color", 0x000000ff,
		"stroked", TRUE,
		"stroke_color", 0x000000ff,
		NULL);

	gtk_signal_connect (GTK_OBJECT (knot->item), "event",
		GTK_SIGNAL_FUNC (sp_knot_handler), knot);

	return knot;
}

void
sp_knot_show (SPKnot * knot)
{
	g_return_if_fail (knot != NULL);
	g_return_if_fail (SP_IS_KNOT (knot));

	sp_knot_set_flag (knot, SP_KNOT_VISIBLE, TRUE);
}

void
sp_knot_hide (SPKnot * knot)
{
	g_return_if_fail (knot != NULL);
	g_return_if_fail (SP_IS_KNOT (knot));

	sp_knot_set_flag (knot, SP_KNOT_VISIBLE, FALSE);
}

void
sp_knot_request_position (SPKnot * knot, ArtPoint * p, guint state)
{
	gboolean done;

	g_return_if_fail (knot != NULL);
	g_return_if_fail (SP_IS_KNOT (knot));
	g_return_if_fail (p != NULL);

	done = FALSE;

	gtk_signal_emit (GTK_OBJECT (knot),
		knot_signals[REQUEST],
		p,
		state,
		&done);

	/* If user did not complete, we simply move knot to new position */

	if (!done) {
		sp_knot_set_position (knot, p, state);
	}
}

gdouble
sp_knot_distance (SPKnot * knot, ArtPoint * p, guint state)
{
	gdouble distance;

	g_return_val_if_fail (knot != NULL, 1e18);
	g_return_val_if_fail (SP_IS_KNOT (knot), 1e18);
	g_return_val_if_fail (p != NULL, 1e18);

	distance = hypot (p->x - knot->x, p->y - knot->y);

	gtk_signal_emit (GTK_OBJECT (knot),
		knot_signals[DISTANCE],
		p,
		state,
		&distance);

	return distance;
}

void
sp_knot_set_position (SPKnot * knot, ArtPoint * p, guint state)
{
	g_return_if_fail (knot != NULL);
	g_return_if_fail (SP_IS_KNOT (knot));
	g_return_if_fail (p != NULL);

	knot->x = p->x;
	knot->y = p->y;

	if (knot->item) sp_ctrl_moveto (SP_CTRL (knot->item), p->x, p->y);

	gtk_signal_emit (GTK_OBJECT (knot),
		knot_signals[MOVED],
		p,
		state);
}

ArtPoint *
sp_knot_position (SPKnot * knot, ArtPoint * p)
{
	g_return_val_if_fail (knot != NULL, NULL);
	g_return_val_if_fail (SP_IS_KNOT (knot), NULL);
	g_return_val_if_fail (p != NULL, NULL);

	p->x = knot->x;
	p->y = knot->y;

	return p;
}

static void
sp_knot_set_flag (SPKnot * knot, guint flag, gboolean set)
{
	g_assert (knot != NULL);
	g_assert (SP_IS_KNOT (knot));

	if (set) {
		knot->flags |= flag;
	} else {
		knot->flags &= ~flag;
	}

	switch (flag) {
	case SP_KNOT_VISIBLE:
		if (set) {
			gnome_canvas_item_show (knot->item);
		} else {
			gnome_canvas_item_hide (knot->item);
		}
		break;
	case SP_KNOT_MOUSEOVER:
	case SP_KNOT_DRAGGING:
		sp_knot_set_ctrl_state (knot);
		break;
	case SP_KNOT_GRABBED:
		break;
	default:
		g_assert_not_reached ();
		break;
	}
}

static void
sp_knot_update_ctrl (SPKnot * knot)
{
	if (!knot->item) return;

	gtk_object_set (GTK_OBJECT (knot->item), "shape", knot->shape, NULL);
	gtk_object_set (GTK_OBJECT (knot->item), "mode", knot->mode, NULL);
	gtk_object_set (GTK_OBJECT (knot->item), "size", (gdouble) knot->size, NULL);
	gtk_object_set (GTK_OBJECT (knot->item), "anchor", knot->anchor, NULL);
	if (knot->pixbuf) gtk_object_set (GTK_OBJECT (knot->item), "pixbuf", knot->pixbuf, NULL);

	sp_knot_set_ctrl_state (knot);
}

static void
sp_knot_set_ctrl_state (SPKnot * knot)
{
	if (knot->flags & SP_KNOT_DRAGGING) {
		gtk_object_set (GTK_OBJECT (knot->item),
				"fill_color",
				knot->fill [SP_KNOT_STATE_DRAGGING],
				NULL);
		gtk_object_set (GTK_OBJECT (knot->item),
				"stroke_color",
				knot->stroke [SP_KNOT_STATE_DRAGGING],
				NULL);
	} else if (knot->flags & SP_KNOT_MOUSEOVER) {
		gtk_object_set (GTK_OBJECT (knot->item),
				"fill_color",
				knot->fill [SP_KNOT_STATE_MOUSEOVER],
				NULL);
		gtk_object_set (GTK_OBJECT (knot->item),
				"stroke_color",
				knot->stroke [SP_KNOT_STATE_MOUSEOVER],
				NULL);
	} else {
		gtk_object_set (GTK_OBJECT (knot->item),
				"fill_color",
				knot->fill [SP_KNOT_STATE_NORMAL],
				NULL);
		gtk_object_set (GTK_OBJECT (knot->item),
				"stroke_color",
				knot->stroke [SP_KNOT_STATE_NORMAL],
				NULL);
	}
}

typedef gboolean (* SPSignal_BOOL__POINTER_UINT) (GtkObject *object,
	gpointer arg1, guint arg2, gpointer user_data);

static void
sp_marshal_BOOL__POINTER_UINT (GtkObject * object,
	GtkSignalFunc func,
	gpointer func_data,
	GtkArg * args)
{
	SPSignal_BOOL__POINTER_UINT rfunc;
	gboolean * return_val;
	return_val = GTK_RETLOC_BOOL (args[2]);
	rfunc = (SPSignal_BOOL__POINTER_UINT) func;
	* return_val =  (* rfunc) (object,
		GTK_VALUE_POINTER(args[0]),
		GTK_VALUE_INT(args[1]),
		func_data);
}

typedef gdouble (* SPSignal_DOUBLE__POINTER_UINT) (GtkObject *object,
	gpointer arg1, guint arg2, gpointer user_data);

static void
sp_marshal_DOUBLE__POINTER_UINT (GtkObject * object,
	GtkSignalFunc func,
	gpointer func_data,
	GtkArg * args)
{
	SPSignal_DOUBLE__POINTER_UINT rfunc;
	gdouble * return_val;
	return_val = GTK_RETLOC_DOUBLE (args[2]);
	rfunc = (SPSignal_DOUBLE__POINTER_UINT) func;
	* return_val =  (* rfunc) (object,
		GTK_VALUE_POINTER(args[0]),
		GTK_VALUE_INT(args[1]),
		func_data);
}


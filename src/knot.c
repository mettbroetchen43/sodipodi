#define SP_KNOT_C

/*
 * SPKnot
 *
 * Copyright (C) Lauris Kaplinski <lauris@ariman.ee> 2000
 *
 */

#include "helper/sodipodi-ctrl.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-events.h"
#include "desktop-affine.h"
#include "knot.h"

enum {
	ARG_NONE,
	ARG_SIZE,
	ARG_X,
	ARG_Y,
	ARG_ANCHOR,
	ARG_SHAPE,
	ARG_FILL, ARG_FILL_MOUSEOVER, ARG_FILL_DRAGGING,
	ARG_STROKE, ARG_STROKE_MOUSEOVER, ARG_STROKE_DRAGGING,
	ARG_IMAGE, ARG_IMAGE_MOUSEOVER, ARG_IMAGE_DRAGGING,
	ARG_CURSOR, ARG_CURSOR_MOUSEOVER, ARG_CURSOR_DRAGGING
};

enum {
	EVENT,
	CLICKED,
	GRABBED,
	UNGRABBED,
	MOVED,
	LAST_SIGNAL
};

static void sp_knot_class_init (SPKnotClass * klass);
static void sp_knot_init (SPKnot * knot);
static void sp_knot_destroy (GtkObject * object);
static void sp_knot_set_arg (GtkObject * object, GtkArg * arg, guint id);

static void sp_knot_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data);
static void sp_knot_set_flag (SPKnot * knot, guint flag, gboolean set);

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
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
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
	gtk_object_add_arg_type ("SPKnot::x", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_X);
	gtk_object_add_arg_type ("SPKnot::y", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_Y);
	gtk_object_add_arg_type ("SPKnot::anchor", GTK_TYPE_ANCHOR_TYPE, GTK_ARG_WRITABLE, ARG_ANCHOR);
	gtk_object_add_arg_type ("SPKnot::shape", GTK_TYPE_ENUM, GTK_ARG_WRITABLE, ARG_SHAPE);
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
}

static void
sp_knot_destroy (GtkObject * object)
{
	SPKnot * knot;
	gint i;

	knot = (SPKnot *) object;

	if (knot->item) gtk_object_destroy (GTK_OBJECT (knot->item));

	for (i = 0; i < SP_KNOT_VISIBLE_STATES; i++) {
		/* fixme: There should be ::unref somewhere */
		if (knot->cursor[i]) gdk_cursor_destroy (knot->cursor[i]);
	}

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
		knot->size = GTK_VALUE_UINT (* arg);
		if (knot->item) {
			gtk_object_set (GTK_OBJECT (knot->item), "size", (gdouble) knot->size, NULL);
		}
		break;
	case ARG_X:
	case ARG_Y:
		if (id == ARG_X) knot->x = GTK_VALUE_DOUBLE (* arg);
		if (id == ARG_Y) knot->y = GTK_VALUE_DOUBLE (* arg);
		if (knot->item) {
			sp_ctrl_moveto (SP_CTRL (knot->item), knot->x, knot->y);
		}
		break;
	case ARG_ANCHOR:
		knot->anchor = GTK_VALUE_ENUM (* arg);
		if (knot->item) {
			gtk_object_set (GTK_OBJECT (knot->item), "anchor", knot->anchor, NULL);
		}
		break;
	case ARG_SHAPE:
		break;
	case ARG_FILL:
		knot->fill[SP_KNOT_STATE_NORMAL] =
		knot->fill[SP_KNOT_STATE_MOUSEOVER] =
		knot->fill[SP_KNOT_STATE_DRAGGING] = GTK_VALUE_UINT (* arg);
		break;
	case ARG_FILL_MOUSEOVER:
		knot->fill[SP_KNOT_STATE_MOUSEOVER] = GTK_VALUE_UINT (* arg);
		break;
	case ARG_FILL_DRAGGING:
		knot->fill[SP_KNOT_STATE_DRAGGING] = GTK_VALUE_UINT (* arg);
		break;
	case ARG_STROKE:
		knot->stroke[SP_KNOT_STATE_NORMAL] =
		knot->stroke[SP_KNOT_STATE_MOUSEOVER] =
		knot->stroke[SP_KNOT_STATE_DRAGGING] = GTK_VALUE_UINT (* arg);
		break;
	case ARG_STROKE_MOUSEOVER:
		knot->stroke[SP_KNOT_STATE_MOUSEOVER] = GTK_VALUE_UINT (* arg);
		break;
	case ARG_STROKE_DRAGGING:
		knot->stroke[SP_KNOT_STATE_DRAGGING] = GTK_VALUE_UINT (* arg);
		break;
	case ARG_IMAGE:
	case ARG_IMAGE_MOUSEOVER:
	case ARG_IMAGE_DRAGGING:
		break;
	case ARG_CURSOR:
		cursor = GTK_VALUE_BOXED (* arg);
		for (i = 0; i < SP_KNOT_VISIBLE_STATES; i++) {
			/* fixme: There should be ::unref somewhere */
			if (knot->cursor[i]) gdk_cursor_destroy (knot->cursor[i]);
			knot->cursor[i] = cursor;
		}
		break;
	case ARG_CURSOR_MOUSEOVER:
		cursor = GTK_VALUE_BOXED (* arg);
		if (knot->cursor[SP_KNOT_STATE_MOUSEOVER]) {
			gdk_cursor_destroy (knot->cursor[SP_KNOT_STATE_MOUSEOVER]);
		}
		knot->cursor[SP_KNOT_STATE_MOUSEOVER] = cursor;
		break;
	case ARG_CURSOR_DRAGGING:
		cursor = GTK_VALUE_BOXED (* arg);
		if (knot->cursor[SP_KNOT_STATE_DRAGGING]) {
			gdk_cursor_destroy (knot->cursor[SP_KNOT_STATE_DRAGGING]);
		}
		knot->cursor[SP_KNOT_STATE_DRAGGING] = cursor;
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	if (knot->item) {
		gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (knot->item));
	}
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
			gnome_canvas_item_grab (knot->item,
				GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				knot->cursor[SP_KNOT_STATE_DRAGGING],
				event->button.time);
			grabbed = TRUE;
			consumed = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (event->button.button == 1) {
			gnome_canvas_item_ungrab (knot->item, event->button.time);
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
			sp_knot_set_position (knot, &p);
			gtk_signal_emit (GTK_OBJECT (knot),
				knot_signals[MOVED],
				&p,
				event->motion.state);
			moved = TRUE;
			consumed = TRUE;
		}
		break;
	case GDK_ENTER_NOTIFY:
		sp_knot_set_flag (knot, SP_KNOT_MOUSEOVER, TRUE);
		consumed = TRUE;
		break;
	case GDK_LEAVE_NOTIFY:
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

void
sp_knot_set_position (SPKnot * knot, ArtPoint * p)
{
	g_return_if_fail (knot != NULL);
	g_return_if_fail (SP_IS_KNOT (knot));
	g_return_if_fail (p != NULL);

	knot->x = p->x;
	knot->y = p->y;

	if (knot->item) sp_ctrl_moveto (SP_CTRL (knot->item), p->x, p->y);
}

static void
sp_knot_set_flag (SPKnot * knot, guint flag, gboolean set)
{
	g_assert (knot != NULL);
	g_assert (SP_IS_KNOT (knot));

	switch (flag) {
	case SP_KNOT_VISIBLE:
		if (set) {
			gnome_canvas_item_show (knot->item);
		} else {
			gnome_canvas_item_hide (knot->item);
		}
		break;
	case SP_KNOT_MOUSEOVER:
		if (!(knot->flags & SP_KNOT_DRAGGING)) {
			gtk_object_set (GTK_OBJECT (knot->item),
				"fill_color",
				knot->fill [set ? SP_KNOT_STATE_MOUSEOVER : SP_KNOT_STATE_NORMAL],
				NULL);
		}
		break;
	case SP_KNOT_DRAGGING:
		if (set) {
			gtk_object_set (GTK_OBJECT (knot->item),
				"fill_color",
				knot->fill [SP_KNOT_STATE_DRAGGING],
				NULL);
		} else if (knot->flags & SP_KNOT_MOUSEOVER) {
			gtk_object_set (GTK_OBJECT (knot->item),
				"fill_color",
				knot->fill [SP_KNOT_STATE_MOUSEOVER],
				NULL);
		} else {
			gtk_object_set (GTK_OBJECT (knot->item),
				"fill_color",
				knot->fill [SP_KNOT_STATE_NORMAL],
				NULL);
		}
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	if (set) {
		knot->flags |= flag;
	} else {
		knot->flags &= ~flag;
	}
}


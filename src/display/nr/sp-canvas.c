#define __SP_ARENA_C__

/*
 * SPArena
 *
 * Antialiased drawing engine for Sodipodi
 *
 * Author: Lauris Kaplinski <lauris@ximian.com>
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 */

#include "sp-canvas-item.h"
#include "sp-arena.h"

static void sp_arena_class_init (SPArenaClass * klass);
static void sp_arena_init (SPArena * arena);
static void sp_arena_destroy (GtkObject * object);

static gint sp_arena_button_press_event (GtkWidget * widget, GdkEventButton * event);
static gint sp_arena_button_release_event (GtkWidget * widget, GdkEventButton * event);
static gint sp_arena_motion_notify_event (GtkWidget * widget, GdkEventMotion * event);
static gint sp_arena_expose_event (GtkWidget * widget, GdkEventExpose * event);
static void sp_arena_size_allocate (GtkWidget * widget, GtkAllocation * allocation);

static void sp_arena_schedule_render (SPArena * arena);
static gint sp_arena_emit_event (SPArena * arena, NRCanvasItem * item, GdkEvent * event);

static GtkDrawingAreaClass * parent_class;

GtkType
sp_arena_get_type (void)
{
	static GtkType arena_type = 0;
	if (!arena_type) {
		GtkTypeInfo arena_info = {
			"SPArena",
			sizeof (SPArena),
			sizeof (SPArenaClass),
			(GtkClassInitFunc) sp_arena_class_init,
			(GtkObjectInitFunc) sp_arena_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		arena_type = gtk_type_unique (gtk_drawing_area_get_type (), &arena_info);
	}
	return arena_type;
}

static void
sp_arena_class_init (SPArenaClass * klass)
{
	GtkObjectClass * object_class;
	GtkWidgetClass * widget_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	parent_class = gtk_type_class (gtk_drawing_area_get_type ());

	object_class->destroy = sp_arena_destroy;

	widget_class->size_allocate = sp_arena_size_allocate;
	widget_class->button_press_event = sp_arena_button_press_event;
	widget_class->button_release_event = sp_arena_button_release_event;
	widget_class->motion_notify_event = sp_arena_motion_notify_event;
	widget_class->expose_event = sp_arena_expose_event;
}

static void
sp_arena_init (SPArena * arena)
{
	arena->aacanvas = NULL;
	nr_irect_set_empty (&arena->viewport);
	arena->uta = NULL;
}

static void
sp_arena_destroy (GtkObject * object)
{
	SPArena * arena;

	arena = (SPArena *) object;

	if (arena->uta) {
		nr_uta_free (arena->uta);
		arena->uta = NULL;
	}

	if (arena->aacanvas) {
		gtk_object_unref (GTK_OBJECT (arena->aacanvas));
		arena->aacanvas = NULL;
	}

	if (((GtkObjectClass *) (parent_class))->destroy) (* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

/* GtkWidget methods */

static void
sp_arena_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
	SPArena * arena;
	gint x0, y0, x1, y1;
	NRUTA * uta;
	NRIRect d;

	x0 = widget->allocation.x;
	y0 = widget->allocation.y;
	x1 = x0 + widget->allocation.width;
	y1 = y0 + widget->allocation.height;

	if (((GtkWidgetClass *) (parent_class))->size_allocate) (* ((GtkWidgetClass *) (parent_class))->size_allocate) (widget, allocation);

	arena = (SPArena *) widget;

	/* fixme: centering & stuff... */
	uta = nr_uta_new (allocation->width, allocation->height);

	if (arena->uta) {
		nr_uta_set_uta (uta, arena->uta, 0, 0);
		nr_uta_free (arena->uta);
	}

	/* Set repaint region */
	d.x0 = allocation->x;
	d.y0 = allocation->y;
	d.x1 = allocation->x + allocation->width;
	d.y1 = y0;
	if (!nr_irect_is_empty (&d)) nr_uta_set_rect (uta, &d);
	d.x0 = allocation->x;
	d.y0 = y0;
	d.x1 = x0;
	d.y1 = y1;
	if (!nr_irect_is_empty (&d)) nr_uta_set_rect (uta, &d);
	d.x0 = x1;
	d.y0 = y0;
	d.x1 = allocation->x + allocation->width;
	d.y1 = y1;
	if (!nr_irect_is_empty (&d)) nr_uta_set_rect (uta, &d);
	d.x0 = allocation->x;
	d.y0 = y1;
	d.x1 = allocation->x + allocation->width;
	d.y1 = allocation->y + allocation->height;
	if (!nr_irect_is_empty (&d)) nr_uta_set_rect (uta, &d);

	arena->uta = uta;

	/* fixme: Not always necessary */
	sp_arena_schedule_render (arena);
}

static gint
sp_arena_button_press_event (GtkWidget * widget, GdkEventButton * event)
{
	SPArena * arena;
	NRPoint point;
	NRCanvasItem * item;

	arena = (SPArena *) widget;

	point.x = event->x;
	point.y = event->y;

	item = nr_canvas_invoke_pick ((NRCanvas *) arena->aacanvas, &point);
	/* fixme: Where do belong picking root, if empty? */

	return sp_arena_emit_event (arena, item, (GdkEvent *) event);
}

static gint
sp_arena_button_release_event (GtkWidget * widget, GdkEventButton * event)
{
	SPArena * arena;
	NRPoint point;
	NRCanvasItem * item;

	arena = (SPArena *) widget;

	point.x = event->x;
	point.y = event->y;

	item = nr_canvas_invoke_pick ((NRCanvas *) arena->aacanvas, &point);
	/* fixme: Where do belong picking root, if empty? */

	return sp_arena_emit_event (arena, item, (GdkEvent *) event);
}

static gint
sp_arena_motion_notify_event (GtkWidget * widget, GdkEventMotion * event)
{
	SPArena * arena;
	NRPoint point;
	NRCanvasItem * item;

	arena = (SPArena *) widget;

	point.x = event->x;
	point.y = event->y;

	item = nr_canvas_invoke_pick ((NRCanvas *) arena->aacanvas, &point);
	/* fixme: Where do belong picking root, if empty? */

	return sp_arena_emit_event (arena, item, (GdkEvent *) event);
}

static gint
sp_arena_expose_event (GtkWidget * widget, GdkEventExpose * event)
{
	SPArena * arena;
	NRIRect erect, rect;

	arena = (SPArena *) widget;

	/* UTA has to be set by ::size_allocate() */
	g_return_val_if_fail (arena->uta != NULL, FALSE);

	erect.x0 = event->area.x;
	erect.y0 = event->area.y;
	erect.x1 = event->area.x + event->area.width;
	erect.y1 = event->area.y + event->area.height;

	/* Not strictly necessary, but... */
	nr_irect_intersection (&rect, &arena->viewport, &erect);

	if (!nr_irect_is_empty (&rect)) {
		nr_uta_set_rect (arena->uta, &rect);
		sp_arena_schedule_render (arena);
	}

	return FALSE;
}

/* Private methods */

static void
sp_arena_schedule_render (SPArena * arena)
{
	g_print ("fixme: sp_arena_schedule_render\n");
}

static gint
sp_arena_emit_event (SPArena * arena, NRCanvasItem * item, GdkEvent * event)
{
	/* fixme: */
	if (item == NULL) item = nr_canvas_get_root ((NRCanvas *) arena->aacanvas);

	return sp_canvas_item_emit_event ((SPCanvasItem *) item, event);
}

/* Public methods */

GtkWidget *
sp_arena_new (void)
{
	SPArena * arena;

	arena = gtk_type_new (SP_TYPE_ARENA);

	arena->aacanvas = nr_aa_canvas_new ();

	return (GtkWidget *) arena;
}




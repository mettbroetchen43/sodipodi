#define __SP_ZOOM_CONTEXT_C__

/*
 * Handy zooming tool
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "pixmaps/cursor-zoom.xpm"
#include "rubberband.h"
#include "desktop.h"
#include "desktop-affine.h"

#include "zoom-context.h"

static void sp_zoom_context_class_init (SPZoomContextClass * klass);
static void sp_zoom_context_init (SPZoomContext * zoom_context);
static void sp_zoom_context_dispose (GObject * object);

static gint sp_zoom_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_zoom_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

void sp_zoom_string (const gchar * zoom_str);

static SPEventContextClass * parent_class;

GType
sp_zoom_context_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPZoomContextClass),
			NULL, NULL,
			(GClassInitFunc) sp_zoom_context_class_init,
			NULL, NULL,
			sizeof (SPZoomContext),
			4,
			(GInstanceInitFunc) sp_zoom_context_init,
		};
		type = g_type_register_static (SP_TYPE_EVENT_CONTEXT, "SPZoomContext", &info, 0);
	}
	return type;
}

static void
sp_zoom_context_class_init (SPZoomContextClass * klass)
{
	GObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	object_class->dispose = sp_zoom_context_dispose;

	event_context_class->root_handler = sp_zoom_context_root_handler;
	event_context_class->item_handler = sp_zoom_context_item_handler;
}

static void
sp_zoom_context_init (SPZoomContext * zoom_context)
{
	SPEventContext * event_context;
	
	event_context = SP_EVENT_CONTEXT (zoom_context);

	event_context->cursor_shape = cursor_zoom_xpm;
	event_context->hot_x = 6;
	event_context->hot_y = 6;
}

static void
sp_zoom_context_dispose (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gint
sp_zoom_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	gint ret;

	ret = FALSE;

	if (((SPEventContextClass *) parent_class)->item_handler)
		ret = ((SPEventContextClass *) parent_class)->item_handler (event_context, item, event);

	return ret;
}

static gint
sp_zoom_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	SPDesktop * desktop;
	NRPointF p;
	NRRectD b;
	gint ret;

	ret = FALSE;

	desktop = event_context->desktop;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
			sp_rubberband_start (desktop, p.x, p.y);
			break;
		default:
			break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (event->motion.state & GDK_BUTTON1_MASK) {
			sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
			sp_rubberband_move (p.x, p.y);
		}
		break;
	case GDK_BUTTON_RELEASE:
		switch (event->button.button) {
		case 1:
			if (sp_rubberband_rect (&b)) {
				sp_rubberband_stop ();
				sp_desktop_set_display_area (desktop, b.x0, b.y0, b.x1, b.y1, 10);
			} else {
				sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
				if (event->button.state & GDK_SHIFT_MASK) {
					sp_desktop_zoom_relative (desktop, p.x, p.y, 1/SP_DESKTOP_ZOOM_INC);
				} else {
					sp_desktop_zoom_relative (desktop, p.x, p.y, SP_DESKTOP_ZOOM_INC);
				}
			}
			ret = TRUE;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	if (!ret) {
		if (((SPEventContextClass *) parent_class)->root_handler)
			ret = ((SPEventContextClass *) parent_class)->root_handler (event_context, event);
	}

	return ret;
}


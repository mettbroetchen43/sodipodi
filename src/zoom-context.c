#define __SP_ZOOM_CONTEXT_C__

/*
 * Base class for event processors
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include <stdlib.h>

#include <gtk/gtkeditable.h>
#include <gtk/gtkeditable.h>

#include "rubberband.h"
#include "sodipodi.h"
#include "document.h"
#include "selection.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "sp-item.h"
#include "pixmaps/cursor-zoom.xpm"
#include "zoom-context.h"

static void sp_zoom_context_class_init (SPZoomContextClass * klass);
static void sp_zoom_context_init (SPZoomContext * zoom_context);
static void sp_zoom_context_dispose (GObject * object);

static gint sp_zoom_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_zoom_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

void sp_zoom_string (const gchar * zoom_str);
void sp_zoom_any (void);

static SPEventContextClass * parent_class;

static GtkWidget *zoom_any = NULL;

GtkType
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
	ArtDRect b;
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
#if 0
		case 3:
			p.x = event->button.x;
			p.y = event->button.y;
			sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
			sp_desktop_zoom_relative (desktop, p.x, p.y, 1/SP_DESKTOP_ZOOM_INC);
			ret = TRUE;
			break;
#endif
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

/* Menu handlers */

void
sp_zoom_selection (GtkWidget * widget)
{
	SPSelection * selection;
	NRRectF d;
	SPDesktop * desktop;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;

	selection = SP_DT_SELECTION (desktop);

	g_return_if_fail (selection != NULL);

	sp_selection_bbox (selection, &d);
	if ((fabs (d.x1 - d.x0) < 0.1) || (fabs (d.y1 - d.y0) < 0.1)) return;
	sp_desktop_set_display_area (desktop, d.x0, d.y0, d.x1, d.y1, 10);
}

void
sp_zoom_drawing (GtkWidget * widget)
{
	SPItem * docitem;
	NRRectF d;
	SPDesktop * desktop;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;

	g_return_if_fail(SP_ACTIVE_DOCUMENT != NULL);

	docitem = SP_ITEM (sp_document_root (SP_ACTIVE_DOCUMENT));

	g_return_if_fail (docitem != NULL);

	sp_item_bbox_desktop (docitem, &d);
	if ((fabs (d.x1 - d.x0) < 1.0) || (fabs (d.y1 - d.y0) < 1.0)) return;
	sp_desktop_set_display_area (desktop, d.x0, d.y0, d.x1, d.y1, 10);
}

void
sp_zoom_page (GtkWidget * widget)
{
	ArtDRect d;
	SPDesktop * desktop;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;

	d.x0 = 0.0;
	d.y0 = 0.0;
	d.x1 = sp_document_width (SP_DT_DOCUMENT (desktop));
	d.y1 = sp_document_height (SP_DT_DOCUMENT (desktop));

	if ((fabs (d.x1 - d.x0) < 1.0) || (fabs (d.y1 - d.y0) < 1.0)) return;

	sp_desktop_set_display_area (desktop, d.x0, d.y0, d.x1, d.y1, 10);
}

void
sp_zoom_in (GtkWidget * widget) {
  SPDesktop * desktop;
  ArtDRect d;

  desktop = SP_ACTIVE_DESKTOP;
  if (desktop == NULL) return;
  g_return_if_fail (SP_IS_DESKTOP (desktop));
  
  sp_desktop_get_visible_area (desktop, &d);

  sp_desktop_zoom_relative (desktop, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2, SP_DESKTOP_ZOOM_INC);

}

void
sp_zoom_out (GtkWidget * widget) {
  SPDesktop * desktop;
  ArtDRect d;

  desktop = SP_ACTIVE_DESKTOP;
  if (desktop == NULL) return;
  g_return_if_fail (SP_IS_DESKTOP (desktop));
  
  sp_desktop_get_visible_area (desktop, &d);

  sp_desktop_zoom_relative (desktop, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2, 1/SP_DESKTOP_ZOOM_INC);
}

void
sp_zoom_1_to_2 (GtkWidget * widget)
{
	SPDesktop * desktop;
	ArtDRect d;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;

	sp_desktop_get_visible_area (desktop, &d);

	sp_desktop_zoom_absolute (desktop, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2, 0.5);

}

void
sp_zoom_1_to_1 (GtkWidget * widget)
{
	SPDesktop * desktop;
	ArtDRect d;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;

	sp_desktop_get_visible_area (desktop, &d);
	sp_desktop_zoom_absolute (desktop, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2, 1.0);
}

void
sp_zoom_2_to_1 (GtkWidget * widget)
{
	SPDesktop * desktop;
	ArtDRect d;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;

	sp_desktop_get_visible_area (desktop, &d);

	sp_desktop_zoom_absolute (desktop, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2, 2.0);
}

void
sp_zoom_string (const gchar * zoom_str) {
	SPDesktop * desktop;
	gchar * zoom_str2;
	ArtDRect d;
	gdouble any;
	
	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	
	zoom_str2 = g_strndup(zoom_str, 5); /* g_new'ed 5+1 gchar */
	zoom_str2[5] = '\0';
	any = strtod(zoom_str2,NULL) /100;
	g_free (zoom_str2);
	if (any < 0.001) return;
	
	sp_desktop_get_visible_area (SP_ACTIVE_DESKTOP, &d);
	sp_desktop_zoom_absolute (SP_ACTIVE_DESKTOP, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2, any);
}

/*
 * zoom toolbox
 */

void
sp_zoom_any (void) {
	gchar * zoom_str;
	zoom_str = gtk_editable_get_chars ((GtkEditable *) zoom_any, 0, 4);
	sp_zoom_string (zoom_str);
}


#define SP_ZOOM_CONTEXT_C

#include <math.h>
#include "rubberband.h"
#include "mdi-document.h"
#include "mdi-desktop.h"
#include "document.h"
#include "selection.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "pixmaps/cursor-zoom.xpm"
#include "zoom-context.h"

static void sp_zoom_context_class_init (SPZoomContextClass * klass);
static void sp_zoom_context_init (SPZoomContext * zoom_context);
static void sp_zoom_context_destroy (GtkObject * object);

static void sp_zoom_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_zoom_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_zoom_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static SPEventContextClass * parent_class;

GtkType
sp_zoom_context_get_type (void)
{
	static GtkType zoom_context_type = 0;

	if (!zoom_context_type) {

		static const GtkTypeInfo zoom_context_info = {
			"SPZoomContext",
			sizeof (SPZoomContext),
			sizeof (SPZoomContextClass),
			(GtkClassInitFunc) sp_zoom_context_class_init,
			(GtkObjectInitFunc) sp_zoom_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		zoom_context_type = gtk_type_unique (sp_event_context_get_type (), &zoom_context_info);
	}

	return zoom_context_type;
}

static void
sp_zoom_context_class_init (SPZoomContextClass * klass)
{
	GtkObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = gtk_type_class (sp_event_context_get_type ());

	object_class->destroy = sp_zoom_context_destroy;

	event_context_class->setup = sp_zoom_context_setup;
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
sp_zoom_context_destroy (GtkObject * object)
{
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_zoom_context_setup (SPEventContext * event_context, SPDesktop * desktop)
{
	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);
}

static gint
sp_zoom_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	gint ret;

	ret = FALSE;

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
		ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);

	return ret;
}

static gint
sp_zoom_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	SPDesktop * desktop;
	ArtPoint p;
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
				sp_desktop_show_region (desktop, b.x0, b.y0, b.x1, b.y1);
			} else {
				sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
				sp_desktop_zoom_relative (desktop, 2, p.x, p.y);
				ret = TRUE;
			}
			break;
		case 3:
			p.x = event->button.x;
			p.y = event->button.y;
			sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
			sp_desktop_zoom_relative (desktop, 0.5, p.x, p.y);
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
		if (SP_EVENT_CONTEXT_CLASS (parent_class)->root_handler)
			ret = SP_EVENT_CONTEXT_CLASS (parent_class)->root_handler (event_context, event);
	}

	return ret;
}

/* Menu handlers */

void
sp_zoom_selection (GtkWidget * widget)
{
	SPSelection * selection;
	ArtDRect d;

	selection = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	g_return_if_fail (selection != NULL);

	sp_selection_bbox (selection, &d);
	if ((fabs (d.x1 - d.x0) < 1.0) || (fabs (d.y1 - d.y0) < 1.0)) return;
	sp_desktop_show_region (SP_ACTIVE_DESKTOP, d.x0 - 20.0, d.y0 - 20.0, d.x1 + 20.0, d.y1 + 20.0);
}

void
sp_zoom_drawing (GtkWidget * widget)
{
	SPItem * docitem;
	ArtDRect d;

	docitem = SP_ITEM (sp_document_root (SP_ACTIVE_DOCUMENT));

	g_return_if_fail (docitem != NULL);

	sp_item_bbox (docitem, &d);
	if ((fabs (d.x1 - d.x0) < 1.0) || (fabs (d.y1 - d.y0) < 1.0)) return;
	sp_desktop_show_region (SP_ACTIVE_DESKTOP, d.x0 - 20.0, d.y0 - 20.0, d.x1 + 20.0, d.y1 + 20.0);
}

void
sp_zoom_page (GtkWidget * widget)
{
	ArtDRect d;

	g_return_if_fail (SP_ACTIVE_DOCUMENT != NULL);

	d.x0 = 0.0;
	d.y0 = 0.0;
	d.x1 = sp_document_width (SP_ACTIVE_DOCUMENT);
	d.y1 = sp_document_height (SP_ACTIVE_DOCUMENT);

	if ((fabs (d.x1 - d.x0) < 1.0) || (fabs (d.y1 - d.y0) < 1.0)) return;

	sp_desktop_show_region (SP_ACTIVE_DESKTOP, d.x0 - 20.0, d.y0 - 20.0, d.x1 + 20.0, d.y1 + 20.0);
}

void
sp_zoom_1_to_2 (GtkWidget * widget)
{
	ArtDRect d;

	sp_desktop_get_visible_area (SP_ACTIVE_DESKTOP, &d);

	sp_desktop_zoom_absolute (SP_ACTIVE_DESKTOP, 0.5, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2);
}

void
sp_zoom_1_to_1 (GtkWidget * widget)
{
	ArtDRect d;

	sp_desktop_get_visible_area (SP_ACTIVE_DESKTOP, &d);

	sp_desktop_zoom_absolute (SP_ACTIVE_DESKTOP, 1.0, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2);
}

void
sp_zoom_2_to_1 (GtkWidget * widget)
{
	ArtDRect d;

	sp_desktop_get_visible_area (SP_ACTIVE_DESKTOP, &d);

	sp_desktop_zoom_absolute (SP_ACTIVE_DESKTOP, 2.0, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2);
}


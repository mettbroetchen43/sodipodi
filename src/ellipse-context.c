#define SP_ELLIPSE_CONTEXT_C

#include <math.h>
#include "sp-ellipse.h"
#include "document.h"
#include "selection.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "pixmaps/cursor-ellipse.xpm"
#include "ellipse-context.h"

static void sp_ellipse_context_class_init (SPEllipseContextClass * klass);
static void sp_ellipse_context_init (SPEllipseContext * ellipse_context);
static void sp_ellipse_context_destroy (GtkObject * object);

static void sp_ellipse_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_ellipse_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_ellipse_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void sp_ellipse_finish (SPEllipse * ellipse);

static SPEventContextClass * parent_class;

GtkType
sp_ellipse_context_get_type (void)
{
	static GtkType ellipse_context_type = 0;

	if (!ellipse_context_type) {

		static const GtkTypeInfo ellipse_context_info = {
			"SPEllipseContext",
			sizeof (SPEllipseContext),
			sizeof (SPEllipseContextClass),
			(GtkClassInitFunc) sp_ellipse_context_class_init,
			(GtkObjectInitFunc) sp_ellipse_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		ellipse_context_type = gtk_type_unique (sp_event_context_get_type (), &ellipse_context_info);
	}

	return ellipse_context_type;
}

static void
sp_ellipse_context_class_init (SPEllipseContextClass * klass)
{
	GtkObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = gtk_type_class (sp_event_context_get_type ());

	object_class->destroy = sp_ellipse_context_destroy;

	event_context_class->setup = sp_ellipse_context_setup;
	event_context_class->root_handler = sp_ellipse_context_root_handler;
	event_context_class->item_handler = sp_ellipse_context_item_handler;
}

static void
sp_ellipse_context_init (SPEllipseContext * ellipse_context)
{
	SPEventContext * event_context;
	
	event_context = SP_EVENT_CONTEXT (ellipse_context);

	event_context->cursor_shape = cursor_ellipse_xpm;
	event_context->hot_x = 4;
	event_context->hot_y = 4;
}

static void
sp_ellipse_context_destroy (GtkObject * object)
{
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_ellipse_context_setup (SPEventContext * event_context, SPDesktop * desktop)
{
	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);
}

static gint
sp_ellipse_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	gint ret;

	ret = FALSE;

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
		ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);

	return ret;
}

static gint
sp_ellipse_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	static int dragging;
	static SPRepr * repr;
	static SPItem * item;
	static double xs, ys;
	SPDesktop * desktop;
	ArtPoint s, c;
	gint ret;

	ret = FALSE;

	desktop = event_context->desktop;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			s.x = xs = event->button.x;
			s.y = ys = event->button.y;
			dragging = TRUE;
			repr = NULL;
			item = NULL;
			ret = TRUE;
			break;
		default:
			break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (dragging && (event->motion.state & GDK_BUTTON1_MASK)) {
			gdouble x0, y0, x1, y1, dx, dy;
			x0 = xs;
			y0 = ys;
			c.x = x1 = event->button.x;
			c.y = y1 = event->button.y;
			dx = x1 - xs;
			dy = y1 - ys;
			if (event->button.state & GDK_CONTROL_MASK) {
				if (fabs (dx) > fabs (dy)) {
					dx = fabs (dy) * dx / fabs (dx);
					x1 = xs + dx;
				} else {
					dy = fabs (dx) * dy / fabs (dy);
					y1 = ys + dy;
				}
			}
			if (event->button.state & GDK_SHIFT_MASK) {
				x0 = xs - dx;
				y0 = ys - dy;
			}
			sp_desktop_w2doc_xy_point (desktop, &s, x0, y0);
			c.x = x1;
			c.y = y1;
			sp_desktop_w2doc_xy_point (desktop, &c, x1, y1);
			x0 = MIN (s.x, c.x);
			y0 = MIN (s.y, c.y);
			x1 = MAX (s.x, c.x);
			y1 = MAX (s.y, c.y);
			if (repr == NULL) {
				repr = sp_repr_new ("ellipse");
				sp_repr_set_attr (repr, "closed", "1");
				item = sp_document_add_repr (SP_DT_DOCUMENT (desktop), repr);
				sp_repr_unref (repr);
			}
			sp_ellipse_set (SP_ELLIPSE (item),
				(x0 + x1) / 2,
				(y0 + y1) / 2,
				(x1 - x0) / 2,
				(y1 - y0) / 2);
			ret = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		switch (event->button.button) {
		case 1:
			dragging = FALSE;
			if (item != NULL) {
				sp_ellipse_finish (SP_ELLIPSE (item));
				sp_selection_set_item (SP_DT_SELECTION (desktop), item);
			}
			sp_document_done (SP_DT_DOCUMENT (desktop));
			repr = NULL;
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

static void
sp_ellipse_finish (SPEllipse * ellipse)
{
	SPRepr * repr;

	repr = SP_OBJECT (ellipse)->repr;

#if 0
	sp_repr_set_attr (repr, "closed", "1");
#endif
	sp_repr_set_double_attribute (repr, "x", ellipse->x);
	sp_repr_set_double_attribute (repr, "y", ellipse->y);
	sp_repr_set_double_attribute (repr, "rx", ellipse->rx);
	sp_repr_set_double_attribute (repr, "ry", ellipse->ry);
}


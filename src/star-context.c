#define SP_STAR_CONTEXT_C

#include <math.h>
#include "sp-star.h"
#include "sodipodi.h"
#include "document.h"
#include "selection.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-snap.h"
#include "pixmaps/cursor-star.xpm"
#include "star-context.h"
#include "sp-metrics.h"

static void sp_star_context_class_init (SPStarContextClass * klass);
static void sp_star_context_init (SPStarContext * star_context);
static void sp_star_context_destroy (GtkObject * object);

static void sp_star_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_star_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_star_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void sp_star_drag (SPStarContext * sc, double x, double y, guint state);
static void sp_star_finish (SPStarContext * sc);

static SPEventContextClass * parent_class;

GtkType
sp_star_context_get_type (void)
{
	static GtkType star_context_type = 0;

	if (!star_context_type) {

		static const GtkTypeInfo star_context_info = {
			"SPStarContext",
			sizeof (SPStarContext),
			sizeof (SPStarContextClass),
			(GtkClassInitFunc) sp_star_context_class_init,
			(GtkObjectInitFunc) sp_star_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		star_context_type = gtk_type_unique (sp_event_context_get_type (), &star_context_info);
	}

	return star_context_type;
}

static void
sp_star_context_class_init (SPStarContextClass * klass)
{
	GtkObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = gtk_type_class (sp_event_context_get_type ());

	object_class->destroy = sp_star_context_destroy;

	event_context_class->setup = sp_star_context_setup;
	event_context_class->root_handler = sp_star_context_root_handler;
	event_context_class->item_handler = sp_star_context_item_handler;
}

static void
sp_star_context_init (SPStarContext * star_context)
{
	SPEventContext * event_context;
	
	event_context = SP_EVENT_CONTEXT (star_context);

	event_context->cursor_shape = cursor_star_xpm;
	event_context->hot_x = 4;
	event_context->hot_y = 4;

	star_context->item = NULL;
}

static void
sp_star_context_destroy (GtkObject * object)
{
	SPStarContext * sc;

	sc = SP_STAR_CONTEXT (object);

	/* fixme: This is necessary because we do not grab */
	if (sc->item) sp_star_finish (sc);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_star_context_setup (SPEventContext * event_context, SPDesktop * desktop)
{
	SPStarContext * sc;

	sc = SP_STAR_CONTEXT (event_context);

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);
}

static gint
sp_star_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	gint ret;

	ret = FALSE;

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
		ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);

	return ret;
}

static gint
sp_star_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	static gboolean dragging;
	SPStarContext * sc;
	gint ret;
	SPDesktop * desktop;

	desktop = event_context->desktop;
	sc = SP_STAR_CONTEXT (event_context);
	ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			dragging = TRUE;
			/* Position center */
			sp_desktop_w2d_xy_point (event_context->desktop, &sc->center, event->button.x, event->button.y);
			/* Snap center to nearest magnetic point */
			sp_desktop_free_snap (event_context->desktop, &sc->center);
			gnome_canvas_item_grab (GNOME_CANVAS_ITEM (desktop->acetate),
						GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK,
						NULL, event->button.time);
			ret = TRUE;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (dragging && event->motion.state && GDK_BUTTON1_MASK) {
			ArtPoint p;
			sp_desktop_w2d_xy_point (event_context->desktop, &p, event->motion.x, event->motion.y);
			sp_star_drag (sc, p.x, p.y, event->motion.state);
			ret = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (event->button.button == 1) {
			dragging = FALSE;
			sp_star_finish (sc);
			ret = TRUE;
			gnome_canvas_item_ungrab (GNOME_CANVAS_ITEM (desktop->acetate), event->button.time);
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
sp_star_drag (SPStarContext * sc, double x, double y, guint state)
{
  SPStar *star;
	SPDesktop * desktop;
	ArtPoint p0, p1;
	gdouble sides, dx, dy, r1, arg1;
	GString * xs, * ys;
	gchar status[80];

	desktop = SP_EVENT_CONTEXT (sc)->desktop;

	if (!sc->item) {
		SPRepr * repr, * style;
		SPCSSAttr * css;
		/* Create object */
		repr = sp_repr_new ("polygon");
                sp_repr_set_attr (repr, "sodipodi:type", "star");
		/* Set style */
		style = sodipodi_get_repr (SODIPODI, "paint.shape.star");
		if (style) {
			css = sp_repr_css_attr_inherited (style, "style");
			sp_repr_css_set (repr, css, "style");
			sp_repr_css_attr_unref (css);
		}
		sc->item = (SPItem *) sp_document_add_repr (SP_DT_DOCUMENT (desktop), repr);
		sp_repr_unref (repr);
	}

	/* This is bit ugly, but so we are */

/*  	if (state & GDK_CONTROL_MASK) { */
/*  	} else if (state & GDK_SHIFT_MASK) { */

	/* Free movement for corner point */
	sp_desktop_d2doc_xy_point (desktop, &p0, sc->center.x, sc->center.y);
	sp_desktop_d2doc_xy_point (desktop, &p1, x, y);
	sp_desktop_free_snap (desktop, &p1);

	star = SP_STAR(sc->item);

	sides = (gdouble)star->sides;
	dx = p1.x - p0.x;
	dy = p1.y - p0.y;
	r1 = hypot (dx, dy);
	arg1 = atan2 (dy, dx);
	
	sp_star_set (star, star->sides,
		     p0.x, p0.y, r1, r1*(sides-2.0)/sides, arg1, arg1 + M_PI/sides);

	/* status text */
	xs = SP_PT_TO_METRIC_STRING (fabs(p0.x), SP_DEFAULT_METRIC);
	ys = SP_PT_TO_METRIC_STRING (fabs(p0.y), SP_DEFAULT_METRIC);
	sprintf (status, "Draw star at (%s,%s)", xs->str, ys->str);
	sp_desktop_set_status (desktop, status);
	g_string_free (xs, FALSE);
	g_string_free (ys, FALSE);
}

static void
sp_star_finish (SPStarContext * sc)
{
	if (sc->item != NULL) {
		SPDesktop *desktop;
		SPObject  *object;

		desktop = SP_EVENT_CONTEXT (sc)->desktop;
		object  = SP_OBJECT(sc->item);
		
                sp_shape_set_shape(SP_SHAPE(sc->item));
		sp_object_invoke_write_repr (object, object->repr);

		sp_selection_set_item (SP_DT_SELECTION (desktop), sc->item);
		sp_document_done (SP_DT_DOCUMENT (desktop));

		sc->item = NULL;
	}
}


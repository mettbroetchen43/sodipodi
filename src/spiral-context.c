#define SP_SPIRAL_CONTEXT_C

#include <math.h>
#include "sp-spiral.h"
#include "sodipodi.h"
#include "document.h"
#include "selection.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-snap.h"
#include "pixmaps/cursor-spiral.xpm"
#include "spiral-context.h"
#include "sp-metrics.h"

static void sp_spiral_context_class_init (SPSpiralContextClass * klass);
static void sp_spiral_context_init (SPSpiralContext * spiral_context);
static void sp_spiral_context_destroy (GtkObject * object);

static void sp_spiral_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_spiral_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_spiral_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void sp_spiral_drag (SPSpiralContext * sc, double x, double y, guint state);
static void sp_spiral_finish (SPSpiralContext * sc);

static SPEventContextClass * parent_class;

GtkType
sp_spiral_context_get_type (void)
{
	static GtkType spiral_context_type = 0;

	if (!spiral_context_type) {

		static const GtkTypeInfo spiral_context_info = {
			"SPSpiralContext",
			sizeof (SPSpiralContext),
			sizeof (SPSpiralContextClass),
			(GtkClassInitFunc) sp_spiral_context_class_init,
			(GtkObjectInitFunc) sp_spiral_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		spiral_context_type = gtk_type_unique (sp_event_context_get_type (), &spiral_context_info);
	}

	return spiral_context_type;
}

static void
sp_spiral_context_class_init (SPSpiralContextClass * klass)
{
	GtkObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = gtk_type_class (sp_event_context_get_type ());

	object_class->destroy = sp_spiral_context_destroy;

	event_context_class->setup = sp_spiral_context_setup;
	event_context_class->root_handler = sp_spiral_context_root_handler;
	event_context_class->item_handler = sp_spiral_context_item_handler;
}

static void
sp_spiral_context_init (SPSpiralContext * spiral_context)
{
	SPEventContext * event_context;
	
	event_context = SP_EVENT_CONTEXT (spiral_context);

	event_context->cursor_shape = cursor_spiral_xpm;
	event_context->hot_x = 4;
	event_context->hot_y = 4;

	spiral_context->item = NULL;
}

static void
sp_spiral_context_destroy (GtkObject * object)
{
	SPSpiralContext * sc;

	sc = SP_SPIRAL_CONTEXT (object);

	/* fixme: This is necessary because we do not grab */
	if (sc->item) sp_spiral_finish (sc);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_spiral_context_setup (SPEventContext * event_context, SPDesktop * desktop)
{
	SPSpiralContext * sc;

	sc = SP_SPIRAL_CONTEXT (event_context);

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);
}

static gint
sp_spiral_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	gint ret;

	ret = FALSE;

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
		ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);

	return ret;
}

static gint
sp_spiral_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	static gboolean dragging;
	SPSpiralContext * sc;
	gint ret;
	SPDesktop * desktop;

	desktop = event_context->desktop;
	sc = SP_SPIRAL_CONTEXT (event_context);
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
			sp_spiral_drag (sc, p.x, p.y, event->motion.state);
			ret = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		if (event->button.button == 1) {
			dragging = FALSE;
			sp_spiral_finish (sc);
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
sp_spiral_drag (SPSpiralContext * sc, double x, double y, guint state)
{
	SPSpiral *spiral;
	SPDesktop *desktop;
	ArtPoint p0, p1;
	gdouble dx, dy, rad, arg;
	GString *xs, *ys;
	gchar status[80];

	desktop = SP_EVENT_CONTEXT (sc)->desktop;

	if (!sc->item) {
		SPRepr * repr, * style;
		SPCSSAttr * css;
		/* Create object */
		repr = sp_repr_new ("path");
                sp_repr_set_attr (repr, "sodipodi:type", "spiral");
		/* Set style */
		style = sodipodi_get_repr (SODIPODI, "paint.spiral");
		if (style) {
			css = sp_repr_css_attr_inherited (style, "style");
			sp_repr_css_set (repr, css, "style");
			sp_repr_css_attr_unref (css);
		}
                else
                  g_print ("sp_spiral_drag: cant get style\n");

		sc->item = (SPItem *) sp_document_add_repr (SP_DT_DOCUMENT (desktop), repr);
		sp_repr_unref (repr);
	}

	/* This is bit ugly, but so we are */

/*  	if (state & GDK_CONTROL_MASK) { */
/*  	} else if (state & GDK_SHIFT_MASK) { */

	/* Free movement for corner point */
	p0.x = sc->center.x;
	p0.y = sc->center.y;
	p1.x = x;
	p1.y = y;
	sp_desktop_free_snap (desktop, &p1);
	
	spiral = SP_SPIRAL(sc->item);

	dx = p1.x - p0.x;
	dy = - (p1.y - p0.y);
	rad = hypot (dx, dy);
	arg = atan2 (dy, dx) - 2.0*M_PI*spiral->revo;
	
        /* Fixme: these parameters should be got from dialog box */
	sp_spiral_set (spiral, p0.x, p0.y,
		       /*expansion*/ spiral->exp,
		       /*revolution*/ spiral->revo,
		       rad, arg,
		       /*t0*/ spiral->t0);
	
	/* status text */
	xs = SP_PT_TO_METRIC_STRING (fabs(p0.x), SP_DEFAULT_METRIC);
	ys = SP_PT_TO_METRIC_STRING (fabs(p0.y), SP_DEFAULT_METRIC);
	sprintf (status, "Draw spiral at (%s,%s)", xs->str, ys->str);
	sp_desktop_set_status (desktop, status);
	g_string_free (xs, FALSE);
	g_string_free (ys, FALSE);
}

static void
sp_spiral_finish (SPSpiralContext * sc)
{
	if (sc->item != NULL) {
		SPDesktop *desktop;
		SPSpiral  *spiral;
		SPRepr    *repr;

		desktop = SP_EVENT_CONTEXT (sc)->desktop;
		spiral = SP_SPIRAL (sc->item);
		repr = SP_OBJECT (sc->item)->repr;

		sp_shape_set_shape(SP_SHAPE(spiral));
		sp_object_invoke_write_repr (SP_OBJECT(spiral), repr);

		sp_selection_set_item (SP_DT_SELECTION (desktop), sc->item);
		sp_document_done (SP_DT_DOCUMENT (desktop));

		sc->item = NULL;
	}
}

#define SP_DRAW_CONTEXT_C

/*
 * drawing-context handlers
 *
 * todo: use intelligent bpathing, i.e. without copying def all the time
 *       make freehand generating curves instead of lines
 *
 */

#include <math.h>
#include "xml/repr.h"
#include "svg/svg.h"
#include "helper/curve.h"
#include "helper/sodipodi-ctrl.h"
#include "display/canvas-shape.h"

#include "document.h"
#include "selection.h"
#include "desktop-events.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "draw-context.h"

#define MIN_FREEHAND_LEN 5.0

struct _SPDrawCtrl {
	GnomeCanvasItem * item;
	gint inside;
	guint fill_color;
	ArtPoint p;
};

static void sp_draw_context_class_init (SPDrawContextClass * klass);
static void sp_draw_context_init (SPDrawContext * draw_context);
static void sp_draw_context_destroy (GtkObject * object);

static void sp_draw_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_draw_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_draw_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void sp_draw_finish_item (SPDrawContext * draw_context);

static void sp_draw_ctrl_test_inside (SPDrawContext * draw_context, double x, double y);
static void sp_draw_move_ctrl (SPDrawContext * draw_context, double x, double y);
static void sp_draw_remove_ctrl (SPDrawContext * draw_context);
static SPCanvasShape * sp_draw_create_item (SPDrawContext * draw_context);

static void sp_draw_path_startline (SPDrawContext * draw_context, double x, double y);
static void sp_draw_path_freehand (SPDrawContext * draw_context, double x, double y);
static void sp_draw_path_endfreehand (SPDrawContext * draw_context, double x, double y);
static void sp_draw_path_lineendpoint (SPDrawContext * draw_context, double x, double y);
static void sp_draw_path_endline (SPDrawContext * draw_context, double x, double y);

static SPEventContextClass * parent_class;

GtkType
sp_draw_context_get_type (void)
{
	static GtkType draw_context_type = 0;

	if (!draw_context_type) {

		static const GtkTypeInfo draw_context_info = {
			"SPDrawContext",
			sizeof (SPDrawContext),
			sizeof (SPDrawContextClass),
			(GtkClassInitFunc) sp_draw_context_class_init,
			(GtkObjectInitFunc) sp_draw_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		draw_context_type = gtk_type_unique (sp_event_context_get_type (), &draw_context_info);
	}

	return draw_context_type;
}

static void
sp_draw_context_class_init (SPDrawContextClass * klass)
{
	GtkObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = gtk_type_class (sp_event_context_get_type ());

	object_class->destroy = sp_draw_context_destroy;

	event_context_class->setup = sp_draw_context_setup;
	event_context_class->root_handler = sp_draw_context_root_handler;
	event_context_class->item_handler = sp_draw_context_item_handler;
}

static void
sp_draw_context_init (SPDrawContext * draw_context)
{
	draw_context->shape = NULL;
	draw_context->curve = sp_curve_new ();
	draw_context->ctrl = NULL;
}

static void
sp_draw_context_destroy (GtkObject * object)
{
	SPDrawContext * draw_context;

	draw_context = SP_DRAW_CONTEXT (object);

	if (draw_context->curve) {
		sp_draw_finish_item (draw_context);
		sp_curve_unref (draw_context->curve);
	}

	if (draw_context->ctrl) {
		if (draw_context->ctrl->item)
			gtk_object_destroy (GTK_OBJECT (draw_context->ctrl->item));
		g_free (draw_context->ctrl);
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_draw_context_setup (SPEventContext * event_context, SPDesktop * desktop)
{
	SPDrawContext * draw_context;

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);

	draw_context = SP_DRAW_CONTEXT (event_context);

	draw_context->ctrl = g_new (SPDrawCtrl, 1);

	draw_context->ctrl->item = NULL;
	draw_context->ctrl->inside = -1;
	draw_context->ctrl->fill_color = 0xff0000ff;
	draw_context->ctrl->p.x = draw_context->ctrl->p.y = 0.0;
}

static void
sp_draw_finish_item (SPDrawContext * draw_context)
{
	SPRepr * repr;
	SPItem * item;
	gchar * str;
	gdouble d2doc[6];
	ArtBpath * abp;

	g_return_if_fail (draw_context != NULL);
	g_return_if_fail (draw_context->curve != NULL);

	if (draw_context->shape != NULL) gtk_object_destroy (GTK_OBJECT (draw_context->shape));
	draw_context->shape = NULL;

	if (sp_curve_first_bpath (draw_context->curve)) {
		sp_desktop_d2doc_affine (SP_EVENT_CONTEXT (draw_context)->desktop, d2doc);
		abp = art_bpath_affine_transform (sp_curve_first_bpath (draw_context->curve), d2doc);
		str = sp_svg_write_path (abp);
		art_free (abp);
		g_return_if_fail (str != NULL);
		repr = sp_repr_new ("path");
		sp_repr_set_attr (repr, "d", str);
		g_free (str);
		sp_repr_set_attr (repr, "style", "stroke:#000; stroke-width:1");
		item = sp_document_add_repr (SP_DT_DOCUMENT (SP_EVENT_CONTEXT (draw_context)->desktop), repr);
		sp_document_done (SP_DT_DOCUMENT (SP_EVENT_CONTEXT (draw_context)->desktop));
		sp_repr_unref (repr);
		sp_selection_set_item (SP_DT_SELECTION (SP_EVENT_CONTEXT (draw_context)->desktop), item);
	}

	sp_curve_reset (draw_context->curve);
}

static gint
sp_draw_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	gint ret;

	ret = FALSE;

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
		ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);

	return ret;
}

gint
sp_draw_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	SPDesktop * desktop;
	static ArtPoint o;
	static gint freehand = FALSE;
	static gint addline = FALSE;
	SPDrawContext * draw_context;
	gint ret;
	ArtPoint p;
	double dx, dy;

	ret = FALSE;

	desktop = event_context->desktop;
	draw_context = SP_DRAW_CONTEXT (event_context);

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			sp_desktop_w2d_xy_point (desktop, &o, event->button.x, event->button.y);
			if (addline) {
				addline = FALSE;
				sp_draw_path_endline (draw_context, o.x, o.y);
			} else {
				addline = TRUE;
				sp_draw_path_startline (draw_context, o.x, o.y);
			}
			ret = TRUE;
			break;
		default:
			break;
		}
		break;
	case GDK_BUTTON_RELEASE:
		switch (event->button.button) {
		case 1:
			sp_desktop_w2d_xy_point (desktop, &o, event->button.x, event->button.y);
			if (freehand) {
				addline = FALSE;
				freehand = FALSE;
				sp_draw_path_endfreehand (draw_context, o.x, o.y);
			}
			ret = TRUE;
			break;
		default:
			break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
		sp_draw_ctrl_test_inside (draw_context, p.x, p.y);
		if (event->motion.state & GDK_BUTTON1_MASK) {
			freehand = TRUE;
			dx = p.x - o.x;
			dy = p.y - o.y;
			if (sqrt (dx * dx + dy * dy) > MIN_FREEHAND_LEN) {
				sp_draw_path_freehand (draw_context, p.x, p.y);
				o.x = p.x;
				o.y = p.y;
			}
			ret = TRUE;
		} else {
			if (addline) {
				sp_draw_path_lineendpoint (draw_context, p.x, p.y);
				ret = TRUE;
			}
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
sp_draw_ctrl_test_inside (SPDrawContext * draw_context, double x, double y)
{
	ArtPoint p;

	if (draw_context->ctrl->item == NULL) return;

	sp_desktop_d2w_xy_point (SP_EVENT_CONTEXT (draw_context)->desktop, &p, x, y);

	if ((fabs (p.x - draw_context->ctrl->p.x) > 4.0) || (fabs (p.y - draw_context->ctrl->p.y) > 4.0)) {
		if (draw_context->ctrl->inside != 0) {
			gnome_canvas_item_set (draw_context->ctrl->item,
				"filled", FALSE, NULL);
			draw_context->ctrl->inside = 0;
		}
	} else {
		if (draw_context->ctrl->inside != 1) {
			gnome_canvas_item_set (draw_context->ctrl->item,
				"filled", TRUE, NULL);
				draw_context->ctrl->inside = 1;
		}
	}
}


static void
sp_draw_move_ctrl (SPDrawContext * draw_context, double x, double y)
{
	if (draw_context->ctrl->item == NULL) {
		draw_context->ctrl->item = gnome_canvas_item_new (SP_DT_CONTROLS (SP_EVENT_CONTEXT (draw_context)->desktop),
			SP_TYPE_CTRL,
			"size", 4.0,
			"filled", 0,
			"fill_color", draw_context->ctrl->fill_color,
			"stroked", 1,
			"stroke_color", 0x000000ff,
			NULL);
		gtk_signal_connect (GTK_OBJECT (draw_context->ctrl->item), "event",
			GTK_SIGNAL_FUNC (sp_desktop_root_handler), SP_EVENT_CONTEXT (draw_context)->desktop);
	}
	sp_ctrl_moveto (SP_CTRL (draw_context->ctrl->item), x, y);
	sp_desktop_d2w_xy_point (SP_EVENT_CONTEXT (draw_context)->desktop, &draw_context->ctrl->p, x, y);
	draw_context->ctrl->inside = -1;
}

static void sp_draw_remove_ctrl (SPDrawContext * draw_context)
{
	if (draw_context->ctrl->item) {
		gtk_object_destroy (GTK_OBJECT (draw_context->ctrl->item));
		draw_context->ctrl->item = NULL;
	}
}

static void
sp_draw_path_startline (SPDrawContext * draw_context, double x, double y)
{
	if (draw_context->shape != NULL) {
		if (draw_context->ctrl->inside == 1) {
			sp_curve_lineto_moving (draw_context->curve, x, y);
			return;
		} else {
			sp_draw_finish_item (draw_context);
		}
	}
	draw_context->shape = sp_draw_create_item (draw_context);
	sp_curve_moveto (draw_context->curve, x, y);
	sp_draw_move_ctrl (draw_context, draw_context->curve->bpath->x3, draw_context->curve->bpath->y3);
	sp_draw_ctrl_test_inside (draw_context, x, y);
}

static void
sp_draw_path_endline (SPDrawContext * draw_context, double x, double y)
{
	if (draw_context->ctrl->inside == 1) {
		/* we are closed path */
		if (draw_context->curve->end < 3) {
			/* path is too small :-( */
			gtk_object_destroy (GTK_OBJECT (draw_context->shape));
			draw_context->shape = NULL;
			sp_curve_reset (draw_context->curve);
			sp_draw_remove_ctrl (draw_context);
			return;
		}
		sp_curve_closepath_current (draw_context->curve);
		sp_draw_finish_item (draw_context);
		sp_draw_remove_ctrl (draw_context);
		return;
	}
	sp_curve_lineto (draw_context->curve, x, y);
	/* fixme: all these */
	sp_canvas_shape_change_bpath (draw_context->shape, draw_context->curve);
	sp_draw_move_ctrl (draw_context, x, y);
	sp_draw_ctrl_test_inside (draw_context, x, y);
	return;
}

static void
sp_draw_path_lineendpoint (SPDrawContext * draw_context, double x, double y)
{
	sp_curve_lineto_moving (draw_context->curve, x, y);
	sp_canvas_shape_change_bpath (draw_context->shape, draw_context->curve);
	sp_draw_move_ctrl (draw_context, draw_context->curve->bpath->x3, draw_context->curve->bpath->y3);
	sp_draw_ctrl_test_inside (draw_context, x, y);
}

static void
sp_draw_path_freehand (SPDrawContext * draw_context, double x, double y)
{
	sp_curve_lineto (draw_context->curve, x, y);
	sp_canvas_shape_change_bpath (draw_context->shape, draw_context->curve);
	sp_draw_move_ctrl (draw_context, draw_context->curve->bpath->x3, draw_context->curve->bpath->y3);
	sp_draw_ctrl_test_inside (draw_context, x, y);
}

static void
sp_draw_path_endfreehand (SPDrawContext * draw_context, double x, double y)
{
	if (draw_context->ctrl->inside == 1) {
		/* we are closed path */
		if (draw_context->curve->end < 3) {
			/* path is too small :-( */
			gtk_object_destroy (GTK_OBJECT (draw_context->shape));
			draw_context->shape = NULL;
			sp_curve_reset (draw_context->curve);
			sp_draw_remove_ctrl (draw_context);
			return;
		}
		sp_curve_closepath (draw_context->curve);
		sp_draw_finish_item (draw_context);
		sp_draw_remove_ctrl (draw_context);
		return;
	}
	sp_curve_lineto (draw_context->curve, x, y);
	sp_canvas_shape_change_bpath (draw_context->shape, draw_context->curve);
	sp_draw_move_ctrl (draw_context, x, y);
	sp_draw_ctrl_test_inside (draw_context, x, y);
}

#if 0
static void
draw_bpath_init (SPDrawContext * draw_context)
{
	draw_context->bpath = art_new (ArtBpath, 1024);
	draw_context->length = 1024;
	draw_context->bpath->code = ART_END;
	draw_context->pos = 0;
	draw_context->moving_end = FALSE;
}

static void
draw_stretch (SPDrawContext * draw_context)
{
	draw_context->bpath = art_realloc (draw_context->bpath, draw_context->length + 1024);
	draw_context->length += 1024;
}

static void
draw_curveto (SPDrawContext * draw_context, gdouble x0, gdouble y0, gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
	ArtBpath * bp;

	g_assert (draw_context->bpath != NULL);
	g_assert (draw_context->pos > 0);

	bp = draw_context->bpath + draw_context->pos;

	bp->code = ART_CURVETO;
	bp->x1 = x0;
	bp->y1 = y0;
	bp->x2 = x1;
	bp->y2 = y1;
	bp->x3 = x2;
	bp->y3 = y2;

	draw_context->pos++;

	if (draw_context->pos >= draw_context->length)
		draw_stretch (draw_context);

	bp = draw_context->bpath + draw_context->pos;

	bp->code = ART_END;

	draw_context->moving_end = FALSE;
}

static void
draw_lineto (SPDrawContext * draw_context, gdouble x, gdouble y)
{
	ArtBpath * bp;

	g_assert (draw_context->bpath != NULL);
	g_assert (draw_context->pos > 0);

	if (draw_context->moving_end) {
		draw_setend (draw_context, x, y);
		draw_context->moving_end = FALSE;
		return;
	}

	bp = draw_context->bpath + draw_context->pos;

	bp->code = ART_LINETO;
	bp->x3 = x;
	bp->y3 = y;

	draw_context->pos++;

	if (draw_context->pos >= draw_context->length)
		draw_stretch (draw_context);

	bp = draw_context->bpath + draw_context->pos;

	bp->code = ART_END;

	draw_context->moving_end = FALSE;
}

static void
draw_moveto (SPDrawContext * draw_context, gdouble x, gdouble y)
{
	ArtBpath * bp;

	g_assert (draw_context->bpath == NULL);

	if (draw_context->bpath == NULL)
		draw_bpath_init (draw_context);

	bp = draw_context->bpath + draw_context->pos;

	bp->code = ART_MOVETO_OPEN;
	bp->x3 = x;
	bp->y3 = y;

	draw_context->pos++;

	if (draw_context->pos >= draw_context->length)
		draw_stretch (draw_context);

	bp = draw_context->bpath + draw_context->pos;

	bp->code = ART_END;

	draw_context->moving_end = FALSE;
}

static void
draw_closepath (SPDrawContext * draw_context)
{
	ArtBpath * bp;

	g_assert (draw_context->bpath != NULL);
	g_assert (draw_context->pos > 1);

	bp = draw_context->bpath + draw_context->pos - 1;

	if ((bp->x3 != draw_context->bpath->x3) || (bp->y3 != draw_context->bpath->y3)) {
		draw_lineto (draw_context, draw_context->bpath->x3, draw_context->bpath->y3);
	}

	draw_context->bpath->code = ART_MOVETO;

	draw_context->pos++;

	if (draw_context->pos >= draw_context->length)
		draw_stretch (draw_context);

	bp = draw_context->bpath + draw_context->pos;

	bp->code = ART_END;
}

static void
draw_closepath_current (SPDrawContext * draw_context)
{
	ArtBpath * bp;

	g_assert (draw_context->bpath != NULL);
	g_assert (draw_context->pos > 2);

	bp = draw_context->bpath + draw_context->pos - 1;

	bp->x3 = draw_context->bpath->x3;
	bp->y3 = draw_context->bpath->y3;
	draw_context->bpath->code = ART_MOVETO;
}

static void
draw_setend (SPDrawContext * draw_context, double x, double y)
{
	ArtBpath * bp;

	g_assert (draw_context->bpath != NULL);
	g_assert (draw_context->pos > 0);

	if (!draw_context->moving_end) {
		draw_lineto (draw_context, x, y);
		draw_context->moving_end = TRUE;
	} else {
		bp = draw_context->bpath + draw_context->pos - 1;

		bp->x3 = x;
		bp->y3 = y;
	}
}
#endif

static SPCanvasShape *
sp_draw_create_item (SPDrawContext * draw_context)
{
	SPStroke * stroke;
	SPCanvasShape * shape;

	stroke = sp_stroke_new_colored (0x000000ff, 2.0, FALSE);
	shape = (SPCanvasShape *)
		gnome_canvas_item_new (SP_DT_SKETCH (SP_EVENT_CONTEXT (draw_context)->desktop),
			SP_TYPE_CANVAS_SHAPE, NULL);
	sp_canvas_shape_set_stroke (shape, stroke);
	sp_stroke_unref (stroke);
	gtk_signal_connect (GTK_OBJECT (shape), "event",
		GTK_SIGNAL_FUNC (sp_desktop_root_handler), SP_EVENT_CONTEXT (draw_context)->desktop);

	return shape;
}


#define SP_DRAW_CONTEXT_C

/*
 * drawing-context handlers
 *
 * todo: use intelligent bpathing, i.e. without copying def all the time
 *       make freehand generating curves instead of lines
 *       do something with canvas grabbing current item, when button is down
 *
 */

#include <math.h>
#include "xml/repr.h"
#include "svg/svg.h"
#include "helper/sodipodi-ctrl.h"
#include "display/canvas-shape.h"

#include "desktop.h"
#include "desktop-affine.h"
#include "event-broker.h"
#include "sp-draw-context.h"

#define MIN_FREEHAND_LEN 5.0

typedef struct _SPDrawing SPDrawing;
typedef struct _SPDrawCtrl SPDrawCtrl;

struct _SPDrawCtrl {
	SPDesktop * desktop;
	GnomeCanvasItem * item;
	gint inside;
	guint fill_color;
	ArtPoint p;
};

struct _SPDrawing {
	SPDesktop * desktop;
	SPCanvasShape * shape;
	ArtBpath * bpath;
	gint pos;
	gint length;
	gboolean moving_end;
	SPDrawCtrl ctrl;
};

static void sp_draw_ctrl_test_inside (SPDrawing * drawing, double x, double y);
static void sp_draw_move_ctrl (SPDrawing * drawing, double x, double y);
static void sp_draw_remove_ctrl (SPDrawing * drawing);
static SPCanvasShape * sp_draw_create_item (SPDrawing * drawing);
static SPRepr * sp_draw_finish_item (SPDrawing * drawing);

static SPDrawing * sp_drawing_get_desktop_drawing (SPDesktop * desktop);

static void draw_bpath_init (SPDrawing * drawing);
static void draw_stretch (SPDrawing * drawing);
static void draw_moveto (SPDrawing * drawing, gdouble x, gdouble y);
static void draw_lineto (SPDrawing * drawing, gdouble x, gdouble y);
static void draw_curveto (SPDrawing * drawing, gdouble x0, gdouble y0, gdouble x1, gdouble y1, gdouble x2, gdouble y2);
static void draw_closepath (SPDrawing * drawing);
static void draw_closepath_current (SPDrawing * drawing);
static void draw_setend (SPDrawing * drawing, gdouble x, gdouble y);

static void sp_draw_path_startline (SPDrawing * drawing, double x, double y);
static void sp_draw_path_freehand (SPDrawing * drawing, double x, double y);
static void sp_draw_path_endfreehand (SPDrawing * drawing, double x, double y);
static void sp_draw_path_lineendpoint (SPDrawing * drawing, double x, double y);
static void sp_draw_path_endline (SPDrawing * drawing, double x, double y);

void sp_draw_context_set (SPDesktop * desktop)
{
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));
}

void sp_draw_context_release (SPDesktop * desktop)
{
	SPDrawing * drawing;

	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	drawing = sp_drawing_get_desktop_drawing (desktop);

	if (drawing->shape != NULL) {
		sp_draw_finish_item (drawing);
	}
	sp_draw_remove_ctrl (drawing);
}

static SPRepr *
sp_draw_finish_item (SPDrawing * drawing)
{
	SPRepr * repr;
	gchar * str;
	gdouble d2doc[6];
	ArtBpath * abp;

	g_return_val_if_fail (drawing != NULL, NULL);

	if (drawing->shape == NULL) return NULL;

	repr = NULL;

	gtk_object_destroy ((GtkObject *) drawing->shape);
	drawing->shape = NULL;

	if ((drawing->bpath != NULL) && (drawing->pos >= 2)) {
		/* fixme: d2doc */
		sp_desktop_d2doc_affine (drawing->desktop, d2doc);
		abp = art_bpath_affine_transform (drawing->bpath, d2doc);
		str = sp_svg_write_path (abp);
		g_assert (str != NULL);
		repr = sp_repr_new_with_name ("path");
		sp_repr_set_attr (repr, "d", str);
		sp_repr_set_attr (repr, "style", "stroke:#000; stroke-width:1");
		sp_repr_append_child (SP_ITEM (SP_DT_DOCUMENT (drawing->desktop))->repr, repr);
		sp_repr_unref (repr);
		g_free (str);
		art_free (abp);
#if 0
		sp_selection_set_repr (repr);
#endif
	}

	art_free (drawing->bpath);
	drawing->bpath = NULL;

	return repr;
}

gint
sp_draw_handler (SPDesktop * desktop, SPItem * item, GdkEvent * event)
{
	return FALSE;
}

gint
sp_draw_root_handler (SPDesktop * desktop, GdkEvent * event)
{
	static ArtPoint o;
	static gint freehand = FALSE;
	static gint addline = FALSE;
	SPDrawing * drawing;
	gint ret;
	ArtPoint p;
	double dx, dy;

	ret = FALSE;

	drawing = sp_drawing_get_desktop_drawing (desktop);

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			sp_desktop_w2d_xy_point (desktop, &o, event->button.x, event->button.y);
			if (addline) {
				addline = FALSE;
				sp_draw_path_endline (drawing, o.x, o.y);
			} else {
				addline = TRUE;
				sp_draw_path_startline (drawing, o.x, o.y);
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
				sp_draw_path_endfreehand (drawing, o.x, o.y);
			}
			ret = TRUE;
			break;
		default:
			break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
		sp_draw_ctrl_test_inside (drawing, p.x, p.y);
		if (event->motion.state & GDK_BUTTON1_MASK) {
			freehand = TRUE;
			dx = p.x - o.x;
			dy = p.y - o.y;
			if (sqrt (dx * dx + dy * dy) > MIN_FREEHAND_LEN) {
				sp_draw_path_freehand (drawing, p.x, p.y);
				o.x = p.x;
				o.y = p.y;
			}
			ret = TRUE;
		} else {
			if (addline) {
				sp_draw_path_lineendpoint (drawing, p.x, p.y);
				ret = TRUE;
			}
		}
		break;
	default:
		break;
	}

	return ret;
}

static void
sp_draw_ctrl_test_inside (SPDrawing * drawing, double x, double y)
{
	ArtPoint p;

	if (drawing->ctrl.item == NULL)
		return;

	sp_desktop_d2w_xy_point (drawing->desktop, &p, x, y);

	if ((fabs (p.x - drawing->ctrl.p.x) > 4.0) || (fabs (p.y - drawing->ctrl.p.y) > 4.0)) {
		if (drawing->ctrl.inside != 0) {
			gnome_canvas_item_set (drawing->ctrl.item,
				"filled", FALSE, NULL);
			drawing->ctrl.inside = 0;
		}
	} else {
		if (drawing->ctrl.inside != 1) {
			gnome_canvas_item_set (drawing->ctrl.item,
				"filled", TRUE, NULL);
				drawing->ctrl.inside = 1;
		}
	}
}


static void
sp_draw_move_ctrl (SPDrawing * drawing, double x, double y)
{
	if (drawing->ctrl.item == NULL) {
		drawing->ctrl.item = gnome_canvas_item_new (SP_DT_CONTROLS (drawing->desktop),
			SP_TYPE_CTRL,
			"size", 4.0,
			"filled", 0,
			"fill_color", drawing->ctrl.fill_color,
			"stroked", 1,
			"stroke_color", 0x000000ff,
			NULL);
		gtk_signal_connect (GTK_OBJECT (drawing->ctrl.item), "event",
			GTK_SIGNAL_FUNC (sp_root_handler), drawing->desktop);
	}
	sp_ctrl_moveto (SP_CTRL (drawing->ctrl.item), x, y);
	sp_desktop_d2w_xy_point (drawing->desktop, &drawing->ctrl.p, x, y);
	drawing->ctrl.inside = -1;
}

static void sp_draw_remove_ctrl (SPDrawing * drawing)
{
	if (drawing->ctrl.item) {
		gtk_object_destroy (GTK_OBJECT (drawing->ctrl.item));
		drawing->ctrl.item = NULL;
	}
}

static void
sp_draw_path_startline (SPDrawing * drawing, double x, double y)
{
	if (drawing->shape != NULL) {
		g_assert (drawing->pos >= 2);

		if (drawing->ctrl.inside == 1) {
			draw_setend (drawing, x, y);
			return;
		} else {
			sp_draw_finish_item (drawing);
		}
	}
	drawing->shape = sp_draw_create_item (drawing);
	draw_moveto (drawing, x, y);
	sp_draw_move_ctrl (drawing, drawing->bpath->x3, drawing->bpath->y3);
	sp_draw_ctrl_test_inside (drawing, x, y);
}

static void
sp_draw_path_endline (SPDrawing * drawing, double x, double y)
{
	if (drawing->ctrl.inside == 1) {
		/* we are closed path */
		if (drawing->pos < 3) {
			/* path is too small :-( */
			gtk_object_destroy (GTK_OBJECT (drawing->shape));
			drawing->shape = NULL;
			art_free (drawing->bpath);
			drawing->bpath = NULL;
			sp_draw_remove_ctrl (drawing);
			return;
		}
		draw_closepath_current (drawing);
		sp_draw_finish_item (drawing);
		sp_draw_remove_ctrl (drawing);
		return;
	}
	draw_lineto (drawing, x, y);
	sp_canvas_shape_change_bpath (drawing->shape, drawing->bpath);
	sp_draw_move_ctrl (drawing, x, y);
	sp_draw_ctrl_test_inside (drawing, x, y);
	return;
}

static void
sp_draw_path_lineendpoint (SPDrawing * drawing, double x, double y)
{
	draw_setend (drawing, x, y);
	sp_canvas_shape_change_bpath (drawing->shape, drawing->bpath);
	sp_draw_move_ctrl (drawing, drawing->bpath->x3, drawing->bpath->y3);
	sp_draw_ctrl_test_inside (drawing, x, y);
}

static void
sp_draw_path_freehand (SPDrawing * drawing, double x, double y)
{
	draw_lineto (drawing, x, y);
	sp_canvas_shape_change_bpath (drawing->shape, drawing->bpath);
	sp_draw_move_ctrl (drawing, drawing->bpath->x3, drawing->bpath->y3);
	sp_draw_ctrl_test_inside (drawing, x, y);
}

static void
sp_draw_path_endfreehand (SPDrawing * drawing, double x, double y)
{
	if (drawing->ctrl.inside == 1) {
		/* we are closed path */
		if (drawing->pos < 3) {
			/* path is too small :-( */
			gtk_object_destroy (GTK_OBJECT (drawing->shape));
			drawing->shape = NULL;
			art_free (drawing->bpath);
			drawing->bpath = NULL;
			sp_draw_remove_ctrl (drawing);
			return;
		}
		draw_closepath (drawing);
		sp_draw_finish_item (drawing);
		sp_draw_remove_ctrl (drawing);
		return;
	}
	draw_lineto (drawing, x, y);
	sp_canvas_shape_change_bpath (drawing->shape, drawing->bpath);
	sp_draw_move_ctrl (drawing, x, y);
	sp_draw_ctrl_test_inside (drawing, x, y);
}

static void
draw_bpath_init (SPDrawing * drawing)
{
	drawing->bpath = art_new (ArtBpath, 1024);
	drawing->length = 1024;
	drawing->bpath->code = ART_END;
	drawing->pos = 0;
	drawing->moving_end = FALSE;
}

static void
draw_stretch (SPDrawing * drawing)
{
	drawing->bpath = art_realloc (drawing->bpath, drawing->length + 1024);
	drawing->length += 1024;
}

static void
draw_curveto (SPDrawing * drawing, gdouble x0, gdouble y0, gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
	ArtBpath * bp;

	g_assert (drawing->bpath != NULL);
	g_assert (drawing->pos > 0);

	bp = drawing->bpath + drawing->pos;

	bp->code = ART_CURVETO;
	bp->x1 = x0;
	bp->y1 = y0;
	bp->x2 = x1;
	bp->y2 = y1;
	bp->x3 = x2;
	bp->y3 = y2;

	drawing->pos++;

	if (drawing->pos >= drawing->length)
		draw_stretch (drawing);

	bp = drawing->bpath + drawing->pos;

	bp->code = ART_END;

	drawing->moving_end = FALSE;
}

static void
draw_lineto (SPDrawing * drawing, gdouble x, gdouble y)
{
	ArtBpath * bp;

	g_assert (drawing->bpath != NULL);
	g_assert (drawing->pos > 0);

	if (drawing->moving_end) {
		draw_setend (drawing, x, y);
		drawing->moving_end = FALSE;
		return;
	}

	bp = drawing->bpath + drawing->pos;

	bp->code = ART_LINETO;
	bp->x3 = x;
	bp->y3 = y;

	drawing->pos++;

	if (drawing->pos >= drawing->length)
		draw_stretch (drawing);

	bp = drawing->bpath + drawing->pos;

	bp->code = ART_END;

	drawing->moving_end = FALSE;
}

static void
draw_moveto (SPDrawing * drawing, gdouble x, gdouble y)
{
	ArtBpath * bp;

	g_assert (drawing->bpath == NULL);

	if (drawing->bpath == NULL)
		draw_bpath_init (drawing);

	bp = drawing->bpath + drawing->pos;

	bp->code = ART_MOVETO_OPEN;
	bp->x3 = x;
	bp->y3 = y;

	drawing->pos++;

	if (drawing->pos >= drawing->length)
		draw_stretch (drawing);

	bp = drawing->bpath + drawing->pos;

	bp->code = ART_END;

	drawing->moving_end = FALSE;
}

static void
draw_closepath (SPDrawing * drawing)
{
	ArtBpath * bp;

	g_assert (drawing->bpath != NULL);
	g_assert (drawing->pos > 1);

	bp = drawing->bpath + drawing->pos - 1;

	if ((bp->x3 != drawing->bpath->x3) || (bp->y3 != drawing->bpath->y3)) {
		draw_lineto (drawing, drawing->bpath->x3, drawing->bpath->y3);
	}

	drawing->bpath->code = ART_MOVETO;

	drawing->pos++;

	if (drawing->pos >= drawing->length)
		draw_stretch (drawing);

	bp = drawing->bpath + drawing->pos;

	bp->code = ART_END;
}

static void
draw_closepath_current (SPDrawing * drawing)
{
	ArtBpath * bp;

	g_assert (drawing->bpath != NULL);
	g_assert (drawing->pos > 2);

	bp = drawing->bpath + drawing->pos - 1;

	bp->x3 = drawing->bpath->x3;
	bp->y3 = drawing->bpath->y3;
	drawing->bpath->code = ART_MOVETO;
}

static void
draw_setend (SPDrawing * drawing, double x, double y)
{
	ArtBpath * bp;

	g_assert (drawing->bpath != NULL);
	g_assert (drawing->pos > 0);

	if (!drawing->moving_end) {
		draw_lineto (drawing, x, y);
		drawing->moving_end = TRUE;
	} else {
		bp = drawing->bpath + drawing->pos - 1;

		bp->x3 = x;
		bp->y3 = y;
	}
}

static SPCanvasShape *
sp_draw_create_item (SPDrawing * drawing)
{
	SPStroke * stroke;
	SPCanvasShape * shape;

	stroke = sp_stroke_new_colored (0x000000ff, 2.0, FALSE);
	shape = (SPCanvasShape *)
		gnome_canvas_item_new (SP_DT_SKETCH (drawing->desktop),
			SP_TYPE_CANVAS_SHAPE, NULL);
	sp_canvas_shape_set_stroke (shape, stroke);
	sp_stroke_unref (stroke);
	gtk_signal_connect (GTK_OBJECT (shape), "event",
		GTK_SIGNAL_FUNC (sp_root_handler), drawing->desktop);

	return shape;
}

static void
sp_drawing_destroy (SPDrawing * drawing)
{
	g_return_if_fail (drawing != NULL);

	if (drawing->shape) gtk_object_destroy (GTK_OBJECT (drawing->shape));
	if (drawing->ctrl.item) gtk_object_destroy (GTK_OBJECT (drawing->ctrl.item));
	if (drawing->bpath) art_free (drawing->bpath);
	g_free (drawing);
}

static SPDrawing *
sp_drawing_get_desktop_drawing (SPDesktop * desktop)
{
	SPDrawing * drawing;

	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	drawing = gtk_object_get_data (GTK_OBJECT (desktop), "SPDrawing");

	if (drawing == NULL) {
		drawing = g_new (SPDrawing, 1);
		drawing->desktop = desktop;
		drawing->shape = NULL;
		drawing->bpath = NULL;
		drawing->ctrl.item = NULL;
		drawing->ctrl.inside = -1;
		drawing->ctrl.fill_color = 0xff0000ff;
		drawing->ctrl.p.x = drawing->ctrl.p.y = 0.0;
		gtk_object_set_data_full (GTK_OBJECT (desktop), "SPDrawing", drawing, (GtkDestroyNotify) sp_drawing_destroy);
	}

	return drawing;
}


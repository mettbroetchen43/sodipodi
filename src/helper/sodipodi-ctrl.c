#define SODIPODI_CTRL_C

/*
 * SPCtrl
 *
 * We render it by hand to reduce allocing/freeing svps & to get clean
 *    (non-aa) images
 *
 */

#include <math.h>
#include <gtk/gtk.h>
#include <libgnomeui/gnome-canvas.h>
#include <libgnomeui/gnome-canvas-util.h>
#include "sodipodi-ctrl.h"
#include "sp-canvas-util.h"

#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath.h>
#include <libart_lgpl/art_svp_vpath_stroke.h>

enum {
	ARG_0,
	ARG_SHAPE,
	ARG_ANCHOR,
	ARG_SIZE,
	ARG_FILLED,
	ARG_FILL_COLOR,
	ARG_STROKED,
	ARG_STROKE_COLOR
};


static void sp_ctrl_class_init (SPCtrlClass *klass);
static void sp_ctrl_init (SPCtrl *ctrl);
static void sp_ctrl_destroy (GtkObject *object);
static void sp_ctrl_set_arg (GtkObject *object, GtkArg *arg, guint arg_id);
static void sp_ctrl_get_arg (GtkObject *object, GtkArg *arg, guint arg_id);

static void sp_ctrl_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void sp_ctrl_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf);

static double sp_ctrl_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item);


static GnomeCanvasItemClass *parent_class;

GtkType
sp_ctrl_get_type (void)
{
	static GtkType ctrl_type = 0;
	if (!ctrl_type) {
		GtkTypeInfo ctrl_info = {
			"SPCtrl",
			sizeof (SPCtrl),
			sizeof (SPCtrlClass),
			(GtkClassInitFunc) sp_ctrl_class_init,
			(GtkObjectInitFunc) sp_ctrl_init,
			NULL, NULL, NULL
		};
		ctrl_type = gtk_type_unique (gnome_canvas_item_get_type (), &ctrl_info);
	}
	return ctrl_type;
}

static void
sp_ctrl_class_init (SPCtrlClass *klass)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (GnomeCanvasItemClass *) klass;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gtk_object_add_arg_type ("SPCtrl::shape", GTK_TYPE_ENUM, GTK_ARG_READWRITE, ARG_SHAPE);
	gtk_object_add_arg_type ("SPCtrl::anchor", GTK_TYPE_ANCHOR_TYPE, GTK_ARG_READWRITE, ARG_ANCHOR);
	gtk_object_add_arg_type ("SPCtrl::size", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_SIZE);
	gtk_object_add_arg_type ("SPCtrl::filled", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_FILLED);
	gtk_object_add_arg_type ("SPCtrl::fill_color", GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_FILL_COLOR);
	gtk_object_add_arg_type ("SPCtrl::stroked", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_STROKED);
	gtk_object_add_arg_type ("SPCtrl::stroke_color", GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_STROKE_COLOR);

	object_class->destroy = sp_ctrl_destroy;
	object_class->set_arg = sp_ctrl_set_arg;
	object_class->get_arg = sp_ctrl_get_arg;

	item_class->update = sp_ctrl_update;
	item_class->render = sp_ctrl_render;
	item_class->point = sp_ctrl_point;
}

static void
sp_ctrl_init (SPCtrl *ctrl)
{
	ctrl->shape = SP_CTRL_SHAPE_SQUARE;
	ctrl->anchor = GTK_ANCHOR_CENTER;
	ctrl->span = 3;
	ctrl->defined = TRUE;
	ctrl->shown = FALSE;
	ctrl->filled = 1;
	ctrl->stroked = 0;
	ctrl->fill_color = 0x000000ff;
	ctrl->stroke_color = 0x000000ff;

	ctrl->box.x0 = ctrl->box.y0 = ctrl->box.x1 = ctrl->box.y1 = 0;
}

static void
sp_ctrl_destroy (GtkObject *object)
{
	SPCtrl *ctrl;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_CTRL (object));

	ctrl = SP_CTRL (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_ctrl_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasItem *item;
	SPCtrl *ctrl;

	item = GNOME_CANVAS_ITEM (object);
	ctrl = SP_CTRL (object);

	switch (arg_id) {
	case ARG_SHAPE:
		ctrl->shape = GTK_VALUE_ENUM (*arg);
		gnome_canvas_item_request_update (item);
		break;
	case ARG_ANCHOR:
		ctrl->anchor = GTK_VALUE_ENUM (*arg);
		gnome_canvas_item_request_update (item);
		break;
	case ARG_SIZE:
		ctrl->span = (gint) ((GTK_VALUE_DOUBLE (*arg) - 1.0) / 2.0 + 0.5);
		ctrl->defined = (ctrl->span > 0);
		gnome_canvas_item_request_update (item);
		break;
	case ARG_FILLED:
		ctrl->filled = GTK_VALUE_BOOL (*arg);
		gnome_canvas_item_request_update (item);
		break;
	case ARG_FILL_COLOR:
		ctrl->fill_color = GTK_VALUE_INT (*arg);
		gnome_canvas_item_request_update (item);
		break;
	case ARG_STROKED:
		ctrl->stroked = GTK_VALUE_BOOL (*arg);
		gnome_canvas_item_request_update (item);
		break;
	case ARG_STROKE_COLOR:
		ctrl->stroke_color = GTK_VALUE_INT (*arg);
		gnome_canvas_item_request_update (item);
		break;
	default:
		break;
	}
}

static void
sp_ctrl_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	SPCtrl *ctrl;

	ctrl = SP_CTRL (object);

	switch (arg_id) {
	case ARG_ANCHOR:
		GTK_VALUE_ENUM (*arg) = ctrl->anchor;
		break;
	case ARG_SIZE:
		GTK_VALUE_DOUBLE (*arg) = 2.0 * ctrl->span + 1.0;
		break;
	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
sp_ctrl_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	SPCtrl *ctrl;
	gint x, y;

	ctrl = SP_CTRL (item);

	if (((GnomeCanvasItemClass *) parent_class)->update)
		(* ((GnomeCanvasItemClass *) parent_class)->update) (item, affine, clip_path, flags);

	gnome_canvas_item_reset_bounds (item);

	if (ctrl->shown) {
		gnome_canvas_request_redraw (item->canvas, ctrl->box.x0, ctrl->box.y0, ctrl->box.x1 + 1, ctrl->box.y1 + 1);
	}

	if (!ctrl->defined) return;

	x = (gint) affine[4] - ctrl->span;
	y = (gint) affine[5] - ctrl->span;

	switch (ctrl->anchor) {
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		break;
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		x += ctrl->span;
		break;
	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		x -= (ctrl->span + 1);
		break;
	}

	switch (ctrl->anchor) {
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		break;
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		y += ctrl->span;
		break;
	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		y -= (ctrl->span + 1);
		break;
	}

	ctrl->box.x0 = x;
	ctrl->box.y0 = y;
	ctrl->box.x1 = ctrl->box.x0 + 2 * ctrl->span;
	ctrl->box.y1 = ctrl->box.y0 + 2 * ctrl->span;

	gnome_canvas_update_bbox (item, ctrl->box.x0, ctrl->box.y0, ctrl->box.x1 + 1, ctrl->box.y1 + 1);

}

static double
sp_ctrl_point (GnomeCanvasItem *item, double x, double y,
	       int cx, int cy, GnomeCanvasItem **actual_item)
{
	SPCtrl *ctrl;

	ctrl = SP_CTRL (item);

	*actual_item = item;

	if ((cx >= ctrl->box.x0) && (cx <= ctrl->box.x1) && (cy >= ctrl->box.y0) && (cy <= ctrl->box.y1)) return 0.0;

	return 1e18;
}

#define set_channel(p,v,a) ((p) + (((((v) - (p)) * (a)) + 0x80) >> 8))

static void
draw_line (GnomeCanvasBuf *buf, gint x0, gint x1, gint y, guint8 r, guint8 g, guint8 b, guint8 a)
{
	gint x;
	guchar *p;

	x0 = MAX (x0, buf->rect.x0);
	x1 = MIN (x1 + 1, buf->rect.x1);

	p = buf->buf + (y - buf->rect.y0) * buf->buf_rowstride + (x0 - buf->rect.x0) * 3;

	for (x = x0; x < x1; x++) {
		*p++ = set_channel (*p, (guint) r, (guint) a);
		*p++ = set_channel (*p, (guint) g, (guint) a);
		*p++ = set_channel (*p, (guint) b, (guint) a);
	}
}

static void
draw_point (GnomeCanvasBuf *buf, gint x, gint y, guint8 r, guint8 g, guint8 b, guint8 a)
{
	guchar *p;

	if (x < buf->rect.x0) return;
	if (x >= buf->rect.x1) return;

	p = buf->buf + (y - buf->rect.y0) * buf->buf_rowstride + (x - buf->rect.x0) * 3;

	*p++ = set_channel (*p, (guint) r, (guint) a);
	*p++ = set_channel (*p, (guint) g, (guint) a);
	*p++ = set_channel (*p, (guint) b, (guint) a);
}

static void
sp_ctrl_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	SPCtrl *ctrl;
	guint8 fr, fg, fb, fa, sr, sg, sb, sa;

	ctrl = SP_CTRL (item);

	if (!ctrl->defined) return;
	if ((!ctrl->filled) && (!ctrl->stroked)) return;

	gnome_canvas_buf_ensure_buf (buf);
	buf->is_bg = FALSE;

	fr = (ctrl->fill_color >> 24) & 0xff;
	fg = (ctrl->fill_color >> 16) & 0xff;
	fb = (ctrl->fill_color >> 8) & 0xff;
	fa = (ctrl->fill_color) & 0xff;
	if (ctrl->stroked) {
		sr = (ctrl->stroke_color >> 24) & 0xff;
		sg = (ctrl->stroke_color >> 16) & 0xff;
		sb = (ctrl->stroke_color >> 8) & 0xff;
		sa = (ctrl->stroke_color) & 0xff;
	} else {
		sr = fr;
		sg = fg;
		sb = fb;
		sa = fa;
	}

	if (ctrl->shape == SP_CTRL_SHAPE_SQUARE) {
		gint y0, y1, y;
		y0 = MAX (ctrl->box.y0, buf->rect.y0);
		y1 = MIN (ctrl->box.y1, buf->rect.y1 - 1);
		if (y0 == ctrl->box.y0) {
			draw_line (buf, ctrl->box.x0, ctrl->box.x1, y0, sr, sg, sb, sa);
			y0 += 1;
		}
		if (y1 == ctrl->box.y1) {
			draw_line (buf, ctrl->box.x0, ctrl->box.x1, y1, sr, sg, sb, sa);
			y1 -= 1;
		}
		for (y = y0; y <= y1; y++) {
			draw_point (buf, ctrl->box.x0, y, sr, sg, sb, sa);
			if (ctrl->filled) draw_line (buf, ctrl->box.x0 + 1, ctrl->box.x1 - 1, y, sr, sg, sb, sa);
			draw_point (buf, ctrl->box.x1, y, sr, sg, sb, sa);
		}
	} else if (ctrl->shape == SP_CTRL_SHAPE_DIAMOND) {
		gint cx, cy, y0, y1, y;
		cx = (ctrl->box.x0 + ctrl->box.x1) / 2;
		cy = (ctrl->box.y0 + ctrl->box.y1) / 2;
		y0 = MAX (ctrl->box.y0, buf->rect.y0);
		y1 = MIN (ctrl->box.y1, buf->rect.y1 - 1);
		if (y0 == ctrl->box.y0) {
			draw_point (buf, cx, y0, sr, sg, sb, sa);
			y0 += 1;
		}
		if (y1 == ctrl->box.y1) {
			draw_point (buf, cx, y1, sr, sg, sb, sa);
			y1 -= 1;
		}
		for (y = y0; y <= y1; y++) {
			gint r, s;
			r = (cy >= y) ? cy - y : y - cy;
			s = ctrl->span - r;
			draw_point (buf, cx - s, y, sr, sg, sb, sa);
			if (ctrl->filled) draw_line (buf, cx - s + 1, cx + s - 1, y, sr, sg, sb, sa);
			draw_point (buf, cx + s, y, sr, sg, sb, sa);
		}
	}

	ctrl->shown = TRUE;
}

void
sp_ctrl_moveto (SPCtrl * ctrl, double x, double y)
{
	double affine[6];

	art_affine_translate (affine, x, y);

	gnome_canvas_item_affine_absolute (GNOME_CANVAS_ITEM (ctrl), affine);
}

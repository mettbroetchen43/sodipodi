#define SODIPODI_CTRL_C

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

static double sp_ctrl_point (GnomeCanvasItem *item, double x, double y,
			     int cx, int cy, GnomeCanvasItem **actual_item);


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
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
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
	ctrl->anchor = GTK_ANCHOR_CENTER;
	ctrl->size = 8;
	ctrl->filled = 1;
	ctrl->stroked = 0;
	ctrl->fill_color = 0x000000ff;
	ctrl->stroke_color = 0x000000ff;
	ctrl->fill_svp = NULL;
	ctrl->stroke_svp = NULL;
}

static void
sp_ctrl_destroy (GtkObject *object)
{
	SPCtrl *ctrl;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_CTRL (object));

	ctrl = SP_CTRL (object);

	if (ctrl->fill_svp)
		art_svp_free (ctrl->fill_svp);
	if (ctrl->stroke_svp)
		art_svp_free (ctrl->stroke_svp);

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
	case ARG_ANCHOR:
		ctrl->anchor = GTK_VALUE_ENUM (*arg);
		gnome_canvas_item_request_update (item);
		break;
	case ARG_SIZE:
		ctrl->size = GTK_VALUE_DOUBLE (*arg);
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
		GTK_VALUE_DOUBLE (*arg) = ctrl->size;
		break;
	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
sp_ctrl_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	SPCtrl *ctrl;

	ctrl = SP_CTRL (item);

	if (ctrl->filled)
		gnome_canvas_render_svp (buf, ctrl->fill_svp, ctrl->fill_color);
	if (ctrl->stroked)
		gnome_canvas_render_svp (buf, ctrl->stroke_svp, ctrl->stroke_color);

}

static void
sp_ctrl_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	SPCtrl *ctrl;
	ArtPoint p;
	ArtVpath vpath[6];
	ArtSVP *svp;

	ctrl = SP_CTRL (item);

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	gnome_canvas_item_reset_bounds (item);

	p.x = 0.0;
	p.y = 0.0;
	art_affine_point (&p, &p, affine);

	switch (ctrl->anchor) {
		case GTK_ANCHOR_N:
		case GTK_ANCHOR_CENTER:
		case GTK_ANCHOR_S:
			break;
		case GTK_ANCHOR_NW:
		case GTK_ANCHOR_W:
		case GTK_ANCHOR_SW:
			p.x += ctrl->size / 2;
			break;
		case GTK_ANCHOR_NE:
		case GTK_ANCHOR_E:
		case GTK_ANCHOR_SE:
			p.x -= ctrl->size / 2;
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
			p.y += ctrl->size / 2;
			break;
		case GTK_ANCHOR_SW:
		case GTK_ANCHOR_S:
		case GTK_ANCHOR_SE:
			p.y -= ctrl->size / 2;
			break;
	}
	vpath[0].code = ART_MOVETO;
	vpath[0].x = p.x - ctrl->size/2;
	vpath[0].y = p.y - ctrl->size/2;
	vpath[1].code = ART_LINETO;
	vpath[1].x = p.x - ctrl->size/2;
	vpath[1].y = p.y + ctrl->size/2;
	vpath[2].code = ART_LINETO;
	vpath[2].x = p.x + ctrl->size/2;
	vpath[2].y = p.y + ctrl->size/2;
	vpath[3].code = ART_LINETO;
	vpath[3].x = p.x + ctrl->size/2;
	vpath[3].y = p.y - ctrl->size/2;
	vpath[4].code = ART_LINETO;
	vpath[4].x = p.x - ctrl->size/2;
	vpath[4].y = p.y - ctrl->size/2;
	vpath[5].code = ART_END;
	vpath[5].x = 0.0;
	vpath[5].y = 0.0;
	
	if (ctrl->filled) {
		svp = art_svp_from_vpath (vpath);
		gnome_canvas_item_update_svp_clip (item, &ctrl->fill_svp, svp, clip_path);
		gnome_canvas_item_request_redraw_svp (item, ctrl->fill_svp);
	}
	if (ctrl->stroked) {
		svp = art_svp_vpath_stroke (vpath,
			ART_PATH_STROKE_CAP_BUTT,
			ART_PATH_STROKE_JOIN_MITER,
			1, 4, 0.25);
		gnome_canvas_item_update_svp_clip (item, &ctrl->stroke_svp, svp, clip_path);
		gnome_canvas_item_request_redraw_svp (item, ctrl->stroke_svp);
	}

}

static double
sp_ctrl_point (GnomeCanvasItem *item, double x, double y,
	       int cx, int cy, GnomeCanvasItem **actual_item)
{
	SPCtrl *ctrl;
	double affine[6];
	ArtPoint p;
	double rx,ry,x0,y0,x1,y1,dx,dy;

	ctrl = SP_CTRL (item);

	*actual_item = item;

	gnome_canvas_item_i2c_affine (item, affine);
	p.x = 0.0;
	p.y = 0.0;
	art_affine_point (&p, &p, affine);

	p.x += ctrl->size / 2;
	p.y += ctrl->size / 2;

	switch (ctrl->anchor) {
		case GTK_ANCHOR_NE:
		case GTK_ANCHOR_E:
		case GTK_ANCHOR_SE:
			p.x -= ctrl->size / 2;
		case GTK_ANCHOR_N:
		case GTK_ANCHOR_CENTER:
		case GTK_ANCHOR_S:
			p.x -= ctrl->size / 2;
		default:
	}

	switch (ctrl->anchor) {
		case GTK_ANCHOR_SW:
		case GTK_ANCHOR_S:
		case GTK_ANCHOR_SE:
			p.y -= ctrl->size / 2;
		case GTK_ANCHOR_W:
		case GTK_ANCHOR_CENTER:
		case GTK_ANCHOR_E:
			p.y -= ctrl->size / 2;
		default:
	}
	rx = cx;
	ry = cy;
	x0 = p.x - ctrl->size / 2;
	y0 = p.y - ctrl->size / 2;
	x1 = p.x + ctrl->size / 2;
	y1 = p.y + ctrl->size / 2;

	if ((rx >= x0) && (rx <= x1) && (ry >= y0) && (ry <= y1))
		return 0.0;

	if (rx < x0) dx = x0 - rx;
	else if (rx > x1) dx = rx - x1;
	else dx = 0.0;

	if (ry < y0) dy = y0 - ry;
	else if (ry > y1) dy = ry - y1;
	else dy = 0.0;

	return sqrt (dx * dx + dy * dy);
}

void
sp_ctrl_moveto (SPCtrl * ctrl, double x, double y)
{
	double affine[6];

	art_affine_translate (affine, x, y);

	gnome_canvas_item_affine_absolute (GNOME_CANVAS_ITEM (ctrl), affine);
}

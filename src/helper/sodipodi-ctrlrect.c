#define SODIPODI_CTRLRECT_C

/* This is prototype
 * Something real should not use canvas at all :-(
 */

#include <math.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath.h>
#include <libart_lgpl/art_svp_vpath_stroke.h>
#include <libgnomeui/gnome-canvas.h>
#include <libgnomeui/gnome-canvas-util.h>
#include "../helper/canvas-helper.h"
#include "sodipodi-ctrlrect.h"

enum {
	ARG_0,
	ARG_X0,
	ARG_Y0,
	ARG_X1,
	ARG_Y1,
	ARG_WIDTH
};

/*
 * Currently we do not have point method, as it should always be painted
 * during some transformation, which takes care of events...
 *
 * Corner coords can be in any order - i.e. x1 < x0 is allowed
 */

static void sp_ctrlrect_class_init (SPCtrlRectClass *klass);
static void sp_ctrlrect_init (SPCtrlRect *ctrlrect);
static void sp_ctrlrect_destroy (GtkObject *object);
static void sp_ctrlrect_set_arg (GtkObject *object, GtkArg *arg, guint arg_id);
static void sp_ctrlrect_get_arg (GtkObject *object, GtkArg *arg, guint arg_id);

static void sp_ctrlrect_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void sp_ctrlrect_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf);


static GnomeCanvasItemClass *parent_class;

GtkType
sp_ctrlrect_get_type (void)
{
	static GtkType ctrlrect_type = 0;

	if (!ctrlrect_type) {
		GtkTypeInfo ctrlrect_info = {
			"SPCtrlRect",
			sizeof (SPCtrlRect),
			sizeof (SPCtrlRectClass),
			(GtkClassInitFunc) sp_ctrlrect_class_init,
			(GtkObjectInitFunc) sp_ctrlrect_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		ctrlrect_type = gtk_type_unique (gnome_canvas_item_get_type (), &ctrlrect_info);
	}
	return ctrlrect_type;
}

static void
sp_ctrlrect_class_init (SPCtrlRectClass *klass)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (GnomeCanvasItemClass *) klass;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gtk_object_add_arg_type ("SPCtrlRect::x0", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_X0);
	gtk_object_add_arg_type ("SPCtrlRect::y0", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_Y0);
	gtk_object_add_arg_type ("SPCtrlRect::x1", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_X1);
	gtk_object_add_arg_type ("SPCtrlRect::y1", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_Y1);
	gtk_object_add_arg_type ("SPCtrlRect::width", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_WIDTH);

	object_class->destroy = sp_ctrlrect_destroy;
	object_class->set_arg = sp_ctrlrect_set_arg;
	object_class->get_arg = sp_ctrlrect_get_arg;

	item_class->update = sp_ctrlrect_update;
	item_class->render = sp_ctrlrect_render;
}

static void
sp_ctrlrect_init (SPCtrlRect * ctrlrect)
{
	ctrlrect->rect.x0 = ctrlrect->rect.y0 = ctrlrect->rect.x1 = ctrlrect->rect.y1 = 0.0;
	ctrlrect->width = 1;
	ctrlrect->irect.x0 = ctrlrect->irect.y0 = ctrlrect->irect.x1 = ctrlrect->irect.y1 = 0;
#if 0
	ctrlrect->svp = NULL;
	ctrlrect->rdsvp = NULL;
#endif
}

static void
sp_ctrlrect_destroy (GtkObject *object)
{
	SPCtrlRect *ctrlrect;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_CTRLRECT (object));

	ctrlrect = SP_CTRLRECT (object);

#if 0
	if (ctrlrect->svp)
		art_svp_free (ctrlrect->svp);
	if (ctrlrect->rdsvp)
		art_svp_free (ctrlrect->rdsvp);
#endif

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_ctrlrect_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasItem *item;
	SPCtrlRect *ctrlrect;

	item = GNOME_CANVAS_ITEM (object);
	ctrlrect = SP_CTRLRECT (object);

	switch (arg_id) {
	case ARG_X0:
		ctrlrect->rect.x0 = GTK_VALUE_DOUBLE (*arg);
		break;
	case ARG_Y0:
		ctrlrect->rect.y0 = GTK_VALUE_DOUBLE (*arg);
		break;
	case ARG_X1:
		ctrlrect->rect.x1 = GTK_VALUE_DOUBLE (*arg);
		break;
	case ARG_Y1:
		ctrlrect->rect.y1 = GTK_VALUE_DOUBLE (*arg);
		break;
	case ARG_WIDTH:
		ctrlrect->width = GTK_VALUE_DOUBLE (*arg);
		break;
	default:
		break;
	}
	gnome_canvas_item_request_update (item);
}

static void
sp_ctrlrect_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	SPCtrlRect *ctrlrect;

	ctrlrect = SP_CTRLRECT (object);

	switch (arg_id) {
	case ARG_X0:
		GTK_VALUE_DOUBLE (*arg) = ctrlrect->rect.x0;
		break;
	case ARG_Y0:
		GTK_VALUE_DOUBLE (*arg) = ctrlrect->rect.y0;
		break;
	case ARG_X1:
		GTK_VALUE_DOUBLE (*arg) = ctrlrect->rect.x1;
		break;
	case ARG_Y1:
		GTK_VALUE_DOUBLE (*arg) = ctrlrect->rect.y1;
		break;
	case ARG_WIDTH:
		GTK_VALUE_DOUBLE (*arg) = ctrlrect->width;
		break;
	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
sp_ctrlrect_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	SPCtrlRect *ctrlrect;
	guint fr, fg, fb, fa;
	gint x0, y0, x1, y1;
	gint x, y;
	guchar *b, *bb;

	ctrlrect = SP_CTRLRECT (item);
	
	if (ctrlrect->irect.x1 < buf->rect.x0) return;
	if (ctrlrect->irect.y1 < buf->rect.y0) return;
	if (ctrlrect->irect.x0 >= buf->rect.x1) return;
	if (ctrlrect->irect.y0 >= buf->rect.y1) return;

	fr = 0x0;
	fg = 0x0;
	fb = 0x0;
	fa = 0xbf;

	x0 = MAX (buf->rect.x0, ctrlrect->irect.x0) - buf->rect.x0;
	y0 = MAX (buf->rect.y0, ctrlrect->irect.y0) - buf->rect.y0;
	x1 = MIN (buf->rect.x1 - 1, ctrlrect->irect.x1) - buf->rect.x0;
	y1 = MIN (buf->rect.y1 - 1, ctrlrect->irect.y1) - buf->rect.y0;

	if (buf->is_bg) {
		gnome_canvas_clear_buffer (buf);
		buf->is_bg = FALSE;
		buf->is_buf = TRUE;
	}

	/* fixme: corners are possibly drawn twice */

	if (ctrlrect->irect.y0 >= buf->rect.y0) {
		b = buf->buf + y0 * buf->buf_rowstride + 3 * x0;
		for (x = x0; x <= x1; x++) {
			*b++ = *b + (((fr - *b) * fa + 0x80) >> 8);
			*b++ = *b + (((fg - *b) * fa + 0x80) >> 8);
			*b++ = *b + (((fb - *b) * fa + 0x80) >> 8);
		}
	}
	if (ctrlrect->irect.y1 < buf->rect.y1) {
		b = buf->buf + y1 * buf->buf_rowstride + 3 * x0;
		for (x = x0; x <= x1; x++) {
			*b++ = *b + (((fr - *b) * fa + 0x80) >> 8);
			*b++ = *b + (((fg - *b) * fa + 0x80) >> 8);
			*b++ = *b + (((fb - *b) * fa + 0x80) >> 8);
		}
	}
	if (ctrlrect->irect.x0 >= buf->rect.x0) {
		bb = buf->buf + y0 * buf->buf_rowstride + 3 * x0;
		for (y = y0; y <= y1; y++) {
			b = bb;
			*b++ = *b + (((fr - *b) * fa + 0x80) >> 8);
			*b++ = *b + (((fg - *b) * fa + 0x80) >> 8);
			*b++ = *b + (((fb - *b) * fa + 0x80) >> 8);
			bb += buf->buf_rowstride;
		}
	}
	if (ctrlrect->irect.x1 < buf->rect.x1) {
		bb = buf->buf + y0 * buf->buf_rowstride + 3 * x1;
		for (y = y0; y <= y1; y++) {
			b = bb;
			*b++ = *b + (((fr - *b) * fa + 0x80) >> 8);
			*b++ = *b + (((fg - *b) * fa + 0x80) >> 8);
			*b++ = *b + (((fb - *b) * fa + 0x80) >> 8);
			bb += buf->buf_rowstride;
		}
	}
}

static void
sp_ctrlrect_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	SPCtrlRect *cr;
	ArtPoint p0, p1;

	cr = SP_CTRLRECT (item);

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	gnome_canvas_item_reset_bounds (item);

	gnome_canvas_request_redraw (item->canvas, cr->irect.x0 - 1, cr->irect.y0 - 1, cr->irect.x1 + 1, cr->irect.y0 + 1);
	gnome_canvas_request_redraw (item->canvas, cr->irect.x0 - 1, cr->irect.y0 - 1, cr->irect.x0 + 1, cr->irect.y1 + 1);
	gnome_canvas_request_redraw (item->canvas, cr->irect.x1 - 1, cr->irect.y0 - 1, cr->irect.x1 + 1, cr->irect.y1 + 1);
	gnome_canvas_request_redraw (item->canvas, cr->irect.x0 - 1, cr->irect.y1 - 1, cr->irect.x1 + 1, cr->irect.y1 + 1);

	p0.x = cr->rect.x0;
	p0.y = cr->rect.y0;
	art_affine_point (&p0, &p0, affine);
	p1.x = cr->rect.x1;
	p1.y = cr->rect.y1;
	art_affine_point (&p1, &p1, affine);

	cr->irect.x0 = (int) floor (MIN (p0.x, p1.x) + 0.5);
	cr->irect.y0 = (int) floor (MIN (p0.y, p1.y) + 0.5);
	cr->irect.x1 = (int) floor (MAX (p0.x, p1.x) + 0.5);
	cr->irect.y1 = (int) floor (MAX (p0.y, p1.y) + 0.5);

	gnome_canvas_request_redraw (item->canvas, cr->irect.x0 - 1, cr->irect.y0 - 1, cr->irect.x1 + 1, cr->irect.y0 + 1);
	gnome_canvas_request_redraw (item->canvas, cr->irect.x0 - 1, cr->irect.y0 - 1, cr->irect.x0 + 1, cr->irect.y1 + 1);
	gnome_canvas_request_redraw (item->canvas, cr->irect.x1 - 1, cr->irect.y0 - 1, cr->irect.x1 + 1, cr->irect.y1 + 1);
	gnome_canvas_request_redraw (item->canvas, cr->irect.x0 - 1, cr->irect.y1 - 1, cr->irect.x1 + 1, cr->irect.y1 + 1);

	item->x1 = cr->irect.x0 - 1;
	item->y1 = cr->irect.y0 - 1;
	item->x2 = cr->irect.x1 + 1;
	item->y2 = cr->irect.y1 + 1;
}

void
sp_ctrlrect_set_rect (SPCtrlRect * cr, ArtDRect * r)
{
	cr->rect.x0 = MIN (r->x0, r->x1);
	cr->rect.y0 = MIN (r->y0, r->y1);
	cr->rect.x1 = MAX (r->x0, r->x1);
	cr->rect.y1 = MAX (r->y0, r->y1);

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (cr));
}

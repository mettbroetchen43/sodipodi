#define SODIPODI_GUIDE_C

/*
 * SPGuide
 *
 * Copyright (C) Lauris Kaplinski 2000
 *
 */

#include <math.h>
#include <libgnomeui/gnome-canvas.h>
#include <libgnomeui/gnome-canvas-util.h>
#include "sp-guide.h"

enum {
	ARG_0,
	ARG_ORIENTATION,
	ARG_COLOR
};


static void sp_guide_class_init (SPGuideClass *klass);
static void sp_guide_init (SPGuide *guide);
static void sp_guide_destroy (GtkObject *object);
static void sp_guide_set_arg (GtkObject *object, GtkArg *arg, guint arg_id);
static void sp_guide_get_arg (GtkObject *object, GtkArg *arg, guint arg_id);

static void sp_guide_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void sp_guide_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf);

static double sp_guide_point (GnomeCanvasItem *item, double x, double y,
			int cx, int cy, GnomeCanvasItem ** actual_item);


static GnomeCanvasItemClass * parent_class;

GtkType
sp_guide_get_type (void)
{
	static GtkType guide_type = 0;

	if (!guide_type) {
		GtkTypeInfo guide_info = {
			"SPGuide",
			sizeof (SPGuide),
			sizeof (SPGuideClass),
			(GtkClassInitFunc) sp_guide_class_init,
			(GtkObjectInitFunc) sp_guide_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		guide_type = gtk_type_unique (gnome_canvas_item_get_type (), &guide_info);
	}
	return guide_type;
}

static void
sp_guide_class_init (SPGuideClass *klass)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (GnomeCanvasItemClass *) klass;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gtk_object_add_arg_type ("SPGuide::orientation", GTK_TYPE_ENUM, GTK_ARG_READWRITE, ARG_ORIENTATION);
	gtk_object_add_arg_type ("SPGuide::color", GTK_TYPE_INT, GTK_ARG_WRITABLE, ARG_COLOR);

	object_class->destroy = sp_guide_destroy;
	object_class->set_arg = sp_guide_set_arg;
	object_class->get_arg = sp_guide_get_arg;

	item_class->update = sp_guide_update;
	item_class->render = sp_guide_render;
	item_class->point = sp_guide_point;
}

static void
sp_guide_init (SPGuide *guide)
{
	guide->color = 0x0000ff7f;
	guide->orientation = SP_GUIDE_ORIENTATION_HORIZONTAL;
	guide->shown = FALSE;
}

static void
sp_guide_destroy (GtkObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_GUIDE (object));

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_guide_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasItem *item;
	SPGuide *guide;

	item = GNOME_CANVAS_ITEM (object);
	guide = SP_GUIDE (object);

	switch (arg_id) {
	case ARG_ORIENTATION:
		guide->orientation = GTK_VALUE_ENUM (* arg);
		gnome_canvas_item_request_update (item);
		break;
	case ARG_COLOR:
		guide->color = GTK_VALUE_INT (* arg);
		gnome_canvas_item_request_update (item);
		break;
	default:
		break;
	}
}

static void
sp_guide_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	SPGuide *guide;

	guide = SP_GUIDE (object);

	switch (arg_id) {
	case ARG_ORIENTATION:
		GTK_VALUE_ENUM (* arg) = guide->orientation;
		break;
	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
sp_guide_render (GnomeCanvasItem * item, GnomeCanvasBuf * buf)
{
	SPGuide *guide;
	gint ix, iy;
	guchar * p;
	guint alpha, tmp;
	guint bg_r, fg_r, bg_g, fg_g, bg_b, fg_b;

	guide = SP_GUIDE (item);

	gnome_canvas_buf_ensure_buf (buf);
	buf->is_bg = FALSE;

	if (guide->orientation == SP_GUIDE_ORIENTATION_HORIZONTAL) {
		iy = guide->position;
		if ((iy >= buf->rect.y0) && (iy < buf->rect.y1)) {
			fg_r = (guide->color >> 24) & 0xff;
			fg_g = (guide->color >> 16) & 0xff;
			fg_b = (guide->color >> 8) & 0xff;
			alpha = guide->color & 0xff;
			p = buf->buf + (iy - buf->rect.y0) * buf->buf_rowstride;
			for (ix = buf->rect.x0; ix < buf->rect.x1; ix++) {
				bg_r = *p;
				tmp = (fg_r - bg_r) * alpha;
				*p++ = bg_r + ((tmp + (tmp >> 8) + 0x80) >> 8);
				bg_g = *p;
				tmp = (fg_g - bg_g) * alpha;
				*p++ = bg_g + ((tmp + (tmp >> 8) + 0x80) >> 8);
				bg_b = *p;
				tmp = (fg_b - bg_b) * alpha;
				*p++ = bg_b + ((tmp + (tmp >> 8) + 0x80) >> 8);
			}
		}
	} else {
		ix = guide->position;
		if ((ix >= buf->rect.x0) && (ix < buf->rect.x1)) {
			fg_r = (guide->color >> 24) & 0xff;
			fg_g = (guide->color >> 16) & 0xff;
			fg_b = (guide->color >> 8) & 0xff;
			alpha = guide->color & 0xff;
			p = buf->buf + 3 * (ix - buf->rect.x0);
			for (iy = buf->rect.y0; iy < buf->rect.y1; iy ++) {
				bg_r = *p;
				tmp = (fg_r - bg_r) * alpha;
				*p++ = bg_r + ((tmp + (tmp >> 8) + 0x80) >> 8);
				bg_g = *p;
				tmp = (fg_g - bg_g) * alpha;
				*p++ = bg_g + ((tmp + (tmp >> 8) + 0x80) >> 8);
				bg_b = *p;
				tmp = (fg_b - bg_b) * alpha;
				*p++ = bg_b + ((tmp + (tmp >> 8) + 0x80) >> 8);
				p += (buf->buf_rowstride - 3);
			}
		}
	}

	guide->shown = TRUE;
}

static void
sp_guide_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	SPGuide *guide;
	ArtPoint p;

	guide = SP_GUIDE (item);

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	if (guide->shown) {
		if (guide->orientation == SP_GUIDE_ORIENTATION_HORIZONTAL) {
			gnome_canvas_request_redraw (item->canvas,
				- 1000000,
				guide->position,
				1000000,
				guide->position + 1);
		} else {
			gnome_canvas_request_redraw (item->canvas,
				guide->position,
				-1000000,
				guide->position + 1,
				1000000);
		}
	}

	gnome_canvas_item_reset_bounds (item);

	p.x = p.y = 0.0;
	art_affine_point (&p, &p, affine);

	if (guide->orientation == SP_GUIDE_ORIENTATION_HORIZONTAL) {
		guide->position = (gint) (p.y + 0.5);
		gnome_canvas_update_bbox (item,
			- 1000000,
			guide->position,
			1000000,
			guide->position + 1);
	} else {
		guide->position = (gint) (p.x + 0.5);
		gnome_canvas_update_bbox (item,
			guide->position,
			-1000000,
			guide->position + 1,
			1000000);
	}
}

static double
sp_guide_point (GnomeCanvasItem *item, double x, double y,
	       int cx, int cy, GnomeCanvasItem **actual_item)
{
	SPGuide * guide;
	gdouble d;

	guide = SP_GUIDE (item);

	* actual_item = item;

	if (guide->orientation == SP_GUIDE_ORIENTATION_HORIZONTAL) {
		d = CLAMP (0, 1e18, fabs ((double) cy - guide->position) - 1.0);
	} else {
		d = CLAMP (0, 1e18, fabs ((double) cx - guide->position) - 1.0);
	}

	return d;
}

void
sp_guide_moveto (SPGuide * guide, double x, double y)
{
	double affine[6];

	art_affine_translate (affine, x, y);

	gnome_canvas_item_affine_absolute (GNOME_CANVAS_ITEM (guide), affine);
}

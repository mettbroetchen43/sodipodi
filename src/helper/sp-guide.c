#define SODIPODI_GUIDELINE_C

/*
 * SPGuideLine
 *
 * Copyright (C) Lauris Kaplinski 2000
 *
 */

#include <math.h>

#include <libart_lgpl/art_affine.h>

#include "sp-canvas.h"
#include "sp-canvas-util.h"
#include "sp-guide.h"

enum {
	ARG_0,
	ARG_ORIENTATION,
	ARG_COLOR
};


static void sp_guideline_class_init (SPGuideLineClass *klass);
static void sp_guideline_init (SPGuideLine *guideline);
static void sp_guideline_destroy (GtkObject *object);
static void sp_guideline_set_arg (GtkObject *object, GtkArg *arg, guint arg_id);
static void sp_guideline_get_arg (GtkObject *object, GtkArg *arg, guint arg_id);

static void sp_guideline_update (SPCanvasItem *item, double *affine, unsigned int flags);
static void sp_guideline_render (SPCanvasItem *item, SPCanvasBuf *buf);

static double sp_guideline_point (SPCanvasItem *item, double x, double y,
			int cx, int cy, SPCanvasItem ** actual_item);


static SPCanvasItemClass * parent_class;

GtkType
sp_guideline_get_type (void)
{
	static GtkType guideline_type = 0;

	if (!guideline_type) {
		GtkTypeInfo guideline_info = {
			"SPGuideLine",
			sizeof (SPGuideLine),
			sizeof (SPGuideLineClass),
			(GtkClassInitFunc) sp_guideline_class_init,
			(GtkObjectInitFunc) sp_guideline_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		guideline_type = gtk_type_unique (sp_canvas_item_get_type (), &guideline_info);
	}
	return guideline_type;
}

static void
sp_guideline_class_init (SPGuideLineClass *klass)
{
	GtkObjectClass *object_class;
	SPCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (SPCanvasItemClass *) klass;

	parent_class = gtk_type_class (sp_canvas_item_get_type ());

	gtk_object_add_arg_type ("SPGuideLine::orientation", GTK_TYPE_ENUM, GTK_ARG_READWRITE, ARG_ORIENTATION);
	gtk_object_add_arg_type ("SPGuideLine::color", GTK_TYPE_INT, GTK_ARG_WRITABLE, ARG_COLOR);

	object_class->destroy = sp_guideline_destroy;
	object_class->set_arg = sp_guideline_set_arg;
	object_class->get_arg = sp_guideline_get_arg;

	item_class->update = sp_guideline_update;
	item_class->render = sp_guideline_render;
	item_class->point = sp_guideline_point;
}

static void
sp_guideline_init (SPGuideLine *guideline)
{
	guideline->color = 0x0000ff7f;
	guideline->orientation = SP_GUIDELINE_ORIENTATION_HORIZONTAL;
	guideline->shown = FALSE;
	guideline->sensitive = FALSE;
}

static void
sp_guideline_destroy (GtkObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_GUIDELINE (object));

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_guideline_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	SPCanvasItem *item;
	SPGuideLine *guideline;

	item = SP_CANVAS_ITEM (object);
	guideline = SP_GUIDELINE (object);

	switch (arg_id) {
	case ARG_ORIENTATION:
		guideline->orientation = GTK_VALUE_ENUM (* arg);
		sp_canvas_item_request_update (item);
		break;
	case ARG_COLOR:
		guideline->color = GTK_VALUE_INT (* arg);
		sp_canvas_item_request_update (item);
		break;
	default:
		break;
	}
}

static void
sp_guideline_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	SPGuideLine *guideline;

	guideline = SP_GUIDELINE (object);

	switch (arg_id) {
	case ARG_ORIENTATION:
		GTK_VALUE_ENUM (* arg) = guideline->orientation;
		break;
	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
sp_guideline_render (SPCanvasItem * item, SPCanvasBuf * buf)
{
	SPGuideLine *guideline;
	gint ix, iy;
	guchar * p;
	guint alpha, tmp;
	guint bg_r, fg_r, bg_g, fg_g, bg_b, fg_b;

	guideline = SP_GUIDELINE (item);

	sp_canvas_buf_ensure_buf (buf);
	buf->is_bg = FALSE;

	if (guideline->orientation == SP_GUIDELINE_ORIENTATION_HORIZONTAL) {
		iy = guideline->position;
		if ((iy >= buf->rect.y0) && (iy < buf->rect.y1)) {
			fg_r = (guideline->color >> 24) & 0xff;
			fg_g = (guideline->color >> 16) & 0xff;
			fg_b = (guideline->color >> 8) & 0xff;
			alpha = guideline->color & 0xff;
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
		ix = guideline->position;
		if ((ix >= buf->rect.x0) && (ix < buf->rect.x1)) {
			fg_r = (guideline->color >> 24) & 0xff;
			fg_g = (guideline->color >> 16) & 0xff;
			fg_b = (guideline->color >> 8) & 0xff;
			alpha = guideline->color & 0xff;
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

	guideline->shown = TRUE;
}

static void
sp_guideline_update (SPCanvasItem *item, double *affine, unsigned int flags)
{
	SPGuideLine *guideline;
	ArtPoint p;

	guideline = SP_GUIDELINE (item);

	if (parent_class->update)
		(* parent_class->update) (item, affine, flags);

	if (guideline->shown) {
		if (guideline->orientation == SP_GUIDELINE_ORIENTATION_HORIZONTAL) {
			sp_canvas_request_redraw (item->canvas,
				- 1000000,
				guideline->position,
				1000000,
				guideline->position + 1);
		} else {
			sp_canvas_request_redraw (item->canvas,
				guideline->position,
				-1000000,
				guideline->position + 1,
				1000000);
		}
	}

	sp_canvas_item_reset_bounds (item);

	p.x = p.y = 0.0;
	art_affine_point (&p, &p, affine);

	if (guideline->orientation == SP_GUIDELINE_ORIENTATION_HORIZONTAL) {
		guideline->position = (gint) (p.y + 0.5);
		sp_canvas_update_bbox (item,
			- 1000000,
			guideline->position,
			1000000,
			guideline->position + 1);
	} else {
		guideline->position = (gint) (p.x + 0.5);
		sp_canvas_update_bbox (item,
			guideline->position,
			-1000000,
			guideline->position + 1,
			1000000);
	}
}

static double
sp_guideline_point (SPCanvasItem *item, double x, double y,
	       int cx, int cy, SPCanvasItem **actual_item)
{
	SPGuideLine * guideline;
	gdouble d;

	guideline = SP_GUIDELINE (item);

	if (!guideline->sensitive) return 1e18;

	* actual_item = item;

	if (guideline->orientation == SP_GUIDELINE_ORIENTATION_HORIZONTAL) {
		d = CLAMP (0, 1e18, fabs ((double) cy - guideline->position) - 1.0);
	} else {
		d = CLAMP (0, 1e18, fabs ((double) cx - guideline->position) - 1.0);
	}

	return d;
}

void
sp_guideline_moveto (SPGuideLine * guideline, double x, double y)
{
	double affine[6];

	art_affine_translate (affine, x, y);

	sp_canvas_item_affine_absolute (SP_CANVAS_ITEM (guideline), affine);
}

void
sp_guideline_sensitize (SPGuideLine * guideline, gboolean sensitive)
{
	g_assert (guideline != NULL);
	g_assert (SP_IS_GUIDELINE (guideline));

	guideline->sensitive = sensitive;
}



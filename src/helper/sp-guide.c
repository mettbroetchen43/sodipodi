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
	PROP_0,
	PROP_ORIENTATION,
	PROP_COLOR
};


static void sp_guideline_class_init (SPGuideLineClass *klass);
static void sp_guideline_init (SPGuideLine *guideline);
static void sp_guideline_destroy (GtkObject *object);
static void sp_guideline_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void sp_guideline_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static void sp_guideline_update (SPCanvasItem *item, double *affine, unsigned int flags);
static void sp_guideline_render (SPCanvasItem *item, SPCanvasBuf *buf);

static double sp_guideline_point (SPCanvasItem *item, double x, double y, SPCanvasItem ** actual_item);


static SPCanvasItemClass * parent_class;

GtkType
sp_guideline_get_type (void)
{
	static GtkType guideline_type = 0;

	if (!guideline_type) {
		static const GTypeInfo guideline_info =
		{
			sizeof (SPGuideLineClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) sp_guideline_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (SPGuideLine),
			16,             /* n_preallocs */
			(GInstanceInitFunc) sp_guideline_init,
		};
		guideline_type = g_type_register_static (SP_TYPE_CANVAS_ITEM, "SPGuideLine", &guideline_info, 0);
	}
	return guideline_type;
}

static void
sp_guideline_class_init (SPGuideLineClass *klass)
{
	GObjectClass *g_object_class;
	GtkObjectClass *object_class;
	SPCanvasItemClass *item_class;

	g_object_class = G_OBJECT_CLASS (klass);
	object_class = (GtkObjectClass *) klass;
	item_class = (SPCanvasItemClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	g_object_class->set_property = sp_guideline_set_property;
	g_object_class->get_property = sp_guideline_get_property;

	object_class->destroy = sp_guideline_destroy;

	item_class->update = sp_guideline_update;
	item_class->render = sp_guideline_render;
	item_class->point = sp_guideline_point;

	g_object_class_install_property (g_object_class,
					 PROP_ORIENTATION,
					 g_param_spec_int ("orientation",
							   "Orientation",
							   "Orientation of guideline",
							   SP_GUIDELINE_ORIENTATION_HORIZONTAL,
							   SP_GUIDELINE_ORIENTATION_VERTICAL,
							   SP_GUIDELINE_ORIENTATION_HORIZONTAL,
							   G_PARAM_READWRITE));
	g_object_class_install_property (g_object_class,
					 PROP_COLOR,
					 g_param_spec_uint ("color",
							    "Color",
							    "Guideline color",
							    0x00000000,
							    0xffffffff,
							    0xff000000,
							    G_PARAM_READWRITE));
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
sp_guideline_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	SPCanvasItem *item;
	SPGuideLine *guideline;

	item = SP_CANVAS_ITEM (object);
	guideline = SP_GUIDELINE (object);

	switch (prop_id) {
	case PROP_ORIENTATION:
		guideline->orientation = g_value_get_int (value);
		sp_canvas_item_request_update (item);
		break;
	case PROP_COLOR:
		guideline->color = g_value_get_uint (value);
		sp_canvas_item_request_update (item);
		break;
	default:
		break;
	}
}

static void
sp_guideline_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	SPGuideLine *guideline;

	guideline = SP_GUIDELINE (object);

	switch (prop_id) {
	case PROP_ORIENTATION:
		g_value_set_int (value, (gint)guideline->orientation);
		break;
	case PROP_COLOR:
		g_value_set_uint (value, (guint)guideline->color);
		break;
	default:
		value->g_type = G_TYPE_INVALID;
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
sp_guideline_point (SPCanvasItem *item, double x, double y, SPCanvasItem **actual_item)
{
	SPGuideLine * guideline;
	gdouble d;

	guideline = SP_GUIDELINE (item);

	if (!guideline->sensitive) return 1e18;

	* actual_item = item;

	if (guideline->orientation == SP_GUIDELINE_ORIENTATION_HORIZONTAL) {
		d = CLAMP (0, 1e18, fabs (y - guideline->position) - 1.0);
	} else {
		d = CLAMP (0, 1e18, fabs (x - guideline->position) - 1.0);
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

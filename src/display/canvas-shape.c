#define SP_CANVAS_SHAPE_C

#include <math.h>
#include <gnome.h>
#include <libart_lgpl/art_alphagamma.h>
#include "../helper/canvas-helper.h"
#include "canvas-shape.h"

#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_vpath_bpath.h>
#include <libart_lgpl/art_svp_wind.h>
#include <libart_lgpl/art_svp_point.h>

#define noCANVAS_SHAPE_VERBOSE

static void sp_canvas_shape_class_init (SPCanvasShapeClass *class);
static void sp_canvas_shape_init (SPCanvasShape *path);
static void sp_canvas_shape_destroy (GtkObject *object);

static void sp_canvas_shape_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_canvas_shape, int flags);
static void sp_canvas_shape_render (GnomeCanvasItem * item, GnomeCanvasBuf * buf);
static double sp_canvas_shape_point (GnomeCanvasItem * item, double x, double y,
	int cx, int cy, GnomeCanvasItem ** actual_item);
 
static GnomeCanvasItemClass * parent_class;

GtkType
sp_canvas_shape_get_type (void)
{
	static GtkType path_type = 0;

	if (!path_type) {
		GtkTypeInfo shape_info = {
			"SPCanvasShape",
			sizeof (SPCanvasShape),
			sizeof (SPCanvasShapeClass),
			(GtkClassInitFunc) sp_canvas_shape_class_init,
			(GtkObjectInitFunc) sp_canvas_shape_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		path_type = gtk_type_unique (gnome_canvas_item_get_type (), &shape_info);
	}
	return path_type;
}

static void
sp_canvas_shape_class_init (SPCanvasShapeClass *class)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	object_class->destroy = sp_canvas_shape_destroy;

	item_class->update = sp_canvas_shape_update;
	item_class->render = sp_canvas_shape_render;
	item_class->point = sp_canvas_shape_point;
}

static void
sp_canvas_shape_init (SPCanvasShape * shape)
{
	shape->fill = sp_fill_default ();
	sp_fill_ref (shape->fill);
	shape->stroke = sp_stroke_default ();
	sp_stroke_ref (shape->stroke);
	shape->comp = NULL;
}

static void
sp_canvas_shape_destroy (GtkObject *object)
{
	SPCanvasShape * shape;

	shape = (SPCanvasShape *) object;

	while (shape->comp) {
		sp_cpath_comp_unref ((SPCPathComp *) shape->comp->data);
		shape->comp = g_list_remove (shape->comp, shape->comp->data);
	}

	if (shape->fill)
		sp_fill_unref (shape->fill);

	if (shape->stroke)
		sp_stroke_unref (shape->stroke);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_canvas_shape_request_redraw (SPCanvasShape * shape)
{
	GnomeCanvasItem * item;
	GList * l;
	SPCPathComp * comp;
	ArtUta * uta;

	item = (GnomeCanvasItem *) shape;

	for (l = shape->comp; l != NULL; l = l->next) {
		comp = (SPCPathComp *) l->data;
		if (comp->archetype) {
			if (((comp->bbox.x1 - comp->bbox.x0) < 512.0) &&
				((comp->bbox.y1 - comp->bbox.y0) < 512.0)) {
				/* fixme: cope with miter lines */
				gnome_canvas_request_redraw (GNOME_CANVAS_ITEM (shape)->canvas,
					comp->bbox.x0 - comp->stroke_width - 1.0,
					comp->bbox.y0 - comp->stroke_width - 1.0,
					comp->bbox.x1 + comp->stroke_width + 1.0,
					comp->bbox.y1 + comp->stroke_width + 1.0);
			} else {
			if (comp->archetype->svp != NULL) {
				uta = art_uta_from_svp_translated (comp->archetype->svp, comp->cx, comp->cy);
				gnome_canvas_request_redraw_uta (item->canvas, uta);
			}
			if (comp->archetype->stroke != NULL) {
				uta = art_uta_from_svp_translated (comp->archetype->stroke, comp->cx, comp->cy);
				gnome_canvas_request_redraw_uta (item->canvas, uta);
			}
			}
		}
	}
}

static void
sp_canvas_shape_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	SPCanvasShape * shape;
	SPCPathComp * comp;
	GList * l;
	gboolean bbox_set;

#ifdef CANVAS_SHAPE_VERBOSE
g_print ("sp_canvas_shape_update: entering\n");
#endif

	shape = (SPCanvasShape *) item;

	gnome_canvas_item_reset_bounds (item);

	if (GNOME_CANVAS_ITEM_CLASS(parent_class)->update)
		(* GNOME_CANVAS_ITEM_CLASS(parent_class)->update) (item, affine, clip_path, flags);

	sp_canvas_shape_request_redraw (shape);

	for (l = shape->comp; l != NULL; l = l->next) {
		comp = (SPCPathComp *) l->data;
		if (shape->stroke->type != SP_STROKE_NONE) {
			comp->stroke_width = shape->stroke->width;
			if (shape->stroke->scaled) {
				gdouble wx, wy;
				wx = affine[0] + affine[1];
				wy = affine[2] + affine[3];
				comp->stroke_width *= sqrt (wx * wx + wy * wy) / 1.414213562;
			}
		} else {
			comp->stroke_width = 0.0;
		}
		comp->join = shape->stroke->join;
		comp->cap = shape->stroke->cap;
		sp_cpath_comp_update (comp, affine);
	}

	sp_canvas_shape_request_redraw (shape);

	bbox_set = FALSE;
	for (l = shape->comp; l != NULL; l = l->next) {
		comp = (SPCPathComp *) l->data;
		if (!bbox_set) {
			item->x1 = comp->bbox.x0;
			item->y1 = comp->bbox.y0;
			item->x2 = comp->bbox.x1;
			item->y2 = comp->bbox.y1;
			bbox_set = TRUE;
		} else {
			if (comp->bbox.x0 < item->x1) item->x1 = comp->bbox.x0;
			if (comp->bbox.y0 < item->y1) item->y1 = comp->bbox.y0;
			if (comp->bbox.x1 > item->x2) item->x2 = comp->bbox.x1;
			if (comp->bbox.y1 > item->y2) item->y2 = comp->bbox.y1;
		}
	}

#ifdef CANVAS_SHAPE_VERBOSE
g_print ("sp_canvas_shape_update: leaving\n");
#endif
}

static void
sp_canvas_shape_render (GnomeCanvasItem * item, GnomeCanvasBuf * buf)
{
	SPCanvasShape * shape;
	SPCPathComp * comp;
	GList * l;

	int x, y, width, height;
	guint32 * rgba, src;
	guchar * b;

#ifdef CANVAS_SHAPE_VERBOSE
	g_print ("sp_canvas_shape_render: entering\n");
#endif

	shape = (SPCanvasShape *) item;

	for (l = shape->comp; l != NULL; l = l->next) {
		comp = (SPCPathComp *) l->data;

		if (comp->closed) {
			switch (shape->fill->type) {

			case SP_FILL_COLOR:
				/* Experimental */
#if 0
			{
				ArtRender * render;
				ArtPixMaxDepth c[3];
				gint o;
				g_print ("! %d %d\n", comp->cx, comp->cy);
				if (buf->is_bg) {
					gnome_canvas_clear_buffer (buf);
					buf->is_bg = FALSE;
					buf->is_buf = TRUE;
				}
				c[0] = ART_PIX_MAX_FROM_8 ((shape->fill->color >> 24) & 0xff);
				c[1] = ART_PIX_MAX_FROM_8 ((shape->fill->color >> 16) & 0xff);
				c[2] = ART_PIX_MAX_FROM_8 ((shape->fill->color >> 8) & 0xff);
				o = shape->fill->color & 0xff;
				o = (o << 8) + o + (o >> 7);
				render = art_render_new (buf->rect.x0 - comp->cx, buf->rect.y0 - comp->cy,
							 buf->rect.x1 - comp->cx, buf->rect.y1 - comp->cy,
							 buf->buf, buf->buf_rowstride, 3, 8, ART_ALPHA_NONE, NULL);
				art_render_svp (render, comp->archetype->svp);
				art_render_mask_solid (render, o);
				art_render_image_solid (render, c);
				art_render_invoke (render);
			}
#else
				gnome_canvas_render_svp_translated (buf, comp->archetype->svp, shape->fill->color,
								    comp->cx, comp->cy);
#endif
				break;

			case SP_FILL_IND:
				if (buf->is_bg) {
				  gnome_canvas_clear_buffer (buf);
				  buf->is_bg = FALSE;
				}

				width = buf->rect.x1 - buf->rect.x0;
				height = buf->rect.y1 - buf->rect.y0;
				rgba = g_new (guint32, height * width);

				for (y = 0; y < height; y++) {
				  for (x = 0; x < width; x++) {
				    * (rgba + width * y + x) =
				      (shape->fill->handler.pixel.ind) (buf->rect.x0 + x, buf->rect.y0 + y, shape->fill->handler.data);
				  }
				}

				art_rgb_svp_rgba (comp->archetype->svp,
				  buf->rect.x0 - comp->cx,
				  buf->rect.y0 - comp->cy,
				  buf->rect.x1 - comp->cx,
				  buf->rect.y1 - comp->cy,
				  rgba,
				  0,0,
				  width,
				  buf->buf, buf->buf_rowstride,
				  NULL);

				g_free (rgba);
				break;

			case SP_FILL_DEP:
				if (buf->is_bg) {
				  gnome_canvas_clear_buffer (buf);
				  buf->is_bg = FALSE;
				}

				width = buf->rect.x1 - buf->rect.x0;
				height = buf->rect.y1 - buf->rect.y0;
				rgba = g_new (guint32, height * width);

				for (y = 0; y < height; y++) {
				  for (x = 0; x < width; x++) {
				    b = buf->buf + y * buf->buf_rowstride + 3 * x;
				    src = ((guint32) * (b    ) << 24) |
				          ((guint32) * (b + 1) << 16) |
				          ((guint32) * (b + 2) <<  8);
				    * (rgba + width * y + x) =
				      (shape->fill->handler.pixel.dep) (buf->rect.x0 + x, buf->rect.y0 + y, src, shape->fill->handler.data);
				  }
				}

				art_rgb_svp_rgba (comp->archetype->svp,
				  buf->rect.x0 - comp->cx,
				  buf->rect.y0 - comp->cy,
				  buf->rect.x1 - comp->cx,
				  buf->rect.y1 - comp->cy,
				  rgba,
				  0,0,
				  width,
				  buf->buf, buf->buf_rowstride,
				  NULL);

				g_free (rgba);
				break;

			default:
				if (buf->is_bg) {
				  gnome_canvas_clear_buffer (buf);
				  buf->is_bg = FALSE;
				}
				break;
			}
		}

		switch (shape->stroke->type) {
			case SP_STROKE_COLOR:
				if (comp->archetype->stroke) {
					gnome_canvas_render_svp_translated (buf, comp->archetype->stroke, shape->stroke->color,
									    comp->cx, comp->cy);
				}
				break;

			default:
				break;
		}
	}
#ifdef SHAPE_VERBOSE
	g_print ("sp_canvas_shape_render: leaving\n");
#endif
}


static double
sp_canvas_shape_point (GnomeCanvasItem * item, double x, double y,
	int cx, int cy, GnomeCanvasItem ** actual_item)
{
	SPCanvasShape * shape;
	SPCPathComp * comp;
	GList * l;

	double dist, best;
	int wind;

	shape = (SPCanvasShape *) item;

	best = 1e36;

	/* todo: update? */

	for (l = shape->comp; l != NULL; l = l->next) {
		comp = l->data;

		if (comp->archetype != NULL) {
		if ((comp->archetype->svp != NULL) && (comp->closed) && (shape->fill->type != SP_FILL_NONE)) {
				wind = art_svp_point_wind (comp->archetype->svp, cx - comp->cx, cy - comp->cy);

				if (wind) {
					*actual_item = item;
					return 0.0;
				}

				dist = art_svp_point_dist (comp->archetype->svp, cx - comp->cx, cy - comp->cy);
				if (dist < best) best = dist;
		}
		if ((comp->archetype->stroke != NULL) && (shape->stroke->type != SP_STROKE_NONE)) {
			wind = art_svp_point_wind (comp->archetype->stroke, cx - comp->cx, cy - comp->cy);

			if (wind) {
				*actual_item = item;
				return 0.0;
			}

			dist = art_svp_point_dist (comp->archetype->stroke, cx - comp->cx, cy - comp->cy);
			if (dist < best) best = dist;
		}
		}
	}

	return best;
}

void
sp_canvas_shape_clear (SPCanvasShape * shape)
{
	SPCPathComp * comp;

	sp_canvas_shape_request_redraw (shape);

	while (shape->comp) {
		comp = (SPCPathComp *)(shape->comp->data);
		sp_cpath_comp_unref (comp);
		shape->comp = g_list_remove (shape->comp, comp);
	}

	gnome_canvas_item_request_update ((GnomeCanvasItem *) shape);
}

void
sp_canvas_shape_add_component (SPCanvasShape * shape, SPCurve * curve, gboolean private, gdouble affine[])
{
	SPCPathComp * comp;

	g_return_if_fail (shape != NULL);
	g_return_if_fail (SP_IS_CANVAS_SHAPE (shape));
	g_return_if_fail (curve != NULL);

	comp = sp_cpath_comp_new (curve, private, affine, shape->stroke->width, shape->stroke->join, shape->stroke->cap);
	g_assert (comp != NULL);
	shape->comp = g_list_prepend (shape->comp, comp);

	gnome_canvas_item_request_update ((GnomeCanvasItem *) shape);
}

void
sp_canvas_shape_set_component (SPCanvasShape * shape, SPCurve * curve, gboolean private, gdouble affine[])
{
	g_return_if_fail (shape != NULL);
	g_return_if_fail (SP_IS_CANVAS_SHAPE (shape));
	g_return_if_fail (curve != NULL);

	sp_canvas_shape_clear (shape);
	sp_canvas_shape_add_component (shape, curve, private, affine);

	gnome_canvas_item_request_update ((GnomeCanvasItem *) shape);
}

/* fixme: */

void
sp_canvas_shape_change_bpath (SPCanvasShape * shape, SPCurve * curve)
{
	SPCPathComp * comp;

	g_return_if_fail (shape != NULL);
	g_return_if_fail (SP_IS_CANVAS_SHAPE (shape));
	g_return_if_fail (curve != NULL);

	if (shape->comp == NULL) {
	/* zero components */
		sp_canvas_shape_add_component (shape, curve, TRUE, NULL);
	} else {
		/* single component */
		g_assert (shape->comp->next == NULL);

		/* and is private */
		comp = (SPCPathComp *) shape->comp->data;
		g_assert (comp->private);

		sp_cpath_comp_change (comp, curve, TRUE, comp->affine, comp->stroke_width, comp->join, comp->cap);
	}

	gnome_canvas_item_request_update ((GnomeCanvasItem *) shape);
}

void
sp_canvas_shape_set_fill (SPCanvasShape * shape, SPFill * fill)
{
	SPFill * old;

	g_return_if_fail (shape != NULL);
	g_return_if_fail (SP_IS_CANVAS_SHAPE (shape));
	g_return_if_fail (fill != NULL);

	old = shape->fill;
	shape->fill = fill;
	sp_fill_ref (shape->fill);
	sp_fill_unref (old);

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (shape));
}

void
sp_canvas_shape_set_stroke (SPCanvasShape * shape, SPStroke * stroke)
{
	SPStroke * old;

	g_return_if_fail (shape != NULL);
	g_return_if_fail (SP_IS_CANVAS_SHAPE (shape));
	g_return_if_fail (stroke != NULL);

	old = shape->stroke;
	shape->stroke = stroke;
	sp_stroke_ref (shape->stroke);
	sp_stroke_unref (old);

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (shape));
}



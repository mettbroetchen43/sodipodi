#define SP_SHAPE_C

#include <config.h>
#include <math.h>
#include <gnome.h>

#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath.h>
#include <libart_lgpl/art_svp_wind.h>
#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_vpath_bpath.h>
#include <libart_lgpl/art_rgb_svp.h>

#include "helper/art-rgba-svp.h"
#include "display/canvas-shape.h"
#include "style.h"
#include "sp-path-component.h"
#include "sp-shape.h"

#define noSHAPE_VERBOSE

static void sp_shape_class_init (SPShapeClass *class);
static void sp_shape_init (SPShape *shape);
static void sp_shape_destroy (GtkObject *object);

static void sp_shape_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_shape_read_attr (SPObject * object, const gchar * attr);

void sp_shape_print (SPItem * item, GnomePrintContext * gpc);
static gchar * sp_shape_description (SPItem * item);
static GnomeCanvasItem * sp_shape_show (SPItem * item, SPDesktop * desktop, GnomeCanvasGroup * canvas_group);
static gboolean sp_shape_paint (SPItem * item, ArtPixBuf * buf, gdouble * affine);

void sp_shape_remove_comp (SPPath * path, SPPathComp * comp);
void sp_shape_add_comp (SPPath * path, SPPathComp * comp);
void sp_shape_change_bpath (SPPath * path, SPPathComp * comp, SPCurve * curve);

static SPPathClass * parent_class;

GtkType
sp_shape_get_type (void)
{
	static GtkType shape_type = 0;

	if (!shape_type) {
		GtkTypeInfo shape_info = {
			"SPShape",
			sizeof (SPShape),
			sizeof (SPShapeClass),
			(GtkClassInitFunc) sp_shape_class_init,
			(GtkObjectInitFunc) sp_shape_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		shape_type = gtk_type_unique (sp_path_get_type (), &shape_info);
	}
	return shape_type;
}

static void
sp_shape_class_init (SPShapeClass * klass)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;
	SPPathClass * path_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;
	path_class = (SPPathClass *) klass;

	parent_class = gtk_type_class (sp_path_get_type ());

	gtk_object_class->destroy = sp_shape_destroy;

	sp_object_class->build = sp_shape_build;
	sp_object_class->read_attr = sp_shape_read_attr;

	item_class->print = sp_shape_print;
	item_class->description = sp_shape_description;
	item_class->show = sp_shape_show;
	item_class->paint = sp_shape_paint;

	path_class->remove_comp = sp_shape_remove_comp;
	path_class->add_comp = sp_shape_add_comp;
	path_class->change_bpath = sp_shape_change_bpath;
}

static void
sp_shape_init (SPShape *shape)
{
	/* Nothing here */
}

static void
sp_shape_destroy (GtkObject *object)
{
	SPShape *shape;

	shape = SP_SHAPE (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_shape_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	if (((SPObjectClass *) (parent_class))->build)
		(*((SPObjectClass *) (parent_class))->build) (object, document, repr);

	sp_shape_read_attr (object, "style");
	sp_shape_read_attr (object, "insensitive");
}

static void
sp_shape_read_attr (SPObject * object, const gchar * attr)
{
	SPShape * shape;
	SPCanvasShape * cs;
	SPItemView * v;

	shape = SP_SHAPE (object);

	if (strcmp (attr, "insensitive") == 0) {
		const gchar * val;
		gboolean sensitive;
		SPItemView * v;

		val = sp_repr_attr (object->repr, attr);
		sensitive = (val == NULL);

		for (v = ((SPItem *) object)->display; v != NULL; v = v->next) {
			sp_canvas_shape_set_sensitive (SP_CANVAS_SHAPE (v->canvasitem), sensitive);
		}
		return;
	}


	if (((SPObjectClass *) (parent_class))->read_attr)
		(* ((SPObjectClass *) (parent_class))->read_attr) (object, attr);

	if (!strcmp (attr, "style")) {
		/* Style was read by item */
		for (v = SP_ITEM (shape)->display; v != NULL; v = v->next) {
			cs = SP_CANVAS_SHAPE (v->canvasitem);
			sp_canvas_shape_set_style (cs, object->style);
		}
	}
}

void
sp_shape_print (SPItem * item, GnomePrintContext * gpc)
{

	gdouble r, g, b, opacity;
	SPObject *object;
	SPPath *path;
	SPShape * shape;
	SPPathComp * comp;
	GSList * l;
	ArtBpath * bpath;

	object = SP_OBJECT (item);
	path = SP_PATH (item);
	shape = SP_SHAPE (item);

#ifndef ENABLE_FRGBA

	opacity = object->style->fill_opacity * object->style->real_opacity;

	if ((object->style->fill.type == SP_FILL_COLOR) && (opacity != 1.0)) {
		gdouble i2d[6], doc2d[6], doc2buf[6], d2buf[6], i2buf[6], d2i[6];
		ArtDRect box, bbox, dbbox;
		gint bx, by, bw, bh;
		art_u8 * b;
		ArtPixBuf * pb;

		dbbox.x0 = 0.0;
		dbbox.y0 = 0.0;
		dbbox.x1 = sp_document_width (SP_OBJECT (item)->document);
		dbbox.y1 = sp_document_height (SP_OBJECT (item)->document);
		sp_item_bbox (item, &bbox);
		art_drect_intersect (&box, &dbbox, &bbox);
		if ((box.x1 - box.x0) < 1.0) return;
		if ((box.y1 - box.y0) < 1.0) return;
		art_affine_identity (d2buf);
		d2buf[4] = -box.x0;
		d2buf[5] = -box.y0;
		bx = box.x0;
		by = box.y0;
		bw = (box.x1 + 1.0) - bx;
		bh = (box.y1 + 1.0) - by;
		b = art_new (art_u8, bw * bh * 3);
		memset (b, 0xff, bw * bh * 3);
		pb = art_pixbuf_new_rgb (b, bw, bh, bw * 3);
		sp_item_i2d_affine (SP_ITEM (sp_document_root (SP_OBJECT (item)->document)), doc2d);
		art_affine_multiply (doc2buf, doc2d, d2buf);
		item->stop_paint = TRUE;
		sp_item_paint (SP_ITEM (sp_document_root (SP_OBJECT (item)->document)), pb, doc2buf);
		item->stop_paint = FALSE;
		sp_item_i2d_affine (item, i2d);
		art_affine_multiply (i2buf, i2d, d2buf);
		sp_item_paint (item, pb, i2buf);

		gnome_print_gsave (gpc);
		art_affine_invert (d2i, i2d);
		gnome_print_concat (gpc, d2i);
		gnome_print_translate (gpc, bx, by + bh);
		gnome_print_scale (gpc, bw, -bh);
		gnome_print_rgbimage (gpc, b, bw, bh, bw * 3);
		gnome_print_grestore (gpc);

		art_pixbuf_free (pb);

		return;
	}

#endif /* ENABLE_FRGBA */

	gnome_print_gsave (gpc);

	for (l = path->comp; l != NULL; l = l->next) {
		comp = (SPPathComp *) l->data;
		if (comp->curve != NULL) {
			gnome_print_gsave (gpc);
			gnome_print_concat (gpc, comp->affine);
			bpath = comp->curve->bpath;

			gnome_print_bpath (gpc, bpath, FALSE);

			if (object->style->fill.type == SP_PAINT_TYPE_COLOR) {
				r = object->style->fill.color.r;
				g = object->style->fill.color.g;
				b = object->style->fill.color.b;
				opacity = object->style->fill_opacity * object->style->real_opacity;
				gnome_print_gsave (gpc);
				gnome_print_setrgbcolor (gpc, r, g, b);
				gnome_print_setopacity (gpc, opacity);
				gnome_print_eofill (gpc);
				gnome_print_grestore (gpc);
			}
			if (object->style->stroke.type == SP_PAINT_TYPE_COLOR) {
				r = object->style->stroke.color.r;
				g = object->style->stroke.color.g;
				b = object->style->stroke.color.b;
				opacity = object->style->stroke_opacity * object->style->real_opacity;
				gnome_print_gsave (gpc);
				gnome_print_setrgbcolor (gpc, r, g, b);
				gnome_print_setopacity (gpc, opacity);
				gnome_print_setlinewidth (gpc, object->style->user_stroke_width);
				gnome_print_setlinejoin (gpc, object->style->stroke_linejoin);
				gnome_print_setlinecap (gpc, object->style->stroke_linecap);
				gnome_print_stroke (gpc);
				gnome_print_grestore (gpc);
			}
		}
		gnome_print_grestore (gpc);
	}
	gnome_print_grestore (gpc);
}

static gchar *
sp_shape_description (SPItem * item)
{
	return g_strdup (_("A path - whatever it means"));
}

static GnomeCanvasItem *
sp_shape_show (SPItem * item, SPDesktop * desktop, GnomeCanvasGroup * canvas_group)
{
	SPObject *object;
	SPShape * shape;
	SPPath * path;
	SPCanvasShape * cs;
	SPPathComp * comp;
	GSList * l;

	object = SP_OBJECT (item);
	shape = SP_SHAPE (item);
	path = SP_PATH (item);

	cs = (SPCanvasShape *) gnome_canvas_item_new (canvas_group, SP_TYPE_CANVAS_SHAPE, NULL);
	g_return_val_if_fail (cs != NULL, NULL);

	sp_canvas_shape_set_style (cs, object->style);

	for (l = path->comp; l != NULL; l = l->next) {
		comp = (SPPathComp *) l->data;
		sp_canvas_shape_add_component (cs, comp->curve, comp->private, comp->affine);
	}

	return (GnomeCanvasItem *) cs;
}

static gboolean
sp_shape_paint (SPItem * item, ArtPixBuf * buf, gdouble * affine)
{
	SPObject *object;
	SPPath *path;
	SPShape * shape;
	SPStyle *style;
	SPPathComp * comp;
	GSList * l;
	gdouble a[6];
	ArtBpath * abp;
	ArtVpath * vp, * perturbed_vpath;
	ArtSVP * svp, * svpa, * svpb;

	object = SP_OBJECT (item);
	path = SP_PATH (item);
	shape = SP_SHAPE (item);
	style = object->style;

	for (l = path->comp; l != NULL; l = l->next) {
		comp = (SPPathComp *) l->data;
		if (comp->curve != NULL) {
			art_affine_multiply (a, comp->affine, affine);
			abp = art_bpath_affine_transform (comp->curve->bpath, a);
			vp = art_bez_path_to_vec (abp, 0.25);
			art_free (abp);

			if (comp->curve->closed) {
				perturbed_vpath = art_vpath_perturb (vp);
				svpa = art_svp_from_vpath (perturbed_vpath);
				art_free (perturbed_vpath);
				svpb = art_svp_uncross (svpa);
				art_svp_free (svpa);
				svp = art_svp_rewind_uncrossed (svpb, ART_WIND_RULE_ODDEVEN);
				art_svp_free (svpb);

				if (style->fill.type == SP_PAINT_TYPE_COLOR) {
					guint32 rgba;
					rgba = SP_RGBA_FROM_COLOR (&style->fill.color, style->fill_opacity * style->real_opacity);
					if (buf->n_channels == 3) {
						art_rgb_svp_alpha (svp,
							0, 0, buf->width, buf->height,
							rgba,
							buf->pixels, buf->rowstride, NULL);
					} else {
						art_rgba_svp_alpha (svp,
							0, 0, buf->width, buf->height,
							rgba,
							buf->pixels, buf->rowstride, NULL);
					}
				}
				art_svp_free (svp);
			}

			if (object->style->stroke.type == SP_PAINT_TYPE_COLOR) {
				gdouble width, wx, wy;
				guint32 rgba;

				width = object->style->user_stroke_width;
				wx = affine[0] + affine[1];
				wy = affine[2] + affine[3];
				width *= sqrt (wx * wx + wy * wy) * 0.707106781;

				svp = art_svp_vpath_stroke (vp,
							    object->style->stroke_linejoin,
							    object->style->stroke_linecap,
							    width,
							    object->style->stroke_miterlimit,
							    0.25);
				rgba = SP_RGBA_FROM_COLOR (&style->stroke.color, style->stroke_opacity * style->real_opacity);
				if (buf->n_channels == 3) {
					art_rgb_svp_alpha (svp,
						0, 0, buf->width, buf->height,
						rgba,
						buf->pixels, buf->rowstride, NULL);
				} else {
					art_rgba_svp_alpha (svp,
						0, 0, buf->width, buf->height,
						rgba,
						buf->pixels, buf->rowstride, NULL);
				}
				art_svp_free (svp);
			}
			art_free (vp);
		}
	}

	return FALSE;
}

void
sp_shape_remove_comp (SPPath * path, SPPathComp * comp)
{
	SPItem * item;
	SPShape * shape;
	SPCanvasShape * cs;
	SPItemView * v;

	item = SP_ITEM (path);
	shape = SP_SHAPE (path);

	/* fixme: */
	for (v = item->display; v != NULL; v = v->next) {
		cs = (SPCanvasShape *) v->canvasitem;
		sp_canvas_shape_clear (cs);
	}

	if (SP_PATH_CLASS (parent_class)->remove_comp)
		SP_PATH_CLASS (parent_class)->remove_comp (path, comp);
}

void
sp_shape_add_comp (SPPath * path, SPPathComp * comp)
{
	SPItem * item;
	SPShape * shape;
	SPCanvasShape * cs;
	SPItemView * v;

	item = SP_ITEM (path);
	shape = SP_SHAPE (path);

	for (v = item->display; v != NULL; v = v->next) {
		cs = (SPCanvasShape *) v->canvasitem;
		sp_canvas_shape_add_component (cs, comp->curve, comp->private, comp->affine);
	}

	if (SP_PATH_CLASS (parent_class)->add_comp)
		SP_PATH_CLASS (parent_class)->add_comp (path, comp);
}

void
sp_shape_change_bpath (SPPath * path, SPPathComp * comp, SPCurve * curve)
{
	SPItem * item;
	SPShape * shape;
	SPCanvasShape * cs;
	SPItemView * v;

	item = SP_ITEM (path);
	shape = SP_SHAPE (path);

	/* fixme: */
	for (v = item->display; v != NULL; v = v->next) {
		cs = (SPCanvasShape *) v->canvasitem;
		sp_canvas_shape_change_bpath (cs, comp->curve);
	}

	if (SP_PATH_CLASS (parent_class)->change_bpath)
		SP_PATH_CLASS (parent_class)->change_bpath (path, comp, curve);
}


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
#include <libart_lgpl/art_gray_svp.h>
#include <libart_lgpl/art_rect_svp.h>

#include "dialogs/fill-style.h"
#include "helper/art-rgba-svp.h"
#include "display/nr-arena-shape.h"
#include "document.h"
#include "desktop.h"
#include "selection.h"
#include "desktop-handles.h"
#include "sp-paint-server.h"
#include "style.h"
#include "sp-path-component.h"
#include "sp-shape.h"

#define noSHAPE_VERBOSE

static void sp_shape_class_init (SPShapeClass *class);
static void sp_shape_init (SPShape *shape);
static void sp_shape_destroy (GtkObject *object);

static void sp_shape_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_shape_read_attr (SPObject * object, const gchar * attr);
static void sp_shape_style_changed (SPObject *object, guint flags);

void sp_shape_print (SPItem * item, GnomePrintContext * gpc);
static gchar * sp_shape_description (SPItem * item);
static NRArenaItem *sp_shape_show (SPItem *item, NRArena *arena);
static gboolean sp_shape_paint (SPItem * item, ArtPixBuf * buf, gdouble * affine);
static void sp_shape_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);

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
	sp_object_class->style_changed = sp_shape_style_changed;

	item_class->print = sp_shape_print;
	item_class->description = sp_shape_description;
	item_class->show = sp_shape_show;
	item_class->paint = sp_shape_paint;
	item_class->menu = sp_shape_menu;

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
}

static void
sp_shape_read_attr (SPObject * object, const gchar * attr)
{
	SPShape * shape;

	shape = SP_SHAPE (object);

	if (((SPObjectClass *) (parent_class))->read_attr)
		(* ((SPObjectClass *) (parent_class))->read_attr) (object, attr);
}

static void
sp_shape_style_changed (SPObject *object, guint flags)
{
	SPShape *shape;
	SPItemView *v;

	shape = SP_SHAPE (object);

	/* Item class reads style */
	if (((SPObjectClass *) (parent_class))->style_changed)
		(* ((SPObjectClass *) (parent_class))->style_changed) (object, flags);

	for (v = SP_ITEM (shape)->display; v != NULL; v = v->next) {
		/* fixme: */
		nr_arena_shape_group_set_style (NR_ARENA_SHAPE_GROUP (v->arenaitem), object->style);
	}
}

void
sp_shape_print (SPItem * item, GnomePrintContext * gpc)
{

	gfloat rgb[3], opacity;
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
			const ArtBpath *bp;
			gboolean closed;

			gnome_print_gsave (gpc);
			gnome_print_concat (gpc, comp->affine);
			bpath = comp->curve->bpath;

			closed = TRUE;
			for (bp = bpath; bp->code != ART_END; bp++) {
				if (bp->code == ART_MOVETO_OPEN) {
					closed = FALSE;
					break;
				}
			}

			gnome_print_bpath (gpc, bpath, FALSE);

			if (closed) {
				if (object->style->fill.type == SP_PAINT_TYPE_COLOR) {
					sp_color_get_rgb_floatv (&object->style->fill.color, rgb);
					opacity = object->style->fill_opacity * object->style->real_opacity;
					gnome_print_gsave (gpc);
					gnome_print_setrgbcolor (gpc, rgb[0], rgb[1], rgb[2]);
					gnome_print_setopacity (gpc, opacity);
					if (object->style->fill_rule == ART_WIND_RULE_ODDEVEN) {
						gnome_print_eofill (gpc);
					} else {
						gnome_print_fill (gpc);
					}
					gnome_print_grestore (gpc);
				} else if (object->style->fill.type == SP_PAINT_TYPE_PAINTSERVER) {
					SPPainter *painter;
					ArtDRect bbox;
					gdouble id[6] = {1,0,0,1,0,0};
					sp_item_bbox (item, &bbox);
					painter = sp_paint_server_painter_new (object->style->fill.server, id, object->style->real_opacity, &bbox);
					if (painter) {
						ArtDRect dbox, cbox;
						ArtIRect ibox;
						gdouble i2d[6], d2i[6];
						gint x, y;
						dbox.x0 = 0.0;
						dbox.y0 = 0.0;
						dbox.x1 = sp_document_width (SP_OBJECT_DOCUMENT (item));
						dbox.y1 = sp_document_height (SP_OBJECT_DOCUMENT (item));
						art_drect_intersect (&cbox, &dbox, &bbox);
						art_drect_to_irect (&ibox, &cbox);
						sp_item_i2d_affine (item, i2d);
						art_affine_invert (d2i, i2d);

						gnome_print_gsave (gpc);
						if (object->style->fill_rule == ART_WIND_RULE_ODDEVEN) {
							gnome_print_eoclip (gpc);
						} else {
							gnome_print_clip (gpc);
						}
						gnome_print_bpath (gpc, bpath, FALSE);
						gnome_print_concat (gpc, d2i);
						/* Now we are in desktop coordinates */
						for (y = ibox.y0; y < ibox.y1; y+= 64) {
							for (x = ibox.x0; x < ibox.x1; x+= 64) {
								static guchar *rgba = NULL;
								if (!rgba) rgba = g_new (guchar, 4 * 64 * 64);
								painter->fill (painter, rgba, x, ibox.y1 + ibox.y0 - y - 64, 64, 64, 64);
								gnome_print_gsave (gpc);
								gnome_print_translate (gpc, x, y);
								gnome_print_scale (gpc, 64, 64);
								gnome_print_rgbaimage (gpc, rgba, 64, 64, 4 * 64);
								gnome_print_grestore (gpc);
							}
						}
						gnome_print_grestore (gpc);
						sp_painter_free (painter);
					}
				}
			}
			if (object->style->stroke.type == SP_PAINT_TYPE_COLOR) {
				sp_color_get_rgb_floatv (&object->style->stroke.color, rgb);
				opacity = object->style->stroke_opacity * object->style->real_opacity;
				gnome_print_gsave (gpc);
				gnome_print_setrgbcolor (gpc, rgb[0], rgb[1], rgb[2]);
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

static NRArenaItem *
sp_shape_show (SPItem *item, NRArena *arena)
{
	SPObject *object;
	SPShape * shape;
	SPPath * path;
	NRArenaItem *arenaitem;
	SPPathComp * comp;
	GSList * l;

	object = SP_OBJECT (item);
	shape = SP_SHAPE (item);
	path = SP_PATH (item);

	arenaitem = nr_arena_item_new (arena, NR_TYPE_ARENA_SHAPE_GROUP);

	nr_arena_shape_group_set_style (NR_ARENA_SHAPE_GROUP (arenaitem), object->style);

	for (l = path->comp; l != NULL; l = l->next) {
		comp = (SPPathComp *) l->data;
		nr_arena_shape_group_add_component (NR_ARENA_SHAPE_GROUP (arenaitem), comp->curve, comp->private, comp->affine);
	}

	return arenaitem;
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
				svp = art_svp_rewind_uncrossed (svpb, style->fill_rule);
				art_svp_free (svpb);

				if (style->fill.type == SP_PAINT_TYPE_COLOR) {
					guint32 rgba;
					rgba = sp_color_get_rgba32_falpha (&style->fill.color, style->fill_opacity * style->real_opacity);
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
				} else if (style->fill.type == SP_PAINT_TYPE_PAINTSERVER) {
					SPPainter *painter;
					ArtDRect bbox;
					/* Find item bbox */
					/* fixme: This does not work for multi-component objects (text) */
					art_drect_svp (&bbox, svp);
					painter = sp_paint_server_painter_new (style->fill.server, affine, style->real_opacity, &bbox);
					if (painter) {
						ArtDRect abox;
						ArtIRect ibox;
						/* Find component bbox */
						art_drect_svp (&abox, svp);
						/* Find integer bbox */
						art_drect_to_irect (&ibox, &abox);
						/* Clip */
						ibox.x0 = MAX (ibox.x0, 0);
						ibox.y0 = MAX (ibox.y0, 0);
						ibox.x1 = MIN (ibox.x1, buf->width);
						ibox.y1 = MIN (ibox.y1, buf->height);
						if ((ibox.x0 < ibox.x1) && (ibox.y0 < ibox.y1)) {
							static guchar *gray = NULL;
							static guchar *rgba = NULL;
							gint x, y;
							if (!gray) gray = g_new (guchar, 64 * 64);
							if (!rgba) rgba = g_new (guchar, 4 * 64 * 64);
							for (y = ibox.y0; y < ibox.y1; y+= 64) {
								for (x = ibox.x0; x < ibox.x1; x+= 64) {
									gint xx, yy, xe, ye;
									/* Draw grayscale image */
									art_gray_svp_aa (svp, x, y, x + 64, y + 64, gray, 64);
									/* Render painter */
									painter->fill (painter, rgba, x, y, 64, 64, 64);
									/* Compose */
									xe = MIN (x + 64, ibox.x1) - x;
									ye = MIN (y + 64, ibox.y1) - y;
									for (yy = 0; yy < ye; yy++) {
										guchar *gp, *bp;
										guchar *pp;
										gp = gray + yy * 64;
										pp = rgba + 4 * yy * 64;
										if (buf->n_channels == 3) {
											/* RGB buffer */
											bp = buf->pixels + (y + yy) * buf->rowstride + x * 3;
											for (xx = 0; xx < xe; xx++) {
												guint a, fc;
												/* fixme: */
												a = (pp[3] * (*gp) + 0x80) >> 8;
												fc = (pp[0] - *bp) * a;
												*bp++ = *bp + ((fc + (fc >> 8) + 0x80) >> 8);
												fc = (pp[1] - *bp) * a;
												*bp++ = *bp + ((fc + (fc >> 8) + 0x80) >> 8);
												fc = (pp[2] - *bp) * a;
												*bp++ = *bp + ((fc + (fc >> 8) + 0x80) >> 8);
												pp += 4;
												gp++;
											}
										} else {
											/* RGBA buffer */
											bp = buf->pixels + (y + yy) * buf->rowstride + x * 4;
											for (xx = 0; xx < xe; xx++) {
												guint a, bg_r, bg_g, bg_b, bg_a, tmp;
												/* fixme: */
												a = ((*pp & 0xff) * (*gp)) / 255;
												bg_r = bp[0];
												bg_g = bp[1];
												bg_b = bp[2];
												bg_a = bp[3];
												tmp = (((*pp >> 24) & 0xff) - bg_r) * a;
												*bp++ = bg_r + ((tmp + (tmp >> 8) + 0x80) >> 8);
												tmp = (((*pp >> 16) & 0xff) - bg_g) * a;
												*bp++ = bg_g + ((tmp + (tmp >> 8) + 0x80) >> 8);
												tmp = (((*pp >> 8) & 0xff) - bg_b) * a;
												*bp++ = bg_b + ((tmp + (tmp >> 8) + 0x80) >> 8);
												*bp++ = bg_a + (((255 - bg_a) * a + 0x80) >> 8);
												pp++;
												gp++;
											}
										}
									}
								}
							}
						}
						sp_painter_free (painter);
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
				rgba = sp_color_get_rgba32_falpha (&style->stroke.color, style->stroke_opacity * style->real_opacity);
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

/* Generate context menu item section */

static void
sp_shape_fill_settings (GtkMenuItem *menuitem, SPItem *item)
{
	SPDesktop *desktop;

	g_assert (SP_IS_ITEM (item));

	desktop = gtk_object_get_data (GTK_OBJECT (menuitem), "desktop");
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	sp_selection_set_item (SP_DT_SELECTION (desktop), item);

	sp_fill_style_dialog (item);
}

static void
sp_shape_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu)
{
	GtkWidget *i, *m, *w;

	if (SP_ITEM_CLASS (parent_class)->menu)
		(* SP_ITEM_CLASS (parent_class)->menu) (item, desktop, menu);

	/* Create toplevel menuitem */
	i = gtk_menu_item_new_with_label (_("Shape"));
	m = gtk_menu_new ();
	/* Item dialog */
	w = gtk_menu_item_new_with_label (_("Fill settings"));
	gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_shape_fill_settings), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Show menu */
	gtk_widget_show (m);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), m);

	gtk_menu_append (menu, i);
	gtk_widget_show (i);
}

void
sp_shape_remove_comp (SPPath * path, SPPathComp * comp)
{
	SPItem * item;
	SPShape * shape;
	SPItemView * v;

	item = SP_ITEM (path);
	shape = SP_SHAPE (path);

	/* fixme: */
	for (v = item->display; v != NULL; v = v->next) {
		nr_arena_shape_group_clear (NR_ARENA_SHAPE_GROUP (v->arenaitem));
	}

	if (SP_PATH_CLASS (parent_class)->remove_comp)
		SP_PATH_CLASS (parent_class)->remove_comp (path, comp);
}

void
sp_shape_add_comp (SPPath * path, SPPathComp * comp)
{
	SPItem * item;
	SPShape * shape;
	SPItemView * v;

	item = SP_ITEM (path);
	shape = SP_SHAPE (path);

	for (v = item->display; v != NULL; v = v->next) {
		nr_arena_shape_group_add_component (NR_ARENA_SHAPE_GROUP (v->arenaitem), comp->curve, comp->private, comp->affine);
	}

	if (SP_PATH_CLASS (parent_class)->add_comp)
		SP_PATH_CLASS (parent_class)->add_comp (path, comp);
}

void
sp_shape_change_bpath (SPPath * path, SPPathComp * comp, SPCurve * curve)
{
	SPItem * item;
	SPShape * shape;
	SPItemView * v;

	item = SP_ITEM (path);
	shape = SP_SHAPE (path);

	/* fixme: */
	for (v = item->display; v != NULL; v = v->next) {
#if 0
		cs = (SPCanvasShape *) v->canvasitem;
		sp_canvas_shape_change_bpath (cs, comp->curve);
#endif
	}

	if (SP_PATH_CLASS (parent_class)->change_bpath)
		SP_PATH_CLASS (parent_class)->change_bpath (path, comp, curve);
}


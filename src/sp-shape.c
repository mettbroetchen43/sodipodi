#define __SP_SHAPE_C__

/*
 * Base class for shapes, including <path> element
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>
#include <math.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_wind.h>

#include "svg/svg.h"
#include "dialogs/fill-style.h"
#include "display/nr-arena-shape.h"
#include "document.h"
#include "desktop.h"
#include "selection.h"
#include "desktop-handles.h"
#include "sp-paint-server.h"
#include "style.h"
#include "sp-shape.h"

#define noSHAPE_VERBOSE

static void sp_shape_class_init (SPShapeClass *class);
static void sp_shape_init (SPShape *shape);
static void sp_shape_destroy (GtkObject *object);

static void sp_shape_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_shape_write_repr (SPObject * object, SPRepr * repr);
static void sp_shape_read_attr (SPObject * object, const gchar * attr);
static void sp_shape_style_modified (SPObject *object, guint flags);

void sp_shape_print (SPItem * item, GnomePrintContext * gpc);
static gchar * sp_shape_description (SPItem * item);
static NRArenaItem *sp_shape_show (SPItem *item, NRArena *arena);
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
	sp_object_class->write_repr = sp_shape_write_repr;
	sp_object_class->read_attr = sp_shape_read_attr;
	sp_object_class->style_modified = sp_shape_style_modified;

	item_class->print = sp_shape_print;
	item_class->description = sp_shape_description;
	item_class->show = sp_shape_show;
	item_class->menu = sp_shape_menu;

	path_class->remove_comp = sp_shape_remove_comp;
	path_class->add_comp = sp_shape_add_comp;
	path_class->change_bpath = sp_shape_change_bpath;

	klass->set_shape = NULL;
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
sp_shape_write_repr (SPObject * object, SPRepr * repr)
{
	SPPath     *path;
	SPPathComp *pathcomp;
	ArtBpath   *abp;
	gchar      *str;

	path = SP_PATH(object);
	g_assert (path->comp);
	pathcomp = path->comp->data;
	g_assert (pathcomp);
	abp = sp_curve_first_bpath (pathcomp->curve);
	str = sp_svg_write_path (abp);
	g_assert (str != NULL);
	sp_repr_set_attr (repr, "d", str);
	g_free (str);

#if 0
	if (((SPObjectClass *) (parent_class))->write_repr)
		(*((SPObjectClass *) (parent_class))->write_repr) (object, repr);
#endif
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
sp_shape_style_modified (SPObject *object, guint flags)
{
	SPShape *shape;
	SPItemView *v;

	shape = SP_SHAPE (object);

	/* Item class reads style */
	if (((SPObjectClass *) (parent_class))->style_modified)
		(* ((SPObjectClass *) (parent_class))->style_modified) (object, flags);

	for (v = SP_ITEM (shape)->display; v != NULL; v = v->next) {
		/* fixme: */
		nr_arena_shape_group_set_style (NR_ARENA_SHAPE_GROUP (v->arenaitem), object->style);
	}
}

void
sp_shape_print (SPItem *item, GnomePrintContext *gpc)
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
					sp_color_get_rgb_floatv (&object->style->fill.value.color, rgb);
					/* fixme: */
					opacity = SP_SCALE24_TO_FLOAT (object->style->fill_opacity.value)
						* SP_SCALE24_TO_FLOAT (object->style->opacity.value);
					gnome_print_gsave (gpc);
					gnome_print_setrgbcolor (gpc, rgb[0], rgb[1], rgb[2]);
					gnome_print_setopacity (gpc, opacity);
					if (object->style->fill_rule.value == ART_WIND_RULE_ODDEVEN) {
						gnome_print_eofill (gpc);
					} else {
						gnome_print_fill (gpc);
					}
					gnome_print_grestore (gpc);
				} else if (object->style->fill.type == SP_PAINT_TYPE_PAINTSERVER) {
					SPPainter *painter;
					ArtDRect bbox;
					gdouble id[6] = {1,0,0,1,0,0};
					sp_item_bbox_desktop (item, &bbox);
					/* fixme: */
					painter = sp_paint_server_painter_new (SP_OBJECT_STYLE_FILL_SERVER (object), id,
									       SP_SCALE24_TO_FLOAT (object->style->opacity.value), &bbox);
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
						if (object->style->fill_rule.value == ART_WIND_RULE_ODDEVEN) {
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
								painter->fill (painter, rgba, x, ibox.y1 + ibox.y0 - y - 64, 64, 64, 4 * 64);
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
				sp_color_get_rgb_floatv (&object->style->stroke.value.color, rgb);
				/* fixme: */
				opacity = SP_SCALE24_TO_FLOAT (object->style->stroke_opacity.value)
					* SP_SCALE24_TO_FLOAT (object->style->opacity.value);
				gnome_print_gsave (gpc);
				gnome_print_setrgbcolor (gpc, rgb[0], rgb[1], rgb[2]);
				gnome_print_setopacity (gpc, opacity);
				gnome_print_setlinewidth (gpc, object->style->stroke_width.computed);
				gnome_print_setlinejoin (gpc, object->style->stroke_linejoin.value);
				gnome_print_setlinecap (gpc, object->style->stroke_linecap.value);
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

	sp_fill_style_dialog ();
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

/* Shape section */
void
sp_shape_set_shape (SPShape *shape)
{
	g_return_if_fail (shape != NULL);
	g_return_if_fail (SP_IS_SHAPE (shape));

	if (SP_SHAPE_CLASS (GTK_OBJECT(shape)->klass)->set_shape)
		SP_SHAPE_CLASS (GTK_OBJECT(shape)->klass)->set_shape (shape);
}

#define SP_ITEM_C

#include <config.h>
#include <math.h>
#include <string.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "svg/svg.h"
#include "helper/sp-canvas-util.h"
#include "document.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "selection.h"
#include "style.h"
/* fixme: I do not like that (Lauris) */
#include "dialogs/item-properties.h"
#include "sp-item.h"

static void sp_item_class_init (SPItemClass * klass);
static void sp_item_init (SPItem * item);
static void sp_item_destroy (GtkObject * object);

static void sp_item_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_item_read_attr (SPObject * object, const gchar * key);

static gchar * sp_item_private_description (SPItem * item);
static GSList * sp_item_private_snappoints (SPItem * item, GSList * points);

static GnomeCanvasItem * sp_item_private_show (SPItem * item, SPDesktop * desktop, GnomeCanvasGroup * canvas_group);
static void sp_item_private_hide (SPItem * item, SPDesktop * desktop);

static void sp_item_private_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);
static void sp_item_properties (GtkMenuItem *menuitem, SPItem *item);
static void sp_item_select_this (GtkMenuItem *menuitem, SPItem *item);
static void sp_item_reset_transformation (GtkMenuItem *menuitem, SPItem *item);
static void sp_item_toggle_sensitivity (GtkMenuItem *menuitem, SPItem *item);

static SPObjectClass * parent_class;

GtkType
sp_item_get_type (void)
{
	static GtkType item_type = 0;
	if (!item_type) {
		GtkTypeInfo item_info = {
			"SPItem",
			sizeof (SPItem),
			sizeof (SPItemClass),
			(GtkClassInitFunc) sp_item_class_init,
			(GtkObjectInitFunc) sp_item_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		item_type = gtk_type_unique (sp_object_get_type (), &item_info);
	}
	return item_type;
}

static void
sp_item_class_init (SPItemClass * klass)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	parent_class = gtk_type_class (sp_object_get_type ());

	gtk_object_class->destroy = sp_item_destroy;

	sp_object_class->build = sp_item_build;
	sp_object_class->read_attr = sp_item_read_attr;

	klass->description = sp_item_private_description;
	klass->show = sp_item_private_show;
	klass->hide = sp_item_private_hide;
	klass->menu = sp_item_private_menu;
	klass->snappoints = sp_item_private_snappoints;
}

static void
sp_item_init (SPItem * item)
{
	SPObject *object;

	object = SP_OBJECT (item);

	art_affine_identity (item->affine);
	item->display = NULL;

	if (!object->style) object->style = sp_style_new ();
}

static void
sp_item_destroy (GtkObject * object)
{
	SPItem * item;

	item = (SPItem *) object;

	while (item->display) {
		gtk_object_destroy (GTK_OBJECT (item->display->canvasitem));
		item->display = sp_item_view_list_remove (item->display, item->display);
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_item_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	if (((SPObjectClass *) (parent_class))->build)
		(* ((SPObjectClass *) (parent_class))->build) (object, document, repr);

	sp_item_read_attr (object, "transform");
	sp_item_read_attr (object, "style");
}

static void
sp_item_read_attr (SPObject * object, const gchar * key)
{
	SPItem *item;
	const gchar *astr;
	SPStyle *style;

	item = SP_ITEM (object);

	astr = sp_repr_attr (object->repr, key);

	if (strcmp (key, "transform") == 0) {
		gdouble a[6];
		art_affine_identity (a);
		if (astr != NULL) sp_svg_read_affine (a, astr);
		sp_item_set_item_transform (item, a);
		/* fixme: in update */
		object->style->real_stroke_width_set = FALSE;
	}

	if (((SPObjectClass *) (parent_class))->read_attr)
		(* ((SPObjectClass *) (parent_class))->read_attr) (object, key);

	style = object->style;

	if (strcmp (key, "style") == 0) {
		sp_style_read_from_object (style, object);
		sp_object_request_modified (SP_OBJECT (item), SP_OBJECT_MODIFIED_FLAG);
	}

	if (!style->real_opacity_set) {
		SPObject *parent;
		style->real_opacity = style->opacity;
		parent = object->parent;
		while (parent && !parent->style) parent = parent->parent;
		/* fixme: Currently it does not work, if parent has not set real_opacity (but it shouldn't happen) */
		if (parent) style->real_opacity = style->real_opacity * parent->style->real_opacity;
		style->real_opacity_set = TRUE;
	}

	if (!style->real_stroke_width_set) {
		gdouble i2doc[6], dx, dy;
		gdouble a2u, u2a;
		sp_item_i2doc_affine (item, i2doc);
		dx = i2doc[0] + i2doc[2];
		dy = i2doc[1] + i2doc[3];
		u2a = sqrt (dx * dx + dy * dy) * 0.707106781;
		a2u = u2a > 1e-9 ? 1 / u2a : 1e9;
		switch (style->stroke_width.unit) {
		case SP_UNIT_PIXELS:
			style->absolute_stroke_width = style->stroke_width.distance * 1.25;
			style->user_stroke_width = style->absolute_stroke_width * a2u;
			break;
		case SP_UNIT_ABSOLUTE:
			style->absolute_stroke_width = style->stroke_width.distance;
			style->user_stroke_width = style->absolute_stroke_width * a2u;
			break;
		case SP_UNIT_USER:
			style->user_stroke_width = style->stroke_width.distance;
			style->absolute_stroke_width = style->user_stroke_width * u2a;
			break;
		case SP_UNIT_PERCENT:
			dx = sp_document_width (object->document);
			dy = sp_document_height (object->document);
			style->absolute_stroke_width = sqrt (dx * dx + dy * dy) * 0.707106781;
			style->user_stroke_width = style->absolute_stroke_width * a2u;
			break;
		case SP_UNIT_EM:
				/* fixme: */
			style->user_stroke_width = style->stroke_width.distance * 12.0;
			style->absolute_stroke_width = style->user_stroke_width * u2a;
			break;
		case SP_UNIT_EX:
				/* fixme: */
			style->user_stroke_width = style->stroke_width.distance * 10.0;
			style->absolute_stroke_width = style->user_stroke_width * u2a;
			break;
		}
		style->real_stroke_width_set = TRUE;
	}
}

/* Update indicates that affine is changed */

void
sp_item_update (SPItem * item, gdouble affine[])
{
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (affine != NULL);

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->update)
		(* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->update) (item, affine);
}

void sp_item_bbox (SPItem * item, ArtDRect * bbox)
{
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (bbox != NULL);

	bbox->x0 = bbox->y0 = 1.0;
	bbox->x1 = bbox->y1 = 0.0;

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->bbox)
		(* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->bbox) (item, bbox);
}

static GSList * 
sp_item_private_snappoints (SPItem * item, GSList * points) 
{
        ArtDRect bbox;
	ArtPoint * p;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));

	sp_item_bbox (item, &bbox);

	p = g_new (ArtPoint,1);
	p->x = bbox.x0;
	p->y = bbox.y0;
	points = g_slist_append (points, p);
	p = g_new (ArtPoint,1);
	p->x = bbox.x1;
	p->y = bbox.y0;
	points = g_slist_append (points, p);
	p = g_new (ArtPoint,1);
	p->x = bbox.x1;
	p->y = bbox.y1;
	points = g_slist_append (points, p);
	p = g_new (ArtPoint,1);
	p->x = bbox.x0;
	p->y = bbox.y1;
	points = g_slist_append (points, p);

	return points;
}

GSList * sp_item_snappoints (SPItem * item)
{
        GSList * points = NULL;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->snappoints)
	        points = (* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->snappoints) (item, points);
	return points;
}

void
sp_item_print (SPItem * item, GnomePrintContext * gpc)
{
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (gpc != NULL);

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->print)
		(* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->print) (item, gpc);
}

static gchar *
sp_item_private_description (SPItem * item)
{
	return g_strdup (_("Unknown item :-("));
}

gchar *
sp_item_description (SPItem * item)
{
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->description)
		return (* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->description) (item);

	g_assert_not_reached ();
	return NULL;
}

static GnomeCanvasItem *
sp_item_private_show (SPItem * item, SPDesktop * desktop, GnomeCanvasGroup * canvas_group)
{
	return NULL;
}

GnomeCanvasItem *
sp_item_show (SPItem * item, SPDesktop * desktop, GnomeCanvasGroup * canvas_group)
{
	SPObject *object;
	GnomeCanvasItem * canvasitem;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (canvas_group != NULL);
	g_assert (GNOME_IS_CANVAS_GROUP (canvas_group));

	object = SP_OBJECT (item);
	canvasitem = NULL;

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->show) {
		canvasitem =  (* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->show) (item, desktop, canvas_group);
	} else {
		canvasitem = NULL;
	}

	if (canvasitem != NULL) {
		item->display = sp_item_view_new_prepend (item->display, item, desktop, canvasitem);
		gnome_canvas_item_affine_absolute (canvasitem, item->affine);
		sp_desktop_connect_item (desktop, item, canvasitem);
	}

	return canvasitem;
}

static void
sp_item_private_hide (SPItem * item, SPDesktop * desktop)
{
	SPItemView * v;

	for (v = item->display; v != NULL; v = v->next) {
		if (v->desktop == desktop) {
			gtk_object_destroy (GTK_OBJECT (v->canvasitem));
			item->display = sp_item_view_list_remove (item->display, v);
			return;
		}
	}

	g_assert_not_reached ();
}

void
sp_item_hide (SPItem * item, SPDesktop * desktop)
{
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->hide)
		(* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->hide) (item, desktop);
}

gboolean
sp_item_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[])
{
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (buf != NULL);
	g_assert (affine != NULL);

	if (SP_ITEM_STOP_PAINT (item)) return TRUE;

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->paint)
		return (* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->paint) (item, buf, affine);

	return FALSE;
}

GnomeCanvasItem *
sp_item_canvas_item (SPItem * item, SPDesktop * desktop)
{
	SPItemView * v;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));

	for (v = item->display; v != NULL; v = v->next) {
		if (v->desktop == desktop) return v->canvasitem;
	}

	return NULL;
}

void
sp_item_request_canvas_update (SPItem * item)
{
	SPItemView * v;

	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));

	for (v = item->display; v != NULL; v = v->next) {
		gnome_canvas_item_request_update (v->canvasitem);
	}
}

/* Sets item private transform (not propagated to repr) */

void
sp_item_set_item_transform (SPItem *item, const gdouble *transform)
{
	SPItemView *v;
	gint i;

	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (transform != NULL);

	for (i = 0; i < 6; i++) {
		if (fabs (transform[i] - item->affine[i]) > 1e-9) break;
	}
	if (i >= 6) return;

	memcpy (item->affine, transform, 6 * sizeof (gdouble));

	for (v = item->display; v != NULL; v = v->next) {
		gnome_canvas_item_affine_absolute (v->canvasitem, transform);
	}

	sp_object_request_modified (SP_OBJECT (item), SP_OBJECT_MODIFIED_FLAG);
}

gdouble *
sp_item_i2d_affine (SPItem * item, gdouble affine[])
{
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (affine != NULL, NULL);

	art_affine_identity (affine);

	while (item) {
		art_affine_multiply (affine, affine, item->affine);
		item = (SPItem *) SP_OBJECT (item)->parent;
	}

	return affine;
}

void
sp_item_set_i2d_affine (SPItem * item, gdouble affine[])
{
	gdouble p2d[6], d2p[6];

	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (affine != NULL);

	if (SP_OBJECT (item)->parent != NULL) {
		sp_item_i2d_affine (SP_ITEM (SP_OBJECT (item)->parent), p2d);
		art_affine_invert (d2p, p2d);
		art_affine_multiply (affine, affine, d2p);
	}

	sp_item_set_item_transform (item, affine);
}

gdouble *
sp_item_i2doc_affine (SPItem * item, gdouble affine[])
{
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (affine != NULL, NULL);

	art_affine_identity (affine);

	while (SP_OBJECT (item)->parent) {
		art_affine_multiply (affine, affine, item->affine);
		item = (SPItem *) SP_OBJECT (item)->parent;
	}

	return affine;
}

void
sp_item_change_canvasitem_position (SPItem * item, gint delta)
{
	SPItemView * v;

	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));

	if (delta == 0) return;

	for (v = item->display; v != NULL; v = v->next) {
		if (delta > 0) {
			gnome_canvas_item_raise (v->canvasitem, delta);
		} else {
			gnome_canvas_item_lower (v->canvasitem, -delta);
		}
	}
}

void
sp_item_raise_canvasitem_to_top (SPItem * item)
{
	SPItemView * v;

	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));

	for (v = item->display; v != NULL; v = v->next) {
		gnome_canvas_item_raise_to_top (v->canvasitem);
	}
}

/* Generate context menu item section */

static void
sp_item_private_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu)
{
	GtkWidget * i, * m, * w;
	gboolean insensitive;

	/* Create toplevel menuitem */
	i = gtk_menu_item_new_with_label (_("Item"));
	m = gtk_menu_new ();
	/* Item dialog */
	w = gtk_menu_item_new_with_label (_("Item Properties"));
	gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_item_properties), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Separator */
	w = gtk_menu_item_new ();
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Select item */
	w = gtk_menu_item_new_with_label (_("Select this"));
	if (sp_selection_item_selected (SP_DT_SELECTION (desktop), item)) {
		gtk_widget_set_sensitive (w, FALSE);
	} else {
		gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);
		gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_item_select_this), item);
	}
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Reset transformations */
	w = gtk_menu_item_new_with_label (_("Reset transformation"));
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_item_reset_transformation), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Toggle sensitivity */
	insensitive = sp_repr_get_int_attribute (((SPObject *) item)->repr, "insensitive", FALSE);
	w = gtk_menu_item_new_with_label (insensitive ? _("Make sensitive") : _("Make insensitive"));
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_item_toggle_sensitivity), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Show menu */
	gtk_widget_show (m);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), m);

	gtk_menu_append (menu, i);
	gtk_widget_show (i);
}

void
sp_item_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu)
{
	g_assert (SP_IS_ITEM (item));
	g_assert (GTK_IS_MENU (menu));

	if (SP_ITEM_CLASS (((GtkObject *) (item))->klass)->menu)
		(* SP_ITEM_CLASS (((GtkObject *) (item))->klass)->menu) (item, desktop, menu);
}

static void
sp_item_properties (GtkMenuItem *menuitem, SPItem *item)
{
	SPDesktop *desktop;

	g_assert (SP_IS_ITEM (item));

	desktop = gtk_object_get_data (GTK_OBJECT (menuitem), "desktop");
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	sp_selection_set_item (SP_DT_SELECTION (desktop), item);

	sp_item_dialog (item);
}

static void
sp_item_select_this (GtkMenuItem *menuitem, SPItem *item)
{
	SPDesktop *desktop;

	g_assert (SP_IS_ITEM (item));

	desktop = gtk_object_get_data (GTK_OBJECT (menuitem), "desktop");
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	sp_selection_set_item (SP_DT_SELECTION (desktop), item);
}

static void
sp_item_reset_transformation (GtkMenuItem * menuitem, SPItem * item)
{
	g_assert (SP_IS_ITEM (item));

	sp_repr_set_attr (((SPObject *) item)->repr, "transform", NULL);
	sp_document_done (SP_OBJECT_DOCUMENT (item));
}

static void
sp_item_toggle_sensitivity (GtkMenuItem * menuitem, SPItem * item)
{
	const gchar * val;

	g_assert (SP_IS_ITEM (item));

	/* fixme: reprs suck */
	val = sp_repr_attr (SP_OBJECT_REPR (item), "insensitive");
	if (val != NULL) {
		val = NULL;
	} else {
		val = "1";
	}
	sp_repr_set_attr (SP_OBJECT_REPR (item), "insensitive", val);
	sp_document_done (SP_OBJECT_DOCUMENT (item));
}

/* Item views */

SPItemView *
sp_item_view_new_prepend (SPItemView * list, SPItem * item, SPDesktop * desktop, GnomeCanvasItem * canvasitem)
{
	SPItemView * new;

	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (canvasitem != NULL);
	g_assert (GNOME_IS_CANVAS_ITEM (canvasitem));

	new = g_new (SPItemView, 1);

	new->next = list;
	new->prev = NULL;
	if (list) list->prev = new;
	new->item = item;
	new->desktop = desktop;
	new->canvasitem = canvasitem;

	return new;
}

SPItemView *
sp_item_view_list_remove (SPItemView * list, SPItemView * view)
{
	SPItemView * v;

	g_assert (list != NULL);
	g_assert (view != NULL);

	for (v = list; v != NULL; v = v->next) {
		if (v == view) {
			if (v->next) v->next->prev = v->prev;
			if (v->prev) {
				v->prev->next = v->next;
			} else {
				list = v->next;
			}
			g_free (v);
			return list;
		}
	}

	g_assert_not_reached ();
	return NULL;
}


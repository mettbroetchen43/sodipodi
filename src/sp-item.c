#define SP_ITEM_GROUP_C

#include <gnome.h>
#include "svg/svg.h"
#include "helper/sp-canvas-util.h"
#include "sp-item.h"

static void sp_item_class_init (SPItemClass * klass);
static void sp_item_init (SPItem * item);
static void sp_item_destroy (GtkObject * object);

static void sp_item_private_update (SPItem * item, gdouble affine[]);
static void sp_item_private_bbox (SPItem * item, ArtDRect * bbox);
static void sp_item_private_print (SPItem * item, GnomePrintContext * gpc);
static gchar * sp_item_private_description (SPItem * item);
static void sp_item_private_read (SPItem * item, SPRepr * repr);
static void sp_item_private_read_attr (SPItem * item, SPRepr * repr, const gchar * key);
static GnomeCanvasItem * sp_item_private_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler);
static void sp_item_private_hide (SPItem * item, GnomeCanvas * canvas);
static void sp_item_private_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[]);

static GtkObjectClass * parent_class;

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
		item_type = gtk_type_unique (gtk_object_get_type (), &item_info);
	}
	return item_type;
}

static void
sp_item_class_init (SPItemClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (gtk_object_get_type ());

	object_class->destroy = sp_item_destroy;

	klass->update = sp_item_private_update;
	klass->bbox = sp_item_private_bbox;
	klass->print = sp_item_private_print;
	klass->description = sp_item_private_description;
	klass->read = sp_item_private_read;
	klass->read_attr = sp_item_private_read_attr;
	klass->show = sp_item_private_show;
	klass->hide = sp_item_private_hide;
	klass->paint = sp_item_private_paint;
}

static void
sp_item_init (SPItem * item)
{
	item->parent = NULL;
	item->repr = NULL;
	art_affine_identity (item->affine);
	item->display = NULL;
}

static void
sp_item_destroy (GtkObject * object)
{
	SPItem * item;

	item = (SPItem *) object;

	while (item->display) {
		gtk_object_destroy ((GtkObject *) item->display->data);
		item->display = g_slist_remove (item->display, item->display->data);
	}

	item->parent->children = g_slist_remove (item->parent->children, item);

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

/* Update indicates that affine is changed */

static void
sp_item_private_update (SPItem * item, gdouble affine[])
{
}

void
sp_item_update (SPItem * item, gdouble affine[])
{
	(* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->update) (item, affine);
}

static void
sp_item_private_bbox (SPItem * item, ArtDRect *bbox)
{
}

void sp_item_bbox (SPItem * item, ArtDRect *bbox)
{
	(* SP_ITEM_CLASS (item->object.klass)->bbox) (item, bbox);

	bbox->x0 -= 0.01;
	bbox->y0 -= 0.01;
	bbox->x1 += 0.01;
	bbox->y1 += 0.01;
}

static void
sp_item_private_print (SPItem * item, GnomePrintContext * gpc)
{
}

void sp_item_print (SPItem * item, GnomePrintContext * gpc)
{
	(* SP_ITEM_CLASS (item->object.klass)->print) (item, gpc);
}

static gchar *
sp_item_private_description (SPItem * item)
{
	/* Bad, bad... our item does not know, who he is */
	return g_strdup (_("Unknown item :-("));
}

gchar * sp_item_description (SPItem * item)
{
	return (* SP_ITEM_CLASS (item->object.klass)->description) (item);
}

static void
sp_item_private_read (SPItem * item, SPRepr * repr)
{
	sp_item_private_read_attr (item, repr, "transform");
}

void sp_item_read (SPItem * item, SPRepr * repr)
{
	(* SP_ITEM_CLASS (item->object.klass)->read) (item, repr);
}

static void
sp_item_private_read_attr (SPItem * item, SPRepr * repr, const gchar * attr)
{
	GnomeCanvasItem * ci;
	SPItem * i;
	GSList * l;
	gdouble a[6];
	const gchar * astr;

	g_assert (item != NULL);
	g_assert (repr != NULL);
	g_assert (attr != NULL);

	astr = sp_repr_attr (repr, attr);

	if (strcmp (attr, "transform") == 0) {
		art_affine_identity (item->affine);
		if (astr != NULL) {
			sp_svg_read_affine (item->affine, astr);
		}
		for (l = item->display; l != NULL; l = l->next) {
			ci = (GnomeCanvasItem *) l->data;
			gnome_canvas_item_affine_absolute (ci, item->affine);
		}
		art_affine_identity (a);
		for (i = item; i != NULL; i = (SPItem *) i->parent)
			art_affine_multiply (a, a, i->affine);
		sp_item_update (item, a);
	}
}

void sp_item_read_attr (SPItem * item, SPRepr * repr, const gchar * attr)
{
	(* SP_ITEM_CLASS (GTK_OBJECT (item)->klass)->read_attr) (item, repr, attr);
}

static GnomeCanvasItem *
sp_item_private_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler)
{
	return NULL;
}

GnomeCanvasItem *
sp_item_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler)
{
	GnomeCanvasItem * ci;
	gint pos;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (canvas_group != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (canvas_group), NULL);

	ci = (* SP_ITEM_CLASS (item->object.klass)->show) (item, canvas_group, handler);

	if (ci != NULL) {
		if (item->parent != NULL) {
			pos = g_slist_index (item->parent->children, item);
			gnome_canvas_item_move_to_z (ci, pos);
		}
		gnome_canvas_item_affine_absolute (ci, item->affine);
		if (handler != NULL)
			gtk_signal_connect (GTK_OBJECT (ci), "event",
				GTK_SIGNAL_FUNC (handler), item);
		item->display = g_slist_prepend (item->display, ci);
	}

	return ci;
}

static void
sp_item_private_hide (SPItem * item, GnomeCanvas * canvas)
{
	GnomeCanvasItem * ci;
	GSList * l;

	for (l = item->display; l != NULL; l = l->next) {
		ci = GNOME_CANVAS_ITEM (l->data);
		if (ci->canvas == canvas) {
			gtk_object_destroy ((GtkObject *) ci);
			item->display = g_slist_remove (item->display, ci);
			return;
		}
	}
	g_assert_not_reached ();
}

void sp_item_hide (SPItem * item, GnomeCanvas * canvas)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	(* SP_ITEM_CLASS (item->object.klass)->hide) (item, canvas);
}

static void
sp_item_private_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[])
{
}

void sp_item_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[])
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (buf != NULL);
	g_return_if_fail (affine != NULL);

	(* SP_ITEM_CLASS (item->object.klass)->paint) (item, buf, affine);
}

GnomeCanvasItem *
sp_item_canvas_item (SPItem * item, GnomeCanvas * canvas)
{
	GnomeCanvasItem * ci;
	GSList * l;

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (canvas != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS (canvas), NULL);

	for (l = item->display; l != NULL; l = l->next) {
		ci = (GnomeCanvasItem *) l->data;
		if (ci->canvas == canvas) return ci;
	}

	return NULL;
}

void
sp_item_request_canvas_update (SPItem * item)
{
	GnomeCanvasItem * ci;
	GSList * l;

	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));

	for (l = item->display; l != NULL; l = l->next) {
		ci = GNOME_CANVAS_ITEM (l->data);
		gnome_canvas_item_request_update (ci);
	}
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
		item = (SPItem *) item->parent;
	}

	return affine;
}

void
sp_item_set_i2d_affine (SPItem * item, gdouble affine[])
{
	GnomeCanvasItem * ci;
	GSList * l;
	gdouble p2d[6], d2p[6];
	gint i;

	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (affine != NULL);

	if (item->parent != NULL) {
		sp_item_i2d_affine (SP_ITEM (item->parent), p2d);
		art_affine_invert (d2p, p2d);
		art_affine_multiply (affine, affine, d2p);
	}

	for (i = 0; i < 6; i++) item->affine[i] = affine[i];

#if 0
	/* fixme: do the updating right way */
	sp_item_update (item, affine);
#else
	for (l = item->display; l != NULL; l = l->next) {
		ci = (GnomeCanvasItem *) l->data;
		gnome_canvas_item_affine_absolute (ci, affine);
	}
#endif
}

gdouble *
sp_item_i2doc_affine (SPItem * item, gdouble affine[])
{
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (affine != NULL, NULL);

	art_affine_identity (affine);

	while (item->parent) {
		art_affine_multiply (affine, affine, item->affine);
		item = (SPItem *) item->parent;
	}

	return affine;
}


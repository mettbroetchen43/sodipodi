#define SP_ITEM_GROUP_C

#include <gnome.h>
#include "svg/svg.h"
#include "helper/sp-canvas-util.h"
#include "sp-item.h"

static void sp_item_class_init (SPItemClass * klass);
static void sp_item_init (SPItem * item);
static void sp_item_destroy (GtkObject * object);

static void sp_item_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_item_read_attr (SPObject * object, const gchar * key);

static gchar * sp_item_private_description (SPItem * item);

static GnomeCanvasItem * sp_item_private_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler);
static void sp_item_private_hide (SPItem * item, GnomeCanvas * canvas);

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
}

static void
sp_item_init (SPItem * item)
{
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

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_item_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	if (((SPObjectClass *) (parent_class))->build)
		(* ((SPObjectClass *) (parent_class))->build) (object, document, repr);

	sp_item_read_attr (object, "transform");
}

static void
sp_item_read_attr (SPObject * object, const gchar * key)
{
	SPItem * item;
	GnomeCanvasItem * ci;
	SPItem * i;
	GSList * l;
	gdouble a[6];
	const gchar * astr;

	item = SP_ITEM (object);

	astr = sp_repr_attr (object->repr, key);

	if (strcmp (key, "transform") == 0) {
		art_affine_identity (item->affine);
		if (astr != NULL) {
			sp_svg_read_affine (item->affine, astr);
		}
		for (l = item->display; l != NULL; l = l->next) {
			ci = (GnomeCanvasItem *) l->data;
			gnome_canvas_item_affine_absolute (ci, item->affine);
		}
		art_affine_identity (a);
		for (i = item; i != NULL; i = (SPItem *) ((SPObject *)i)->parent)
			art_affine_multiply (a, a, i->affine);
		sp_item_update (item, a);
		return;
	}

	if (((SPObjectClass *) (parent_class))->read_attr)
		(* ((SPObjectClass *) (parent_class))->read_attr) (object, key);
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

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->bbox)
		(* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->bbox) (item, bbox);

	bbox->x0 -= 0.01;
	bbox->y0 -= 0.01;
	bbox->x1 += 0.01;
	bbox->y1 += 0.01;
}

void sp_item_print (SPItem * item, GnomePrintContext * gpc)
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

gchar * sp_item_description (SPItem * item)
{
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->description)
		return (* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->description) (item);

	g_assert_not_reached ();
	return NULL;
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

	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM (item), NULL);
	g_return_val_if_fail (canvas_group != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (canvas_group), NULL);

	ci = NULL;

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->show)
		ci =  (* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->show) (item, canvas_group, handler);

	if (ci != NULL) {
#if 0
/* fixme: this goes to SPGroup */
		if (item->parent != NULL) {
			pos = g_slist_index (item->parent->children, item);
			gnome_canvas_item_move_to_z (ci, pos);
		}
#endif
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
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (canvas != NULL);
	g_assert (GNOME_IS_CANVAS (canvas));

	if (SP_ITEM_CLASS (((GtkObject *)(item))->klass)->hide)
		(* SP_ITEM_CLASS (((GtkObject *)(item))->klass)->hide) (item, canvas);
}

gboolean sp_item_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[])
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
		item = (SPItem *) SP_OBJECT (item)->parent;
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

	if (SP_OBJECT (item)->parent != NULL) {
		sp_item_i2d_affine (SP_ITEM (SP_OBJECT (item)->parent), p2d);
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

	while (SP_OBJECT (item)->parent) {
		art_affine_multiply (affine, affine, item->affine);
		item = (SPItem *) SP_OBJECT (item)->parent;
	}

	return affine;
}

void
sp_item_raise_canvasitem_to_top (SPItem * item)
{
	GSList * l;

	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));

	for (l = item->display; l != NULL; l = l->next) {
		gnome_canvas_item_raise_to_top (GNOME_CANVAS_ITEM (l->data));
	}
}

#define SP_OBJECT_GROUP_C

/*
 * SPObject
 *
 * This is most abstract of all typed objects
 *
 * Copyright (C) Lauris Kaplinski <lauris@ariman.ee> 1999-2000
 *
 */

#include "sp-object.h"

static void sp_object_class_init (SPObjectClass * klass);
static void sp_object_init (SPObject * object);
static void sp_object_destroy (GtkObject * object);

static void sp_object_build (SPObject * object, SPDocument * document, SPRepr * repr);

static void sp_object_read_attr (SPObject * object, const gchar * key);

/* Real handlers of repr signals */

static void sp_object_repr_destroy (SPRepr * repr, gpointer data);

static void sp_object_repr_change_attr (SPRepr * repr, const gchar * key, gpointer data);
static void sp_object_repr_change_content (SPRepr * repr, gpointer data);
static void sp_object_repr_change_order (SPRepr * repr, gpointer data);
static void sp_object_repr_add_child (SPRepr * repr, SPRepr * child, gpointer data);
static void sp_object_repr_remove_child (SPRepr * repr, SPRepr * child, gpointer data);

static gchar * sp_object_get_unique_id (SPObject * object, const gchar * defid);

static GtkObjectClass * parent_class;

GtkType
sp_object_get_type (void)
{
	static GtkType object_type = 0;
	if (!object_type) {
		GtkTypeInfo object_info = {
			"SPObject",
			sizeof (SPObject),
			sizeof (SPObjectClass),
			(GtkClassInitFunc) sp_object_class_init,
			(GtkObjectInitFunc) sp_object_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		object_type = gtk_type_unique (gtk_object_get_type (), &object_info);
	}
	return object_type;
}

static void
sp_object_class_init (SPObjectClass * klass)
{
	GtkObjectClass * gtk_object_class;

	gtk_object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (gtk_object_get_type ());

	gtk_object_class->destroy = sp_object_destroy;

	klass->build = sp_object_build;
	klass->read_attr = sp_object_read_attr;
}

static void
sp_object_init (SPObject * object)
{
	object->document = NULL;
	object->parent = NULL;
	object->order = 0;
	object->repr = NULL;
	object->id = NULL;
}

static void
sp_object_destroy (GtkObject * object)
{
	SPObject * spobject;

	spobject = (SPObject *) object;

	g_assert (spobject->parent == NULL);

	if (spobject->id) {
		if (spobject->document)
			sp_document_undef_id (spobject->document, spobject->id);
		g_free (spobject->id);
	}

	if (spobject->repr) {
		sp_repr_remove_signals (spobject->repr);
		sp_repr_unref (spobject->repr);
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_object_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	const gchar * id;
	gchar * realid;

	object->document = document;
	object->repr = repr;

	id = sp_repr_attr (repr, "id");
	realid = sp_object_get_unique_id (object, id);

	sp_document_def_id (document, realid, object);
	object->id = realid;
	sp_repr_set_attr (repr, "id", realid);
}

void
sp_object_invoke_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	g_assert (object != NULL);
	g_assert (SP_IS_OBJECT (object));
	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));
	g_assert (repr != NULL);

	g_assert (object->document == NULL);
	g_assert (object->repr == NULL);
	g_assert (object->id == NULL);

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->build)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->build) (object, document, repr);

	sp_repr_set_signal (repr, "destroy", sp_object_repr_destroy, object);
	sp_repr_set_signal (repr, "attr_changed", sp_object_repr_change_attr, object);
	sp_repr_set_signal (repr, "content_changed", sp_object_repr_change_content, object);
	sp_repr_set_signal (repr, "order_changed", sp_object_repr_change_order, object);
	sp_repr_set_signal (repr, "child_added", sp_object_repr_add_child, object);
	sp_repr_set_signal (repr, "child_removed", sp_object_repr_remove_child, object);

	sp_repr_ref (repr);
}

static void
sp_object_read_attr (SPObject * object, const gchar * key)
{
	const gchar * reprid;
	gchar * newid;

	g_assert (SP_IS_DOCUMENT (object->document));
	g_assert (object->id != NULL);
	g_assert (key != NULL);

	if (strcmp (key, "id") == 0) {
		reprid = sp_repr_attr (object->repr, "id");
		g_assert (reprid != NULL);
		if (strcmp (reprid, object->id) == 0) {
			return;
		}
		newid = sp_object_get_unique_id (object, reprid);
		g_assert (newid != NULL);
		if (strcmp (reprid, newid) != 0) {
			sp_repr_set_attr (object->repr, "id", newid);
			g_free (newid);
			return;
		}
		sp_document_undef_id (object->document, object->id);
		g_free (object->id);
		sp_document_def_id (object->document, newid, object);
		object->id = newid;
		return;
	}
}

void
sp_object_invoke_read_attr (SPObject * object, const gchar * key)
{
	g_assert (object != NULL);
	g_assert (SP_IS_OBJECT (object));
	g_assert (key != NULL);

	g_assert (SP_IS_DOCUMENT (object->document));
	g_assert (object->repr != NULL);
	g_assert (object->id != NULL);

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->read_attr)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->read_attr) (object, key);
}

static gchar *
sp_object_get_unique_id (SPObject * object, const gchar * id)
{
	static gint count = 0;
	const gchar * name;
	gchar * realid;
	gchar * b;
	gint len;

	g_assert (SP_IS_OBJECT (object));
	g_assert (SP_IS_DOCUMENT (object->document));

	count++;

	realid = NULL;

	if (id != NULL) {
		if (sp_document_lookup_id (object->document, id) == NULL) {
			realid = g_strdup (id);
			g_assert (realid != NULL);
		}
	}

	if (realid == NULL) {
		name = sp_repr_name (object->repr);
		g_assert (name != NULL);
		len = strlen (name) + 17;
		b = alloca (len);
		g_assert (b != NULL);
	}

	while (realid == NULL) {
		g_snprintf (b, len, "%s%d", name, count);
		if (sp_document_lookup_id (object->document, b) == NULL) {
			realid = g_strdup (b);
			g_assert (realid != NULL);
		}
	}

	return realid;
}

/*
 * Repr cannot be destroyed while "destroy" connected, because we ref it
 */

static void
sp_object_repr_destroy (SPRepr * repr, gpointer data)
{
	g_assert_not_reached ();
}

static void
sp_object_repr_change_attr (SPRepr * repr, const gchar * key, gpointer data)
{
	SPObject * object;

	g_assert (repr != NULL);
	g_assert (key != NULL);
	g_assert (SP_IS_OBJECT (data));

	object = SP_OBJECT (data);

	g_assert (object->repr = repr);

	sp_object_invoke_read_attr (object, key);
}

static void
sp_object_repr_change_content (SPRepr * repr, gpointer data)
{
	SPObject * object;

	g_assert (repr != NULL);
	g_assert (SP_IS_OBJECT (data));

	object = SP_OBJECT (data);

	g_assert (object->repr = repr);

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->read_content)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->read_content) (object);
}

static void
sp_object_repr_change_order (SPRepr * repr, gpointer data)
{
	SPObject * object;

	g_assert (repr != NULL);
	g_assert (SP_IS_OBJECT (data));

	object = SP_OBJECT (data);

	g_assert (object->repr = repr);

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->set_order)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->set_order) (object);
}

static void
sp_object_repr_add_child (SPRepr * repr, SPRepr * child, gpointer data)
{
	SPObject * object;

	g_assert (repr != NULL);
	g_assert (child != NULL);
	g_assert (SP_IS_OBJECT (data));

	object = SP_OBJECT (data);

	g_assert (object->repr = repr);
	g_assert (sp_repr_parent (child) == repr);

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->add_child)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->add_child) (object, child);
}

static void
sp_object_repr_remove_child (SPRepr * repr, SPRepr * child, gpointer data)
{
	SPObject * object;

	g_assert (repr != NULL);
	g_assert (child != NULL);
	g_assert (SP_IS_OBJECT (data));

	object = SP_OBJECT (data);

	g_assert (object->repr = repr);

	if (((SPObjectClass *)(((GtkObject *) object)->klass))->remove_child)
		(*((SPObjectClass *)(((GtkObject *) object)->klass))->remove_child) (object, child);
}


#if 0
void
sp_object_update (SPObject * object, gdouble affine[])
{
	(* SP_OBJECT_CLASS (((GtkObject *)(object))->klass)->update) (object, affine);
}

static void
sp_object_private_bbox (SPObject * object, ArtDRect *bbox)
{
}

void sp_object_bbox (SPObject * object, ArtDRect *bbox)
{
	(* SP_OBJECT_CLASS (object->object.klass)->bbox) (object, bbox);

	bbox->x0 -= 0.01;
	bbox->y0 -= 0.01;
	bbox->x1 += 0.01;
	bbox->y1 += 0.01;
}

static void
sp_object_private_print (SPObject * object, GnomePrintContext * gpc)
{
}

void sp_object_print (SPObject * object, GnomePrintContext * gpc)
{
	(* SP_OBJECT_CLASS (object->object.klass)->print) (object, gpc);
}

static gchar *
sp_object_private_description (SPObject * object)
{
	/* Bad, bad... our object does not know, who he is */
	return g_strdup (_("Unknown object :-("));
}

gchar * sp_object_description (SPObject * object)
{
	return (* SP_OBJECT_CLASS (object->object.klass)->description) (object);
}

static void
sp_object_private_read (SPObject * object, SPRepr * repr)
{
	sp_object_private_read_attr (object, repr, "transform");
}

void sp_object_read (SPObject * object, SPRepr * repr)
{
	(* SP_OBJECT_CLASS (object->object.klass)->read) (object, repr);
}

static void
sp_object_private_read_attr (SPObject * object, SPRepr * repr, const gchar * attr)
{
	GnomeCanvasObject * ci;
	SPObject * i;
	GSList * l;
	gdouble a[6];
	const gchar * astr;

	g_assert (object != NULL);
	g_assert (repr != NULL);
	g_assert (attr != NULL);

	astr = sp_repr_attr (repr, attr);

	if (strcmp (attr, "transform") == 0) {
		art_affine_identity (object->affine);
		if (astr != NULL) {
			sp_svg_read_affine (object->affine, astr);
		}
		for (l = object->display; l != NULL; l = l->next) {
			ci = (GnomeCanvasObject *) l->data;
			gnome_canvas_object_affine_absolute (ci, object->affine);
		}
		art_affine_identity (a);
		for (i = object; i != NULL; i = (SPObject *) i->parent)
			art_affine_multiply (a, a, i->affine);
		sp_object_update (object, a);
	}
}

void sp_object_read_attr (SPObject * object, SPRepr * repr, const gchar * attr)
{
	(* SP_OBJECT_CLASS (GTK_OBJECT (object)->klass)->read_attr) (object, repr, attr);
}

static GnomeCanvasObject *
sp_object_private_show (SPObject * object, GnomeCanvasGroup * canvas_group, gpointer handler)
{
	return NULL;
}

GnomeCanvasObject *
sp_object_show (SPObject * object, GnomeCanvasGroup * canvas_group, gpointer handler)
{
	GnomeCanvasObject * ci;
	gint pos;

	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);
	g_return_val_if_fail (canvas_group != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (canvas_group), NULL);

	ci = (* SP_OBJECT_CLASS (object->object.klass)->show) (object, canvas_group, handler);

	if (ci != NULL) {
		if (object->parent != NULL) {
			pos = g_slist_index (object->parent->children, object);
			gnome_canvas_object_move_to_z (ci, pos);
		}
		gnome_canvas_object_affine_absolute (ci, object->affine);
		if (handler != NULL)
			gtk_signal_connect (GTK_OBJECT (ci), "event",
				GTK_SIGNAL_FUNC (handler), object);
		object->display = g_slist_prepend (object->display, ci);
	}

	return ci;
}

static void
sp_object_private_hide (SPObject * object, GnomeCanvas * canvas)
{
	GnomeCanvasObject * ci;
	GSList * l;

	for (l = object->display; l != NULL; l = l->next) {
		ci = GNOME_CANVAS_OBJECT (l->data);
		if (ci->canvas == canvas) {
			gtk_object_destroy ((GtkObject *) ci);
			object->display = g_slist_remove (object->display, ci);
			return;
		}
	}
	g_assert_not_reached ();
}

void sp_object_hide (SPObject * object, GnomeCanvas * canvas)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	(* SP_OBJECT_CLASS (object->object.klass)->hide) (object, canvas);
}

static void
sp_object_private_paint (SPObject * object, ArtPixBuf * buf, gdouble affine[])
{
}

void sp_object_paint (SPObject * object, ArtPixBuf * buf, gdouble affine[])
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));
	g_return_if_fail (buf != NULL);
	g_return_if_fail (affine != NULL);

	(* SP_OBJECT_CLASS (object->object.klass)->paint) (object, buf, affine);
}

GnomeCanvasObject *
sp_object_canvas_object (SPObject * object, GnomeCanvas * canvas)
{
	GnomeCanvasObject * ci;
	GSList * l;

	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);
	g_return_val_if_fail (canvas != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS (canvas), NULL);

	for (l = object->display; l != NULL; l = l->next) {
		ci = (GnomeCanvasObject *) l->data;
		if (ci->canvas == canvas) return ci;
	}

	return NULL;
}

void
sp_object_request_canvas_update (SPObject * object)
{
	GnomeCanvasObject * ci;
	GSList * l;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));

	for (l = object->display; l != NULL; l = l->next) {
		ci = GNOME_CANVAS_OBJECT (l->data);
		gnome_canvas_object_request_update (ci);
	}
}

gdouble *
sp_object_i2d_affine (SPObject * object, gdouble affine[])
{
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);
	g_return_val_if_fail (affine != NULL, NULL);

	art_affine_identity (affine);

	while (object) {
		art_affine_multiply (affine, affine, object->affine);
		object = (SPObject *) object->parent;
	}

	return affine;
}

void
sp_object_set_i2d_affine (SPObject * object, gdouble affine[])
{
	GnomeCanvasObject * ci;
	GSList * l;
	gdouble p2d[6], d2p[6];
	gint i;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));
	g_return_if_fail (affine != NULL);

	if (object->parent != NULL) {
		sp_object_i2d_affine (SP_OBJECT (object->parent), p2d);
		art_affine_invert (d2p, p2d);
		art_affine_multiply (affine, affine, d2p);
	}

	for (i = 0; i < 6; i++) object->affine[i] = affine[i];

#if 0
	/* fixme: do the updating right way */
	sp_object_update (object, affine);
#else
	for (l = object->display; l != NULL; l = l->next) {
		ci = (GnomeCanvasObject *) l->data;
		gnome_canvas_object_affine_absolute (ci, affine);
	}
#endif
}

gdouble *
sp_object_i2doc_affine (SPObject * object, gdouble affine[])
{
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_OBJECT (object), NULL);
	g_return_val_if_fail (affine != NULL, NULL);

	art_affine_identity (affine);

	while (object->parent) {
		art_affine_multiply (affine, affine, object->affine);
		object = (SPObject *) object->parent;
	}

	return affine;
}
#endif


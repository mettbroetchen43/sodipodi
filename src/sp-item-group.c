#define SP_GROUP_C

#include <gnome.h>
#include "display/canvas-bgroup.h"
#include "sp-object-repr.h"
#include "sp-item.h"

/* fixme: */
#include "desktop-events.h"

static void sp_group_class_init (SPGroupClass *klass);
static void sp_group_init (SPGroup *group);
static void sp_group_destroy (GtkObject *object);

static void sp_group_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_group_add_child (SPObject * object, SPRepr * child);
static void sp_group_remove_child (SPObject * object, SPRepr * child);
static void sp_group_set_order (SPObject * object);

static void sp_group_update (SPItem *item, gdouble affine[]);
static void sp_group_bbox (SPItem * item, ArtDRect * bbox);
static void sp_group_print (SPItem * item, GnomePrintContext * gpc);
static gchar * sp_group_description (SPItem * item);
static GnomeCanvasItem * sp_group_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler);
static void sp_group_hide (SPItem * item, GnomeCanvas * canvas);
static gboolean sp_group_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[]);

static SPItemClass * parent_class;

GtkType
sp_group_get_type (void)
{
	static GtkType group_type = 0;
	if (!group_type) {
		GtkTypeInfo group_info = {
			"SPGroup",
			sizeof (SPGroup),
			sizeof (SPGroupClass),
			(GtkClassInitFunc) sp_group_class_init,
			(GtkObjectInitFunc) sp_group_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		group_type = gtk_type_unique (sp_item_get_type (), &group_info);
	}
	return group_type;
}

static void
sp_group_class_init (SPGroupClass *klass)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;

	parent_class = gtk_type_class (sp_item_get_type ());

	gtk_object_class->destroy = sp_group_destroy;

	sp_object_class->build = sp_group_build;
	sp_object_class->add_child = sp_group_add_child;
	sp_object_class->remove_child = sp_group_remove_child;
	sp_object_class->set_order = sp_group_set_order;

	item_class->update = sp_group_update;
	item_class->bbox = sp_group_bbox;
	item_class->print = sp_group_print;
	item_class->description = sp_group_description;
	item_class->show = sp_group_show;
	item_class->hide = sp_group_hide;
	item_class->paint = sp_group_paint;
}

static void
sp_group_init (SPGroup *group)
{
	group->children = NULL;
	group->transparent = FALSE;
}

static void
sp_group_destroy (GtkObject *object)
{
	SPGroup * group;
	SPObject * spobject;

	group = SP_GROUP (object);

	while (group->children) {
		spobject = SP_OBJECT (group->children->data);
		spobject->parent = NULL;
		gtk_object_destroy ((GtkObject *) spobject);
		group->children = g_slist_remove (group->children, spobject);
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void sp_group_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPGroup * group;
	const GList * l;
	gint order;
	SPRepr * crepr;
	const gchar * name;
	GtkType type;
	SPObject * child;

	group = SP_GROUP (object);

	if (SP_OBJECT_CLASS (parent_class)->build)
		(* SP_OBJECT_CLASS (parent_class)->build) (object, document, repr);

	l = sp_repr_children (repr);
	order = 0;

	while (l != NULL) {
		crepr = (SPRepr *) l->data;
		name = sp_repr_name (crepr);
		g_assert (name != NULL);
		type = sp_object_type_lookup (name);
		if (gtk_type_is_a (type, SP_TYPE_ITEM)) {
			child = gtk_type_new (type);
			g_assert (child != NULL);
			child->parent = object;
			child->order = order;
			group->children = g_slist_append (group->children, child);
			sp_object_invoke_build (child, document, crepr);
		}
		l = l->next;
		order++;
	}
}

static void
sp_group_add_child (SPObject * object, SPRepr * child)
{
	SPGroup * group;
	const gchar * name;
	GtkType type;
	SPObject * childobject;
	GSList * l;
	GnomeCanvasItem * ci;

	group = SP_GROUP (object);

	if (SP_OBJECT_CLASS (parent_class)->add_child)
		(* SP_OBJECT_CLASS (parent_class)->add_child) (object, child);

	name = sp_repr_name (child);
	g_assert (name != NULL);
	type = sp_object_type_lookup (name);
	if (gtk_type_is_a (type, SP_TYPE_ITEM)) {
		childobject = gtk_type_new (type);
		g_assert (childobject != NULL);
		childobject->parent = object;
		childobject->order = sp_repr_position (child);
		group->children = g_slist_append (group->children, childobject);
		sp_object_invoke_build (childobject, object->document, child);

		g_print ("sp-item-group.c: Please fix signals\n");

		for (l = SP_ITEM (object)->display; l != NULL; l = l->next) {
			ci = sp_item_show (SP_ITEM (childobject),
				GNOME_CANVAS_GROUP (l->data),
				sp_desktop_item_handler);
		}
		sp_group_set_order (object);
	}
}

static void
sp_group_remove_child (SPObject * object, SPRepr * child)
{
	SPGroup * group;
	GSList * l;
	SPObject * childobject;

	group = SP_GROUP (object);

	if (SP_OBJECT_CLASS (parent_class)->remove_child)
		(* SP_OBJECT_CLASS (parent_class)->remove_child) (object, child);

	for (l = group->children; l != NULL; l = l->next) {
		childobject = (SPObject *) l->data;
		if (childobject->repr == child) {
			group->children = g_slist_remove (group->children, childobject);
			childobject->parent = NULL;
			gtk_object_destroy (GTK_OBJECT (childobject));
#if 0
			sp_group_set_order (object);
#endif
			return;
		}
	}
}

static gint
sp_group_compare_children_pos (gconstpointer a, gconstpointer b)
{
	return sp_repr_compare_position (SP_OBJECT (a)->repr, SP_OBJECT (b)->repr);
}

/* fixme: all this is horribly ineffective */

static void
sp_group_set_order (SPObject * object)
{
	SPGroup * group;
	GSList * neworder;
	GSList * l;
	SPItem * child;
	gint delta;

	group = SP_GROUP (object);

	neworder = g_slist_copy (group->children);
	neworder = g_slist_sort (neworder, sp_group_compare_children_pos);

	for (l = neworder; l != NULL; l = l->next) {
		if (l->data == group->children->data) {
			group->children = g_slist_remove (group->children, group->children->data);
		} else {
			child = SP_ITEM (l->data);
			delta = g_slist_index (group->children, child);
			sp_item_change_canvasitem_position (child, -delta);
			group->children = g_slist_remove (group->children, child);
		}
	}

	group->children = neworder;
#if 0
	group->children = g_slist_sort (group->children, sp_group_compare_children_pos);

	for (l = group->children; l != NULL; l = l->next) {
		child = SP_ITEM (l->data);
		sp_item_raise_canvasitem_to_top (child);
	}
#endif
}

static void
sp_group_update (SPItem *item, gdouble affine[])
{
	SPGroup *group;
	SPItem * child;
	GSList * l;
	gdouble a[6];

	group = SP_GROUP (item);

	if (SP_ITEM_CLASS (parent_class)->update)
		(* SP_ITEM_CLASS (parent_class)->update) (item, affine);

	for (l = group->children; l != NULL; l = l->next) {
		child = (SPItem *) l->data;
		art_affine_multiply (a, child->affine, affine);
		sp_item_update (child, a);
	}
}

static void
sp_group_bbox (SPItem * item, ArtDRect *bbox)
{
	SPGroup * group;
	SPItem * child;
	ArtDRect child_bbox;
	GSList * l;

	group = SP_GROUP (item);

	bbox->x0 = bbox->y0 = bbox->x1 = bbox->y1 = 0.0;

	for (l = group->children; l != NULL; l = l->next) {
		child = (SPItem *) l->data;
		sp_item_bbox (child, &child_bbox);
		art_drect_union (bbox, bbox, &child_bbox);
	}
}

static void
sp_group_print (SPItem * item, GnomePrintContext * gpc)
{
	SPGroup * group;
	SPItem * child;
	GSList * l;

	group = SP_GROUP (item);

	for (l = group->children; l != NULL; l = l->next) {
		child = (SPItem *) l->data;
		gnome_print_gsave (gpc);
		gnome_print_concat (gpc, child->affine);
		sp_item_print (child, gpc);
		gnome_print_grestore (gpc);
	}
}

static gchar * sp_group_description (SPItem * item)
{
	SPGroup * group;
	gint n;
	static char c[128];

	group = SP_GROUP (item);

	n = g_slist_length (group->children);

	snprintf (c, 128, _("Group of %d items"), n);

	return g_strdup (c);
}

static GnomeCanvasItem *
sp_group_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler)
{
	SPGroup * group;
	GnomeCanvasGroup * cg;
	SPItem * child;
	GSList * l;

	group = (SPGroup *) item;

	cg = (GnomeCanvasGroup *) gnome_canvas_item_new (canvas_group, SP_TYPE_CANVAS_BGROUP, NULL);
	SP_CANVAS_BGROUP(cg)->transparent = group->transparent;

	for (l = group->children; l != NULL; l = l->next) {
		child = (SPItem *) l->data;
		sp_item_show (child, cg, handler);
	}

	return (GnomeCanvasItem *) cg;
}

static void
sp_group_hide (SPItem * item, GnomeCanvas * canvas)
{
	SPGroup * group;
	SPItem * child;
	GSList * l;

	group = (SPGroup *) item;

	for (l = group->children; l != NULL; l = l->next) {
		child = (SPItem *) l->data;
		sp_item_hide (child, canvas);
	}

	if (SP_ITEM_CLASS (parent_class)->hide)
		(* SP_ITEM_CLASS (parent_class)->hide) (item, canvas);
}

static gboolean
sp_group_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[])
{
	SPGroup * group;
	SPItem * child;
	gdouble a[6];
	GSList * l;
	gboolean ret;

	group = (SPGroup *) item;

	if (SP_ITEM_CLASS (parent_class)->paint)
		(* SP_ITEM_CLASS (parent_class)->paint) (item, buf, affine);

	ret = FALSE;

	for (l = group->children; l != NULL; l = l->next) {
		child = (SPItem *) l->data;
		art_affine_multiply (a, child->affine, affine);
		ret = sp_item_paint (child, buf, a);
		if (ret) return TRUE;
	}

	return FALSE;
}



#define SP_GROUP_C

#include <gnome.h>
#include "helper/sp-canvas-util.h"
#include "display/canvas-bgroup.h"
#include "xml/repr-private.h"
#include "sp-object-repr.h"
#include "sp-item-group.h"

static void sp_group_class_init (SPGroupClass *klass);
static void sp_group_init (SPGroup *group);
static void sp_group_destroy (GtkObject *object);

static void sp_group_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_group_read_attr (SPObject * object, const gchar * attr);
static void sp_group_child_added (SPObject * object, SPRepr * child, SPRepr * ref);
static void sp_group_remove_child (SPObject * object, SPRepr * child);
static void sp_group_order_changed (SPObject * object, SPRepr * child, SPRepr * old, SPRepr * new);

static void sp_group_update (SPItem *item, gdouble affine[]);
static void sp_group_bbox (SPItem * item, ArtDRect * bbox);
static void sp_group_print (SPItem * item, GnomePrintContext * gpc);
static gchar * sp_group_description (SPItem * item);
static GnomeCanvasItem * sp_group_show (SPItem * item, SPDesktop * desktop, GnomeCanvasGroup * canvas_group);
static void sp_group_hide (SPItem * item, SPDesktop * desktop);
static gboolean sp_group_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[]);
static void sp_group_menu (SPItem * item, GtkMenu * menu);

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
			NULL, NULL, NULL
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
	sp_object_class->read_attr = sp_group_read_attr;
	sp_object_class->child_added = sp_group_child_added;
	sp_object_class->remove_child = sp_group_remove_child;
	sp_object_class->order_changed = sp_group_order_changed;

	item_class->update = sp_group_update;
	item_class->bbox = sp_group_bbox;
	item_class->print = sp_group_print;
	item_class->description = sp_group_description;
	item_class->show = sp_group_show;
	item_class->hide = sp_group_hide;
	item_class->paint = sp_group_paint;
	item_class->menu = sp_group_menu;
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

	group = SP_GROUP (object);

	while (group->children) {
		SPObject * child;
		child = group->children;
		group->children = child->next;
		child->parent = NULL;
		child->next = NULL;
		gtk_object_unref (GTK_OBJECT (child));
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void sp_group_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPGroup * group;
	SPObject * last;
	SPRepr * rchild;

	group = SP_GROUP (object);

	if (((SPObjectClass *) (parent_class))->build)
		(* ((SPObjectClass *) (parent_class))->build) (object, document, repr);

	last = NULL;
	for (rchild = repr->children; rchild != NULL; rchild = rchild->next) {
		GtkType type;
		SPObject * child;
		type = sp_object_type_lookup (sp_repr_name (rchild));
		child = gtk_type_new (type);
		child->parent = object;
		if (last) {
			last->next = child;
		} else {
			group->children = child;
		}
		sp_object_invoke_build (child, document, rchild, SP_OBJECT_IS_CLONED (object));
		last = child;
	}

	sp_group_read_attr (object, "insensitive");
}

static void
sp_group_read_attr (SPObject * object, const gchar * attr)
{
	SPGroup * group;

	group = SP_GROUP (object);

	if (strcmp (attr, "insensitive") == 0) {
		const gchar * val;
		gboolean sensitive;
		SPItemView * v;

		val = sp_repr_attr (object->repr, attr);
		sensitive = (val == NULL);

		for (v = ((SPItem *) object)->display; v != NULL; v = v->next) {
			sp_canvas_bgroup_set_sensitive (SP_CANVAS_BGROUP (v->canvasitem), sensitive);
		}
		return;
	}

	if (SP_OBJECT_CLASS (parent_class)->read_attr)
		SP_OBJECT_CLASS (parent_class)->read_attr (object, attr);

	if (strcmp (attr, "style") == 0) {
		SPObject * child;
		for (child = group->children; child != NULL; child = child->next) {
			sp_object_invoke_read_attr (child, attr);
		}
	}
}

static void
sp_group_child_added (SPObject * object, SPRepr * child, SPRepr * ref)
{
	SPGroup * group;
	SPItem * item;
	GtkType type;
	SPObject * ochild, * prev;
	gint position;

	item = SP_ITEM (object);
	group = SP_GROUP (object);

	if (((SPObjectClass *) (parent_class))->child_added)
		(* ((SPObjectClass *) (parent_class))->child_added) (object, child, ref);

	type = sp_object_type_lookup (sp_repr_name (child));
	ochild = gtk_type_new (type);
	ochild->parent = object;

	prev = NULL;
	position = 0;
	if (ref != NULL) {
		prev = group->children;
		while (prev->repr != ref) {
			if (SP_IS_ITEM (prev)) position += 1;
			prev = prev->next;
		}
		if (SP_IS_ITEM (prev)) position += 1;
	}

	if (prev) {
		ochild->next = prev->next;
		prev->next = ochild;
	} else {
		ochild->next = group->children;
		group->children = ochild;
	}

	sp_object_invoke_build (ochild, object->document, child, SP_OBJECT_IS_CLONED (object));

	if (SP_IS_ITEM (ochild)) {
		SPItemView * v;
		for (v = item->display; v != NULL; v = v->next) {
			GnomeCanvasItem * ci;
			ci = sp_item_show (SP_ITEM (ochild), v->desktop, GNOME_CANVAS_GROUP (v->canvasitem));
			gnome_canvas_item_move_to_z (ci, position);
		}
	}
}

static void
sp_group_remove_child (SPObject * object, SPRepr * child)
{
	SPGroup * group;
	SPObject * prev, * ochild;

	group = SP_GROUP (object);

	if (((SPObjectClass *) (parent_class))->remove_child)
		(* ((SPObjectClass *) (parent_class))->remove_child) (object, child);

	prev = NULL;
	ochild = group->children;
	while (ochild->repr != child) {
		prev = ochild;
		ochild = ochild->next;
	}

	if (prev) {
		prev->next = ochild->next;
	} else {
		group->children = ochild->next;
	}

	ochild->parent = NULL;
	ochild->next = NULL;
	gtk_object_unref (GTK_OBJECT (ochild));
}

static void
sp_group_order_changed (SPObject * object, SPRepr * child, SPRepr * old, SPRepr * new)
{
	SPGroup * group;
	SPObject * ochild, * oold, * onew, * o;
	gint pos, oldpos, newpos;

	group = SP_GROUP (object);

	if (((SPObjectClass *) (parent_class))->order_changed)
		(* ((SPObjectClass *) (parent_class))->order_changed) (object, child, old, new);

	ochild = NULL;
	oold = NULL;
	onew = NULL;
	pos = oldpos = newpos = 0;

	o = group->children;
	while ((!ochild) || (old && !oold) || (new && !onew)) {
		if (SP_IS_ITEM (o)) pos += 1;
		if (o->repr == child) ochild = o;
		if (old && o->repr == old) {
			oold = o;
			oldpos = pos;
		}
		if (new && o->repr == new) {
			onew = o;
			newpos = pos;
		}
		o = o->next;
	}

	g_print ("oldpos %d newpos %d\n", oldpos, newpos);

	if (oold) {
		oold->next = ochild->next;
	} else {
		group->children = ochild->next;
	}
	if (onew) {
		ochild->next = onew->next;
		onew->next = ochild;
	} else {
		ochild->next = group->children;
		group->children = ochild;
	}

	if (SP_IS_ITEM (ochild)) {
		sp_item_change_canvasitem_position (SP_ITEM (ochild), newpos - oldpos);
	}
}

static void
sp_group_update (SPItem *item, gdouble affine[])
{
	SPGroup *group;
	SPItem * child;
	SPObject * o;
	gdouble a[6];

	group = SP_GROUP (item);

	if (SP_ITEM_CLASS (parent_class)->update)
		(* SP_ITEM_CLASS (parent_class)->update) (item, affine);

	for (o = group->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			art_affine_multiply (a, child->affine, affine);
			sp_item_update (child, a);
		}
	}
}

static void
sp_group_bbox (SPItem * item, ArtDRect *bbox)
{
	SPGroup * group;
	SPItem * child;
	ArtDRect child_bbox;
	SPObject * o;

	group = SP_GROUP (item);

	bbox->x0 = bbox->y0 = bbox->x1 = bbox->y1 = 0.0;

	for (o = group->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			sp_item_bbox (child, &child_bbox);
			art_drect_union (bbox, bbox, &child_bbox);
		}
	}
}

static void
sp_group_print (SPItem * item, GnomePrintContext * gpc)
{
	SPGroup * group;
	SPItem * child;
	SPObject * o;

	group = SP_GROUP (item);

	for (o = group->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			gnome_print_gsave (gpc);
			gnome_print_concat (gpc, child->affine);
			sp_item_print (child, gpc);
			gnome_print_grestore (gpc);
		}
	}
}

static gchar * sp_group_description (SPItem * item)
{
	SPGroup * group;
	SPObject * o;
	gint len;
	static char c[128];

	group = SP_GROUP (item);

	len = 0;
	for (o = group->children; o != NULL; o = o->next) len += 1;

	snprintf (c, 128, _("Group of %d objects"), len);

	return g_strdup (c);
}

static GnomeCanvasItem *
sp_group_show (SPItem * item, SPDesktop * desktop, GnomeCanvasGroup * canvas_group)
{
	SPGroup * group;
	GnomeCanvasGroup * cg;
	SPItem * child;
	SPObject * o;

	group = (SPGroup *) item;

	cg = (GnomeCanvasGroup *) gnome_canvas_item_new (canvas_group, SP_TYPE_CANVAS_BGROUP, NULL);
	SP_CANVAS_BGROUP(cg)->transparent = group->transparent;

	for (o = group->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			sp_item_show (child, desktop, cg);
		}
	}

	return (GnomeCanvasItem *) cg;
}

static void
sp_group_hide (SPItem * item, SPDesktop * desktop)
{
	SPGroup * group;
	SPItem * child;
	SPObject * o;

	group = (SPGroup *) item;

	for (o = group->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			sp_item_hide (child, desktop);
		}
	}

	if (SP_ITEM_CLASS (parent_class)->hide)
		(* SP_ITEM_CLASS (parent_class)->hide) (item, desktop);
}

static gboolean
sp_group_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[])
{
	SPGroup * group;
	SPItem * child;
	gdouble a[6];
	SPObject * o;
	gboolean ret;

	group = (SPGroup *) item;

	if (SP_ITEM_CLASS (parent_class)->paint)
		(* SP_ITEM_CLASS (parent_class)->paint) (item, buf, affine);

	ret = FALSE;

	for (o = group->children; o != NULL; o = o->next) {
		if (SP_IS_ITEM (o)) {
			child = SP_ITEM (o);
			art_affine_multiply (a, child->affine, affine);
			ret = sp_item_paint (child, buf, a);
			if (ret) return TRUE;
		}
	}

	return FALSE;
}

static void
sp_group_menu (SPItem * item, GtkMenu * menu)
{
	GtkWidget * i, * m, * w;

	if (SP_ITEM_CLASS (parent_class)->menu)
		(* SP_ITEM_CLASS (parent_class)->menu) (item, menu);

	i = gtk_menu_item_new_with_label (_("Group"));

	m = gtk_menu_new ();
	w = gtk_menu_item_new_with_label (_("Ungroup"));
	gtk_signal_connect (GTK_OBJECT (w), "activate",
			    GTK_SIGNAL_FUNC (sp_item_group_ungroup), item);

	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	gtk_widget_show (m);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), m);

	gtk_menu_append (menu, i);
	gtk_widget_show (i);
}

void
sp_item_group_ungroup (SPGroup *group)
{
#if 0
	SPItem *object, *item, *root;
	gdouble pa[6], ca[6];

	g_return_if_fail (group != NULL);
	g_return_if_fail (SP_IS_GROUP (group));

	object = SP_OBJECT (group);
	parent = object->parent;

	memcpy (

	art_affine_identity (pa);
	pastr = sp_repr_attr (current, "transform");
	sp_svg_read_affine (pa, pastr);

	while ((list = (GList *) sp_repr_children (current))) {
		child = (SPRepr *) list->data;

		art_affine_identity (ca);
		castr = sp_repr_attr (child, "transform");
		sp_svg_read_affine (ca, castr);
		art_affine_multiply (ca, ca, pa);

		css = sp_repr_css_attr_inherited (child, "style");
		sp_repr_ref (child);
		sp_repr_remove_child (current, child);
		
		sp_svg_write_affine (affinestr, 79, ca);
		affinestr[79] = '\0';
		sp_repr_set_attr (child, "transform", affinestr);

		sp_repr_css_set (child, css, "style");
		sp_repr_css_attr_unref (css);

		item = sp_document_add_repr (SP_DT_DOCUMENT (desktop), child);
		sp_repr_unref (child);
		sp_selection_add_item (selection, item);
	}

	sp_document_del_repr (SP_DT_DOCUMENT (desktop), current);
	sp_document_done (SP_DT_DOCUMENT (desktop));
#endif
}


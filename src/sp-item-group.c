#define SP_GROUP_C

#include <gnome.h>
#include "display/canvas-bgroup.h"
#include "sp-item.h"

static void sp_group_class_init (SPGroupClass *klass);
static void sp_group_init (SPGroup *group);
static void sp_group_destroy (GtkObject *object);

static void sp_group_update (SPItem *item, gdouble affine[]);
static void sp_group_bbox (SPItem * item, ArtDRect * bbox);
static void sp_group_print (SPItem * item, GnomePrintContext * gpc);
static gchar * sp_group_description (SPItem * item);
static void sp_group_read (SPItem * item, SPRepr * repr);
static void sp_group_read_attr (SPItem * item, SPRepr * repr, const gchar * attr);
static GnomeCanvasItem * sp_group_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler);
static void sp_group_hide (SPItem * item, GnomeCanvas * canvas);
static void sp_group_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[]);

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
	GtkObjectClass *object_class;
	SPItemClass * item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (SPItemClass *) klass;

	parent_class = gtk_type_class (sp_item_get_type ());

	object_class->destroy = sp_group_destroy;

	item_class->update = sp_group_update;
	item_class->bbox = sp_group_bbox;
	item_class->print = sp_group_print;
	item_class->description = sp_group_description;
	item_class->read = sp_group_read;
	item_class->read_attr = sp_group_read_attr;
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

	group = SP_GROUP (object);

	while (group->children) {
		gtk_object_destroy ((GtkObject *) group->children->data);
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
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

static void sp_group_read (SPItem * item, SPRepr * repr)
{
	if (SP_ITEM_CLASS (parent_class)->read)
		(* SP_ITEM_CLASS (parent_class)->read) (item, repr);
}

static void sp_group_read_attr (SPItem * item, SPRepr * repr, const gchar * attr)
{
	if (SP_ITEM_CLASS (parent_class)->read_attr)
		(* SP_ITEM_CLASS (parent_class)->read_attr) (item, repr, attr);
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

static void
sp_group_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[])
{
	SPGroup * group;
	SPItem * child;
	gdouble a[6];
	GSList * l;

	group = (SPGroup *) item;

	for (l = group->children; l != NULL; l = l->next) {
		child = (SPItem *) l->data;
		art_affine_multiply (a, child->affine, affine);
		sp_item_paint (child, buf, a);
	}

	if (SP_ITEM_CLASS (parent_class)->paint)
		(* SP_ITEM_CLASS (parent_class)->paint) (item, buf, affine);
}



#define SP_ROOT_C

#include "svg/svg.h"
#include "document.h"
#include "sp-namedview.h"
#include "sp-root.h"

enum {
	ARG_0,
	ARG_WIDTH,
	ARG_HEIGHT,
	ARG_NAMEDVIEWS
};

#define SP_SVG_DEFAULT_WIDTH 595.27
#define SP_SVG_DEFAULT_HEIGHT 841.89

static void sp_root_class_init (SPRootClass * klass);
static void sp_root_init (SPRoot * root);
static void sp_root_destroy (GtkObject * object);
static void sp_root_get_arg (GtkObject * object, GtkArg * arg, guint id);

static void sp_root_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_root_read_attr (SPObject * object, const gchar * key);
static void sp_root_add_child (SPObject * object, SPRepr * child);
static void sp_root_remove_child (SPObject * object, SPRepr * child);

static void sp_root_print (SPItem * item, GnomePrintContext * gpc);

static SPGroupClass * parent_class;

GtkType
sp_root_get_type (void)
{
	static GtkType root_type = 0;
	if (!root_type) {
		GtkTypeInfo root_info = {
			"SPRoot",
			sizeof (SPRoot),
			sizeof (SPRootClass),
			(GtkClassInitFunc) sp_root_class_init,
			(GtkObjectInitFunc) sp_root_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		root_type = gtk_type_unique (sp_group_get_type (), &root_info);
	}
	return root_type;
}

static void
sp_root_class_init (SPRootClass *klass)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	item_class = (SPItemClass *) klass;

	parent_class = gtk_type_class (sp_group_get_type ());

	gtk_object_add_arg_type ("SPRoot::width", GTK_TYPE_DOUBLE, GTK_ARG_READABLE, ARG_WIDTH);
	gtk_object_add_arg_type ("SPRoot::height", GTK_TYPE_DOUBLE, GTK_ARG_READABLE, ARG_HEIGHT);
	gtk_object_add_arg_type ("SPRoot::namedviews", GTK_TYPE_POINTER, GTK_ARG_READABLE, ARG_NAMEDVIEWS);

	gtk_object_class->destroy = sp_root_destroy;
	gtk_object_class->get_arg = sp_root_get_arg;

	sp_object_class->build = sp_root_build;
	sp_object_class->read_attr = sp_root_read_attr;
	sp_object_class->add_child = sp_root_add_child;
	sp_object_class->remove_child = sp_root_remove_child;

	item_class->print = sp_root_print;
}

static void
sp_root_init (SPRoot *root)
{
	root->group.transparent = TRUE;
	root->width = SP_SVG_DEFAULT_WIDTH;
	root->height = SP_SVG_DEFAULT_HEIGHT;
	root->viewbox.x0 = root->viewbox.y0 = 0.0;
	root->viewbox.x1 = root->width;
	root->viewbox.y1 = root->height;
}

static void
sp_root_destroy (GtkObject *object)
{
	SPRoot * root;

	root = (SPRoot *) object;

	g_slist_free (root->namedviews);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_root_get_arg (GtkObject * object, GtkArg * arg, guint id)
{
	SPRoot * root;

	root = SP_ROOT (object);

	switch (id) {
	case ARG_WIDTH:
		GTK_VALUE_DOUBLE (* arg) = root->width;
		break;
	case ARG_HEIGHT:
		GTK_VALUE_DOUBLE (* arg) = root->height;
		break;
	case ARG_NAMEDVIEWS:
		GTK_VALUE_POINTER (* arg) = root->namedviews;
		break;
	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
sp_root_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPGroup * group;
	SPRoot * root;
	GSList * l;

	group = (SPGroup *) object;
	root = (SPRoot *) object;

	if (((SPObjectClass *) parent_class)->build)
		(* ((SPObjectClass *) parent_class)->build) (object, document, repr);

	sp_root_read_attr (object, "width");
	sp_root_read_attr (object, "height");
	sp_root_read_attr (object, "viewBox");

	for (l = group->other; l != NULL; l = l->next) {
		if (SP_IS_NAMEDVIEW (l->data)) {
			root->namedviews = g_slist_prepend (root->namedviews, l->data);
		}
	}

	root->namedviews = g_slist_reverse (root->namedviews);
}

static void
sp_root_read_attr (SPObject * object, const gchar * key)
{
	SPItem * item;
	SPRoot * root;
	const gchar * astr;
	SPSVGUnit unit;
	gdouble len;
	GSList * l;

	item = SP_ITEM (object);
	root = SP_ROOT (object);

	astr = sp_repr_attr (object->repr, key);

	if (strcmp (key, "width") == 0) {
		len = sp_svg_read_length (&unit, astr);
		if (len >= 1.0) root->width = len;
		return;
	}
	if (strcmp (key, "height") == 0) {
		len = sp_svg_read_length (&unit, astr);
		if (len >= 1.0) root->height = len;
		/* fixme: */
		art_affine_scale (item->affine, 1.0, -1.0);
		item->affine[5] = root->height;
		for (l = item->display; l != NULL; l = l->next) {
			gnome_canvas_item_affine_absolute (GNOME_CANVAS_ITEM (l->data), item->affine);
		}
		return;
	}
	/* fixme: */
	if (strcmp (key, "viewBox") == 0) {
		gdouble x, y, width, height;
		gchar * eptr;
		gdouble t0[6], s[6], t1[6], a[6];

		if (!astr) return;
		eptr = (gchar *) astr;
		x = strtod (eptr, &eptr);
		while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
		y = strtod (eptr, &eptr);
		while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
		width = strtod (eptr, &eptr);
		while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
		height = strtod (eptr, &eptr);
		while (*eptr && ((*eptr == ',') || (*eptr == ' '))) eptr++;
		if ((width > 0) && (height > 0)) {
			root->viewbox.x0 = x;
			root->viewbox.y0 = y;
			root->viewbox.x1 = x + width;
			root->viewbox.y1 = y + height;
			art_affine_translate (t0, x, y);
			art_affine_scale (s, root->width / width, -root->height / height);
			art_affine_translate (t1, 0, root->height);
			art_affine_multiply (a, t0, s);
			art_affine_multiply (a, a, t1);
			memcpy (item->affine, a, 6 * sizeof (gdouble));
			for (l = item->display; l != NULL; l = l->next) {
				gnome_canvas_item_affine_absolute (GNOME_CANVAS_ITEM (l->data), item->affine);
			}
			return;
		}
	}

	if (((SPObjectClass *) parent_class)->read_attr)
		(* ((SPObjectClass *) parent_class)->read_attr) (object, key);
}

static void
sp_root_add_child (SPObject * object, SPRepr * child)
{
	SPRoot * root;
	SPObject * co;
	const gchar * id;

	root = (SPRoot *) object;

	if (SP_OBJECT_CLASS (parent_class)->add_child)
		(* SP_OBJECT_CLASS (parent_class)->add_child) (object, child);

	/* Hope, parent invoked children's ::build method */

	id = sp_repr_attr (child, "id");
	co = sp_document_lookup_id (object->document, id);
	g_assert (co != NULL);

	if (SP_IS_NAMEDVIEW (co)) {
		root->namedviews = g_slist_append (root->namedviews, co);
	}
}

static void
sp_root_remove_child (SPObject * object, SPRepr * child)
{
	SPRoot * root;
	SPObject * co;
	const gchar * id;

	root = (SPRoot *) object;

	id = sp_repr_attr (child, "id");
	co = sp_document_lookup_id (object->document, id);
	g_assert (co != NULL);

	if (SP_IS_NAMEDVIEW (co)) {
		root->namedviews = g_slist_remove (root->namedviews, co);
	}

	if (SP_OBJECT_CLASS (parent_class)->remove_child)
		(* SP_OBJECT_CLASS (parent_class)->remove_child) (object, child);

}

static void
sp_root_print (SPItem * item, GnomePrintContext * gpc)
{
	/* We translate here from SVG to PS coordinates */

	gnome_print_gsave (gpc);
	gnome_print_concat (gpc, item->affine);

	if (((SPItemClass *) parent_class)->print)
		(* ((SPItemClass *) parent_class)->print) (item, gpc);

	gnome_print_grestore (gpc);
}


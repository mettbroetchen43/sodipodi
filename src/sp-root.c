#define SP_ROOT_C

#if 0
#include <libgnomeui/gnome-canvas.h>
#include <libgnomeui/gnome-canvas-util.h>
#include "xml/repr.h"
#include "canvas-helper/sp-canvas-util.h"
#include "sp-item-util.h"
#endif
#include "sp-root.h"

#define SP_SVG_DEFAULT_WIDTH 595.27
#define SP_SVG_DEFAULT_HEIGHT 841.89

static void sp_root_class_init (SPRootClass * klass);
static void sp_root_init (SPRoot * root);
static void sp_root_destroy (GtkObject * object);

static void sp_root_print (SPItem * item, GnomePrintContext * gpc);
static void sp_root_read (SPItem * item, SPRepr * repr);
static void sp_root_read_attr (SPItem * item, SPRepr * repr, const gchar * attr);

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
	GtkObjectClass *object_class;
	SPItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (SPItemClass *) klass;

	parent_class = gtk_type_class (sp_group_get_type ());

	object_class->destroy = sp_root_destroy;

	item_class->print = sp_root_print;
	item_class->read = sp_root_read;
	item_class->read_attr = sp_root_read_attr;
}

static void
sp_root_init (SPRoot *root)
{
	root->group.transparent = TRUE;
	root->width = SP_SVG_DEFAULT_WIDTH;
	root->height = SP_SVG_DEFAULT_HEIGHT;
}

static void
sp_root_destroy (GtkObject *object)
{
	SPRoot * root;

	root = (SPRoot *) object;

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
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

static void
sp_root_read (SPItem * item, SPRepr * repr)
{
	if (((SPItemClass *) parent_class)->read)
		(* ((SPItemClass *) parent_class)->read) (item, repr);
	sp_root_read_attr (item, repr, "width");
	sp_root_read_attr (item, repr, "height");
}

static void
sp_root_read_attr (SPItem * item, SPRepr * repr, const gchar * attr)
{
	SPRoot * root;

	root = (SPRoot *) item;

	if (strcmp (attr, "width") == 0) {
		root->width = sp_repr_get_double_attribute (repr, attr, root->width);
		return;
	}
	if (strcmp (attr, "height") == 0) {
		root->height = sp_repr_get_double_attribute (repr, attr, root->height);
		/* fixme: */
		art_affine_scale (item->affine, 1.0, -1.0);
		item->affine[5] = root->height;
		return;
	}
	if (((SPItemClass *) parent_class)->read_attr)
		(* ((SPItemClass *) parent_class)->read_attr) (item, repr, attr);
}


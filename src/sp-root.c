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

static void sp_root_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_root_read_attr (SPObject * object, const gchar * key);

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

	gtk_object_class->destroy = sp_root_destroy;

	sp_object_class->build = sp_root_build;
	sp_object_class->read_attr = sp_root_read_attr;

	item_class->print = sp_root_print;
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
sp_root_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	if (((SPObjectClass *) parent_class)->build)
		(* ((SPObjectClass *) parent_class)->build) (object, document, repr);

	sp_root_read_attr (object, "width");
	sp_root_read_attr (object, "height");
}

static void
sp_root_read_attr (SPObject * object, const gchar * key)
{
	SPRoot * root;

	root = SP_ROOT (object);

	if (strcmp (key, "width") == 0) {
		root->width = sp_repr_get_double_attribute (object->repr, key, root->width);
		return;
	}
	if (strcmp (key, "height") == 0) {
		root->height = sp_repr_get_double_attribute (object->repr, key, root->height);
		/* fixme: */
		art_affine_scale (SP_ITEM (root)->affine, 1.0, -1.0);
		SP_ITEM (root)->affine[5] = root->height;
		return;
	}

	if (((SPObjectClass *) parent_class)->read_attr)
		(* ((SPObjectClass *) parent_class)->read_attr) (object, key);
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


#define SP_GUIDE_C

#include "helper/sp-guide.h"
#include "sp-guide.h"

static void sp_guide_class_init (SPGuideClass * klass);
static void sp_guide_init (SPGuide * guide);
static void sp_guide_destroy (GtkObject * object);

static void sp_guide_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_guide_read_attr (SPObject * object, const gchar * key);

static SPObjectClass * parent_class;

GtkType
sp_guide_get_type (void)
{
	static GtkType guide_type = 0;
	if (!guide_type) {
		GtkTypeInfo guide_info = {
			"SPGuide",
			sizeof (SPGuide),
			sizeof (SPGuideClass),
			(GtkClassInitFunc) sp_guide_class_init,
			(GtkObjectInitFunc) sp_guide_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		guide_type = gtk_type_unique (sp_object_get_type (), &guide_info);
	}
	return guide_type;
}

static void
sp_guide_class_init (SPGuideClass * klass)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	parent_class = gtk_type_class (sp_object_get_type ());

	gtk_object_class->destroy = sp_guide_destroy;

	sp_object_class->build = sp_guide_build;
	sp_object_class->read_attr = sp_guide_read_attr;
}

static void
sp_guide_init (SPGuide * guide)
{
	guide->orientation = SP_GUIDE_HORIZONTAL;
	guide->position = 0.0;
}

static void
sp_guide_destroy (GtkObject * object)
{
	SPGuide * guide;

	guide = (SPGuide *) object;

	/* fixme: destroy views */

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_guide_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	if (((SPObjectClass *) (parent_class))->build)
		(* ((SPObjectClass *) (parent_class))->build) (object, document, repr);

	sp_guide_read_attr (object, "orientation");
	sp_guide_read_attr (object, "position");

}

static void
sp_guide_read_attr (SPObject * object, const gchar * key)
{
	SPGuide * guide;
	const gchar * astr;

	guide = SP_GUIDE (object);

	if (strcmp (key, "orientation") == 0) {
		astr = sp_repr_attr (object->repr, key);
		if (strcmp (astr, "horizontal") == 0) {
			guide->orientation = SP_GUIDE_HORIZONTAL;
		} else if (strcmp (astr, "vertical") == 0) {
			guide->orientation = SP_GUIDE_VERTICAL;
		}
		return;
	}
	if (strcmp (key, "position") == 0) {
		guide->position = sp_repr_get_double_attribute (object->repr, key, 0.0);
		return;
	}

	if (((SPObjectClass *) (parent_class))->read_attr)
		(* ((SPObjectClass *) (parent_class))->read_attr) (object, key);
}

void
sp_guide_show (SPGuide * guide, GnomeCanvasGroup * group)
{
	GnomeCanvasItem * item;

	item = gnome_canvas_item_new (group,
		SP_TYPE_GUIDELINE,
		"orientation", guide->orientation,
		NULL);

	g_assert (item != NULL);

	sp_guideline_moveto ((SPGuideLine *) item, guide->position, guide->position);

	guide->views = g_slist_prepend (guide->views, item);
}

gint
sp_guide_compare (gconstpointer a, gconstpointer b)
{
	SPGuide * ga, * gb;

	ga = SP_GUIDE (a);
	gb = SP_GUIDE (b);

	if (ga->position < gb->position) return -1;
	if (ga->position > gb->position) return 1;
	return 0;
}


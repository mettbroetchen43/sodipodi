#define SP_NAMEDVIEW_C

#include "sp-guide.h"
#include "sp-namedview.h"

static void sp_namedview_class_init (SPNamedViewClass * klass);
static void sp_namedview_init (SPNamedView * namedview);
static void sp_namedview_destroy (GtkObject * object);

static void sp_namedview_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_namedview_read_attr (SPObject * object, const gchar * key);

static SPObjectClass * parent_class;

GtkType
sp_namedview_get_type (void)
{
	static GtkType namedview_type = 0;
	if (!namedview_type) {
		GtkTypeInfo namedview_info = {
			"SPNamedView",
			sizeof (SPNamedView),
			sizeof (SPNamedViewClass),
			(GtkClassInitFunc) sp_namedview_class_init,
			(GtkObjectInitFunc) sp_namedview_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		namedview_type = gtk_type_unique (sp_object_get_type (), &namedview_info);
	}
	return namedview_type;
}

static void
sp_namedview_class_init (SPNamedViewClass * klass)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	parent_class = gtk_type_class (sp_object_get_type ());

	gtk_object_class->destroy = sp_namedview_destroy;

	sp_object_class->build = sp_namedview_build;
	sp_object_class->read_attr = sp_namedview_read_attr;
}

static void
sp_namedview_init (SPNamedView * namedview)
{
	namedview->editable = TRUE;
	namedview->hguides = NULL;
	namedview->vguides = NULL;
}

static void
sp_namedview_destroy (GtkObject * object)
{
	SPNamedView * namedview;

	namedview = (SPNamedView *) object;

	while (namedview->hguides) {
		gtk_object_unref ((GtkObject *) namedview->hguides->data);
		namedview->hguides = g_slist_remove_link (namedview->hguides, namedview->hguides);
	}

	while (namedview->vguides) {
		gtk_object_unref ((GtkObject *) namedview->vguides->data);
		namedview->vguides = g_slist_remove_link (namedview->vguides, namedview->vguides);
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_namedview_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPNamedView * namedview;
	const GList * l;
	SPRepr * crepr;
	const gchar * name;
	SPGuide * guide;

	namedview = SP_NAMEDVIEW (object);

	if (((SPObjectClass *) (parent_class))->build)
		(* ((SPObjectClass *) (parent_class))->build) (object, document, repr);

	sp_namedview_read_attr (object, "editable");

	l = sp_repr_children (repr);

	while (l != NULL) {
		crepr = (SPRepr *) l->data;
		name = sp_repr_name (crepr);
		if (strcmp (name, "sodipodi:guide") == 0) {
			guide = gtk_type_new (SP_TYPE_GUIDE);
			sp_object_invoke_build (SP_OBJECT (guide), document, crepr);
			if (guide->orientation == SP_GUIDE_HORIZONTAL) {
				namedview->hguides = g_slist_insert_sorted (namedview->hguides, guide, sp_guide_compare);
			} else {
				namedview->vguides = g_slist_insert_sorted (namedview->vguides, guide, sp_guide_compare);
			}
		}
		l = l->next;
	}
}

static void
sp_namedview_read_attr (SPObject * object, const gchar * key)
{
	SPNamedView * namedview;
	const gchar * astr;

	namedview = SP_NAMEDVIEW (object);

	astr = sp_repr_attr (object->repr, key);

	if (strcmp (key, "editable") == 0) {
		namedview->editable = (astr != NULL);
		return;
	}

	if (((SPObjectClass *) (parent_class))->read_attr)
		(* ((SPObjectClass *) (parent_class))->read_attr) (object, key);
}


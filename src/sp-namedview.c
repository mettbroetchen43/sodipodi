#define SP_NAMEDVIEW_C

#include "svg/svg.h"
#include "document.h"
#include "desktop.h"
#include "desktop-events.h"
#include "sp-guide.h"
#include "sp-namedview.h"

static void sp_namedview_class_init (SPNamedViewClass * klass);
static void sp_namedview_init (SPNamedView * namedview);
static void sp_namedview_destroy (GtkObject * object);

static void sp_namedview_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_namedview_read_attr (SPObject * object, const gchar * key);
static void sp_namedview_add_child (SPObject * object, SPRepr * child);
static void sp_namedview_remove_child (SPObject * object, SPRepr * child);

static SPObjectGroupClass * parent_class;

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
		namedview_type = gtk_type_unique (sp_objectgroup_get_type (), &namedview_info);
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

	parent_class = gtk_type_class (sp_objectgroup_get_type ());

	gtk_object_class->destroy = sp_namedview_destroy;

	sp_object_class->build = sp_namedview_build;
	sp_object_class->read_attr = sp_namedview_read_attr;
	sp_object_class->add_child = sp_namedview_add_child;
	sp_object_class->remove_child = sp_namedview_remove_child;
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

	g_slist_free (namedview->hguides);
	g_slist_free (namedview->vguides);

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_namedview_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPNamedView * nv;
	SPObjectGroup * og;
	GSList * l;

	nv = (SPNamedView *) object;
	og = (SPObjectGroup *) object;

	if (((SPObjectClass *) (parent_class))->build)
		(* ((SPObjectClass *) (parent_class))->build) (object, document, repr);

	sp_namedview_read_attr (object, "viewonly");
	sp_namedview_read_attr (object, "snaptoguides");
	sp_namedview_read_attr (object, "guidetolerance");

	/* Construct guideline list */

	for (l = og->children; l != NULL; l = l->next) {
		if (SP_IS_GUIDE (l->data)) {
			SPGuide * g;
			g = (SPGuide *) l->data;
			if (g->orientation == SP_GUIDE_HORIZONTAL) {
				nv->hguides = g_slist_prepend (nv->hguides, g);
			} else {
				nv->vguides = g_slist_prepend (nv->vguides, g);
			}
		}
	}
}

static void
sp_namedview_read_attr (SPObject * object, const gchar * key)
{
	SPNamedView * namedview;
	SPSVGUnit unit;
	const gchar * astr;

	namedview = SP_NAMEDVIEW (object);

	astr = sp_repr_attr (object->repr, key);

	if (strcmp (key, "viewonly") == 0) {
		namedview->editable = (astr == NULL);
		return;
	}
	if (strcmp (key, "snaptoguides") == 0) {
		namedview->snaptoguides = (astr != NULL);
		return;
	}
	if (strcmp (key, "guidetolerance") == 0) {
		namedview->guidetolerance = sp_svg_read_length (&unit, astr);
		return;
	}

	if (((SPObjectClass *) (parent_class))->read_attr)
		(* ((SPObjectClass *) (parent_class))->read_attr) (object, key);
}

static void
sp_namedview_add_child (SPObject * object, SPRepr * child)
{
	SPNamedView * nv;
	SPObject * no;
	const gchar * id;
	GSList * l;

	nv = (SPNamedView *) object;

	if (((SPObjectClass *) (parent_class))->add_child)
		(* ((SPObjectClass *) (parent_class))->add_child) (object, child);

	id = sp_repr_attr (child, "id");

	no = sp_document_lookup_id (object->document, id);
	g_assert (SP_IS_OBJECT (no));

	if (SP_IS_GUIDE (no)) {
		SPGuide * g;
		g = (SPGuide *) no;
		if (g->orientation == SP_GUIDE_HORIZONTAL) {
			nv->hguides = g_slist_prepend (nv->hguides, g);
		} else {
			nv->vguides = g_slist_prepend (nv->vguides, g);
		}
		if (nv->editable) {
			for (l = nv->views; l != NULL; l = l->next) {
				sp_guide_show (g, SP_DESKTOP (l->data)->guides, sp_dt_guide_event);
				if (SP_DESKTOP (l->data)->guides_active) sp_guide_sensitize (g, SP_DESKTOP (l->data)->canvas, TRUE);
			}
		}
	}
}

static void
sp_namedview_remove_child (SPObject * object, SPRepr * child)
{
	SPNamedView * nv;
	SPObject * no;
	const gchar * id;

	nv = (SPNamedView *) object;

	id = sp_repr_attr (child, "id");

	no = sp_document_lookup_id (object->document, id);
	g_assert (no != NULL);

	if (SP_IS_GUIDE (no)) {
		SPGuide * g;
		g = (SPGuide *) no;
		if (g->orientation == SP_GUIDE_HORIZONTAL) {
			nv->hguides = g_slist_remove (nv->hguides, g);
		} else {
			nv->vguides = g_slist_remove (nv->hguides, g);
		}
	}

	if (((SPObjectClass *) (parent_class))->remove_child)
		(* ((SPObjectClass *) (parent_class))->remove_child) (object, child);
}

void
sp_namedview_show (SPNamedView * namedview, gpointer desktop)
{
	SPDesktop * dt;
	GSList * l;

	dt = SP_DESKTOP (desktop);

	for (l = namedview->hguides; l != NULL; l = l->next) {
		sp_guide_show (SP_GUIDE (l->data), dt->guides, sp_dt_guide_event);
		if (dt->guides_active) sp_guide_sensitize (SP_GUIDE (l->data), dt->canvas, TRUE);
	}

	for (l = namedview->vguides; l != NULL; l = l->next) {
		sp_guide_show (SP_GUIDE (l->data), dt->guides, sp_dt_guide_event);
		if (dt->guides_active) sp_guide_sensitize (SP_GUIDE (l->data), dt->canvas, TRUE);
	}

	namedview->views = g_slist_prepend (namedview->views, desktop);
}

void
sp_namedview_hide (SPNamedView * nv, gpointer desktop)
{
	SPDesktop * dt;
	GSList * l;

	g_assert (nv != NULL);
	g_assert (SP_IS_NAMEDVIEW (nv));
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (g_slist_find (nv->views, desktop));

	dt = SP_DESKTOP (desktop);

	for (l = nv->hguides; l != NULL; l = l->next) {
		sp_guide_hide (SP_GUIDE (l->data), dt->canvas);
	}

	for (l = nv->vguides; l != NULL; l = l->next) {
		sp_guide_hide (SP_GUIDE (l->data), dt->canvas);
	}

	nv->views = g_slist_remove (nv->views, desktop);
}

void
sp_namedview_activate_guides (SPNamedView * nv, gpointer desktop, gboolean active)
{
	SPDesktop * dt;
	GSList * l;

	g_assert (nv != NULL);
	g_assert (SP_IS_NAMEDVIEW (nv));
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (g_slist_find (nv->views, desktop));

	dt = SP_DESKTOP (desktop);

	for (l = nv->hguides; l != NULL; l = l->next) {
		sp_guide_sensitize (SP_GUIDE (l->data), dt->canvas, active);
	}

	for (l = nv->vguides; l != NULL; l = l->next) {
		sp_guide_sensitize (SP_GUIDE (l->data), dt->canvas, active);
	}
}



#define SP_NAMEDVIEW_C

#include <string.h>
#include "helper/canvas-grid.h"
#include "svg/svg.h"
#include "document.h"
#include "desktop.h"
#include "desktop-events.h"
#include "desktop-handles.h"
#include "sp-guide.h"
#include "sp-namedview.h"

#define PTPERMM (72.0 / 25.4)
#define DEFAULTTOLERANCE 5.0

static void sp_namedview_class_init (SPNamedViewClass * klass);
static void sp_namedview_init (SPNamedView * namedview);
static void sp_namedview_destroy (GtkObject * object);

static void sp_namedview_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_namedview_read_attr (SPObject * object, const gchar * key);
static void sp_namedview_child_added (SPObject * object, SPRepr * child, SPRepr * ref);
static void sp_namedview_remove_child (SPObject * object, SPRepr * child);

static void sp_namedview_setup_grid (SPNamedView * nv);
static void sp_namedview_setup_grid_item (SPNamedView * nv, GnomeCanvasItem * item);

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
	sp_object_class->child_added = sp_namedview_child_added;
	sp_object_class->remove_child = sp_namedview_remove_child;
}

static void
sp_namedview_init (SPNamedView * nv)
{
	nv->editable = TRUE;
	nv->showgrid = FALSE;
	nv->snaptogrid = FALSE;
	nv->showguides = FALSE;
	nv->snaptoguides = FALSE;
	nv->gridtolerance = DEFAULTTOLERANCE;
	nv->guidetolerance = DEFAULTTOLERANCE;
	nv->gridorigin.x = nv->gridorigin.y = 0.0;
	nv->gridspacing.x = nv->gridspacing.y = PTPERMM;
	nv->gridcolor = 0x3f3fff3f;
	nv->guidecolor = 0x0000ff7f;
	nv->guidehicolor = 0xff00007f;

	nv->hguides = NULL;
	nv->vguides = NULL;
	nv->viewcount = 0;
}

static void
sp_namedview_destroy (GtkObject * object)
{
	SPNamedView * namedview;

	namedview = (SPNamedView *) object;

	g_slist_free (namedview->hguides);
	g_slist_free (namedview->vguides);

	g_assert (!namedview->views);

	while (namedview->gridviews) {
		gtk_object_destroy (GTK_OBJECT (namedview->gridviews->data));
		namedview->gridviews = g_slist_remove (namedview->gridviews, namedview->gridviews->data);
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_namedview_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPNamedView * nv;
	SPObjectGroup * og;
        SPObject * o;

	nv = (SPNamedView *) object;
	og = (SPObjectGroup *) object;

	if (((SPObjectClass *) (parent_class))->build)
		(* ((SPObjectClass *) (parent_class))->build) (object, document, repr);

	sp_namedview_read_attr (object, "viewonly");
	sp_namedview_read_attr (object, "showgrid");
	sp_namedview_read_attr (object, "snaptogrid");
	sp_namedview_read_attr (object, "showguides");
	sp_namedview_read_attr (object, "snaptoguides");
	sp_namedview_read_attr (object, "gridtolerance");
	sp_namedview_read_attr (object, "guidetolerance");
	sp_namedview_read_attr (object, "gridoriginx");
	sp_namedview_read_attr (object, "gridoriginy");
	sp_namedview_read_attr (object, "gridspacingx");
	sp_namedview_read_attr (object, "gridspacingy");
	sp_namedview_read_attr (object, "gridcolor");
	sp_namedview_read_attr (object, "gridopacity");
	sp_namedview_read_attr (object, "guidecolor");
	sp_namedview_read_attr (object, "guideopacity");
	sp_namedview_read_attr (object, "guidehicolor");
	sp_namedview_read_attr (object, "guidehiopacity");

	/* Construct guideline list */

	for (o = og->children; o != NULL; o = o->next) {
		if (SP_IS_GUIDE (o)) {
			SPGuide * g;
			g = SP_GUIDE (o);
			if (g->orientation == SP_GUIDE_HORIZONTAL) {
				nv->hguides = g_slist_prepend (nv->hguides, g);
			} else {
				nv->vguides = g_slist_prepend (nv->vguides, g);
			}
			gtk_object_set (GTK_OBJECT (g), "color", nv->guidecolor, "hicolor", nv->guidehicolor, NULL);
		}
	}
}

static void
sp_namedview_read_attr (SPObject * object, const gchar * key)
{
	SPNamedView * namedview;
	SPSVGUnit unit;
	const gchar * astr;
	GSList * l;
	gdouble v;

	namedview = SP_NAMEDVIEW (object);

	astr = sp_repr_attr (object->repr, key);

	if (strcmp (key, "viewonly") == 0) {
		namedview->editable = (astr == NULL);
		return;
	}
	if (strcmp (key, "showgrid") == 0) {
		namedview->showgrid = (astr != NULL);
		sp_namedview_setup_grid (namedview);
		return;
	}
	if (strcmp (key, "snaptogrid") == 0) {
		namedview->snaptogrid = (astr != NULL);
		return;
	}
	if (strcmp (key, "showguides") == 0) {
		namedview->showguides = (astr != NULL);
		return;
	}
	if (strcmp (key, "snaptoguides") == 0) {
		namedview->snaptoguides = (astr != NULL);
		return;
	}
	if (strcmp (key, "gridtolerance") == 0) {
		if (astr) {
			namedview->gridtolerance = sp_svg_read_length (&unit, astr, 5.0);
		} else {
			namedview->gridtolerance = DEFAULTTOLERANCE;
		}
		return;
	}
	if (strcmp (key, "guidetolerance") == 0) {
		if (astr) {
			namedview->guidetolerance = sp_svg_read_length (&unit, astr, 5.0);
		} else {
			namedview->guidetolerance = DEFAULTTOLERANCE;
		}
		return;
	}
	if (strcmp (key, "gridoriginx") == 0) {
		namedview->gridorigin.x = sp_svg_read_length (&unit, astr, 0.0);
		sp_namedview_setup_grid (namedview);
		return;
	}
	if (strcmp (key, "gridoriginy") == 0) {
		namedview->gridorigin.y = sp_svg_read_length (&unit, astr, 0.0);
		sp_namedview_setup_grid (namedview);
		return;
	}
	if (strcmp (key, "gridspacingx") == 0) {
		if (astr) {
			namedview->gridspacing.x = sp_svg_read_length (&unit, astr, 16.0);
		} else {
			namedview->gridspacing.x = PTPERMM;
		}
		sp_namedview_setup_grid (namedview);
		return;
	}
	if (strcmp (key, "gridspacingy") == 0) {
		if (astr) {
			namedview->gridspacing.y = sp_svg_read_length (&unit, astr, 16.0);
		} else {
			namedview->gridspacing.y = PTPERMM;
		}
		sp_namedview_setup_grid (namedview);
		return;
	}
	if (strcmp (key, "gridcolor") == 0) {
		if (astr) {
			namedview->gridcolor = (namedview->gridcolor & 0xff) | sp_svg_read_color (astr, 0x0000ff00);
		}
		sp_namedview_setup_grid (namedview);
		return;
	}
	if (strcmp (key, "gridopacity") == 0) {
		v = sp_repr_get_double_attribute (object->repr, key, 0.25);
		v = CLAMP (v, 0.0, 1.0);
		namedview->gridcolor = (namedview->gridcolor & 0xffffff00) | (guint) (v * 255.0);
		sp_namedview_setup_grid (namedview);
		return;
	}
	if (strcmp (key, "guidecolor") == 0) {
		if (astr) {
			namedview->guidecolor = (namedview->guidecolor & 0xff) | sp_svg_read_color (astr, 0xff000000);
			for (l = namedview->hguides; l != NULL; l = l->next) {
				gtk_object_set (GTK_OBJECT (l->data), "color", namedview->guidecolor, NULL);
			}
			for (l = namedview->vguides; l != NULL; l = l->next) {
				gtk_object_set (GTK_OBJECT (l->data), "color", namedview->guidecolor, NULL);
			}
		}
		return;
	}
	if (strcmp (key, "guideopacity") == 0) {
		v = sp_repr_get_double_attribute (object->repr, key, 0.5);
		v = CLAMP (v, 0.0, 1.0);
		namedview->guidecolor = (namedview->guidecolor & 0xffffff00) | (guint) (v * 255.0);
		for (l = namedview->hguides; l != NULL; l = l->next) {
			gtk_object_set (GTK_OBJECT (l->data), "color", namedview->guidecolor, NULL);
		}
		for (l = namedview->vguides; l != NULL; l = l->next) {
			gtk_object_set (GTK_OBJECT (l->data), "color", namedview->guidecolor, NULL);
		}
		return;
	}
	if (strcmp (key, "guidehicolor") == 0) {
		if (astr) {
			namedview->guidehicolor = (namedview->guidehicolor & 0xff) | sp_svg_read_color (astr, 0x7f7fff00);
			for (l = namedview->hguides; l != NULL; l = l->next) {
				gtk_object_set (GTK_OBJECT (l->data), "hicolor", namedview->guidehicolor, NULL);
			}
			for (l = namedview->vguides; l != NULL; l = l->next) {
				gtk_object_set (GTK_OBJECT (l->data), "hicolor", namedview->guidehicolor, NULL);
			}
		}
		return;
	}
	if (strcmp (key, "guidehiopacity") == 0) {
		v = sp_repr_get_double_attribute (object->repr, key, 0.5);
		v = CLAMP (v, 0.0, 1.0);
		namedview->guidehicolor = (namedview->guidehicolor & 0xffffff00) | (guint) (v * 255.0);
		for (l = namedview->hguides; l != NULL; l = l->next) {
			gtk_object_set (GTK_OBJECT (l->data), "hicolor", namedview->guidehicolor, NULL);
		}
		for (l = namedview->vguides; l != NULL; l = l->next) {
			gtk_object_set (GTK_OBJECT (l->data), "hicolor", namedview->guidehicolor, NULL);
		}
		return;
	}

	if (((SPObjectClass *) (parent_class))->read_attr)
		(* ((SPObjectClass *) (parent_class))->read_attr) (object, key);
}

static void
sp_namedview_child_added (SPObject * object, SPRepr * child, SPRepr * ref)
{
	SPNamedView * nv;
	SPObject * no;
	const gchar * id;
	GSList * l;

	nv = (SPNamedView *) object;

	if (((SPObjectClass *) (parent_class))->child_added)
		(* ((SPObjectClass *) (parent_class))->child_added) (object, child, ref);

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
		gtk_object_set (GTK_OBJECT (g), "color", nv->guidecolor, "hicolor", nv->guidehicolor, NULL);
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
			nv->vguides = g_slist_remove (nv->vguides, g);
		}
	}

	if (((SPObjectClass *) (parent_class))->remove_child)
		(* ((SPObjectClass *) (parent_class))->remove_child) (object, child);
}

void
sp_namedview_show (SPNamedView * nv, gpointer desktop)
{
	SPDesktop * dt;
	GSList * l;
	GnomeCanvasItem * item;

	dt = SP_DESKTOP (desktop);

	for (l = nv->hguides; l != NULL; l = l->next) {
		sp_guide_show (SP_GUIDE (l->data), dt->guides, sp_dt_guide_event);
		if (dt->guides_active) sp_guide_sensitize (SP_GUIDE (l->data), dt->canvas, TRUE);
	}

	for (l = nv->vguides; l != NULL; l = l->next) {
		sp_guide_show (SP_GUIDE (l->data), dt->guides, sp_dt_guide_event);
		if (dt->guides_active) sp_guide_sensitize (SP_GUIDE (l->data), dt->canvas, TRUE);
	}

	nv->views = g_slist_prepend (nv->views, desktop);

	item = gnome_canvas_item_new (SP_DT_GRID (dt), SP_TYPE_CGRID, NULL);
	nv->gridviews = g_slist_prepend (nv->gridviews, item);
	sp_namedview_setup_grid_item (nv, item);
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

	for (l = nv->gridviews; l != NULL; l = l->next) {
		if (GNOME_CANVAS_ITEM (l->data)->canvas == SP_DT_CANVAS (dt)) break;
	}

	g_assert (l);

	gtk_object_destroy (GTK_OBJECT (l->data));
	nv->gridviews = g_slist_remove (nv->gridviews, l->data);
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

static void
sp_namedview_setup_grid (SPNamedView * nv)
{
	GSList * l;

	for (l = nv->gridviews; l != NULL; l = l->next) {
		sp_namedview_setup_grid_item (nv, GNOME_CANVAS_ITEM (l->data));
	}
}

static void
sp_namedview_setup_grid_item (SPNamedView * nv, GnomeCanvasItem * item)
{
	if (nv->showgrid) {
		gnome_canvas_item_show (item);
	} else {
		gnome_canvas_item_hide (item);
	}

	gnome_canvas_item_set (item,
			       "color", nv->gridcolor,
			       "originx", nv->gridorigin.x,
			       "originy", nv->gridorigin.y,
			       "spacingx", nv->gridspacing.x,
			       "spacingy", nv->gridspacing.y,
			       NULL);
}

const gchar *
sp_namedview_get_name (SPNamedView * nv)
{
  gchar * name;
  
  name = (gchar *)sp_object_getAttribute (SP_OBJECT (nv), "id", NULL);
  return name;
}

guint
sp_namedview_viewcount (SPNamedView * nv)
{
 g_assert (SP_IS_NAMEDVIEW (nv));

 return ++nv->viewcount;
}

const GSList *
sp_namedview_view_list (SPNamedView * nv)
{
 g_assert (SP_IS_NAMEDVIEW (nv));

 return nv->views;
}


#define SP_GUIDE_C

#include <string.h>
#include <gtk/gtksignal.h>
#include "helper/sp-guide.h"
#include "sp-guide.h"

enum {ARG_0, ARG_COLOR, ARG_HICOLOR};

static void sp_guide_class_init (SPGuideClass * klass);
static void sp_guide_init (SPGuide * guide);
static void sp_guide_destroy (GtkObject * object);
static void sp_guide_set_arg (GtkObject * object, GtkArg * arg, guint arg_id);

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

	gtk_object_add_arg_type ("SPGuide::color", GTK_TYPE_UINT, GTK_ARG_WRITABLE, ARG_COLOR);
	gtk_object_add_arg_type ("SPGuide::hicolor", GTK_TYPE_UINT, GTK_ARG_WRITABLE, ARG_HICOLOR);

	gtk_object_class->destroy = sp_guide_destroy;
	gtk_object_class->set_arg = sp_guide_set_arg;

	sp_object_class->build = sp_guide_build;
	sp_object_class->read_attr = sp_guide_read_attr;
}

static void
sp_guide_init (SPGuide * guide)
{
	guide->orientation = SP_GUIDE_HORIZONTAL;
	guide->position = 0.0;
	guide->color = 0x0000ff7f;
	guide->hicolor = 0xff00007f;
}

static void
sp_guide_destroy (GtkObject * object)
{
	SPGuide * guide;

	guide = (SPGuide *) object;

	while (guide->views) {
		gtk_object_destroy (GTK_OBJECT (guide->views->data));
		guide->views = g_slist_remove_link (guide->views, guide->views);
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_guide_set_arg (GtkObject * object, GtkArg * arg, guint arg_id)
{
	SPGuide * guide;
	GSList * l;

	guide = SP_GUIDE (object);

	switch (arg_id) {
	case ARG_COLOR:
		guide->color = GTK_VALUE_UINT (*arg);
		for (l = guide->views; l != NULL; l = l->next) {
			gtk_object_set (GTK_OBJECT (l->data), "color", guide->color, NULL);
		}
		break;
	case ARG_HICOLOR:
		guide->hicolor = GTK_VALUE_UINT (*arg);
		break;
	}
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
sp_guide_show (SPGuide * guide, GnomeCanvasGroup * group, gpointer handler)
{
	GnomeCanvasItem * item;

	item = gnome_canvas_item_new (group, SP_TYPE_GUIDELINE,
				      "orientation", guide->orientation,
				      "color", guide->color,
				      NULL);
	g_assert (item != NULL);
	gtk_signal_connect (GTK_OBJECT (item), "event",
			    GTK_SIGNAL_FUNC (handler), guide);

	sp_guideline_moveto ((SPGuideLine *) item, guide->position, guide->position);

	guide->views = g_slist_prepend (guide->views, item);
}

void
sp_guide_hide (SPGuide * guide, GnomeCanvas * canvas)
{
	GSList * l;

	g_assert (guide != NULL);
	g_assert (SP_IS_GUIDE (guide));
	g_assert (canvas != NULL);
	g_assert (GNOME_IS_CANVAS (canvas));

	for (l = guide->views; l != NULL; l = l->next) {
		if (canvas == GNOME_CANVAS_ITEM (l->data)->canvas) {
			gtk_object_destroy (GTK_OBJECT (l->data));
			guide->views = g_slist_remove_link (guide->views, l);
			return;
		}
	}
	g_assert_not_reached ();
}

void
sp_guide_sensitize (SPGuide * guide, GnomeCanvas * canvas, gboolean sensitive)
{
	GSList * l;

	g_assert (guide != NULL);
	g_assert (SP_IS_GUIDE (guide));
	g_assert (canvas != NULL);
	g_assert (GNOME_IS_CANVAS (canvas));

	for (l = guide->views; l != NULL; l = l->next) {
		if (canvas == GNOME_CANVAS_ITEM (l->data)->canvas) {
			sp_guideline_sensitize (SP_GUIDELINE (l->data), sensitive);
			return;
		}
	}
	g_assert_not_reached ();
}

void
sp_guide_moveto (SPGuide * guide, gdouble x, gdouble y)
{
	GSList * l;

	g_assert (guide != NULL);
	g_assert (SP_IS_GUIDE (guide));

	for (l = guide->views; l != NULL; l = l->next) {
		sp_guideline_moveto (SP_GUIDELINE (l->data), x, y);
	}
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


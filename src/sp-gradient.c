#define SP_GRADIENT_C

/*
 * Gradient stops
 */

#include <libart_lgpl/art_affine.h>
#include "svg/svg.h"
#include "sp-gradient.h"

static void sp_stop_class_init (SPStopClass * klass);
static void sp_stop_init (SPStop * stop);

static void sp_stop_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_stop_read_attr (SPObject * object, const gchar * key);

static SPObjectClass * stop_parent_class;

GtkType
sp_stop_get_type (void)
{
	static GtkType stop_type = 0;
	if (!stop_type) {
		GtkTypeInfo stop_info = {
			"SPStop",
			sizeof (SPStop),
			sizeof (SPStop),
			(GtkClassInitFunc) sp_stop_class_init,
			(GtkObjectInitFunc) sp_stop_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		stop_type = gtk_type_unique (sp_object_get_type (), &stop_info);
	}
	return stop_type;
}

static void
sp_stop_class_init (SPStopClass * klass)
{
	SPObjectClass * sp_object_class;

	sp_object_class = (SPObjectClass *) klass;

	stop_parent_class = gtk_type_class (sp_object_get_type ());

	sp_object_class->build = sp_stop_build;
	sp_object_class->read_attr = sp_stop_read_attr;
}

static void
sp_stop_init (SPStop * stop)
{
	stop->offset = 0.0;
	stop->color = 0x000000ff;
}

static void
sp_stop_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	if (SP_OBJECT_CLASS (stop_parent_class)->build)
		(* SP_OBJECT_CLASS (stop_parent_class)->build) (object, document, repr);

	sp_stop_read_attr (object, "offset");
	sp_stop_read_attr (object, "style");
}

static void
sp_stop_read_attr (SPObject * object, const gchar * key)
{
	SPStop * stop;
	SPCSSAttr * cssr, * csso;
	const gchar * cstr, * ostr;
	guint32 color;
	gdouble opacity;

	stop = SP_STOP (object);

	if (strcmp (key, "style") == 0) {
		cssr = sp_repr_css_attr (object->repr, key);
		csso = sp_repr_css_attr_inherited (object->parent->repr, key);
		cstr = sp_repr_css_property (cssr, "stop-color", NULL);
		if (cstr == NULL) {
			cstr = sp_repr_css_property (csso, "stop-color", NULL);
		}
		ostr = sp_repr_css_property (cssr, "stop-opacity", NULL);
		if (ostr == NULL) {
			ostr = sp_repr_css_property (csso, "stop-opacity", NULL);
		}
		if (cstr != NULL) {
			color = sp_svg_read_color (cstr);
		} else {
			color = 0x00000000;
		}
		if (ostr != NULL) {
			opacity = sp_svg_read_percentage (ostr);
		} else {
			opacity = 1.0;
		}
		color = (color & 0xffffff00) | ((gint) (opacity * 255.0));
		stop->color = color;
		sp_repr_css_attr_unref (cssr);
		sp_repr_css_attr_unref (csso);
		return;
	}
	if (strcmp (key, "offset") == 0) {
		stop->offset = sp_repr_get_double_attribute (object->repr, key, 0.0);
		return;
	}

	if (SP_OBJECT_CLASS (stop_parent_class)->read_attr)
		(* SP_OBJECT_CLASS (stop_parent_class)->read_attr) (object, key);
}

/*
 * Linear Gradient
 */

static void sp_lineargradient_class_init (SPLinearGradientClass * klass);
static void sp_lineargradient_init (SPLinearGradient * lg);
static void sp_lineargradient_destroy (GtkObject * object);

static void sp_lineargradient_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_lineargradient_read_attr (SPObject * object, const gchar * key);

static SPObjectClass * lg_parent_class;

GtkType
sp_lineargradient_get_type (void)
{
	static GtkType lineargradient_type = 0;
	if (!lineargradient_type) {
		GtkTypeInfo lineargradient_info = {
			"SPLinearGradient",
			sizeof (SPLinearGradient),
			sizeof (SPLinearGradientClass),
			(GtkClassInitFunc) sp_lineargradient_class_init,
			(GtkObjectInitFunc) sp_lineargradient_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		lineargradient_type = gtk_type_unique (sp_object_get_type (), &lineargradient_info);
	}
	return lineargradient_type;
}

static void
sp_lineargradient_class_init (SPLinearGradientClass * klass)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	lg_parent_class = gtk_type_class (sp_object_get_type ());

	gtk_object_class->destroy = sp_lineargradient_destroy;

	sp_object_class->build = sp_lineargradient_build;
	sp_object_class->read_attr = sp_lineargradient_read_attr;
}

static void
sp_lineargradient_init (SPLinearGradient * lg)
{
	lg->units = SP_GRADIENT_UNITS_USERSPACE;
	art_affine_identity (lg->transform);
	lg->box.x0 = lg->box.y0 = lg->box.y1 = 0.0;
	lg->box.x1 = 1.0;
	lg->spread = SP_GRADIENT_SPREAD_PAD;
	lg->stops = NULL;
}

static void
sp_lineargradient_destroy (GtkObject * object)
{
	SPLinearGradient * lg;
	SPObject * spobject;

	lg = (SPLinearGradient *) object;

	while (lg->stops) {
		spobject = SP_OBJECT (lg->stops->data);
		spobject->parent = NULL;
		gtk_object_unref (GTK_OBJECT (spobject));
		lg->stops = g_slist_remove_link (lg->stops, lg->stops);
	}

	if (GTK_OBJECT_CLASS (lg_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (lg_parent_class)->destroy) (object);
}

static void
sp_lineargradient_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPLinearGradient * lg;
	SPRepr * crepr;
	SPObject * child;
	const gchar * cname;
	const GList * l;

	lg = SP_LINEARGRADIENT (object);

	if (SP_OBJECT_CLASS (lg_parent_class)->build)
		(* SP_OBJECT_CLASS (lg_parent_class)->build) (object, document, repr);

	sp_lineargradient_read_attr (object, "gradientUnits");
	sp_lineargradient_read_attr (object, "gradientTransform");
	sp_lineargradient_read_attr (object, "x1");
	sp_lineargradient_read_attr (object, "y1");
	sp_lineargradient_read_attr (object, "x2");
	sp_lineargradient_read_attr (object, "y2");
	sp_lineargradient_read_attr (object, "spreadMethod");

	l = sp_repr_children (repr);

	while (l != NULL) {
		crepr = (SPRepr *) l->data;
		cname = sp_repr_name (crepr);
		if (strcmp (cname, "stop") == 0) {
			child = gtk_type_new (SP_TYPE_STOP);
			child->parent = object;
			lg->stops = g_slist_append (lg->stops, child);
			sp_object_invoke_build (child, document, crepr);
			l = l->next;
		}
	}
}

static void
sp_lineargradient_read_attr (SPObject * object, const gchar * key)
{
	SPLinearGradient * lg;

	lg = SP_LINEARGRADIENT (object);

	/* fixme: implement this */

	if (SP_OBJECT_CLASS (lg_parent_class)->read_attr)
		(* SP_OBJECT_CLASS (lg_parent_class)->read_attr) (object, key);
}

#define SP_UNITMENU_C

/*
 * SPUnitMenu
 *
 * Copyright (C) Lauris Kaplinski 2000
 *
 */

#include <gnome.h>
#include "unit-menu.h"

enum {
	SET_UNIT,
	LAST_SIGNAL
};

static void sp_unitmenu_class_init (SPUnitMenuClass * klass);
static void sp_unitmenu_init (SPUnitMenu * menu);
static void sp_unitmenu_destroy (GtkObject * object);

static void units_chosen (GtkMenuItem * menuitem, gpointer data);

static GtkOptionMenuClass * parent_class;
static guint unitmenu_signals[LAST_SIGNAL] = {0};

GtkType
sp_unitmenu_get_type (void)
{
	static GtkType unitmenu_type = 0;

	if (!unitmenu_type) {
		GtkTypeInfo unitmenu_info = {
			"SPUnitmenu",
			sizeof (SPUnitMenu),
			sizeof (SPUnitMenuClass),
			(GtkClassInitFunc) sp_unitmenu_class_init,
			(GtkObjectInitFunc) sp_unitmenu_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		unitmenu_type = gtk_type_unique (gtk_option_menu_get_type (), &unitmenu_info);
	}
	return unitmenu_type;
}

static void
sp_unitmenu_class_init (SPUnitMenuClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (gtk_option_menu_get_type ());

	unitmenu_signals[SET_UNIT] = gtk_signal_new ("set_unit",
						     GTK_RUN_LAST,
						     object_class->type,
						     GTK_SIGNAL_OFFSET (SPUnitMenuClass, set_unit),
						     gtk_marshal_NONE__INT_INT,
						     GTK_TYPE_NONE, 2,
						     GTK_TYPE_INT, GTK_TYPE_INT);

	gtk_object_class_add_signals (object_class, unitmenu_signals, LAST_SIGNAL);

	object_class->destroy = sp_unitmenu_destroy;
}

static void
sp_unitmenu_init (SPUnitMenu * menu)
{
	menu->system = SP_SVG_UNIT_ABSOLUTE;
	menu->metric = SP_MM;
}

static void
sp_unitmenu_destroy (GtkObject * object)
{
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

typedef struct {
	const gchar * plural, * singular, * abbreviation;
	SPSVGUnit system;
	SPMetric metric;
} SPUnitDesc;

SPUnitDesc unitdesc[] = {
	{N_("Millimeters"), N_("Millimeter"), N_("mm"), SP_SVG_UNIT_ABSOLUTE, SP_MM},
	{N_("Centimeters"), N_("Centimeter"), N_("cm"), SP_SVG_UNIT_ABSOLUTE, SP_CM},
	{N_("Inches"), N_("Inch"), N_("\""), SP_SVG_UNIT_ABSOLUTE, SP_IN},
	{N_("Points"), N_("Point"), N_("pt"), SP_SVG_UNIT_ABSOLUTE, SP_PT},
	{N_("Pixels"), N_("Pixel"), N_("px"), SP_SVG_UNIT_PIXELS, NONE},
	{N_("Userspace"), N_("Userspace"), N_("us"), SP_SVG_UNIT_USER, NONE},
	{N_("Percent"), N_("Percent"), N_("%"), SP_SVG_UNIT_PERCENT, NONE},
	{N_("Em squares"), N_("Em square"), N_("em"), SP_SVG_UNIT_EM, NONE},
	{N_("Ex squares"), N_("Ex square"), N_("ex"), SP_SVG_UNIT_EX, NONE},
};
#define n_unitdesc (sizeof (unitdesc) / sizeof (unitdesc[0]))

GtkWidget *
sp_unitmenu_new (guint systems, SPSVGUnit defaultsystem, SPMetric defaultmetric, SPUnitLabelType label)
{
	SPUnitMenu * u;
	GtkWidget * m;
	gint i, idx;

	u = gtk_type_new (SP_TYPE_UNITMENU);
	idx = 0;

	m = gtk_menu_new ();
	gtk_widget_show (m);

	for (i = 0; i < n_unitdesc; i++) {
		if (systems & unitdesc[i].system) {
		        GtkWidget * item = NULL;
			switch (label) {
			case SP_UNIT_SINGULAR:
			  item = gtk_menu_item_new_with_label (unitdesc[i].singular);
			  break;
			case SP_UNIT_PLURAL:
			  item = gtk_menu_item_new_with_label (unitdesc[i].plural);
			  break;
			case SP_UNIT_ABBREVIATION:
			  item = gtk_menu_item_new_with_label (unitdesc[i].abbreviation);
			  break;
			}
			gtk_widget_show (item);
			gtk_signal_connect (GTK_OBJECT (item), "activate",
					    GTK_SIGNAL_FUNC (units_chosen), GUINT_TO_POINTER (unitdesc[i].system | unitdesc[i].metric));
			gtk_object_set_data (GTK_OBJECT (item), "UnitMenu", u);
			gtk_menu_append (GTK_MENU (m), item);
			if ((defaultsystem | defaultmetric) == (unitdesc[i].system | unitdesc[i].metric)) {
				u->system = defaultsystem;
				u->metric = defaultmetric;
				idx = i;
			}
		}
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (u), m);
	gtk_option_menu_set_history (GTK_OPTION_MENU (u), idx);

	return GTK_WIDGET (u);
}

SPSVGUnit
sp_unitmenu_get_system (SPUnitMenu * unitmenu)
{
	g_return_val_if_fail (unitmenu != NULL, 0);
	g_return_val_if_fail (SP_IS_UNITMENU (unitmenu), 0);

	return unitmenu->system;
}

SPMetric
sp_unitmenu_get_metric (SPUnitMenu * unitmenu)
{
	g_return_val_if_fail (unitmenu != NULL, 0);
	g_return_val_if_fail (SP_IS_UNITMENU (unitmenu), 0);

	return unitmenu->metric;
}

static void
units_chosen (GtkMenuItem * menuitem, gpointer data)
{
	SPUnitMenu * u;

	u = gtk_object_get_data (GTK_OBJECT (menuitem), "UnitMenu");
	g_assert (u != NULL);
	g_assert (SP_IS_UNITMENU (u));

	u->system = GPOINTER_TO_UINT (data) & SP_SVG_UNIT_MASK;
	u->metric = GPOINTER_TO_UINT (data) & 0x0f;

	gtk_signal_emit (GTK_OBJECT (u), unitmenu_signals[SET_UNIT], u->system, u->metric);

	u->system = GPOINTER_TO_UINT (data) & SP_SVG_UNIT_MASK;
	u->metric = GPOINTER_TO_UINT (data) & 0x0f;
}

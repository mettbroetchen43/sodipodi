#define __SP_UNIT_MENU_C__

/*
 * SPUnitMenu
 *
 * Copyright (C) Lauris Kaplinski 2000
 *
 */

#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include "unit-menu.h"

struct _SPUnitSelector {
	GtkHBox box;

	GtkWidget *menu;

	guint bases;
	GSList *units;
	const SPUnit *unit;
	gdouble ctmscale, devicescale;
	guint plural : 1;
	guint abbr : 1;

	GSList *adjustments;
};

struct _SPUnitSelectorClass {
	GtkHBoxClass parent_class;
};

static void sp_unit_selector_class_init (SPUnitSelectorClass *klass);
static void sp_unit_selector_init (SPUnitSelector *selector);
static void sp_unit_selector_finalize (GtkObject *object);

static GtkHBoxClass *unit_selector_parent_class;

GtkType
sp_unit_selector_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		static const GtkTypeInfo info = {
			"SPUnitSelector",
			sizeof (SPUnitSelector),
			sizeof (SPUnitSelectorClass),
			(GtkClassInitFunc) sp_unit_selector_class_init,
			(GtkObjectInitFunc) sp_unit_selector_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_HBOX, &info);
	}
	return type;
}

static void
sp_unit_selector_class_init (SPUnitSelectorClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = GTK_OBJECT_CLASS (klass);
	widget_class = GTK_WIDGET_CLASS (klass);

	unit_selector_parent_class = gtk_type_class (GTK_TYPE_HBOX);

	object_class->finalize = sp_unit_selector_finalize;
}

static void
sp_unit_selector_init (SPUnitSelector *us)
{
	us->ctmscale = 1.0;
	us->devicescale = 1.0;
	us->abbr = FALSE;
	us->plural = TRUE;

	us->menu = gtk_option_menu_new ();
	gtk_widget_show (us->menu);
	gtk_box_pack_start (GTK_BOX (us), us->menu, TRUE, TRUE, 0);
}

static void
sp_unit_selector_finalize (GtkObject *object)
{
	SPUnitSelector *selector;

	selector = SP_UNIT_SELECTOR (object);

	if (selector->menu) {
		selector->menu = NULL;
	}

	while (selector->adjustments) {
		gtk_object_unref (GTK_OBJECT (selector->adjustments->data));
		selector->adjustments = g_slist_remove (selector->adjustments, selector->adjustments->data);
	}

	if (selector->units) {
		sp_unit_free_list (selector->units);
	}

	selector->unit = NULL;

	GTK_OBJECT_CLASS (unit_selector_parent_class)->finalize (object);
}

GtkWidget *
sp_unit_selector_new (guint bases)
{
	SPUnitSelector *us;

	us = gtk_type_new (SP_TYPE_UNIT_SELECTOR);

	sp_unit_selector_set_bases (us, bases);

	return (GtkWidget *) us;
}

const SPUnit *
sp_unit_selector_get_unit (SPUnitSelector *us)
{
	g_return_val_if_fail (us != NULL, NULL);
	g_return_val_if_fail (SP_IS_UNIT_SELECTOR (us), NULL);

	return us->unit;
}

static void
spus_unit_activate (GtkWidget *widget, SPUnitSelector *us)
{
	const SPUnit *unit, *old;
	GSList *l;

	unit = gtk_object_get_data (GTK_OBJECT (widget), "unit");
	g_return_if_fail (unit != NULL);

	g_print ("Old unit %s new unit %s\n", us->unit->name, unit->name);

	old = us->unit;
	us->unit = unit;

	/* Recalculate adjustments */
	for (l = us->adjustments; l != NULL; l = l->next) {
		GtkAdjustment *adj;
		gdouble val;
		adj = GTK_ADJUSTMENT (l->data);
		val = adj->value;
		g_print ("Old val %g ... ", val);
		sp_convert_distance_full (&val, old, unit, us->ctmscale, us->devicescale);
		g_print ("new val %g\n", val);
		gtk_adjustment_set_value (adj, val);
	}
}

static void
spus_rebuild_menu (SPUnitSelector *us)
{
	GtkWidget *m, *i;
	GSList *l;
	gint pos, p;

	if (GTK_OPTION_MENU (us->menu)->menu) {
		gtk_option_menu_remove_menu (GTK_OPTION_MENU (us->menu));
	}

	m = gtk_menu_new ();
	gtk_widget_show (m);

	pos = p = 0;
	for (l = us->units; l != NULL; l = l->next) {
		const SPUnit *u;
		u = l->data;
		i = gtk_menu_item_new_with_label ((us->abbr) ? (us->plural) ? u->abbr_plural : u->abbr : (us->plural) ? u->plural : u->name);
		gtk_object_set_data (GTK_OBJECT (i), "unit", (gpointer) u);
		gtk_signal_connect (GTK_OBJECT (i), "activate", GTK_SIGNAL_FUNC (spus_unit_activate), us);
		gtk_widget_show (i);
		gtk_menu_shell_append (GTK_MENU_SHELL (m), i);
		if (u == us->unit) pos = p;
		p += 1;
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (us->menu), m);

	gtk_option_menu_set_history (GTK_OPTION_MENU (us->menu), pos);
}

void
sp_unit_selector_set_bases (SPUnitSelector *us, guint bases)
{
	GSList *units;

	g_return_if_fail (us != NULL);
	g_return_if_fail (SP_IS_UNIT_SELECTOR (us));

	if (bases == us->bases) return;

	units = sp_unit_get_list (bases);
	g_return_if_fail (units != NULL);
	sp_unit_free_list (us->units);
	us->units = units;
	us->unit = units->data;

	spus_rebuild_menu (us);
}

void
sp_unit_selector_set_unit (SPUnitSelector *us, const SPUnit *unit)
{
	const SPUnit *old;
	GSList *l;
	gint pos;

	g_return_if_fail (us != NULL);
	g_return_if_fail (SP_IS_UNIT_SELECTOR (us));
	g_return_if_fail (unit != NULL);

	if (unit == us->unit) return;

	pos = g_slist_index (us->units, (gpointer) unit);
	g_return_if_fail (pos >= 0);

	gtk_option_menu_set_history (GTK_OPTION_MENU (us->menu), pos);

	old = us->unit;
	us->unit = unit;

	/* Recalculate adjustments */
	for (l = us->adjustments; l != NULL; l = l->next) {
		GtkAdjustment *adj;
		gdouble val;
		adj = GTK_ADJUSTMENT (l->data);
		val = adj->value;
		sp_convert_distance_full (&val, old, unit, us->ctmscale, us->devicescale);
		gtk_adjustment_set_value (adj, val);
	}
}

void
sp_unit_selector_add_adjustment (SPUnitSelector *us, GtkAdjustment *adj)
{
	g_return_if_fail (us != NULL);
	g_return_if_fail (SP_IS_UNIT_SELECTOR (us));
	g_return_if_fail (adj != NULL);
	g_return_if_fail (GTK_IS_ADJUSTMENT (adj));

	g_return_if_fail (!g_slist_find (us->adjustments, adj));

	gtk_object_ref (GTK_OBJECT (adj));
	us->adjustments = g_slist_prepend (us->adjustments, adj);
}

void
sp_unit_selector_remove_adjustment (SPUnitSelector *us, GtkAdjustment *adj)
{
	g_return_if_fail (us != NULL);
	g_return_if_fail (SP_IS_UNIT_SELECTOR (us));
	g_return_if_fail (adj != NULL);
	g_return_if_fail (GTK_IS_ADJUSTMENT (adj));

	g_return_if_fail (g_slist_find (us->adjustments, adj));

	us->adjustments = g_slist_remove (us->adjustments, adj);
	gtk_object_unref (GTK_OBJECT (adj));
}


#if 0

/* Old unit menu */

enum {
	SET_UNIT,
	LAST_SIGNAL
};

static void sp_unitmenu_class_init (SPUnitMenuClass *klass);
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

#endif

#define __SP_UNIT_MENU_C__

/*
 * Unit selector with autupdate capability
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000-2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define noUNIT_SELECTOR_VERBOSE

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

	guint update : 1;

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

#ifdef UNIT_SELECTOR_VERBOSE
	g_print ("Old unit %s new unit %s\n", us->unit->name, unit->name);
#endif

	old = us->unit;
	us->unit = unit;

	us->update = TRUE;

	/* Recalculate adjustments */
	for (l = us->adjustments; l != NULL; l = l->next) {
		GtkAdjustment *adj;
		gdouble val;
		adj = GTK_ADJUSTMENT (l->data);
		val = adj->value;
#ifdef UNIT_SELECTOR_VERBOSE
		g_print ("Old val %g ... ", val);
#endif
		sp_convert_distance_full (&val, old, unit, us->ctmscale, us->devicescale);
#ifdef UNIT_SELECTOR_VERBOSE
		g_print ("new val %g\n", val);
#endif
		gtk_adjustment_set_value (adj, val);
	}

	us->update = FALSE;
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

gboolean
sp_unit_selector_update_test (SPUnitSelector *selector)
{
	g_return_val_if_fail (selector != NULL, FALSE);
	g_return_val_if_fail (SP_IS_UNIT_SELECTOR (selector), FALSE);

	return selector->update;
}


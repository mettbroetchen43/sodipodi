#ifndef __SP_UNIT_MENU_H__
#define __SP_UNIT_MENU_H__

/*
 * SPUnitMenu
 *
 * Generic (and quite unintelligent) grid item for gnome canvas
 *
 * Copyright (C) Lauris Kaplinski 2000
 *
 */

#include <gtk/gtkoptionmenu.h>
#include <libgnome/gnome-defs.h>
#include "units.h"

BEGIN_GNOME_DECLS

/* Unit selector Widget */

#define SP_TYPE_UNIT_SELECTOR (sp_unit_selector_get_type ())
#define SP_UNIT_SELECTOR(o) (GTK_CHECK_CAST ((o), SP_TYPE_UNIT_SELECTOR, SPUnitSelector))
#define SP_UNIT_SELECTOR_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_UNIT_SELECTOR, SPUnitSelectorClass))
#define SP_IS_UNIT_SELECTOR(o) (GTK_CHECK_TYPE ((o), SP_TYPE_UNIT_SELECTOR))
#define SP_IS_UNIT_SELECTOR_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_UNIT_SELECTOR))

typedef struct _SPUnitSelector SPUnitSelector;
typedef struct _SPUnitSelectorClass SPUnitSelectorClass;

GtkType sp_unit_selector_get_type (void);

GtkWidget *sp_unit_selector_new (guint bases);

const SPUnit *sp_unit_selector_get_unit (SPUnitSelector *selector);

void sp_unit_selector_set_bases (SPUnitSelector *selector, guint bases);
void sp_unit_selector_set_unit (SPUnitSelector *selector, const SPUnit *unit);
void sp_unit_selector_add_adjustment (SPUnitSelector *selector, GtkAdjustment *adjustment);
void sp_unit_selector_remove_adjustment (SPUnitSelector *selector, GtkAdjustment *adjustment);


#if 0

#define SP_TYPE_UNITMENU            (sp_unitmenu_get_type ())
#define SP_UNITMENU(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_UNITMENU, SPUnitMenu))
#define SP_UNITMENU_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_UNITMENU, SPUnitMenuClass))
#define SP_IS_UNITMENU(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_UNITMENU))
#define SP_IS_UNITMENU_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_UNITMENU))


typedef struct _SPUnitMenu SPUnitMenu;
typedef struct _SPUnitMenuClass SPUnitMenuClass;

struct _SPUnitMenu {
	GtkOptionMenu optionmenu;
	SPSVGUnit system;
	SPMetric metric;
};

struct _SPUnitMenuClass {
	GtkOptionMenuClass parent_class;
	void (* set_unit) (SPUnitMenu * unitmenu, SPSVGUnit system, SPMetric metric);
};


typedef enum {
	SP_UNIT_SINGULAR,
	SP_UNIT_PLURAL,
	SP_UNIT_ABBREVIATION
} SPUnitLabelType;


/* Standard Gtk function */
GtkType sp_unitmenu_get_type (void);

GtkWidget * sp_unitmenu_new (guint systems, SPSVGUnit defaultsystem, SPMetric defaultmetric, SPUnitLabelType label);
SPSVGUnit sp_unitmenu_get_system (SPUnitMenu * unitmenu);
SPMetric sp_unitmenu_get_metric (SPUnitMenu * unitmenu);

#endif

END_GNOME_DECLS

#endif







#ifndef SP_UNITMENU_H
#define SP_UNITMENU_H

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
#include "../svg/svg.h"
#include "../sp-metrics.h"

BEGIN_GNOME_DECLS

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


/* Standard Gtk function */
GtkType sp_unitmenu_get_type (void);

GtkWidget * sp_unitmenu_new (guint systems, SPSVGUnit defaultsystem, SPMetric defaultmetric, gboolean plural);
SPSVGUnit sp_unitmenu_get_system (SPUnitMenu * unitmenu);
SPMetric sp_unitmenu_get_metric (SPUnitMenu * unitmenu);

END_GNOME_DECLS

#endif







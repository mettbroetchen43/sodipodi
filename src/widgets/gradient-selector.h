#ifndef __SP_GRADIENT_SELECTOR_H__
#define __SP_GRADIENT_SELECTOR_H__

/*
 * A gradient paint style widget
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

typedef struct _SPGradientSelector SPGradientSelector;
typedef struct _SPGradientSelectorClass SPGradientSelectorClass;

#define SP_TYPE_GRADIENT_SELECTOR (sp_gradient_selector_get_type ())
#define SP_GRADIENT_SELECTOR(o) (GTK_CHECK_CAST ((o), SP_TYPE_GRADIENT_SELECTOR, SPGradientSelector))
#define SP_GRADIENT_SELECTOR_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_GRADIENT_SELECTOR, SPGradientSelectorClass))
#define SP_IS_GRADIENT_SELECTOR(o) (GTK_CHECK_TYPE ((o), SP_TYPE_GRADIENT_SELECTOR))
#define SP_IS_GRADIENT_SELECTOR_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_GRADIENT_SELECTOR))

#include <gtk/gtkvbox.h>

#include "../forward.h"

struct _SPGradientSelector {
	GtkVBox vbox;

	/* Vector selector */
	GtkWidget *vectors;
	/* Editing buttons */
	GtkWidget *edit, *add;
	/* Position widget */
	GtkWidget *position;
	/* Reset button */
	GtkWidget *reset;
};

struct _SPGradientSelectorClass {
	GtkVBoxClass parent_class;

	void (* grabbed) (SPGradientSelector *sel);
	void (* dragged) (SPGradientSelector *sel);
	void (* released) (SPGradientSelector *sel);
	void (* changed) (SPGradientSelector *sel);
};

GtkType sp_gradient_selector_get_type (void);

GtkWidget *sp_gradient_selector_new (void);

void sp_gradient_selector_set_vector (SPGradientSelector *sel, SPDocument *doc, SPGradient *vector);

void sp_gradient_selector_set_gradient_bbox (SPGradientSelector *sel, gdouble x0, gdouble y0, gdouble x1, gdouble y1);
void sp_gradient_selector_set_gradient_position (SPGradientSelector *sel, gdouble x0, gdouble y0, gdouble x1, gdouble y1);

SPGradient *sp_gradient_selector_get_vector (SPGradientSelector *sel);
void sp_gradient_selector_get_position_floatv (SPGradientSelector *gsel, gfloat *pos);

END_GNOME_DECLS

#endif

#ifndef __SP_GRADIENT_VECTOR_H__
#define __SP_GRADIENT_VECTOR_H__

/*
 * Gradient vector selector and editor
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#include <gtk/gtkvbox.h>
#include "../forward.h"

typedef struct _SPGradientVectorSelector SPGradientVectorSelector;
typedef struct _SPGradientVectorSelectorClass SPGradientVectorSelectorClass;

#define SP_TYPE_GRADIENT_VECTOR_SELECTOR (sp_gradient_vector_selector_get_type ())
#define SP_GRADIENT_VECTOR_SELECTOR(o) (GTK_CHECK_CAST ((o), SP_TYPE_GRADIENT_VECTOR_SELECTOR, SPGradientVectorSelector))
#define SP_GRADIENT_VECTOR_SELECTOR_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_GRADIENT_VECTOR_SELECTOR, SPGradientVectorSelectorClass))
#define SP_IS_GRADIENT_VECTOR_SELECTOR(o) (GTK_CHECK_TYPE ((o), SP_TYPE_GRADIENT_VECTOR_SELECTOR))
#define SP_IS_GRADIENT_VECTOR_SELECTOR_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_GRADIENT_VECTOR_SELECTOR))

struct _SPGradientVectorSelector {
	GtkVBox vbox;

	/* Our gradient */
	SPGradient *gr;
	/* Parent document <defs> node */
	SPObject *defs;

	/* Vector menu */
	GtkWidget *menu;
	/* Buttons */
	GtkWidget *chg, *add, *del;
};

struct _SPGradientVectorSelectorClass {
	GtkVBoxClass parent_class;

	void (* vector_set) (SPGradientVectorSelector *gvs, SPGradient *gr);
};

GtkType sp_gradient_vector_selector_get_type (void);

GtkWidget *sp_gradient_vector_selector_new (SPGradient *gradient);

void sp_gradient_vector_selector_set_gradient (SPGradientVectorSelector *gvs, SPGradient *gr);

/* fixme: rethink this (Lauris) */
void sp_gradient_vector_dialog (SPGradient *gradient);

END_GNOME_DECLS

#endif

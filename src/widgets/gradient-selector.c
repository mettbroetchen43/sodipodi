#define __SP_GRADIENT_SELECTOR_C__

/*
 * A simple OptionMenu for choosing gradient vectors visually
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <config.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkbutton.h>

#include "../document.h"
#include "../document-private.h"
#include "../gradient-chemistry.h"

#include "gradient-vector.h"
#include "gradient-position.h"

#include "gradient-selector.h"

enum {
	GRABBED,
	DRAGGED,
	RELEASED,
	CHANGED,
	LAST_SIGNAL
};

static void sp_gradient_selector_class_init (SPGradientSelectorClass *klass);
static void sp_gradient_selector_init (SPGradientSelector *selector);
static void sp_gradient_selector_destroy (GtkObject *object);

/* Signal handlers */
static void sp_gradient_selector_vector_set (SPGradientVectorSelector *gvs, SPGradient *gr, SPGradientSelector *sel);
static void sp_gradient_selector_position_dragged (SPGradientPosition *pos, SPGradientSelector *sel);
static void sp_gradient_selector_position_changed (SPGradientPosition *pos, SPGradientSelector *sel);
static void sp_gradient_selector_edit_vector_clicked (GtkWidget *w, SPGradientSelector *sel);
static void sp_gradient_selector_add_vector_clicked (GtkWidget *w, SPGradientSelector *sel);

static GtkVBoxClass *parent_class;
static guint signals[LAST_SIGNAL] = {0};

GtkType
sp_gradient_selector_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPGradientSelector",
			sizeof (SPGradientSelector),
			sizeof (SPGradientSelectorClass),
			(GtkClassInitFunc) sp_gradient_selector_class_init,
			(GtkObjectInitFunc) sp_gradient_selector_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_VBOX, &info);
	}
	return type;
}

static void
sp_gradient_selector_class_init (SPGradientSelectorClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (GTK_TYPE_VBOX);

	signals[GRABBED] =  gtk_signal_new ("grabbed",
					    GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
					    object_class->type,
					    GTK_SIGNAL_OFFSET (SPGradientSelectorClass, grabbed),
					    gtk_marshal_NONE__NONE,
					    GTK_TYPE_NONE, 0);
	signals[DRAGGED] =  gtk_signal_new ("dragged",
					    GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
					    object_class->type,
					    GTK_SIGNAL_OFFSET (SPGradientSelectorClass, dragged),
					    gtk_marshal_NONE__NONE,
					    GTK_TYPE_NONE, 0);
	signals[RELEASED] = gtk_signal_new ("released",
					    GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
					    object_class->type,
					    GTK_SIGNAL_OFFSET (SPGradientSelectorClass, released),
					    gtk_marshal_NONE__NONE,
					    GTK_TYPE_NONE, 0);
	signals[CHANGED] =  gtk_signal_new ("changed",
					    GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
					    object_class->type,
					    GTK_SIGNAL_OFFSET (SPGradientSelectorClass, changed),
					    gtk_marshal_NONE__NONE,
					    GTK_TYPE_NONE, 0);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	object_class->destroy = sp_gradient_selector_destroy;
}

static void
sp_gradient_selector_init (SPGradientSelector *sel)
{
	GtkWidget *hb, *bb;

	sel->mode = SP_GRADIENT_SELECTOR_MODE_LINEAR;

	/* Create box for vector & buttons */
	hb = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hb);
	gtk_box_pack_start (GTK_BOX (sel), hb, FALSE, FALSE, 4);

	sel->vectors = sp_gradient_vector_selector_new (NULL, NULL);
	gtk_widget_show (sel->vectors);
	gtk_box_pack_start (GTK_BOX (hb), sel->vectors, TRUE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (sel->vectors), "vector_set", GTK_SIGNAL_FUNC (sp_gradient_selector_vector_set), sel);

	/* Create box for buttons */
	bb = gtk_hbox_new (TRUE, 0);
	gtk_widget_show (bb);
	gtk_box_pack_start (GTK_BOX (hb), bb, TRUE, TRUE, 0);

	sel->edit = gtk_button_new_with_label (_("Edit"));
	gtk_widget_show (sel->edit);
	gtk_box_pack_start (GTK_BOX (bb), sel->edit, TRUE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (sel->edit), "clicked", GTK_SIGNAL_FUNC (sp_gradient_selector_edit_vector_clicked), sel);
	gtk_widget_set_sensitive (sel->edit, FALSE);

	sel->add = gtk_button_new_with_label (_("Add"));
	gtk_widget_show (sel->add);
	gtk_box_pack_start (GTK_BOX (bb), sel->add, TRUE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (sel->add), "clicked", GTK_SIGNAL_FUNC (sp_gradient_selector_add_vector_clicked), sel);
	gtk_widget_set_sensitive (sel->add, FALSE);

	/* Create gradient position widget */
	sel->position = sp_gradient_position_new (NULL);
	gtk_widget_show (sel->position);
	gtk_box_pack_start (GTK_BOX (sel), sel->position, TRUE, TRUE, 4);
	gtk_signal_connect (GTK_OBJECT (sel->position), "dragged", GTK_SIGNAL_FUNC (sp_gradient_selector_position_dragged), sel);
	gtk_signal_connect (GTK_OBJECT (sel->position), "changed", GTK_SIGNAL_FUNC (sp_gradient_selector_position_changed), sel);
}

static void
sp_gradient_selector_destroy (GtkObject *object)
{
	SPGradientSelector *sel;

	sel = SP_GRADIENT_SELECTOR (object);

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

GtkWidget *
sp_gradient_selector_new (void)
{
	SPGradientSelector *sel;

	sel = gtk_type_new (SP_TYPE_GRADIENT_SELECTOR);

	return (GtkWidget *) sel;
}

void
sp_gradient_selector_set_mode (SPGradientSelector *sel, guint mode)
{
	g_return_if_fail (sel != NULL);
	g_return_if_fail (SP_IS_GRADIENT_SELECTOR (sel));

	sel->mode = mode;

	sp_gradient_position_set_mode (SP_GRADIENT_POSITION (sel->position), mode);
}

void
sp_gradient_selector_set_vector (SPGradientSelector *sel, SPDocument *doc, SPGradient *vector)
{
	g_return_if_fail (sel != NULL);
	g_return_if_fail (SP_IS_GRADIENT_SELECTOR (sel));
	g_return_if_fail (!doc || SP_IS_DOCUMENT (doc));
	g_return_if_fail (!vector || (doc != NULL));
	g_return_if_fail (!vector || SP_IS_GRADIENT (vector));
	g_return_if_fail (!vector || (SP_OBJECT_DOCUMENT (vector) == doc));
	g_return_if_fail (!vector || SP_GRADIENT_HAS_STOPS (vector));

	sp_gradient_vector_selector_set_gradient (SP_GRADIENT_VECTOR_SELECTOR (sel->vectors), doc, vector);
	sp_gradient_position_set_gradient (SP_GRADIENT_POSITION (sel->position), vector);

	if (vector) {
		gtk_widget_set_sensitive (sel->edit, TRUE);
		gtk_widget_set_sensitive (sel->add, TRUE);
	} else {
		gtk_widget_set_sensitive (sel->edit, FALSE);
		gtk_widget_set_sensitive (sel->add, (doc != NULL));
	}
}

void
sp_gradient_selector_set_bbox (SPGradientSelector *sel, gdouble x0, gdouble y0, gdouble x1, gdouble y1)
{
	g_return_if_fail (sel != NULL);
	g_return_if_fail (SP_IS_GRADIENT_SELECTOR (sel));

	sp_gradient_position_set_bbox (SP_GRADIENT_POSITION (sel->position), x0, y0, x1, y1);
}

void
sp_gradient_selector_set_lgradient_position (SPGradientSelector *sel, gdouble x0, gdouble y0, gdouble x1, gdouble y1)
{
	g_return_if_fail (sel != NULL);
	g_return_if_fail (SP_IS_GRADIENT_SELECTOR (sel));
	g_return_if_fail (sel->mode == SP_GRADIENT_SELECTOR_MODE_LINEAR);

	sp_gradient_position_set_vector (SP_GRADIENT_POSITION (sel->position), x0, y0, x1, y1);
}

void
sp_gradient_selector_set_rgradient_position (SPGradientSelector *sel, gdouble cx, gdouble cy, gdouble fx, gdouble fy, gdouble r)
{
	g_return_if_fail (sel != NULL);
	g_return_if_fail (SP_IS_GRADIENT_SELECTOR (sel));
	g_return_if_fail (sel->mode == SP_GRADIENT_SELECTOR_MODE_RADIAL);

	sp_gradient_position_set_vector (SP_GRADIENT_POSITION (sel->position), cx, cy, cx + r, cy);
}

SPGradient *
sp_gradient_selector_get_vector (SPGradientSelector *sel)
{
	g_return_val_if_fail (sel != NULL, NULL);
	g_return_val_if_fail (SP_IS_GRADIENT_SELECTOR (sel), NULL);

	/* fixme: */
	return SP_GRADIENT_VECTOR_SELECTOR (sel->vectors)->gr;
}

void
sp_gradient_selector_get_lgradient_position_floatv (SPGradientSelector *gsel, gfloat *pos)
{
	g_return_if_fail (gsel != NULL);
	g_return_if_fail (SP_IS_GRADIENT_SELECTOR (gsel));
	g_return_if_fail (gsel->mode == SP_GRADIENT_SELECTOR_MODE_LINEAR);

	sp_gradient_position_get_position_floatv (SP_GRADIENT_POSITION (gsel->position), pos);
}

void
sp_gradient_selector_get_rgradient_position_floatv (SPGradientSelector *gsel, gfloat *pos)
{
	gfloat p[4];

	g_return_if_fail (gsel != NULL);
	g_return_if_fail (SP_IS_GRADIENT_SELECTOR (gsel));
	g_return_if_fail (gsel->mode == SP_GRADIENT_SELECTOR_MODE_RADIAL);

	sp_gradient_position_get_position_floatv (SP_GRADIENT_POSITION (gsel->position), p);

	pos[0] = p[0];
	pos[1] = p[1];
	pos[2] = p[0];
	pos[3] = p[1];
	pos[4] = hypot (p[2] - p[0], p[3] - p[1]);
}

static void
sp_gradient_selector_vector_set (SPGradientVectorSelector *gvs, SPGradient *gr, SPGradientSelector *sel)
{
	static gboolean blocked = FALSE;

	if (!blocked) {
		blocked = TRUE;
		gr = sp_gradient_ensure_vector_normalized (gr);
		sp_gradient_selector_set_vector (sel, (gr) ? SP_OBJECT_DOCUMENT (gr) : NULL, gr);
		gtk_signal_emit (GTK_OBJECT (sel), signals[CHANGED], gr);
		blocked = FALSE;
	}
}

static void
sp_gradient_selector_position_dragged (SPGradientPosition *pos, SPGradientSelector *sel)
{
	gtk_signal_emit (GTK_OBJECT (sel), signals[DRAGGED]);
}

static void
sp_gradient_selector_position_changed (SPGradientPosition *pos, SPGradientSelector *sel)
{
	gtk_signal_emit (GTK_OBJECT (sel), signals[CHANGED]);
}

static void
sp_gradient_selector_edit_vector_clicked (GtkWidget *w, SPGradientSelector *sel)
{
	GtkWidget *dialog;

	/* fixme: */
	dialog = sp_gradient_vector_editor_new (SP_GRADIENT_VECTOR_SELECTOR (sel->vectors)->gr);

	gtk_widget_show (dialog);
}

static void
sp_gradient_selector_add_vector_clicked (GtkWidget *w, SPGradientSelector *sel)
{
	SPDocument *doc;
	SPGradient *gr;
	SPRepr *repr;

	doc = sp_gradient_vector_selector_get_document (SP_GRADIENT_VECTOR_SELECTOR (sel->vectors));
	if (!doc) return;
	gr = sp_gradient_vector_selector_get_gradient (SP_GRADIENT_VECTOR_SELECTOR (sel->vectors));

	if (gr) {
		repr = sp_repr_duplicate (SP_OBJECT_REPR (gr));
	} else {
		SPRepr *stop;
		repr = sp_repr_new ("linearGradient");
		stop = sp_repr_new ("stop");
		sp_repr_set_attr (stop, "offset", "0");
		sp_repr_set_attr (stop, "style", "stop-color:#000;stop-opacity:1;");
		sp_repr_append_child (repr, stop);
		sp_repr_unref (stop);
		stop = sp_repr_new ("stop");
		sp_repr_set_attr (stop, "offset", "1");
		sp_repr_set_attr (stop, "style", "stop-color:#fff;stop-opacity:1;");
		sp_repr_append_child (repr, stop);
		sp_repr_unref (stop);
	}

	sp_repr_add_child (SP_OBJECT_REPR (SP_DOCUMENT_DEFS (doc)), repr, NULL);
	sp_repr_unref (repr);

	gr = (SPGradient *) sp_document_lookup_id (doc, sp_repr_attr (repr, "id"));
	sp_gradient_vector_selector_set_gradient (SP_GRADIENT_VECTOR_SELECTOR (sel->vectors), doc, gr);
}


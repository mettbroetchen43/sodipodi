#define __SP_GRADIENT_VECTOR_C__

/*
 * Gradient vector selector and editor
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtktable.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "../widgets/gradient-image.h"
#include "../document-private.h"
#include "../sp-gradient.h"
#include "../gradient-chemistry.h"
#include "gradient-vector.h"

enum {
	VECTOR_SET,
	LAST_SIGNAL
};
static void sp_gradient_vector_selector_class_init (SPGradientVectorSelectorClass *klass);
static void sp_gradient_vector_selector_init (SPGradientVectorSelector *gvs);
static void sp_gradient_vector_selector_destroy (GtkObject *object);

static void sp_gvs_gradient_destroy (SPGradient *gr, SPGradientVectorSelector *gvs);
static void sp_gvs_defs_destroy (SPObject *defs, SPGradientVectorSelector *gvs);
static void sp_gvs_defs_modified (SPObject *defs, guint flags, SPGradientVectorSelector *gvs);

static void sp_gvs_rebuild_gui_full (SPGradientVectorSelector *gvs);
static void sp_gvs_gradient_activate (GtkMenuItem *mi, SPGradientVectorSelector *gvs);

static GtkVBoxClass *parent_class;
static guint signals[LAST_SIGNAL] = {0};

GtkType
sp_gradient_vector_selector_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPGradientVectorSelector",
			sizeof (SPGradientVectorSelector),
			sizeof (SPGradientVectorSelectorClass),
			(GtkClassInitFunc) sp_gradient_vector_selector_class_init,
			(GtkObjectInitFunc) sp_gradient_vector_selector_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_VBOX, &info);
	}
	return type;
}

static void
sp_gradient_vector_selector_class_init (SPGradientVectorSelectorClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);

	parent_class = gtk_type_class (GTK_TYPE_VBOX);

	signals[VECTOR_SET] = gtk_signal_new ("vector_set",
					      GTK_RUN_LAST,
					      object_class->type,
					      GTK_SIGNAL_OFFSET (SPGradientVectorSelectorClass, vector_set),
					      gtk_marshal_NONE__POINTER,
					      GTK_TYPE_NONE, 1,
					      GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	object_class->destroy = sp_gradient_vector_selector_destroy;
}

static void
sp_gradient_vector_selector_init (SPGradientVectorSelector *gvs)
{
	GtkWidget *t;

	t = gtk_table_new (2, 3, FALSE);
	gtk_widget_show (t);
	gtk_box_pack_start (GTK_BOX (gvs), t, FALSE, FALSE, 0);

	gvs->menu = gtk_option_menu_new ();
	gtk_widget_show (gvs->menu);
	gtk_table_attach (GTK_TABLE (t), gvs->menu, 0, 3, 0, 1, 0, 0, 0, 0);

	gvs->chg = gtk_button_new_with_label (_("Edit"));
	gtk_widget_show (gvs->chg);
	gtk_table_attach (GTK_TABLE (t), gvs->chg, 0, 1, 1, 2, 0, 0, 0, 0);

	gvs->add = gtk_button_new_with_label (_("Add"));
	gtk_widget_show (gvs->add);
	gtk_table_attach (GTK_TABLE (t), gvs->add, 1, 2, 1, 2, 0, 0, 0, 0);

	gvs->del = gtk_button_new_with_label (_("Delete"));
	gtk_widget_show (gvs->del);
	gtk_table_attach (GTK_TABLE (t), gvs->del, 2, 3, 1, 2, 0, 0, 0, 0);
}

static void
sp_gradient_vector_selector_destroy (GtkObject *object)
{
	SPGradientVectorSelector *gvs;

	gvs = SP_GRADIENT_VECTOR_SELECTOR (object);

	if (gvs->gr) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (gvs->gr), gvs);
		gvs->gr = NULL;
	}

	if (gvs->defs) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (gvs->defs), gvs);
		gvs->defs = NULL;
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

GtkWidget *
sp_gradient_vector_selector_new (SPGradient *gradient)
{
	GtkWidget *gvs;

	/* fixme: Do we allow zero gradient? (Lauris) */
	g_return_val_if_fail (!gradient || SP_IS_GRADIENT (gradient), NULL);

	gvs = gtk_type_new (SP_TYPE_GRADIENT_VECTOR_SELECTOR);

	sp_gradient_vector_selector_set_gradient (SP_GRADIENT_VECTOR_SELECTOR (gvs), gradient);

	return gvs;
}

void
sp_gradient_vector_selector_set_gradient (SPGradientVectorSelector *gvs, SPGradient *gr)
{
	SPObject *defs;

	g_return_if_fail (gvs != NULL);
	g_return_if_fail (SP_IS_GRADIENT_VECTOR_SELECTOR (gvs));
	/* fixme: Do we allow zero gradient? (Lauris) */
	g_return_if_fail (!gr || SP_IS_GRADIENT (gr));

	/* fixme: Check for has_stops? (Lauris) */

	defs = (gr) ? SP_DOCUMENT_DEFS (SP_OBJECT_DOCUMENT (gr)) : NULL;

	if ((!gr && gvs->gr) || (gr && !gvs->gr) || (gr && (SP_OBJECT_DOCUMENT (gr) != SP_OBJECT_DOCUMENT (gvs->gr)))) {
		/* Somewhat easier case - switch document */
		/* Detach old */
		if (gvs->gr) {
			gtk_signal_disconnect_by_data (GTK_OBJECT (gvs->gr), gvs);
			gvs->gr = NULL;
		}
		if (gvs->defs) {
			gtk_signal_disconnect_by_data (GTK_OBJECT (gvs->defs), gvs);
			gvs->defs = NULL;
		}
		/* Attach new */
		if (defs) {
			gtk_signal_connect (GTK_OBJECT (defs), "destroy", GTK_SIGNAL_FUNC (sp_gvs_defs_destroy), gvs);
			gtk_signal_connect (GTK_OBJECT (defs), "modified", GTK_SIGNAL_FUNC (sp_gvs_defs_modified), gvs);
			gvs->defs = defs;
		}
		if (gr) {
			gtk_signal_connect (GTK_OBJECT (gr), "destroy", GTK_SIGNAL_FUNC (sp_gvs_gradient_destroy), gvs);
			/* fixme: Connect 'modified'? (Lauris) */
			/* fixme: I think we do not need it (Lauris) */
			gvs->gr = gr;
		}
		sp_gvs_rebuild_gui_full (gvs);
	} else if (gr != NULL) {
		/* Harder case - keep document, rebuild menus and stuff */
		/* fixme: (Lauris) */
		sp_gradient_vector_selector_set_gradient (gvs, NULL);
		sp_gradient_vector_selector_set_gradient (gvs, gr);
	}
	/* The case of setting NULL -> NULL is not very interesting */
}

static void
sp_gvs_rebuild_gui_full (SPGradientVectorSelector *gvs)
{
	GtkWidget *m;
	GSList *gl;

	/* Clear old menu, if there is any */
	if (gtk_option_menu_get_menu (GTK_OPTION_MENU (gvs->menu))) {
		gtk_option_menu_remove_menu (GTK_OPTION_MENU (gvs->menu));
	}

	/* Create new menu widget */
	m = gtk_menu_new ();
	gtk_widget_show (m);

	/* Pick up all gradients with vectors */
	gl = NULL;
	if (gvs->gr) {
		const GSList *gradients, *l;
		gradients = sp_document_get_resource_list (SP_OBJECT_DOCUMENT (gvs->gr), "gradient");
		for (l = gradients; l != NULL; l = l->next) {
			if (SP_GRADIENT_HAS_STOPS (l->data)) {
				gl = g_slist_prepend (gl, l->data);
			}
		}
	}
	gl = g_slist_reverse (gl);

	if (!gl || !gvs->gr) {
		GtkWidget *i;
		/* No document or no gradients */
		i = gtk_menu_item_new_with_label (_("No gradients defined"));
		gtk_widget_show (i);
		gtk_menu_append (GTK_MENU (m), i);
		/* Set sensitivity */
		/* fixme: */
		gtk_widget_set_sensitive (gvs->menu, FALSE);
		gtk_widget_set_sensitive (gvs->chg, FALSE);
		gtk_widget_set_sensitive (gvs->del, FALSE);
	} else {
		while (gl) {
			SPGradient *gr;
			GtkWidget *w, *i, *hb, *g;
			gr = SP_GRADIENT (gl->data);
			gl = g_slist_remove (gl, gr);

			/* We have to know: */
			/* Gradient destroy */
			/* Gradient name change */
			i = gtk_menu_item_new ();
			gtk_widget_show (i);
			gtk_object_set_data (GTK_OBJECT (i), "gradient", gr);
			gtk_signal_connect (GTK_OBJECT (i), "activate",
					    GTK_SIGNAL_FUNC (sp_gvs_gradient_activate), gvs);

			hb = gtk_hbox_new (FALSE, 4);
			gtk_widget_show (hb);
			gtk_container_add (GTK_CONTAINER (i), hb);

			w = gtk_label_new (SP_OBJECT_ID (gr));
			gtk_widget_show (w);
			gtk_box_pack_start (GTK_BOX (hb), w, TRUE, TRUE, 4);

			g = sp_gradient_image_new (gr);
			gtk_widget_show (g);
			gtk_box_pack_start (GTK_BOX (hb), g, FALSE, FALSE, 4);

			gtk_menu_append (GTK_MENU (m), i);
		}
		/* Set sensitivity */
		/* fixme: */
		gtk_widget_set_sensitive (gvs->menu, TRUE);
		gtk_widget_set_sensitive (gvs->chg, TRUE);
		gtk_widget_set_sensitive (gvs->del, TRUE);
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (gvs->menu), m);

}

static void
sp_gvs_gradient_activate (GtkMenuItem *mi, SPGradientVectorSelector *gvs)
{
	SPGradient *gr, *norm;

	gr = gtk_object_get_data (GTK_OBJECT (mi), "gradient");
	g_assert (gr != NULL);
	g_assert (SP_IS_GRADIENT (gr));
	/* Hmmm... bad things may happen here, as actual gradient is something new */
	norm = sp_gradient_ensure_vector_normalized (gr);

	/* fixme: Really we would want to use _set_vector */
	/* Detach old */
	if (gvs->gr) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (gvs->gr), gvs);
		gvs->gr = NULL;
	}
	/* Attach new */
	if (norm) {
		gtk_signal_connect (GTK_OBJECT (norm), "destroy", GTK_SIGNAL_FUNC (sp_gvs_gradient_destroy), gvs);
		/* fixme: Connect 'modified'? (Lauris) */
		/* fixme: I think we do not need it (Lauris) */
		gvs->gr = norm;
	}

	gtk_signal_emit (GTK_OBJECT (gvs), signals[VECTOR_SET], norm);

	if (norm != gr) {
		/* We do extra undo push here */
		/* If handler has already done it, it is just NOP */
		sp_document_done (SP_OBJECT_DOCUMENT (norm));
	}
}

static void
sp_gvs_gradient_destroy (SPGradient *gr, SPGradientVectorSelector *gvs)
{
	/* fixme: (Lauris) */
	/* fixme: Not sure, what to do here (Lauris) */
	gvs->gr = NULL;
}

static void
sp_gvs_defs_destroy (SPObject *defs, SPGradientVectorSelector *gvs)
{
	gvs->defs = NULL;
	/* Disconnect gradient as well */
	if (gvs->gr) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (gvs->gr), gvs);
		gvs->gr = NULL;
	}
}

static void
sp_gvs_defs_modified (SPObject *defs, guint flags, SPGradientVectorSelector *gvs)
{
	/* fixme: (Lauris) */
	/* fixme: We probably have to check some flags here (Lauris) */
	/* fixme: Not exactly sure, what we have to do here (Lauris) */
}


#if 0
#include <stdlib.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkwindow.h>
#include "../widgets/sp-color-selector.h"
#include "../widgets/gradient-image.h"
#include "../gradient-chemistry.h"
#include "gradient-vector.h"

#define PAD 4

static GtkWidget *sp_gradient_vector_widget_new (SPGradient *gradient);

static void sp_gradient_vector_widget_load_gradient (GtkWidget *widget, SPGradient *gradient);
static void sp_gradient_vector_dialog_close (GtkWidget *widget, GtkWidget *dialog);
static gint sp_gradient_vector_dialog_delete (GtkWidget *widget, GdkEvent *event, GtkWidget *dialog);

static void sp_gradient_vector_widget_destroy (GtkObject *object, gpointer data);
static void sp_gradient_vector_gradient_destroy (SPGradient *gradient, GtkWidget *widget);
static void sp_gradient_vector_gradient_modified (SPGradient *gradient, guint flags, GtkWidget *widget);
static void sp_gradient_vector_color_dragged (SPColorSelector *csel, GtkObject *object);
static void sp_gradient_vector_color_changed (SPColorSelector *csel, GtkObject *object);

static gboolean blocked = FALSE;

static GtkWidget *
sp_gradient_vector_widget_new (SPGradient *gradient)
{
	GtkWidget *vb, *w, *f, *csel;

	g_return_val_if_fail (!gradient || SP_IS_GRADIENT (gradient), NULL);

	vb = gtk_vbox_new (FALSE, PAD);
	gtk_signal_connect (GTK_OBJECT (vb), "destroy",
			    GTK_SIGNAL_FUNC (sp_gradient_vector_widget_destroy), NULL);

	w = sp_gradient_image_new (gradient);
	gtk_object_set_data (GTK_OBJECT (vb), "preview", w);
	gtk_widget_show (w);
	gtk_box_pack_start (GTK_BOX (vb), w, TRUE, TRUE, PAD);

	f = gtk_frame_new (_("Start color"));
	gtk_widget_show (f);
	gtk_box_pack_start (GTK_BOX (vb), f, TRUE, TRUE, PAD);
	csel = sp_color_selector_new ();
	gtk_object_set_data (GTK_OBJECT (vb), "start", csel);
	gtk_widget_show (csel);
	gtk_container_add (GTK_CONTAINER (f), csel);
	gtk_signal_connect (GTK_OBJECT (csel), "dragged",
			    GTK_SIGNAL_FUNC (sp_gradient_vector_color_dragged), vb);
	gtk_signal_connect (GTK_OBJECT (csel), "changed",
			    GTK_SIGNAL_FUNC (sp_gradient_vector_color_changed), vb);

	f = gtk_frame_new (_("End color"));
	gtk_widget_show (f);
	gtk_box_pack_start (GTK_BOX (vb), f, TRUE, TRUE, PAD);
	csel = sp_color_selector_new ();
	gtk_object_set_data (GTK_OBJECT (vb), "end", csel);
	gtk_widget_show (csel);
	gtk_container_add (GTK_CONTAINER (f), csel);
	gtk_signal_connect (GTK_OBJECT (csel), "dragged",
			    GTK_SIGNAL_FUNC (sp_gradient_vector_color_dragged), vb);
	gtk_signal_connect (GTK_OBJECT (csel), "changed",
			    GTK_SIGNAL_FUNC (sp_gradient_vector_color_changed), vb);

	gtk_widget_show (vb);

	sp_gradient_vector_widget_load_gradient (vb, gradient);

	return vb;
}

void
sp_gradient_vector_dialog (SPGradient *gradient)
{
	static GtkWidget *dialog = NULL;

	if (dialog == NULL) {
		GtkWidget *w;
		dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (dialog), _("Gradient vector"));
		gtk_container_set_border_width (GTK_CONTAINER (dialog), PAD);
		gtk_signal_connect (GTK_OBJECT (dialog), "delete_event", GTK_SIGNAL_FUNC (sp_gradient_vector_dialog_delete), dialog);
		w = sp_gradient_vector_widget_new (gradient);
		gtk_object_set_data (GTK_OBJECT (dialog), "gradient-vector-widget", w);
		/* Connect signals */
		gtk_widget_show (w);
		gtk_container_add (GTK_CONTAINER (dialog), w);
	} else {
		GtkWidget *w;
		w = gtk_object_get_data (GTK_OBJECT (dialog), "gradient-vector-widget");
		sp_gradient_vector_widget_load_gradient (w, gradient);
	}

	gtk_widget_show (dialog);
}

static void
sp_gradient_vector_widget_load_gradient (GtkWidget *widget, SPGradient *gradient)
{
	SPGradient *old;
	GtkWidget *w;
	guint32 cs, ce;

	old = gtk_object_get_data (GTK_OBJECT (widget), "gradient");
	if (old != gradient) {
		if (old) {
			gtk_signal_disconnect_by_data (GTK_OBJECT (old), widget);
		}
		if (gradient) {
			gtk_signal_connect (GTK_OBJECT (gradient), "destroy",
					    GTK_SIGNAL_FUNC (sp_gradient_vector_gradient_destroy), widget);
			gtk_signal_connect (GTK_OBJECT (gradient), "modified",
					    GTK_SIGNAL_FUNC (sp_gradient_vector_gradient_modified), widget);
		}
	}

	gtk_object_set_data (GTK_OBJECT (widget), "gradient", gradient);

	if (gradient) {
		sp_gradient_ensure_vector (gradient);
		cs = sp_color_get_rgba32_falpha (&gradient->vector->stops[0].color, gradient->vector->stops[0].opacity);
		ce = sp_color_get_rgba32_falpha (&gradient->vector->stops[1].color, gradient->vector->stops[1].opacity);

		/* Set color selector values */
		w = gtk_object_get_data (GTK_OBJECT (widget), "start");
		sp_color_selector_set_any_rgba32 (SP_COLOR_SELECTOR (w), cs);
		w = gtk_object_get_data (GTK_OBJECT (widget), "end");
		sp_color_selector_set_any_rgba32 (SP_COLOR_SELECTOR (w), ce);

		/* Fixme: Sensitivity */
	}

	/* Fill preview */
	w = gtk_object_get_data (GTK_OBJECT (widget), "preview");
	sp_gradient_image_set_gradient (SP_GRADIENT_IMAGE (w), gradient);
}

static void
sp_gradient_vector_dialog_close (GtkWidget *widget, GtkWidget *dialog)
{
	gtk_widget_hide (dialog);
}

static gint
sp_gradient_vector_dialog_delete (GtkWidget *widget, GdkEvent *event, GtkWidget *dialog)
{
	sp_gradient_vector_dialog_close (widget, dialog);

	return TRUE;
}

/* Widget destroy handler */

static void
sp_gradient_vector_widget_destroy (GtkObject *object, gpointer data)
{
	GtkObject *gradient;

	gradient = gtk_object_get_data (object, "gradient");

	if (gradient) {
		/* Remove signals connected to us */
		/* fixme: may use _connect_while_alive as well */
		gtk_signal_disconnect_by_data (gradient, object);
	}
}

static void
sp_gradient_vector_gradient_destroy (SPGradient *gradient, GtkWidget *widget)
{
	sp_gradient_vector_widget_load_gradient (widget, NULL);
}

static void
sp_gradient_vector_gradient_modified (SPGradient *gradient, guint flags, GtkWidget *widget)
{
	if (!blocked) {
		blocked = TRUE;
		sp_gradient_vector_widget_load_gradient (widget, gradient);
		blocked = FALSE;
	}
}

static void
sp_gradient_vector_color_dragged (SPColorSelector *csel, GtkObject *object)
{
	SPGradient *gradient, *ngr;
	SPGradientVector *vector;
	gfloat c[4];

	if (blocked) return;

	gradient = gtk_object_get_data (object, "gradient");
	if (!gradient) return;

	blocked = TRUE;

	ngr = sp_gradient_ensure_vector_normalized (gradient);
	if (ngr != gradient) {
		/* Our master gradient has changed */
		sp_gradient_vector_widget_load_gradient (GTK_WIDGET (object), ngr);
	}

	sp_gradient_ensure_vector (ngr);

	vector = alloca (sizeof (SPGradientVector) + sizeof (SPGradientStop));
	vector->nstops = 2;
	vector->start = ngr->vector->start;
	vector->end = ngr->vector->end;

	csel = gtk_object_get_data (object, "start");
	sp_color_selector_get_rgba_floatv (csel, c);
	vector->stops[0].offset = 0.0;
	sp_color_set_rgb_float (&vector->stops[0].color, c[0], c[1], c[2]);
	vector->stops[0].opacity = c[3];
	csel = gtk_object_get_data (object, "end");
	sp_color_selector_get_rgba_floatv (csel, c);
	vector->stops[1].offset = 1.0;
	sp_color_set_rgb_float (&vector->stops[1].color, c[0], c[1], c[2]);
	vector->stops[1].opacity = c[3];

	sp_gradient_set_vector (ngr, vector);

	blocked = FALSE;
}

static void
sp_gradient_vector_color_changed (SPColorSelector *csel, GtkObject *object)
{
	SPGradient *gradient, *ngr;
	gdouble start, end;
	SPObject *child;
	guint32 color;
	gchar c[256];

	if (blocked) return;

	gradient = gtk_object_get_data (object, "gradient");
	if (!gradient) return;

	blocked = TRUE;

	ngr = sp_gradient_ensure_vector_normalized (gradient);
	if (ngr != gradient) {
		/* Our master gradient has changed */
		sp_gradient_vector_widget_load_gradient (GTK_WIDGET (object), ngr);
	}

	sp_gradient_ensure_vector (ngr);

	start = ngr->vector->start;
	end = ngr->vector->end;

	/* Set start parameters */
	/* We rely on normalized vector, i.e. stops HAVE to exist */
	for (child = ngr->stops; child != NULL; child = child->next) {
		if (SP_IS_STOP (child)) break;
	}
	g_return_if_fail (child != NULL);

	csel = gtk_object_get_data (object, "start");
	color = sp_color_selector_get_rgba32 (csel);

	sp_repr_set_double_attribute (SP_OBJECT_REPR (child), "offset", start);
	g_snprintf (c, 256, "stop-color:#%06x;stop-opacity:%g;", color >> 8, (gdouble) (color & 0xff) / 255.0);
	sp_repr_set_attr (SP_OBJECT_REPR (child), "style", c);

	for (child = child->next; child != NULL; child = child->next) {
		if (SP_IS_STOP (child)) break;
	}
	g_return_if_fail (child != NULL);

	csel = gtk_object_get_data (object, "end");
	color = sp_color_selector_get_rgba32 (csel);

	sp_repr_set_double_attribute (SP_OBJECT_REPR (child), "offset", end);
	g_snprintf (c, 256, "stop-color:#%06x;stop-opacity:%g;", color >> 8, (gdouble) (color & 0xff) / 255.0);
	sp_repr_set_attr (SP_OBJECT_REPR (child), "style", c);

	/* Remove other stops */
	while (child->next) {
		if (SP_IS_STOP (child->next)) {
			sp_repr_remove_child (SP_OBJECT_REPR (ngr), SP_OBJECT_REPR (child->next));
		} else {
			child = child->next;
		}
	}

	sp_document_done (SP_OBJECT_DOCUMENT (ngr));

	blocked = FALSE;
}

#endif

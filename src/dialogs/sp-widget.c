#define __SP_WIDGET_C__

/*
 * SPWidget
 *
 * Abstract base class for sodipodi property control widgets
 *
 * Authors:
 *  Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include "../sodipodi.h"
#include "../desktop.h"
#include "../document.h"
#include "sp-widget.h"

static void sp_widget_class_init (SPWidgetClass *klass);
static void sp_widget_init (SPWidget *widget);

static void sp_widget_destroy (GtkObject *object);

static void sp_widget_show (GtkWidget *widget);
static void sp_widget_hide (GtkWidget *widget);
static void sp_widget_draw (GtkWidget *widget, GdkRectangle *area);
static gint sp_widget_expose (GtkWidget *widget, GdkEventExpose *event);
static void sp_widget_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void sp_widget_size_allocate (GtkWidget *widget, GtkAllocation *allocation);

static void sp_widget_change_selection (Sodipodi *sodipodi, SPSelection *selection, SPWidget *spw);
static void sp_widget_set_selection (Sodipodi *sodipodi, SPSelection *selection, SPWidget *spw);

static GtkBinClass *parent_class;

GtkType
sp_widget_get_type (void)
{
	static GtkType widget_type = 0;
	if (!widget_type) {
		static const GtkTypeInfo widget_info = {
			"SPWidget",
			sizeof (SPWidget),
			sizeof (SPWidgetClass),
			(GtkClassInitFunc) sp_widget_class_init,
			(GtkObjectInitFunc) sp_widget_init,
			NULL, NULL, NULL
		};
		widget_type = gtk_type_unique (GTK_TYPE_BIN, &widget_info);
	}
	return widget_type;
}

static void
sp_widget_class_init (SPWidgetClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	parent_class = gtk_type_class (GTK_TYPE_BIN);

	object_class->destroy = sp_widget_destroy;

	widget_class->show = sp_widget_show;
	widget_class->hide = sp_widget_hide;
	widget_class->draw = sp_widget_draw;
	widget_class->expose_event = sp_widget_expose;
	widget_class->size_request = sp_widget_size_request;
	widget_class->size_allocate = sp_widget_size_allocate;
}

static void
sp_widget_init (SPWidget *spw)
{
	spw->sodipodi = NULL;
	spw->desktop = NULL;
	spw->document = NULL;
}

static void
sp_widget_destroy (GtkObject *object)
{
	SPWidget *spw;

	spw = (SPWidget *) object;

	g_assert (spw->change_selection_id == 0);
	g_assert (spw->set_selection_id == 0);

	if (((GtkObjectClass *) parent_class)->destroy)
		(* ((GtkObjectClass *) parent_class)->destroy) (object);
}

static void
sp_widget_show (GtkWidget *widget)
{
	SPWidget *spw;

	spw = SP_WIDGET (widget);

	g_assert (spw->change_selection_id == 0);
	g_assert (spw->set_selection_id == 0);

	spw->change_selection_id = gtk_signal_connect (GTK_OBJECT (sodipodi), "change_selection",
						       GTK_SIGNAL_FUNC (sp_widget_change_selection), spw);
	spw->set_selection_id = gtk_signal_connect (GTK_OBJECT (sodipodi), "set_selection",
						    GTK_SIGNAL_FUNC (sp_widget_set_selection), spw);

	if (((GtkWidgetClass *) parent_class)->show)
		(* ((GtkWidgetClass *) parent_class)->show) (widget);
}

static void
sp_widget_hide (GtkWidget *widget)
{
	SPWidget *spw;

	spw = SP_WIDGET (widget);

	g_assert (spw->change_selection_id != 0);
	g_assert (spw->set_selection_id != 0);

	gtk_signal_disconnect (GTK_OBJECT (widget), spw->change_selection_id);
	spw->change_selection_id = 0;
	gtk_signal_disconnect (GTK_OBJECT (widget), spw->set_selection_id);
	spw->set_selection_id = 0;

	if (((GtkWidgetClass *) parent_class)->show)
		(* ((GtkWidgetClass *) parent_class)->show) (widget);
}

static void
sp_widget_draw (GtkWidget *widget, GdkRectangle *area)
{
	if (((GtkBin *) widget)->child)
		gtk_widget_draw (((GtkBin *) widget)->child, area);
}

static gint
sp_widget_expose (GtkWidget *widget, GdkEventExpose *event)
{
	GtkBin *bin;

	bin = GTK_BIN (widget);

	if ((bin->child) && (GTK_WIDGET_NO_WINDOW (bin->child))) {
		GdkEventExpose ce;
		ce = *event;
		gtk_widget_event (bin->child, (GdkEvent *) &ce);
	}

	return FALSE;
}

static void
sp_widget_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	if (((GtkBin *) widget)->child)
		gtk_widget_size_request (((GtkBin *) widget)->child, requisition);
}

static void
sp_widget_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	widget->allocation = *allocation;

	if (((GtkBin *) widget)->child)
		gtk_widget_size_allocate (((GtkBin *) widget)->child, allocation);
}

/* Methods */

SPWidget *
sp_widget_construct (SPWidget *spw, Sodipodi *sodipodi, SPDesktop *desktop, SPDocument *document)
{
	g_return_val_if_fail (spw != NULL, NULL);
	g_return_val_if_fail (SP_IS_WIDGET (spw), NULL);
	g_return_val_if_fail ((sodipodi != NULL) || (desktop != NULL) || (document != NULL), NULL);
	g_return_val_if_fail ((!sodipodi) || SP_IS_SODIPODI (sodipodi), NULL);
	g_return_val_if_fail ((!desktop) || SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail ((!document) || SP_IS_DOCUMENT (document), NULL);

	/* fixme: support desktop and document */
	g_return_val_if_fail (sodipodi != NULL, NULL);

	spw->sodipodi = sodipodi;

	return spw;
}

static void
sp_widget_change_selection (Sodipodi *sodipodi, SPSelection *selection, SPWidget *spw)
{
	if (((SPWidgetClass *) ((GtkObject *) spw)->klass)->change_selection)
		(* ((SPWidgetClass *) ((GtkObject *) spw)->klass)->change_selection) (spw, selection);
}

static void
sp_widget_set_selection (Sodipodi *sodipodi, SPSelection *selection, SPWidget *spw)
{
	if (((SPWidgetClass *) ((GtkObject *) spw)->klass)->change_selection)
		(* ((SPWidgetClass *) ((GtkObject *) spw)->klass)->change_selection) (spw, selection);
}



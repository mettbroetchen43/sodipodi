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

#include <gtk/gtksignal.h>
#include "../sodipodi.h"
#include "../desktop.h"
#include "../selection.h"
#include "../document.h"
#include "sp-widget.h"

enum {
	MODIFY_SELECTION,
	CHANGE_SELECTION,
	SET_SELECTION,
	SET_DIRTY,
	LAST_SIGNAL
};

static void sp_widget_class_init (SPWidgetClass *klass);
static void sp_widget_init (SPWidget *widget);

static void sp_widget_destroy (GtkObject *object);

static void sp_widget_show (GtkWidget *widget);
static void sp_widget_hide (GtkWidget *widget);
static void sp_widget_draw (GtkWidget *widget, GdkRectangle *area);
static gint sp_widget_expose (GtkWidget *widget, GdkEventExpose *event);
static void sp_widget_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void sp_widget_size_allocate (GtkWidget *widget, GtkAllocation *allocation);

static void sp_widget_modify_selection (Sodipodi *sodipodi, SPSelection *selection, guint flags, SPWidget *spw);
static void sp_widget_change_selection (Sodipodi *sodipodi, SPSelection *selection, SPWidget *spw);
static void sp_widget_set_selection (Sodipodi *sodipodi, SPSelection *selection, SPWidget *spw);

static GtkBinClass *parent_class;
static guint signals[LAST_SIGNAL] = {0};

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

	signals[CHANGE_SELECTION] = gtk_signal_new ("change_selection",
						    GTK_RUN_FIRST,
						    object_class->type,
						    GTK_SIGNAL_OFFSET (SPWidgetClass, change_selection),
						    gtk_marshal_NONE__POINTER,
						    GTK_TYPE_NONE, 1,
						    GTK_TYPE_POINTER);
	signals[MODIFY_SELECTION] = gtk_signal_new ("modify_selection",
						    GTK_RUN_FIRST,
						    object_class->type,
						    GTK_SIGNAL_OFFSET (SPWidgetClass, modify_selection),
						    gtk_marshal_NONE__POINTER_UINT,
						    GTK_TYPE_NONE, 1,
						    GTK_TYPE_POINTER, GTK_TYPE_UINT);
	signals[SET_SELECTION] =    gtk_signal_new ("set_selection",
						    GTK_RUN_FIRST,
						    object_class->type,
						    GTK_SIGNAL_OFFSET (SPWidgetClass, set_selection),
						    gtk_marshal_NONE__POINTER,
						    GTK_TYPE_NONE, 1,
						    GTK_TYPE_POINTER);
	signals[SET_DIRTY] =        gtk_signal_new ("set_dirty",
						    GTK_RUN_FIRST,
						    object_class->type,
						    GTK_SIGNAL_OFFSET (SPWidgetClass, set_dirty),
						    gtk_marshal_NONE__BOOL,
						    GTK_TYPE_NONE, 1,
						    GTK_TYPE_BOOL);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

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
	spw->dirty = FALSE;
	spw->autoupdate = TRUE;
	spw->sodipodi = NULL;
	spw->desktop = NULL;
	spw->document = NULL;
}

static void
sp_widget_destroy (GtkObject *object)
{
	SPWidget *spw;

	spw = (SPWidget *) object;

	/* Disconnect signals */
	gtk_signal_disconnect_by_data (GTK_OBJECT (sodipodi), spw);

	if (((GtkObjectClass *) parent_class)->destroy)
		(* ((GtkObjectClass *) parent_class)->destroy) (object);
}

static void
sp_widget_show (GtkWidget *widget)
{
	SPWidget *spw;

	spw = SP_WIDGET (widget);

	/* Connect signals */
	gtk_signal_connect (GTK_OBJECT (sodipodi), "modify_selection", GTK_SIGNAL_FUNC (sp_widget_modify_selection), spw);
	gtk_signal_connect (GTK_OBJECT (sodipodi), "change_selection", GTK_SIGNAL_FUNC (sp_widget_change_selection), spw);
	gtk_signal_connect (GTK_OBJECT (sodipodi), "set_selection", GTK_SIGNAL_FUNC (sp_widget_set_selection), spw);

	if (((GtkWidgetClass *) parent_class)->show)
		(* ((GtkWidgetClass *) parent_class)->show) (widget);
}

static void
sp_widget_hide (GtkWidget *widget)
{
	SPWidget *spw;

	spw = SP_WIDGET (widget);

	/* Disconnect signals */
	gtk_signal_disconnect_by_data (GTK_OBJECT (sodipodi), spw);

	if (((GtkWidgetClass *) parent_class)->hide)
		(* ((GtkWidgetClass *) parent_class)->hide) (widget);
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

GtkWidget *
sp_widget_new (Sodipodi *sodipodi, SPDesktop *desktop, SPDocument *document)
{
	SPWidget *spw;

	spw = gtk_type_new (SP_TYPE_WIDGET);

	if (!sp_widget_construct (spw, sodipodi, desktop, document)) {
		gtk_object_unref (GTK_OBJECT (spw));
		return NULL;
	}

	return (GtkWidget *) spw;
}

GtkWidget *
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
	spw->desktop = SP_ACTIVE_DESKTOP;
	spw->document = SP_ACTIVE_DOCUMENT;

	return (GtkWidget *) spw;
}

static void
sp_widget_modify_selection (Sodipodi *sodipodi, SPSelection *selection, guint flags, SPWidget *spw)
{
	if (((SPWidgetClass *) ((GtkObject *) spw)->klass)->modify_selection)
		(* ((SPWidgetClass *) ((GtkObject *) spw)->klass)->modify_selection) (spw, selection, flags);

	gtk_signal_emit (GTK_OBJECT (spw), signals[MODIFY_SELECTION], selection, flags);
}

static void
sp_widget_change_selection (Sodipodi *sodipodi, SPSelection *selection, SPWidget *spw)
{
	if (((SPWidgetClass *) ((GtkObject *) spw)->klass)->change_selection)
		(* ((SPWidgetClass *) ((GtkObject *) spw)->klass)->change_selection) (spw, selection);

	gtk_signal_emit (GTK_OBJECT (spw), signals[CHANGE_SELECTION], selection);
}

static void
sp_widget_set_selection (Sodipodi *sodipodi, SPSelection *selection, SPWidget *spw)
{
	/* Set desktop and document members */
	spw->desktop = SP_ACTIVE_DESKTOP;
	spw->document = SP_ACTIVE_DOCUMENT;
	/* Call virtual method */
	if (((SPWidgetClass *) ((GtkObject *) spw)->klass)->change_selection)
		(* ((SPWidgetClass *) ((GtkObject *) spw)->klass)->change_selection) (spw, selection);
	/* Emit "set_selection" signal */
	gtk_signal_emit (GTK_OBJECT (spw), signals[SET_SELECTION], selection);
}

void
sp_widget_set_dirty (SPWidget *spw, gboolean dirty)
{
	if (dirty && !spw->dirty) {
		spw->dirty = TRUE;
		if (!spw->autoupdate) {
			gtk_signal_emit (GTK_OBJECT (spw), signals[SET_DIRTY], TRUE);
		}
	} else if (!dirty && spw->dirty) {
		spw->dirty = FALSE;
		if (!spw->autoupdate) {
			gtk_signal_emit (GTK_OBJECT (spw), signals[SET_DIRTY], FALSE);
		}
	}
}

void
sp_widget_set_autoupdate (SPWidget *spw, gboolean autoupdate)
{
	if (autoupdate && spw->dirty) spw->dirty = FALSE;
	spw->autoupdate = autoupdate;
}

const GSList *
sp_widget_get_item_list (SPWidget *spw)
{
	return sp_selection_item_list (spw->desktop->selection);
}




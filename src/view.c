#define __SP_VIEW_C__

/*
 * Abstract base class for all SVG document views
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtk/gtksignal.h>
#include "helper/sp-marshal.h"
#include "document.h"
#include "view.h"

enum {
	SHUTDOWN,
	URI_SET,
	RESIZED,
	POSITION_SET,
	STATUS_SET,
	LAST_SIGNAL
};

static void sp_view_class_init (SPViewClass *klass);
static void sp_view_init (SPView *view);
static void sp_view_destroy (GtkObject *object);

static void sp_view_document_uri_set (SPDocument *doc, const guchar *uri, SPView *view);
static void sp_view_document_resized (SPDocument *doc, gdouble width, gdouble height, SPView *view);

static GtkObjectClass *parent_class;
static guint signals[LAST_SIGNAL] = {0};

GtkType
sp_view_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPView",
			sizeof (SPView),
			sizeof (SPViewClass),
			(GtkClassInitFunc) sp_view_class_init,
			(GtkObjectInitFunc) sp_view_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_OBJECT, &info);
	}
	return type;
}

static void
sp_view_class_init (SPViewClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);

	parent_class = gtk_type_class (GTK_TYPE_OBJECT);

	signals[SHUTDOWN] =     gtk_signal_new ("shutdown",
						GTK_RUN_LAST,
						GTK_CLASS_TYPE(object_class),
						GTK_SIGNAL_OFFSET (SPViewClass, shutdown),
						gtk_marshal_BOOL__NONE,
						GTK_TYPE_BOOL, 0);
	signals[URI_SET] =      gtk_signal_new ("uri_set",
						GTK_RUN_FIRST,
						GTK_CLASS_TYPE(object_class),
						GTK_SIGNAL_OFFSET (SPViewClass, uri_set),
						gtk_marshal_NONE__STRING,
						GTK_TYPE_NONE, 1, GTK_TYPE_STRING);
	signals[RESIZED] =      gtk_signal_new ("resized",
						GTK_RUN_FIRST,
						GTK_CLASS_TYPE(object_class),
						GTK_SIGNAL_OFFSET (SPViewClass, resized),
						sp_marshal_NONE__DOUBLE_DOUBLE,
						GTK_TYPE_NONE, 2, GTK_TYPE_DOUBLE, GTK_TYPE_DOUBLE);
	signals[POSITION_SET] = gtk_signal_new ("position_set",
						GTK_RUN_FIRST,
						GTK_CLASS_TYPE(object_class),
						GTK_SIGNAL_OFFSET (SPViewClass, position_set),
						sp_marshal_NONE__DOUBLE_DOUBLE,
						GTK_TYPE_NONE, 2, GTK_TYPE_DOUBLE, GTK_TYPE_DOUBLE);
	signals[STATUS_SET] =   gtk_signal_new ("status_set",
						GTK_RUN_FIRST,
						GTK_CLASS_TYPE(object_class),
						GTK_SIGNAL_OFFSET (SPViewClass, status_set),
						sp_marshal_NONE__STRING_BOOLEAN,
						GTK_TYPE_NONE, 2, GTK_TYPE_STRING, GTK_TYPE_BOOL);

	object_class->destroy = sp_view_destroy;
}

static void
sp_view_init (SPView *view)
{
	view->doc = NULL;
}

static void
sp_view_destroy (GtkObject *object)
{
	SPView *view;

	view = SP_VIEW (object);

	if (view->doc) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (view->doc), view);
		view->doc = sp_document_unref (view->doc);
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

gboolean
sp_view_shutdown (SPView *view)
{
	gboolean result;

	g_return_val_if_fail (view != NULL, TRUE);
	g_return_val_if_fail (SP_IS_VIEW (view), TRUE);

	result = FALSE;

	gtk_signal_emit (GTK_OBJECT (view), signals[SHUTDOWN], &result);

	return result;
}

void
sp_view_request_redraw (SPView *view)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (SP_IS_VIEW (view));

	if (((SPViewClass *) G_OBJECT_GET_CLASS(view))->request_redraw)
		((SPViewClass *) G_OBJECT_GET_CLASS(view))->request_redraw (view);
}

void
sp_view_set_document (SPView *view, SPDocument *doc)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (SP_IS_VIEW (view));
	g_return_if_fail (!doc || SP_IS_DOCUMENT (doc));

	if (((SPViewClass *) G_OBJECT_GET_CLASS(view))->set_document)
		((SPViewClass *) G_OBJECT_GET_CLASS(view))->set_document (view, doc);

	if (view->doc) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (view->doc), view);
		view->doc = sp_document_unref (view->doc);
	}

	if (doc) {
		view->doc = sp_document_ref (doc);
		gtk_signal_connect (GTK_OBJECT (doc), "uri_set", GTK_SIGNAL_FUNC (sp_view_document_uri_set), view);
		gtk_signal_connect (GTK_OBJECT (doc), "resized", GTK_SIGNAL_FUNC (sp_view_document_resized), view);
	}

	gtk_signal_emit (GTK_OBJECT (view), signals[URI_SET], (doc) ? SP_DOCUMENT_URI (doc) : NULL);
}

void
sp_view_emit_resized (SPView *view, gdouble width, gdouble height)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (SP_IS_VIEW (view));

	gtk_signal_emit (GTK_OBJECT (view), signals[RESIZED], width, height);
}

void
sp_view_set_position (SPView *view, gdouble x, gdouble y)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (SP_IS_VIEW (view));

	gtk_signal_emit (GTK_OBJECT (view), signals[POSITION_SET], x, y);
}

void
sp_view_set_status (SPView *view, const guchar *status, gboolean isdefault)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (SP_IS_VIEW (view));

	gtk_signal_emit (GTK_OBJECT (view), signals[STATUS_SET], status, isdefault);
}

static void
sp_view_document_uri_set (SPDocument *doc, const guchar *uri, SPView *view)
{
	gtk_signal_emit (GTK_OBJECT (view), signals[URI_SET], uri);
}

static void
sp_view_document_resized (SPDocument *doc, gdouble width, gdouble height, SPView *view)
{
	if (((SPViewClass *) G_OBJECT_GET_CLASS(view))->document_resized)
		((SPViewClass *) G_OBJECT_GET_CLASS(view))->document_resized (view, doc, width, height);
}

/* SPViewWidget */

static void sp_view_widget_class_init (SPViewWidgetClass *klass);
static void sp_view_widget_init (SPViewWidget *widget);
static void sp_view_widget_destroy (GtkObject *object);

static void sp_view_widget_view_destroy (GtkObject *object, SPViewWidget *vw);
static void sp_view_widget_view_resized (SPView *view, gdouble width, gdouble height, SPViewWidget *vw);

static GtkEventBoxClass *widget_parent_class;

GtkType
sp_view_widget_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPViewWidget",
			sizeof (SPViewWidget),
			sizeof (SPViewWidgetClass),
			(GtkClassInitFunc) sp_view_widget_class_init,
			(GtkObjectInitFunc) sp_view_widget_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_EVENT_BOX, &info);
	}
	return type;
}

static void
sp_view_widget_class_init (SPViewWidgetClass *klass)
{
	GtkObjectClass *object_class;

	object_class = GTK_OBJECT_CLASS (klass);

	widget_parent_class = gtk_type_class (GTK_TYPE_EVENT_BOX);

	object_class->destroy = sp_view_widget_destroy;
}

static void
sp_view_widget_init (SPViewWidget *vw)
{
	vw->view = NULL;
}

static void
sp_view_widget_destroy (GtkObject *object)
{
	SPViewWidget *vw;

	vw = SP_VIEW_WIDGET (object);

	if (vw->view) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (vw->view), vw);
		gtk_object_unref (GTK_OBJECT (vw->view));
		vw->view = NULL;
	}

	if (((GtkObjectClass *) (widget_parent_class))->destroy)
		(* ((GtkObjectClass *) (widget_parent_class))->destroy) (object);
}

void
sp_view_widget_set_view (SPViewWidget *vw, SPView *view)
{
	g_return_if_fail (vw != NULL);
	g_return_if_fail (SP_IS_VIEW_WIDGET (vw));
	g_return_if_fail (view != NULL);
	g_return_if_fail (SP_IS_VIEW (view));

	g_return_if_fail (vw->view == NULL);

	vw->view = view;
	gtk_object_ref (GTK_OBJECT (view));
	gtk_signal_connect (GTK_OBJECT (view), "destroy", GTK_SIGNAL_FUNC (sp_view_widget_view_destroy), vw);
	gtk_signal_connect (GTK_OBJECT (view), "resized", GTK_SIGNAL_FUNC (sp_view_widget_view_resized), vw);

	if (((SPViewWidgetClass *) G_OBJECT_GET_CLASS(vw))->set_view)
		((SPViewWidgetClass *) G_OBJECT_GET_CLASS(vw))->set_view (vw, view);
}

gboolean
sp_view_widget_shutdown (SPViewWidget *vw)
{
	g_return_val_if_fail (vw != NULL, TRUE);
	g_return_val_if_fail (SP_IS_VIEW_WIDGET (vw), TRUE);

	if (((SPViewWidgetClass *) G_OBJECT_GET_CLASS(vw))->shutdown)
		return ((SPViewWidgetClass *) G_OBJECT_GET_CLASS(vw))->shutdown (vw);

	return FALSE;
}

static void
sp_view_widget_view_destroy (GtkObject *object, SPViewWidget *vw)
{
	vw->view = NULL;
}

static void
sp_view_widget_view_resized (SPView *view, gdouble width, gdouble height, SPViewWidget *vw)
{
	if (((SPViewWidgetClass *) G_OBJECT_GET_CLASS(vw))->view_resized)
		((SPViewWidgetClass *) G_OBJECT_GET_CLASS(vw))->view_resized (vw, view, width, height);
}


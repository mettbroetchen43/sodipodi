#define SP_SLIDE_CONTEXT_C

/*
 * Event context for slideshow
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include <gdk/gdkkeysyms.h>
#include "sodipodi.h"
#include "interface.h"
#include "document.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "slide-context.h"

static void sp_slide_context_class_init (SPSlideContextClass * klass);
static void sp_slide_context_init (SPSlideContext * slide_context);
static void sp_slide_context_dispose (GObject *object);

static void sp_slide_context_setup (SPEventContext *ec);
static gint sp_slide_context_root_handler (SPEventContext * event_context, GdkEvent * event);
#if 0
static gint sp_slide_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);
#endif

static void sp_slide_context_zoom (SPDesktop *desktop);

static SPEventContextClass * parent_class;

GtkType
sp_slide_context_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPSlideContextClass),
			NULL, NULL,
			(GClassInitFunc) sp_slide_context_class_init,
			NULL, NULL,
			sizeof (SPSlideContext),
			4,
			(GInstanceInitFunc) sp_slide_context_init,
		};
		type = g_type_register_static (SP_TYPE_EVENT_CONTEXT, "SPSlideContext", &info, 0);
	}
	return type;
}

static void
sp_slide_context_class_init (SPSlideContextClass * klass)
{
	GObjectClass *object_class;
	SPEventContextClass * event_context_class;

	object_class = (GObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	object_class->dispose = sp_slide_context_dispose;

	event_context_class->setup = sp_slide_context_setup;
	event_context_class->root_handler = sp_slide_context_root_handler;
#if 0
	event_context_class->item_handler = sp_slide_context_item_handler;
#endif
}

static void
sp_slide_context_init (SPSlideContext * slide_context)
{
	SPEventContext * event_context;
	
	event_context = SP_EVENT_CONTEXT (slide_context);

	slide_context->forward = NULL;
}

static void
sp_slide_context_dispose (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
sp_slide_context_setup (SPEventContext *ec)
{
	/* fixme: This is not widget after all (Lauris) */
	sp_desktop_toggle_borders ((GtkWidget *) ec->desktop);
	sp_slide_context_zoom (ec->desktop);

	if (((SPEventContextClass *) parent_class)->setup)
		((SPEventContextClass *) parent_class)->setup (ec);
}

static gint
sp_slide_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	SPSlideContext *sc;
	SPDesktop * desktop;
	GSList *slides;
	gint ret;

	sc = SP_SLIDE_CONTEXT (event_context);

	ret = FALSE;

	desktop = event_context->desktop;

	switch (event->type) {
	case GDK_KEY_PRESS:
		if (event->key.keyval == GDK_z) {
			sp_slide_context_zoom (desktop);
			ret = TRUE;
			break;
		} else if (event->key.keyval == GDK_BackSpace) {
			if (sc->forward) {
				SPDocument *doc;
				doc = SP_DT_DOCUMENT (desktop);
				slides = gtk_object_get_data (GTK_OBJECT (desktop), "slides");
				slides = g_slist_prepend (slides, doc);
				gtk_object_set_data (GTK_OBJECT (desktop), "slides", slides);
				doc = SP_DOCUMENT (sc->forward->data);
				sc->forward = g_slist_remove (sc->forward, doc);
				sp_desktop_change_document (desktop, doc);
				sp_slide_context_zoom (desktop);
			}
			ret = TRUE;
			break;
		}
	case GDK_BUTTON_PRESS:
		slides = gtk_object_get_data (GTK_OBJECT (desktop), "slides");
		if (slides) {
			SPDocument *doc;
			doc = SP_DT_DOCUMENT (desktop);
			sc->forward = g_slist_prepend (sc->forward, doc);
			doc = SP_DOCUMENT (slides->data);
			slides = g_slist_remove (slides, doc);
			gtk_object_set_data (GTK_OBJECT (desktop), "slides", slides);
			sp_desktop_change_document (desktop, doc);
			sp_slide_context_zoom (desktop);
		} else {
			sp_ui_close_view (NULL);
			while (sc->forward) {
				SPDocument *doc;
				doc = SP_DOCUMENT (sc->forward->data);
				sc->forward = g_slist_remove (sc->forward, doc);
				sp_document_unref (doc);
			}
		}
		ret = TRUE;
	default:
		break;
	}

	if (!ret) {
		if (((SPEventContextClass *) parent_class)->root_handler)
			ret = ((SPEventContextClass *) parent_class)->root_handler (event_context, event);
	}

	return ret;
}

static void
sp_slide_context_zoom (SPDesktop *desktop)
{
	NRRectD d;

	d.x0 = 0.0;
	d.y0 = 0.0;
	d.x1 = sp_document_width (SP_DT_DOCUMENT (desktop));
	d.y1 = sp_document_height (SP_DT_DOCUMENT (desktop));

	if ((fabs (d.x1 - d.x0) < 1.0) || (fabs (d.y1 - d.y0) < 1.0)) return;

	sp_desktop_set_display_area (desktop, d.x0, d.y0, d.x1 - 1.0, d.y1 - 1.0, 0);
}


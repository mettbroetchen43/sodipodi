#define __SP_TEXT_CONTEXT_C__

/*
 * SPTextContext
 *
 * Authors:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2000 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Licensed under GNU GPL
 */

#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <gal/widgets/e-unicode.h>
#include "sp-text.h"
#include "sodipodi.h"
#include "document.h"
#include "selection.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "pixmaps/cursor-text.xpm"
#include "text-context.h"

static void sp_text_context_class_init (SPTextContextClass * klass);
static void sp_text_context_init (SPTextContext * text_context);
static void sp_text_context_destroy (GtkObject * object);

static void sp_text_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_text_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_text_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void sp_text_context_selection_changed (SPSelection *selection, SPTextContext *tc);

static SPEventContextClass * parent_class;

GtkType
sp_text_context_get_type (void)
{
	static GtkType text_context_type = 0;

	if (!text_context_type) {

		static const GtkTypeInfo text_context_info = {
			"SPTextContext",
			sizeof (SPTextContext),
			sizeof (SPTextContextClass),
			(GtkClassInitFunc) sp_text_context_class_init,
			(GtkObjectInitFunc) sp_text_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		text_context_type = gtk_type_unique (sp_event_context_get_type (), &text_context_info);
	}

	return text_context_type;
}

static void
sp_text_context_class_init (SPTextContextClass * klass)
{
	GtkObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = gtk_type_class (sp_event_context_get_type ());

	object_class->destroy = sp_text_context_destroy;

	event_context_class->setup = sp_text_context_setup;
	event_context_class->root_handler = sp_text_context_root_handler;
	event_context_class->item_handler = sp_text_context_item_handler;
}

static void
sp_text_context_init (SPTextContext *tc)
{
	SPEventContext * event_context;
	
	event_context = SP_EVENT_CONTEXT (tc);

	event_context->cursor_shape = cursor_text_xpm;
	event_context->hot_x = 0;
	event_context->hot_y = 0;

	tc->item = NULL;
	tc->pdoc.x = 0.0;
	tc->pdoc.y = 0.0;
}

static void
sp_text_context_destroy (GtkObject *object)
{
	SPEventContext *ec;
	SPTextContext *tc;

	ec = SP_EVENT_CONTEXT (object);
	tc = SP_TEXT_CONTEXT (object);

#if 0
	if (text_context->text)
		sp_text_complete (text_context);
#endif

	gtk_signal_disconnect_by_data (GTK_OBJECT (SP_DT_CANVAS (ec->desktop)), tc);
#ifdef SP_TC_XIM
	gdk_ic_destroy (tc->ic);
	gdk_ic_attr_destroy (tc->ic_attr);
	gdk_window_set_events (GTK_WIDGET (SP_DT_CANVAS (ec->desktop))->window, tc->savedmask);
#endif

	tc->item = NULL;

	if (ec->desktop) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (SP_DT_SELECTION (ec->desktop)), ec);
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static gint
sptc_focus_in (GtkWidget *widget, GdkEventFocus *event, SPTextContext *tc)
{
	g_print ("focus in\n");
	gdk_im_begin (tc->ic, GTK_WIDGET (SP_DT_CANVAS (SP_EVENT_CONTEXT (tc)->desktop))->window);
}

static gint
sptc_focus_out (GtkWidget *widget, GdkEventFocus *event, SPTextContext *tc)
{
	g_print ("focus out\n");
	gdk_im_end ();
}

static void
sp_text_context_setup (SPEventContext *event_context, SPDesktop *desktop)
{
	SPTextContext *tc;

	tc = SP_TEXT_CONTEXT (event_context);

#ifdef SP_TC_XIM
	if (gdk_im_ready () && (tc->ic_attr = gdk_ic_attr_new ()) != NULL) {
		GdkICAttr *attr;
		GdkColormap *colormap;
		GdkICAttributesType attrmask;
		GdkIMStyle style;
		GdkIMStyle supported_style;

		attr = tc->ic_attr;
		attrmask = GDK_IC_ALL_REQ;
		supported_style = GDK_IM_PREEDIT_NONE |
			GDK_IM_PREEDIT_NOTHING |
			/* GDK_IM_PREEDIT_POSITION | */
			GDK_IM_STATUS_NONE |
			GDK_IM_STATUS_NOTHING;
		attr->style = style = gdk_im_decide_style (supported_style);
		/* fixme: is this OK? */
		attr->client_window = GTK_WIDGET (SP_DT_CANVAS (desktop))->window;
		if ((colormap = gtk_widget_get_colormap (GTK_WIDGET (SP_DT_CANVAS (desktop)))) != gtk_widget_get_default_colormap ()) {
			attrmask |= GDK_IC_PREEDIT_COLORMAP;
			attr->preedit_colormap = colormap;
		}
		tc->ic = gdk_ic_new (attr, attrmask);
		if (tc->ic) {
			tc->savedmask = gdk_window_get_events (GTK_WIDGET (SP_DT_CANVAS (desktop))->window);
			gdk_window_set_events (GTK_WIDGET (SP_DT_CANVAS (desktop))->window, tc->savedmask | gdk_ic_get_events (tc->ic));
		}
		if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (SP_DT_CANVAS (desktop)))) {
			gdk_im_begin (tc->ic, GTK_WIDGET (SP_DT_CANVAS (desktop))->window);
		}
		gtk_signal_connect (GTK_OBJECT (SP_DT_CANVAS (desktop)), "focus_in_event", GTK_SIGNAL_FUNC (sptc_focus_in), tc);
		gtk_signal_connect (GTK_OBJECT (SP_DT_CANVAS (desktop)), "focus_out_event", GTK_SIGNAL_FUNC (sptc_focus_out), tc);
	}
#endif
	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);

	gtk_signal_connect (GTK_OBJECT (SP_DT_SELECTION (desktop)), "changed",
			    GTK_SIGNAL_FUNC (sp_text_context_selection_changed), tc);

	sp_text_context_selection_changed (SP_DT_SELECTION (desktop), tc);
}

static gint
sp_text_context_item_handler (SPEventContext *ec, SPItem *item, GdkEvent *event)
{
	SPTextContext *tc;
	gint ret;

	tc = SP_TEXT_CONTEXT (ec);

	ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			if (SP_IS_TEXT (item)) {
				sp_selection_set_item (SP_DT_SELECTION (ec->desktop), item);
				ret = TRUE;
			}
		}
		break;
	default:
		break;
	}

	if (!ret) {
		if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
			ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (ec, item, event);
	}

	return ret;
}

static gint
sp_text_context_root_handler (SPEventContext *ec, GdkEvent *event)
{
	SPTextContext *tc;
	guchar *utf8;
	const guchar *content;
	gint ret;

	tc = SP_TEXT_CONTEXT (ec);

	ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			/* Button 1, set X & Y & new item */
			sp_selection_empty (SP_DT_SELECTION (ec->desktop));
			sp_desktop_w2doc_xy_point (ec->desktop, &tc->pdoc, event->button.x, event->button.y);
			ret = TRUE;
		}
		break;
	case GDK_KEY_PRESS:
		/* fixme: */
		// filter control-modifier for desktop shortcuts
                if (event->key.state & GDK_CONTROL_MASK) return FALSE;

		if (!tc->item) {
			SPRepr *repr, *style;
			/* Create object */
			repr = sp_repr_new ("text");
			/* Set style */
			style = sodipodi_get_repr (SODIPODI, "paint.text");
			if (style) {
				SPCSSAttr *css;
				css = sp_repr_css_attr_inherited (style, "style");
				sp_repr_css_set (repr, css, "style");
				sp_repr_css_attr_unref (css);
			}
			sp_repr_set_double_attribute (repr, "x", tc->pdoc.x);
			sp_repr_set_double_attribute (repr, "y", tc->pdoc.y);
			tc->item = (SPItem *) sp_document_add_repr (SP_DT_DOCUMENT (ec->desktop), repr);
			sp_selection_set_item (SP_DT_SELECTION (ec->desktop), tc->item);
			sp_document_done (SP_DT_DOCUMENT (ec->desktop));
			sp_repr_unref (repr);
		}
		utf8 = NULL;
		if (event->key.keyval == GDK_Return) {
			utf8 = g_strdup ("\n");
		} else if (event->key.string) {
			g_print ("Key %d string %s\n", event->key.keyval, event->key.string);
#if 0
			utf8 = e_utf8_from_gtk_string (GTK_WIDGET (ec->desktop->owner->canvas), event->key.string);
#else
			utf8 = e_utf8_from_locale_string (event->key.string);
#endif
			g_print ("UTF8 %s\n", utf8);
		}
		content = sp_repr_content (SP_OBJECT_REPR (tc->item));
		if (content) {
			guchar *new;
			new = g_strconcat (content, utf8, NULL);
			g_print ("Old %s new %s\n", content, new);
			sp_repr_set_content (SP_OBJECT_REPR (tc->item), new);
			g_free (new);
		} else {
			g_print ("Setting %s\n", utf8);
			sp_repr_set_content (SP_OBJECT_REPR (tc->item), utf8);
		}
		if (utf8) g_free (utf8);
		sp_document_done (SP_DT_DOCUMENT (ec->desktop));

		ret = TRUE;
		break;
	default:
		break;
	}

	if (!ret) {
		if (SP_EVENT_CONTEXT_CLASS (parent_class)->root_handler)
			ret = SP_EVENT_CONTEXT_CLASS (parent_class)->root_handler (ec, event);
	}

	return ret;
}

#if 0
static void
sp_text_complete (SPTextContext * text_context)
{
	gchar * c;

	if (text_context->text == NULL) return;

	c = (gchar *) sp_repr_content (text_context->text);
	if (c != NULL) {
		if (*c != '\0') {
			gdk_keyboard_ungrab (gdk_time_get ());
			text_context->canvasitem = NULL;
			text_context->item = NULL;
			text_context->text = NULL;
			return;
		}
	}
	g_print ("empty text created - removing\n");
	sp_repr_unparent (text_context->text);

	sp_document_done (SP_DT_DOCUMENT (SP_EVENT_CONTEXT (text_context)->desktop));

	text_context->text = NULL;
}
#endif

static void
sp_text_context_selection_changed (SPSelection *selection, SPTextContext *tc)
{
	SPItem *item;

	tc->item = NULL;

	item = sp_selection_item (selection);

	if (SP_IS_TEXT (item)) {
		tc->item = item;
	}
}



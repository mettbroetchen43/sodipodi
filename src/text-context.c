#define __SP_TEXT_CONTEXT_C__

/*
 * SPTextContext
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include <ctype.h>
#include <libart_lgpl/art_affine.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkmain.h>
#include <gal/widgets/e-unicode.h>
#include <helper/sp-ctrlline.h>
#include "sp-text.h"
#include "sodipodi.h"
#include "document.h"
#include "style.h"
#include "selection.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "pixmaps/cursor-text.xpm"
#include "text-context.h"

static void sp_text_context_class_init (SPTextContextClass * klass);
static void sp_text_context_init (SPTextContext * text_context);
static void sp_text_context_finalize (GtkObject *object);

static void sp_text_context_setup (SPEventContext *ec);
static void sp_text_context_finish (SPEventContext *ec);
static gint sp_text_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_text_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static void sp_text_context_selection_changed (SPSelection *selection, SPTextContext *tc);
static void sp_text_context_selection_modified (SPSelection *selection, guint flags, SPTextContext *tc);

static void sp_text_context_update_cursor (SPTextContext *tc);
static gint sp_text_context_timeout (SPTextContext *tc);
static void sp_text_context_forget_text (SPTextContext *tc);

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

	object_class->finalize = sp_text_context_finalize;

	event_context_class->setup = sp_text_context_setup;
	event_context_class->finish = sp_text_context_finish;
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

	tc->text = NULL;
	tc->pdoc.x = 0.0;
	tc->pdoc.y = 0.0;
	tc->ipos = 0;

	tc->unimode = FALSE;

	tc->cursor = NULL;
	tc->timeout = 0;
	tc->show = FALSE;
	tc->phase = 0;
}

static void
sp_text_context_finalize (GtkObject *object)
{
	SPEventContext *ec;
	SPTextContext *tc;

	ec = SP_EVENT_CONTEXT (object);
	tc = SP_TEXT_CONTEXT (object);

	if (tc->timeout) {
		gtk_timeout_remove (tc->timeout);
	}

	if (tc->cursor) {
		gtk_object_destroy (GTK_OBJECT (tc->cursor));
	}

	if (ec->desktop) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (SP_DT_CANVAS (ec->desktop)), tc);
		gtk_signal_disconnect_by_data (GTK_OBJECT (SP_DT_SELECTION (ec->desktop)), ec);
	}

	GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint
sptc_focus_in (GtkWidget *widget, GdkEventFocus *event, SPTextContext *tc)
{
	gdk_im_begin (tc->ic, GTK_WIDGET (SP_DT_CANVAS (SP_EVENT_CONTEXT (tc)->desktop))->window);

	return FALSE;
}

static gint
sptc_focus_out (GtkWidget *widget, GdkEventFocus *event, SPTextContext *tc)
{
	gdk_im_end ();

	return FALSE;
}

static void
sp_text_context_setup (SPEventContext *ec)
{
	SPTextContext *tc;
	SPDesktop *desktop;

	tc = SP_TEXT_CONTEXT (ec);
	desktop = ec->desktop;

	tc->cursor = sp_canvas_item_new (SP_DT_CONTROLS (desktop), SP_TYPE_CTRLLINE, NULL);
	sp_ctrlline_set_coords (SP_CTRLLINE (tc->cursor), 100, 0, 100, 100);
	sp_canvas_item_hide (tc->cursor);

	tc->timeout = gtk_timeout_add (250, (GtkFunction) sp_text_context_timeout, ec);

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
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (ec);

	gtk_signal_connect (GTK_OBJECT (SP_DT_SELECTION (desktop)), "changed", GTK_SIGNAL_FUNC (sp_text_context_selection_changed), tc);
	gtk_signal_connect (GTK_OBJECT (SP_DT_SELECTION (desktop)), "modified", GTK_SIGNAL_FUNC (sp_text_context_selection_modified), tc);

	sp_text_context_selection_changed (SP_DT_SELECTION (desktop), tc);
}

static void
sp_text_context_finish (SPEventContext *ec)
{
	SPTextContext *tc;

	tc = SP_TEXT_CONTEXT (ec);

#ifdef SP_TC_XIM
	gdk_ic_destroy (tc->ic);
	gdk_ic_attr_destroy (tc->ic_attr);
	gdk_window_set_events (GTK_WIDGET (SP_DT_CANVAS (ec->desktop))->window, tc->savedmask);
#endif

	if (tc->timeout) {
		gtk_timeout_remove (tc->timeout);
		tc->timeout = 0;
	}

	if (tc->cursor) {
		gtk_object_destroy (GTK_OBJECT (tc->cursor));
		tc->cursor = 0;
	}

	if (ec->desktop) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (SP_DT_CANVAS (ec->desktop)), tc);
		gtk_signal_disconnect_by_data (GTK_OBJECT (SP_DT_SELECTION (ec->desktop)), ec);
	}

	sp_text_context_forget_text (tc);
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
	SPTSpan *new;
	SPStyle *style;
	guchar *utf8;
	gint ret;

	tc = SP_TEXT_CONTEXT (ec);

	ret = FALSE;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			NRPointF dtp;
			/* Button 1, set X & Y & new item */
			sp_selection_empty (SP_DT_SELECTION (ec->desktop));
			sp_desktop_w2d_xy_point (ec->desktop, &dtp, event->button.x, event->button.y);
			sp_desktop_dt2root_xy_point (ec->desktop, &tc->pdoc, dtp.x, dtp.y);
			/* Cursor */
			tc->show = TRUE;
			tc->phase = 1;
			sp_canvas_item_show (tc->cursor);
			sp_desktop_w2d_xy_point (ec->desktop, &dtp, event->button.x, event->button.y);
			sp_ctrlline_set_coords (SP_CTRLLINE (tc->cursor), dtp.x, dtp.y, dtp.x + 32, dtp.y);
			/* Processed */
			ret = TRUE;
		}
		break;
	case GDK_KEY_PRESS:
		if (!tc->text) {
			SPRepr *rtext, *rtspan, *rstring, *style;
			/* Create <text> */
			rtext = sp_repr_new ("text");
			/* Set style */
			style = sodipodi_get_repr (SODIPODI, "tools.text");
			if (style) {
				SPCSSAttr *css;
				css = sp_repr_css_attr_inherited (style, "style");
				sp_repr_css_set (rtext, css, "style");
				sp_repr_css_attr_unref (css);
			}
			sp_repr_set_double_attribute (rtext, "x", tc->pdoc.x);
			sp_repr_set_double_attribute (rtext, "y", tc->pdoc.y);
			/* Create <tspan> */
			rtspan = sp_repr_new ("tspan");
			sp_repr_add_child (rtext, rtspan, NULL);
			sp_repr_unref (rtspan);
			/* Create TEXT */
			rstring = sp_xml_document_createTextNode (sp_repr_document (rtext), "");
			sp_repr_add_child (rtspan, rstring, NULL);
			sp_repr_unref (rstring);
			sp_document_add_repr (SP_DT_DOCUMENT (ec->desktop), rtext);
			/* fixme: Is selection::changed really immediate? */
			sp_selection_set_repr (SP_DT_SELECTION (ec->desktop), rtext);
			sp_repr_unref (rtext);
			sp_document_done (SP_DT_DOCUMENT (ec->desktop));
		}

		g_assert (tc->text != NULL);
		style = SP_OBJECT_STYLE (tc->text);

		if (event->key.state & GDK_CONTROL_MASK) {
			switch (event->key.keyval) {
			case GDK_space:
				/* Nonbreaking space */
				tc->ipos = sp_text_insert (SP_TEXT (tc->text), tc->ipos, "\302\240", TRUE);
				break;
			case GDK_u:
				/* fixme: We need indication etc. for unicode mode */
				if (tc->unimode) {
					tc->unimode = FALSE;
				} else {
					tc->unimode = TRUE;
					tc->unipos = 0;
				}
				break;
			default:
				if (event->key.string) {
					utf8 = e_utf8_from_locale_string (event->key.string);
					tc->ipos = sp_text_insert (SP_TEXT (tc->text), tc->ipos, utf8, FALSE);
					if (utf8) g_free (utf8);
				}
				break;
			}
		} else {
			switch (event->key.keyval) {
			case GDK_Return:
				new = sp_text_insert_line (SP_TEXT (tc->text), tc->ipos);
				tc->ipos += 1;
				break;
			case GDK_space:
				tc->ipos = sp_text_insert (SP_TEXT (tc->text), tc->ipos, " ", FALSE);
				break;
			case GDK_BackSpace:
				tc->ipos = sp_text_delete (SP_TEXT (tc->text), MAX (tc->ipos - 1, 0), tc->ipos);
				break;
			case GDK_Delete:
				tc->ipos = sp_text_delete (SP_TEXT (tc->text), tc->ipos, MIN (tc->ipos + 1, sp_text_get_length (SP_TEXT (tc->text))));
				break;
			case GDK_Left:
				if (style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
					tc->ipos = sp_text_down (SP_TEXT (tc->text), tc->ipos);
				} else {
					tc->ipos = MAX (tc->ipos - 1, 0);
				}
				sp_text_context_update_cursor (tc);
				break;
			case GDK_Right:
				if (style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
					tc->ipos = sp_text_up (SP_TEXT (tc->text), tc->ipos);
				} else {
					tc->ipos = MIN (tc->ipos + 1, sp_text_get_length (SP_TEXT (tc->text)));
				}
				sp_text_context_update_cursor (tc);
				break;
			case GDK_Up:
				if (style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
					tc->ipos = MAX (tc->ipos - 1, 0);
				} else {
					tc->ipos = sp_text_up (SP_TEXT (tc->text), tc->ipos);
				}
				sp_text_context_update_cursor (tc);
				break;
			case GDK_Down:
				if (style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
					tc->ipos = MIN (tc->ipos + 1, sp_text_get_length (SP_TEXT (tc->text)));
				} else {
					tc->ipos = sp_text_down (SP_TEXT (tc->text), tc->ipos);
				}
				sp_text_context_update_cursor (tc);
				break;
			default:
				if (tc->unimode && isxdigit (event->key.keyval)) {
					tc->uni[tc->unipos] = event->key.keyval;
					if (tc->unipos == 3) {
						guchar u[7];
						guint uv, len;
						sscanf (tc->uni, "%x", &uv);
						len = g_unichar_to_utf8 (uv, u);
						u[len] = '\0';
						tc->ipos = sp_text_insert (SP_TEXT (tc->text), tc->ipos, u, FALSE);
						tc->unipos = 0;
					} else {
						tc->unipos += 1;
					}
				} else if (event->key.string) {
					tc->unimode = FALSE;
					utf8 = e_utf8_from_locale_string (event->key.string);
					tc->ipos = sp_text_insert (SP_TEXT (tc->text), tc->ipos, utf8, FALSE);
					if (utf8) g_free (utf8);
					break;
				}
			}
		}

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

static void
sp_text_context_selection_changed (SPSelection *selection, SPTextContext *tc)
{
	SPItem *item;

	item = sp_selection_item (selection);

	if (tc->text && (item != tc->text)) {
		sp_text_context_forget_text (tc);
	}
	tc->text = NULL;

	if (SP_IS_TEXT (item)) {
		tc->text = item;
		tc->ipos = sp_text_get_length (SP_TEXT (tc->text));
	} else {
		tc->text = NULL;
	}

	sp_text_context_update_cursor (tc);
}

static void
sp_text_context_selection_modified (SPSelection *selection, guint flags, SPTextContext *tc)
{
	sp_text_context_update_cursor (tc);
}

static void
sp_text_context_update_cursor (SPTextContext *tc)
{
	if (tc->text) {
		ArtPoint p0, p1;
		gdouble i2d[6];
		sp_text_get_cursor_coords (SP_TEXT (tc->text), tc->ipos, &p0, &p1);
		sp_item_i2d_affine (SP_ITEM (tc->text), i2d);
		art_affine_point (&p0, &p0, i2d);
		art_affine_point (&p1, &p1, i2d);
		sp_canvas_item_show (tc->cursor);
		sp_ctrlline_set_coords (SP_CTRLLINE (tc->cursor), p0.x, p0.y, p1.x, p1.y);
		tc->show = TRUE;
		tc->phase = 1;
	} else {
		sp_canvas_item_hide (tc->cursor);
		tc->show = FALSE;
	}
}

static gint
sp_text_context_timeout (SPTextContext *tc)
{
	if (tc->show) {
		if (tc->phase) {
			tc->phase = 0;
			sp_canvas_item_hide (tc->cursor);
		} else {
			tc->phase = 1;
			sp_canvas_item_show (tc->cursor);
		}
	}

	return TRUE;
}

static void
sp_text_context_forget_text (SPTextContext *tc)
{
	if (tc->text && sp_text_is_empty (SP_TEXT (tc->text))) {
		sp_repr_unparent (SP_OBJECT_REPR (tc->text));
	}

	tc->text = NULL;
}

#define SP_TEXT_CONTEXT_C

#include <math.h>
#include <gdk/gdkkeysyms.h>
#include "sp-text.h"
#include "sodipodi.h"
#include "document.h"
#include "selection.h"
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

static void sp_text_complete (SPTextContext * text_context);

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
sp_text_context_init (SPTextContext * text_context)
{
	SPEventContext * event_context;
	
	event_context = SP_EVENT_CONTEXT (text_context);

	event_context->cursor_shape = cursor_text_xpm;
	event_context->hot_x = 0;
	event_context->hot_y = 0;

	text_context->text = NULL;
	text_context->item = NULL;
	text_context->canvasitem = NULL;
}

static void
sp_text_context_destroy (GtkObject * object)
{
	SPTextContext * text_context;

	text_context = SP_TEXT_CONTEXT (object);

	if (text_context->text)
		sp_text_complete (text_context);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_text_context_setup (SPEventContext * event_context, SPDesktop * desktop)
{
	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);
}

static gint
sp_text_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	gint ret;
	gchar k[] = {'\0', '\0'};
	gchar * c;

	ret = FALSE;

	switch (event->type) {
	case GDK_KEY_PRESS:
                // filter control-modifier for desktop shortcuts
                if (event->key.state & GDK_CONTROL_MASK) return FALSE;
                                
		if (!SP_TEXT_CONTEXT (event_context)->text) {
			ret = FALSE;
			break;
		}
		if (event->key.keyval == GDK_Return) {
			k[0] = '\n';
		} else {
			if (event->key.keyval > 0x8000) {
				return TRUE;
			}
			k[0] = event->key.keyval;
		}
		c = (gchar *) sp_repr_content (SP_TEXT_CONTEXT (event_context)->text);
		if (c == NULL) c = "";
		c = g_strconcat (c, &k, NULL);
		sp_repr_set_content (SP_TEXT_CONTEXT (event_context)->text, c);
		sp_document_done (SP_DT_DOCUMENT (event_context->desktop));

		ret = TRUE;
		break;
	default:
		break;
	}

	if (!ret) {
		if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
			ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);
	}

	return ret;
}

static gint
sp_text_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	SPTextContext * text_context;
	SPDesktop * desktop;
	SPItem * item;
	ArtPoint p;
	gint ret;
	SPRepr * repr, * style;
	SPCSSAttr * css;

	ret = FALSE;

	text_context = SP_TEXT_CONTEXT (event_context);
	desktop = event_context->desktop;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			if (text_context->text)
				sp_text_complete (SP_TEXT_CONTEXT (event_context));

			sp_desktop_w2doc_xy_point (desktop, &p, event->button.x, event->button.y);

			/* Create object */
			repr = sp_repr_new ("text");
			/* Set style */
			style = sodipodi_get_repr (SODIPODI, "paint.text");
			if (style) {
				css = sp_repr_css_attr_inherited (style, "style");
				sp_repr_css_set (repr, css, "style");
				sp_repr_css_attr_unref (css);
			}
			text_context->text = repr;
			sp_repr_set_double_attribute (text_context->text, "x", p.x);
			sp_repr_set_double_attribute (text_context->text, "y", p.y);
			item = sp_document_add_repr (SP_DT_DOCUMENT (desktop), text_context->text);
			sp_document_done (SP_DT_DOCUMENT (desktop));
			sp_repr_unref (text_context->text);
/* fixme: */
			if (item != NULL) {
				text_context->item = item;
				text_context->canvasitem = sp_item_canvas_item (item, desktop);
				sp_selection_set_item (SP_DT_SELECTION (desktop), item);
				gnome_canvas_item_grab_focus (text_context->canvasitem);
			}

			ret = TRUE;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	if (!ret) {
		if (SP_EVENT_CONTEXT_CLASS (parent_class)->root_handler)
			ret = SP_EVENT_CONTEXT_CLASS (parent_class)->root_handler (event_context, event);
	}

	return ret;
}

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
	sp_document_del_repr (SP_DT_DOCUMENT (SP_EVENT_CONTEXT (text_context)->desktop), text_context->text);

	sp_document_done (SP_DT_DOCUMENT (SP_EVENT_CONTEXT (text_context)->desktop));

	text_context->text = NULL;
}


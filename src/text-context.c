#define TEXT_CONTEXT_C

#include <gdk/gdkkeysyms.h>
#include <libgnomeui/gnome-canvas.h>
#include "xml/repr.h"
#include "font-wrapper.h"
#include "sp-text.h"
#include "sp-rect.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "event-broker.h"
#include "selection.h"
#include "text-context.h"

static void sp_text_complete (void);

SPRepr * text;

GnomeCanvasItem * cursor = NULL;

static void sp_text_cursor_move (double x, double y, double size);

void
sp_text_context_set (SPDesktop * desktop)
{
	text = NULL;
}

void sp_text_release (SPDesktop * desktop) {

	if (text) {
		sp_text_complete ();
	}

	if (cursor) {
		gtk_object_destroy (GTK_OBJECT (cursor));
		cursor = NULL;
	}

	return;
}

static void sp_text_cursor_move (double x, double y, double size) {
#if 0
	if (cursor == NULL) {
		cursor = gnome_canvas_item_new (
			gnome_canvas_root (GNOME_CANVAS_ITEM (drawing_root_get ())->canvas),
			SP_TYPE_RECT,
			"x0", x, "y0", y - size,
			"x1", x+2, "y1", y,
			NULL);
	} else {
g_print ("moving\n");
		gnome_canvas_item_set (cursor,
			"x0", x, "y0", y - size,
			"x1", x+2, "y1", y,
			NULL);
	}
#endif
}

gint
sp_text_handler (SPDesktop * desktop, SPItem * item, GdkEvent * event)
{
	gint ret = FALSE;
	gchar k[] = {'\0', '\0'};
	gchar * c;

	switch (event->type) {
	case GDK_KEY_PRESS:
		if (event->key.keyval == GDK_Return) {
			k[0] = '\n';
		} else {
			if (event->key.keyval > 0x8000) {
				return TRUE;
			}
			k[0] = event->key.keyval;
		}
		c = (gchar *) sp_repr_content (text);
		if (c == NULL) c = "";
		c = g_strconcat (c, &k, NULL);
		sp_repr_set_content (text, c);
#if 0
		sp_editable_bbox (SP_EDITABLE (text), &bbox);
		sp_text_cursor_move (bbox.x1, bbox.y1, SP_TEXT (text)->size);
#endif
		ret = TRUE;
		break;
	default:
		break;
	}

	return ret;
}

gint
sp_text_root_handler (SPDesktop * desktop, GdkEvent * event)
{
	SPItem * item;
	gint ret = FALSE;
	ArtPoint p;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			if (text)
				sp_text_complete ();

			sp_desktop_w2doc_xy_point (desktop, &p, event->button.x, event->button.y);

			text = sp_repr_new_with_name ("text");
			sp_repr_set_double_attribute (text, "x", p.x);
			sp_repr_set_double_attribute (text, "y", p.y);
			item = sp_document_add_repr (SP_DT_DOCUMENT (desktop), text);
			sp_repr_unref (text);
/* fixme: */
			if (item != NULL) {
				sp_selection_set_item (SP_DT_SELECTION (desktop), item);
				gnome_canvas_item_grab_focus (sp_item_canvas_item (item, SP_DT_CANVAS (desktop)));
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

	return ret;
}


static void
sp_text_complete (void)
{
	gchar * c;

	if (text == NULL)
		return;

	c = (gchar *) sp_repr_content (text);
	if (c != NULL) {
		if (*c != '\0') {
			text = NULL;
			return;
		}
	}
	g_print ("empty text created - removing\n");
	sp_repr_unparent_and_destroy (text);
#if 0
	sp_selection_empty ();
#endif
	text = NULL;
}

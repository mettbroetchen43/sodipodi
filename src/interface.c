#define SP_INTERFACE_C

#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-stock.h>
#include <libgnomeui/gnome-messagebox.h>
#include <libgnomeui/gnome-window-icon.h>
#include "sodipodi.h"
#include "document.h"
#include "desktop-handles.h"
#include "file.h"
#include "interface.h"
#include "desktop.h"
#include "svg-view.h"

#include "dialogs/text-edit.h"
#include "dialogs/export.h"
#include "dialogs/xml-tree.h"
#include "dialogs/align.h"
#include "dialogs/transformation.h"
#include "dialogs/object-properties.h"
#include "dialogs/desktop-properties.h"
#include "dialogs/document-properties.h"
#include "dialogs/display-settings.h"

void fake_dialogs (void);

static gint sp_ui_delete (GtkWidget *widget, GdkEvent *event, SPViewWidget *vw);

void
sp_create_window (SPViewWidget *vw, gboolean editable)
{
	GtkWidget *w;

	g_return_if_fail (vw != NULL);
	g_return_if_fail (SP_IS_VIEW_WIDGET (vw));

	w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_object_set_data (GTK_OBJECT (vw), "window", w);

	/* fixme: */
	if (editable) {
		gtk_window_set_default_size ((GtkWindow *) w, 400, 400);
		gtk_object_set_data (GTK_OBJECT (w), "desktop", SP_DESKTOP_WIDGET (vw)->desktop);
		gtk_object_set_data (GTK_OBJECT (w), "desktopwidget", vw);
		gtk_signal_connect (GTK_OBJECT (w), "delete_event", GTK_SIGNAL_FUNC (sp_ui_delete), vw);
		gtk_signal_connect (GTK_OBJECT (w), "focus_in_event", GTK_SIGNAL_FUNC (sp_desktop_set_focus), SP_DESKTOP_WIDGET (vw)->desktop);
#if 0
		sp_desktop_set_title (desktop);
#endif
	} else {
		gtk_window_set_policy (GTK_WINDOW (w), TRUE, TRUE, TRUE);
	}

	gtk_container_add (GTK_CONTAINER (w), GTK_WIDGET (vw));
	gtk_widget_show (GTK_WIDGET (vw));

	gnome_window_icon_set_from_default (GTK_WINDOW (w));

	gtk_widget_show (w);
}

void
sp_ui_new_view (GtkWidget * widget)
{
	SPDocument * document;
	SPViewWidget *dtw;

	document = SP_ACTIVE_DOCUMENT;
	if (!document) return;

	dtw = sp_desktop_widget_new (document, sp_document_namedview (document, NULL));
	g_return_if_fail (dtw != NULL);

	sp_create_window (dtw, TRUE);
}

/* fixme: */
void
sp_ui_new_view_preview (GtkWidget *widget)
{
	SPDocument *document;
	SPViewWidget *dtw;

	document = SP_ACTIVE_DOCUMENT;
	if (!document) return;

	dtw = (SPViewWidget *) sp_svg_view_widget_new (document);
	g_return_if_fail (dtw != NULL);
#if 1
	sp_svg_view_widget_set_resize (SP_SVG_VIEW_WIDGET (dtw), TRUE, 400.0, 400.0);
#endif

	sp_create_window (dtw, FALSE);
}

void
sp_ui_close_view (GtkWidget * widget)
{
	gpointer w;

	if (SP_ACTIVE_DESKTOP == NULL) return;

	if (sp_ui_delete (NULL, NULL, SP_VIEW_WIDGET ((SP_ACTIVE_DESKTOP)->owner))) return;

	w = gtk_object_get_data (GTK_OBJECT (SP_ACTIVE_DESKTOP), "window");

	if (w) gtk_object_destroy (GTK_OBJECT (w));

	/*
	 * We have to fake dialog initialization, or corresponding code is
	 * not compiled into application.
	 */

	if (GTK_IS_WIDGET (SODIPODI)) fake_dialogs ();
}

static gint
sp_ui_delete (GtkWidget *widget, GdkEvent *event, SPViewWidget *vw)
{
	return sp_view_widget_shutdown (vw);
}

void
fake_dialogs (void)
{
	sp_object_properties_dialog ();
    	sp_object_properties_fill ();
    	sp_object_properties_stroke ();
    	sp_object_properties_layout ();
	sp_text_edit_dialog ();
	sp_export_dialog ();
	sp_xml_tree_dialog ();
	sp_quick_align_dialog ();
	sp_transformation_dialog ();
	sp_desktop_dialog ();
	sp_document_dialog ();
	sp_display_dialog ();
}





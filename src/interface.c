#define SP_INTERFACE_C

#include "sodipodi.h"
#include "interface.h"

#include "dialogs/text-edit.h"
#include "dialogs/export.h"
#include "dialogs/xml-tree.h"
#include "dialogs/align.h"
#include "dialogs/transformation.h"
#include "dialogs/object-properties.h"
#include "dialogs/desktop-properties.h"
#include "dialogs/document-properties.h"

void fake_dialogs (void);

void
sp_create_window (SPDesktop * desktop, gboolean editable)
{
	GtkWidget * w, * vb, * hb, * b;

	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size ((GtkWindow *) w, 400, 400);
	gtk_object_set_data (GTK_OBJECT (desktop), "window", w);
	gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);

	vb = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vb);
	gtk_container_add (GTK_CONTAINER (w), vb);

	gtk_box_pack_start (GTK_BOX (vb), GTK_WIDGET (desktop), TRUE, TRUE, 0);
	gtk_widget_show (GTK_WIDGET (desktop));

	hb = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hb);
	gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 0);

#if 0
	b = gtk_toggle_button_new_with_label ("Show guides");
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (hb), b, FALSE, FALSE, 0);

	b = gtk_toggle_button_new_with_label ("Snap to guides");
	gtk_widget_show (b);
	gtk_box_pack_start (GTK_BOX (hb), b, FALSE, FALSE, 0);
#endif

	gnome_window_icon_set_from_default (GTK_WINDOW (w));

	gtk_widget_show (w);
}

void
sp_ui_new_view (GtkWidget * widget)
{
	SPDocument * document;
	SPDesktop * desktop;

	document = SP_ACTIVE_DOCUMENT;
	if (!document) return;

	desktop = sp_desktop_new (document, sp_document_namedview (document, NULL));
	g_return_if_fail (desktop != NULL);

	sp_create_window (desktop, TRUE);
}

void
sp_ui_close_view (GtkWidget * widget)
{
	gpointer w;

	if (SP_ACTIVE_DESKTOP == NULL) return;

	w = gtk_object_get_data (GTK_OBJECT (SP_ACTIVE_DESKTOP), "window");

	if (w) gtk_object_destroy (GTK_OBJECT (w));

	/*
	 * We have to fake dialog initialization, or corresponding code is
	 * not compiled into application.
	 */

	if (GTK_IS_WIDGET (SODIPODI)) fake_dialogs ();
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
}





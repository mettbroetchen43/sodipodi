#define __HELP_C__

#include <gtk/gtkwindow.h>
#include <gtk/gtksignal.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "document.h"
#include "sp-object.h"
#include "svg-view.h"
#include "help.h"

static gint
sp_help_about_delete (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	return FALSE;
}

void
sp_help_about (void)
{
	SPDocument *doc;
	SPObject *title;
	GtkWidget *w, *v;

	doc = sp_document_new (SODIPODI_GLADEDIR "/about.svg");
	g_return_if_fail (doc != NULL);
	title = sp_document_lookup_id (doc, "title");
	if (title) {
		gchar *t;
		t = g_strdup_printf ("Sodipodi %s", SODIPODI_VERSION);
		sp_repr_set_content (SP_OBJECT_REPR (title), t);
	}

	w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (w), _("About sodipodi"));
#if 1
	gtk_window_set_policy (GTK_WINDOW (w), TRUE, TRUE, FALSE);
#endif
	gtk_signal_connect (GTK_OBJECT (w), "delete_event", GTK_SIGNAL_FUNC (sp_help_about_delete), NULL);

	v = sp_svg_view_widget_new (doc);
	sp_document_unref (doc);
	sp_svg_view_widget_set_resize (SP_SVG_VIEW_WIDGET (v), FALSE, 600.0, 400.0);
	gtk_widget_show (v);
	gtk_container_add (GTK_CONTAINER (w), v);

	gtk_widget_show (w);
}

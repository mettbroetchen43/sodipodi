#define SP_MDI_C

#include <libgnomeui/gnome-mdi.h>
#include <glade/glade.h>
#include "mdi.h"

static void sp_mdi_destroy (GnomeMDI * mdi);
static gint sp_mdi_add_child (GnomeMDI * mdi, GnomeMDIChild * child, gpointer data);
static gint sp_mdi_remove_child (GnomeMDI * mdi, GnomeMDIChild * child, gpointer data);
static gint sp_mdi_add_view (GnomeMDI * mdi, GtkWidget * view, gpointer data);
static gint sp_mdi_remove_view (GnomeMDI * mdi, GtkWidget * view, gpointer data);
static void sp_mdi_child_changed (GnomeMDI * mdi, GnomeMDIChild * child, gpointer data);
static void sp_mdi_view_changed (GnomeMDI * mdi, GtkWidget * view, gpointer data);
static void sp_mdi_app_created (GnomeMDI * mdi, GnomeApp * app, gpointer data);

GnomeMDI * sodipodi = NULL;

void
sp_mdi_create (void)
{
	sodipodi = GNOME_MDI (gnome_mdi_new ("Sodipodi", "Sodipodi"));

	gtk_signal_connect (GTK_OBJECT (sodipodi), "destroy",
		GTK_SIGNAL_FUNC (sp_mdi_destroy), NULL);
	gtk_signal_connect (GTK_OBJECT (sodipodi), "add_child",
		GTK_SIGNAL_FUNC (sp_mdi_add_child), NULL);
	gtk_signal_connect (GTK_OBJECT (sodipodi), "remove_child",
		GTK_SIGNAL_FUNC (sp_mdi_remove_child), NULL);
	gtk_signal_connect (GTK_OBJECT (sodipodi), "add_view",
		GTK_SIGNAL_FUNC (sp_mdi_add_view), NULL);
	gtk_signal_connect (GTK_OBJECT (sodipodi), "remove_view",
		GTK_SIGNAL_FUNC (sp_mdi_remove_view), NULL);
	gtk_signal_connect (GTK_OBJECT (sodipodi), "child_changed",
		GTK_SIGNAL_FUNC (sp_mdi_child_changed), NULL);
	gtk_signal_connect (GTK_OBJECT (sodipodi), "view_changed",
		GTK_SIGNAL_FUNC (sp_mdi_view_changed), NULL);
	gtk_signal_connect (GTK_OBJECT (sodipodi), "app_created",
		GTK_SIGNAL_FUNC (sp_mdi_app_created), NULL);
}

static gint
sp_mdi_add_child (GnomeMDI * mdi, GnomeMDIChild * child, gpointer data)
{
	g_print ("app: add_child\n");

	return TRUE;
}

static gint
sp_mdi_remove_child (GnomeMDI * mdi, GnomeMDIChild * child, gpointer data)
{
	g_print ("app: remove_child\n");

	return TRUE;
}

static gint
sp_mdi_add_view (GnomeMDI * mdi, GtkWidget * view, gpointer data)
{
	g_print ("app: add_view\n");

	return TRUE;
}

static gint
sp_mdi_remove_view (GnomeMDI * mdi, GtkWidget * view, gpointer data)
{
	g_print ("app: remove_view\n");

	return TRUE;
}

static void
sp_mdi_child_changed (GnomeMDI * mdi, GnomeMDIChild * child, gpointer data)
{
	g_print ("app: child_changed\n");
}

static void
sp_mdi_view_changed (GnomeMDI * mdi, GtkWidget * view, gpointer data)
{
	g_print ("app: view_changed\n");
}

static void
sp_mdi_app_created (GnomeMDI * mdi, GnomeApp * app, gpointer data)
{
	static GladeXML * xmlmenu = NULL;
	static GladeXML * xmlfile = NULL;
	static GladeXML * xmledit = NULL;
	static GladeXML * xmlcontext = NULL;
	GnomeDockItem * ditem;

	/* fixme: just cannot understand, why libglade does not set */
	/* gnome_dock_item name ? */

	xmlmenu = glade_xml_new (SODIPODI_GLADEDIR "/sodipodi.glade", "dockitem_menu");
	if (xmlmenu != NULL) {
		glade_xml_signal_autoconnect (xmlmenu);
		ditem = (GnomeDockItem *) glade_xml_get_widget (xmlmenu, "dockitem_menu");
		ditem->name = g_strdup ("dockitem_menu");
		gnome_app_add_dock_item (app, ditem, GNOME_DOCK_TOP, 0, 0, 0);
	} else {
		g_log ("Sodipodi", G_LOG_LEVEL_ERROR,
			"sp_mdi_app_created: cannot create menubar\n");
	}

	xmlfile = glade_xml_new (SODIPODI_GLADEDIR "/sodipodi.glade", "dockitem_file");
	if (xmlfile != NULL) {
		glade_xml_signal_autoconnect (xmlfile);
		ditem = (GnomeDockItem *) glade_xml_get_widget (xmlfile, "dockitem_file");
		ditem->name = g_strdup ("dockitem_file");
		gnome_app_add_dock_item (app, ditem, GNOME_DOCK_TOP, 1, 0, 0);
	} else {
		g_log ("Sodipodi", G_LOG_LEVEL_ERROR,
			"sp_mdi_app_created: cannot create file toolbar\n");
	}

	xmledit = glade_xml_new (SODIPODI_GLADEDIR "/sodipodi.glade", "dockitem_edit");
	if (xmledit != NULL) {
		glade_xml_signal_autoconnect (xmledit);
		ditem = (GnomeDockItem *) glade_xml_get_widget (xmledit, "dockitem_edit");
		ditem->name = g_strdup ("dockitem_edit");
		gnome_app_add_dock_item (app, ditem, GNOME_DOCK_TOP, 1, 1, 0);
	} else {
		g_log ("Sodipodi", G_LOG_LEVEL_ERROR,
			"sp_mdi_app_created: cannot create edit toolbar\n");
	}

	xmlcontext = glade_xml_new (SODIPODI_GLADEDIR "/sodipodi.glade", "dockitem_context");
	if (xmlcontext != NULL) {
		glade_xml_signal_autoconnect (xmlcontext);
		ditem = (GnomeDockItem *) glade_xml_get_widget (xmlcontext, "dockitem_context");
		ditem->name = g_strdup ("dockitem_context");
		gnome_app_add_dock_item (app, ditem, GNOME_DOCK_LEFT, 0, 0, 0);
	} else {
		g_log ("Sodipodi", G_LOG_LEVEL_ERROR,
			"sp_mdi_app_created: cannot create context toolbar\n");
	}
}

static void
sp_mdi_destroy (GnomeMDI * mdi)
{
	gtk_main_quit ();
}



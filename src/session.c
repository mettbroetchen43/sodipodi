#define SESSION_C

#include "mdi-child.h"
#include "sodipodi.h"
#include "session.h"

static GnomeMDIChild * sp_sm_create_child_from_config (const gchar * config);

gint
sp_sm_save_yourself (GnomeClient * client,
	gint phase,
	GnomeSaveStyle save_style,
	gint is_shutdown,
	GnomeInteractStyle interact_style,
	gint is_fast,
	gpointer client_data)
{
	gchar * prefix;
	gchar * argv[] = {"rm", "-r", NULL};

	prefix = gnome_client_get_config_prefix (client);
	gnome_config_push_prefix (prefix);

	gnome_mdi_save_state (SODIPODI, "Session");

	gnome_config_pop_prefix ();
	gnome_config_sync ();

	argv[2] = gnome_config_get_real_path (prefix);
	gnome_client_set_discard_command (client, 3, argv);

	argv[0] = (gchar *) client_data;
	gnome_client_set_clone_command (client, 1, argv);
	gnome_client_set_restart_command (client, 1, argv);

	return TRUE;
}


gint
sp_sm_die (GnomeClient * client, gpointer client_data)
{
	gtk_main_quit ();

	return FALSE;
}


gint
sp_sm_restore_children (void)
{
	gint restored;

	restored = gnome_mdi_restore_state (SODIPODI, "Session",
		(GnomeMDIChildCreator) sp_sm_create_child_from_config);

	return restored;
}

static GnomeMDIChild *
sp_sm_create_child_from_config (const gchar * config)
{
	SPDocument * doc;
	SPMDIChild * child;

	doc = sp_document_new (config);
	g_return_val_if_fail (doc != NULL, NULL);

	child = sp_mdi_child_new (doc);
	g_return_val_if_fail (child != NULL, NULL);

	gnome_mdi_add_child (SODIPODI, GNOME_MDI_CHILD (child));
	gnome_mdi_add_view (SODIPODI, GNOME_MDI_CHILD (child));

	return GNOME_MDI_CHILD (child);
}



#define SP_NODE_PATH_EDIT_C

#include <glade/glade.h>
#include "sp-node-path-edit.h"

GladeXML * node_path_xml = NULL;
GtkWidget * node_path_dialog = NULL;

void
sp_node_path_edit_dialog (void)
{
	if (node_path_dialog == NULL) {
		node_path_xml = glade_xml_new (SODIPODI_GLADEDIR "/sodipodi.glade", "window_node_path");
		if (node_path_xml == NULL) return;
		glade_xml_signal_autoconnect (node_path_xml);
		node_path_dialog = glade_xml_get_widget (node_path_xml, "window_node_path");
		if (node_path_dialog == NULL) return;
	} else {
		if (!GTK_WIDGET_VISIBLE (node_path_dialog))
			gtk_widget_show (node_path_dialog);
	}
}

void
sp_node_path_edit_dialog_close (void)
{
	g_assert (node_path_dialog != NULL);

	if (GTK_WIDGET_VISIBLE (node_path_dialog))
		gtk_widget_hide (node_path_dialog);
}

void
sp_node_path_edit_add (void)
{
	sp_node_selected_add_node ();
}

void
sp_node_path_edit_delete (void)
{
	sp_node_selected_delete ();
}

void
sp_node_path_edit_break (void)
{
	sp_node_selected_break ();
}

void
sp_node_path_edit_join (void)
{
	sp_node_selected_join ();
}

void
sp_node_path_edit_toline (void)
{
	sp_node_selected_set_line_type (ART_LINETO);
}

void
sp_node_path_edit_tocurve (void)
{
	sp_node_selected_set_line_type (ART_CURVETO);
}

void
sp_node_path_edit_cusp (void)
{
	sp_node_selected_set_type (SP_PATHNODE_CUSP);
}

void
sp_node_path_edit_smooth (void)
{
	sp_node_selected_set_type (SP_PATHNODE_SMOOTH);
}

void
sp_node_path_edit_symmetric (void)
{
	sp_node_selected_set_type (SP_PATHNODE_SYMM);
}


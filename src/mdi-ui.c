#define SP_MDI_UI_C

#include "mdi-child.h"
#include "mdi-document.h"
#include "mdi-ui.h"

#include "dialogs/object-fill.h"
#include "dialogs/object-stroke.h"
#include "dialogs/text-edit.h"
#include "dialogs/export.h"
#include "dialogs/xml-tree.h"

/* Unreachable code, which guarantees that reqd functions are linked */

void sp_mdi_dialog_handles (void);

void
sp_mdi_new_view (void)
{
	SPDocument * document;
	SPMDIChild * new_child;

	document = SP_ACTIVE_DOCUMENT;

	g_return_if_fail (document != NULL);
	g_return_if_fail (SP_IS_DOCUMENT (document));
	
	new_child = sp_mdi_child_new (document);

	g_return_if_fail (new_child != NULL);
	g_return_if_fail (SP_IS_MDI_CHILD (new_child));

	gnome_mdi_add_view (SODIPODI, GNOME_MDI_CHILD (new_child));
}

void
sp_mdi_close_view (void)
{
	GtkWidget * active_view;

	active_view = gnome_mdi_get_active_view (SODIPODI);

	g_return_if_fail (active_view != NULL);
	g_return_if_fail (GTK_IS_WIDGET (active_view));

	gnome_mdi_remove_view (SODIPODI, active_view, FALSE);
}

/* fixme: */

void
sp_mdi_dialog_handles (void)
{
	sp_object_fill_dialog ();
	sp_object_stroke_dialog ();
	sp_text_edit_dialog ();
	sp_export_dialog ();
	sp_xml_tree_dialog ();
}

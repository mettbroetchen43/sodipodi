#define SP_MDI_DOCUMENT_C

#include "mdi-child.h"
#include "mdi-document.h"

SPDocument *
sp_active_document (GnomeMDI * mdi)
{
	GnomeMDIChild * active_child;

	g_return_val_if_fail (mdi != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_MDI (mdi), NULL);

	active_child = gnome_mdi_get_active_child (mdi);

	g_return_val_if_fail (active_child != NULL, NULL);
	g_return_val_if_fail (SP_IS_MDI_CHILD (active_child), NULL);

	return SP_MDI_CHILD (active_child)->document;
}

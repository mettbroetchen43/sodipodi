#define SP_MDI_DOCUMENT_C

#include "mdi-document.h"
#include "mdi-desktop.h"
#include "desktop-handles.h"

SPDocument *
sp_active_document (GnomeMDI * mdi)
{
	SPDesktop * desktop;

	desktop = SP_ACTIVE_DESKTOP;

	if (desktop == NULL) return NULL;

	return SP_DT_DOCUMENT (desktop);
}

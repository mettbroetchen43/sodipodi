#ifndef SP_MDI_DOCUMENT_H
#define SP_MDI_DOCUMENT_H

/*
 * mdi-document.h
 *
 * here we define MDI_ACTIVE_DOCUMENT macro
 *
 */

#include "sodipodi.h"
#include "document.h"

#define SP_ACTIVE_DOCUMENT (sp_active_document (SODIPODI))

SPDocument * sp_active_document (GnomeMDI * mdi);

#endif

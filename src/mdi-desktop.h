#ifndef SP_MDI_DESKTOP_H
#define SP_MDI_DESKTOP_H

/*
 * mdi-desktop.h
 *
 * Here we define SP_ACTIVE_DESKTOP macro
 *
 */

#include "sodipodi.h"

#ifndef SP_DESKTOP_DEFINED
#define SP_DESKTOP_DEFINED
typedef struct _SPDesktop SPDesktop;
typedef struct _SPDesktopClass SPDesktopClass;
#endif

#define SP_ACTIVE_DESKTOP (sp_active_desktop (SODIPODI))

SPDesktop * sp_active_desktop (GnomeMDI * mdi);

void sp_active_desktop_set (SPDesktop * desktop);

#endif

#ifndef SODIPODI_H
#define SODIPODI_H

/*
 * sodipodi.h
 *
 * Here we define SODIPODI, which is a center of universe, although it
 * does not do anything itself
 *
 */

#include <libgnomeui/gnome-mdi.h>

#define SODIPODI sodipodi

#ifndef SP_MDI_C
	extern GnomeMDI * sodipodi;
#endif

#endif

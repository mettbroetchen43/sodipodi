#ifndef SP_DESKTOP_SNAP_H
#define SP_DESKTOP_SNAP_H

/*
 * desktop-snap
 *
 * Here are functions calculating snap positions on desktop
 *
 * Copyright (C) Lauris Kaplinski, 2000
 *
 */

#include "desktop.h"

gdouble sp_desktop_free_snap (SPDesktop * desktop, ArtPoint * req);
gdouble sp_desktop_horizontal_snap (SPDesktop * desktop, ArtPoint * req);
gdouble sp_desktop_vertical_snap (SPDesktop * desktop, ArtPoint * req);
gdouble sp_desktop_vector_snap (SPDesktop * desktop, ArtPoint * req, gdouble dx, gdouble dy);
gdouble sp_desktop_circular_snap (SPDesktop * desktop, ArtPoint * req, gdouble cx, gdouble cy);

#endif

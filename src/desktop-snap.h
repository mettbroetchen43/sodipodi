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

#include <libart_lgpl/art_point.h>
#include "desktop.h"

// snap points
gdouble sp_desktop_free_snap (SPDesktop * desktop, ArtPoint * req);
gdouble sp_desktop_horizontal_snap (SPDesktop * desktop, ArtPoint * req);
gdouble sp_desktop_vertical_snap (SPDesktop * desktop, ArtPoint * req);
gdouble sp_desktop_vector_snap (SPDesktop * desktop, ArtPoint * req, gdouble dx, gdouble dy);
gdouble sp_desktop_circular_snap (SPDesktop * desktop, ArtPoint * req, gdouble cx, gdouble cy);

// snap list of points
gdouble sp_desktop_horizontal_snap_list (SPDesktop * desktop, GSList * l, gdouble dx);
gdouble sp_desktop_vertical_snap_list (SPDesktop * desktop, GSList * l, gdouble dy);
gdouble sp_desktop_horizontal_snap_list_scale (SPDesktop * desktop, GSList * l, ArtPoint * norm, gdouble sx);
gdouble sp_desktop_vertical_snap_list_scale (SPDesktop * desktop, GSList * l, ArtPoint * norm, gdouble sy);
gdouble sp_desktop_vector_snap_list (SPDesktop * desktop, GSList * l, ArtPoint * norm, gdouble sx, gdouble sy);
gdouble sp_desktop_horizontal_snap_list_skew (SPDesktop * desktop, GSList * l, ArtPoint * norm, gdouble sy);
gdouble sp_desktop_vertical_snap_list_skew (SPDesktop * desktop, GSList * l, ArtPoint * norm, gdouble sx);
gdouble * sp_desktop_circular_snap_list (SPDesktop * desktop, GSList * l, ArtPoint * norm, gdouble * rotate);



#endif

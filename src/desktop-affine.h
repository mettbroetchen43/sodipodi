#ifndef SP_DESKTOP_AFFINE_H
#define SP_DESKTOP_AFFINE_H

/*
 * desktop-affine
 *
 * Here are collected all useful affine transformation utilities
 * for canvas <-> desktop <-> document
 *
 * w2d - canvas world to desktop
 * w2doc - canvas world to document root
 * d2doc - dektop to document root
 *
 * _affine sets its arg affine to specified transformation
 * _xy_point transforms (x, y) in source coords to (ArtPoint *) in dest
 *
 */

#include <glib.h>
#include <libart_lgpl/art_point.h>

#ifndef SP_DESKTOP_DEFINED
#define SP_DESKTOP_DEFINED
typedef struct _SPDesktop SPDesktop;
typedef struct _SPDesktopClass SPDesktopClass;
#endif

gdouble * sp_desktop_w2d_affine (SPDesktop * desktop, gdouble w2d[]);
gdouble * sp_desktop_d2w_affine (SPDesktop * desktop, gdouble d2w[]);
gdouble * sp_desktop_w2doc_affine (SPDesktop * desktop, gdouble w2doc[]);
gdouble * sp_desktop_doc2w_affine (SPDesktop * desktop, gdouble doc2w[]);
gdouble * sp_desktop_d2doc_affine (SPDesktop * desktop, gdouble d2doc[]);
gdouble * sp_desktop_doc2d_affine (SPDesktop * desktop, gdouble doc2d[]);

ArtPoint * sp_desktop_w2d_xy_point (SPDesktop * desktop, ArtPoint * p, double x, double y);
ArtPoint * sp_desktop_d2w_xy_point (SPDesktop * desktop, ArtPoint * p, double x, double y);
ArtPoint * sp_desktop_w2doc_xy_point (SPDesktop * desktop, ArtPoint * p, double x, double y);
ArtPoint * sp_desktop_doc2w_xy_point (SPDesktop * desktop, ArtPoint * p, double x, double y);
ArtPoint * sp_desktop_d2doc_xy_point (SPDesktop * desktop, ArtPoint * p, double x, double y);
ArtPoint * sp_desktop_doc2d_xy_point (SPDesktop * desktop, ArtPoint * p, double x, double y);

#endif

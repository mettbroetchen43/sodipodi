#ifndef SP_ART_UTILS_H
#define SP_ART_UTILS_H

#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_alphagamma.h>
#include <libart_lgpl/art_affine.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_uta.h>

void
art_rgb_svp_rgba (const ArtSVP *svp,
                  int x0, int y0, int x1, int y1,
                  art_u32 *src,
                  int sx, int sy, int srs,
                  art_u8 *buf, int rowstride,
                  ArtAlphaGamma *alphagamma);

ArtSVP * art_svp_translate (const ArtSVP * svp, double dx, double dy);
ArtUta * art_uta_from_svp_translated (const ArtSVP * svp, double cx, double cy);

int art_affine_is_identity (double affine[]);

#endif


#define SP_ART_UTILS_C

#include <libart_lgpl/art_misc.h>

#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_render_aa.h>
#include <libart_lgpl/art_rgb.h>
#include <libart_lgpl/art_rgb_svp.h>
#include <libart_lgpl/art_uta.h>
#include <libart_lgpl/art_uta_svp.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_uta_vpath.h>
#include <libart_lgpl/art_vpath_svp.h>

#include "art-utils.h"

double identity[] = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};

int
art_affine_is_identity (double affine[])
{
	return art_affine_equal (affine, identity);
}

ArtSVP *
art_svp_translate (const ArtSVP * svp, double dx, double dy)
{
	ArtSVP * new;
	int i, j;

	new = (ArtSVP *) art_alloc (sizeof (ArtSVP) + (svp->n_segs - 1) * sizeof (ArtSVPSeg));

	new->n_segs = svp->n_segs;

	for (i = 0; i < new->n_segs; i++) {
		new->segs[i].n_points = svp->segs[i].n_points;
		new->segs[i].dir = svp->segs[i].dir;
		new->segs[i].bbox.x0 = svp->segs[i].bbox.x0 + dx;
		new->segs[i].bbox.y0 = svp->segs[i].bbox.y0 + dy;
		new->segs[i].bbox.x1 = svp->segs[i].bbox.x1 + dx;
		new->segs[i].bbox.y1 = svp->segs[i].bbox.y1 + dy;
		new->segs[i].points = art_new (ArtPoint, new->segs[i].n_points);

		for (j = 0; j < new->segs[i].n_points; j++) {
			new->segs[i].points[j].x = svp->segs[i].points[j].x + dx;
			new->segs[i].points[j].y = svp->segs[i].points[j].y + dy;
		}
	}

	return new;
}

ArtUta *
art_uta_from_svp_translated (const ArtSVP * svp, double x, double y)
{
	ArtSVP * tsvp;
	ArtUta * uta;

	tsvp = art_svp_translate (svp, x, y);

	uta = art_uta_from_svp (tsvp);

	art_svp_free (tsvp);

	return uta;
}


typedef struct _ArtRgbSVPRGBAData ArtRgbSVPRGBAData;

struct _ArtRgbSVPRGBAData {
/*  int alphatab[256];
  art_u8 r, g, b, alpha; */
  art_u8 *buf;
  int rowstride;
  int x0, x1;
  art_u32 *src;
  int sx, sy;
  int srs;
};

static void
art_rgb_run_rgba (art_u8 *buf, art_u32 *src, art_u8 coverage, int n);

static void
art_rgb_svp_rgba_callback (void *callback_data, int y,
			    int start, ArtSVPRenderAAStep *steps, int n_steps);

static void
art_rgb_run_rgba (art_u8 *buf, art_u32 *src, art_u8 coverage, int n)
{
  int i;
  int r,g,b,a;
  art_u32 rgba;

  for (i = 0; i < n; i++)
    {
      rgba = *src++;
      r = rgba >> 24;
      g = (rgba >> 16) & 0xff;
      b = (rgba >> 8) & 0xff;
      a = ((rgba & 0xff) * coverage + 0x80) / 255;
      *buf++ = *buf + (((r - *buf) * a + 0x80) / 255);
      *buf++ = *buf + (((g - *buf) * a + 0x80) / 255);
      *buf++ = *buf + (((b - *buf) * a + 0x80) / 255);
    }

}

static void
art_rgb_svp_rgba_callback (void *callback_data, int y,
			    int start, ArtSVPRenderAAStep *steps, int n_steps)
{
  ArtRgbSVPRGBAData *data = callback_data;
  art_u8 *linebuf;
  int run_x0, run_x1;
  art_u32 running_sum = start;
  int x0, x1;
  int k;
  
  art_u32 *srcbuf;
/*
  art_u8 r, g, b;
  int *alphatab;
*/
  int alpha;

  linebuf = data->buf;
  x0 = data->x0;
  x1 = data->x1;

  srcbuf = data->src + data->sy * data->srs + data->sx;

/*
  r = data->r;
  g = data->g;
  b = data->b;
  alphatab = data->alphatab;
*/

  if (n_steps > 0)
    {
      run_x1 = steps[0].x;
      if (run_x1 > x0)
	{
	  alpha = (running_sum >> 16) & 0xff;
	  if (alpha)
	    art_rgb_run_rgba (linebuf, srcbuf,
			       alpha,
			       run_x1 - x0);
	}

      /* render the steps into tmpbuf */
      for (k = 0; k < n_steps - 1; k++)
	{
	  running_sum += steps[k].delta;
	  run_x0 = run_x1;
	  run_x1 = steps[k + 1].x;
	  if (run_x1 > run_x0)
	    {
	      alpha = (running_sum >> 16) & 0xff;
	      if (alpha)
		art_rgb_run_rgba (linebuf + (run_x0 - x0) * 3,
				   srcbuf + (run_x0 - x0),
				   alpha,
				   run_x1 - run_x0);
	    }
	}
      running_sum += steps[k].delta;
      if (x1 > run_x1)
	{
	  alpha = (running_sum >> 16) & 0xff;
	  if (alpha)
	    art_rgb_run_rgba (linebuf + (run_x1 - x0) * 3,
	    		       srcbuf + (run_x1 - x0),
			       alpha,
			       x1 - run_x1);
	}
    }
  else
    {
      alpha = (running_sum >> 16) & 0xff;
      if (alpha)
	art_rgb_run_rgba (linebuf,
			   srcbuf,
			   alpha,
			   x1 - x0);
    }

  data->buf += data->rowstride;
  data->src += data->srs;
}

void
art_rgb_svp_rgba (const ArtSVP *svp,
		   int x0, int y0, int x1, int y1,
		   art_u32 *src,
		   int sx, int sy, int srs,
		   art_u8 *buf, int rowstride,
		   ArtAlphaGamma *alphagamma)
{
  ArtRgbSVPRGBAData data;
#if 0
  int r, g, b, alpha;
  int i;
  int a, da;

  r = rgba >> 24;
  g = (rgba >> 16) & 0xff;
  b = (rgba >> 8) & 0xff;
  alpha = rgba & 0xff;

  data.r = r;
  data.g = g;
  data.b = b;
  data.alpha = alpha;

  a = 0x8000;
  da = (alpha * 66051 + 0x80) >> 8; /* 66051 equals 2 ^ 32 / (255 * 255) */

  for (i = 0; i < 256; i++)
    {
      data.alphatab[i] = a >> 16;
      a += da;
    }
#endif
  data.buf = buf;
  data.rowstride = rowstride;
  data.x0 = x0;
  data.x1 = x1;
  data.src = src;
  data.sx = sx;
  data.sy = sy;
  data.srs = srs;

  art_svp_render_aa (svp, x0, y0, x1, y1, art_rgb_svp_rgba_callback, &data);

}


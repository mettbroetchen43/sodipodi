#define __NR_COMPOSE_TRANSFORM_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include "nr-pixops.h"
#include "nr-compose-transform.h"

void nr_R8G8B8A8_N_EMPTY_R8G8B8A8_N_TRANSFORM (unsigned char *px, int w, int h, int rs,
					       const unsigned char *spx, int sw, int sh, int srs,
					       const NRMatrixF *d2s, unsigned int alpha, int xd, int yd);
void nr_R8G8B8A8_N_EMPTY_R8G8B8A8_P_TRANSFORM (unsigned char *px, int w, int h, int rs,
					       const unsigned char *spx, int sw, int sh, int srs,
					       const NRMatrixF *d2s, unsigned int alpha, int xd, int yd);
void nr_R8G8B8A8_P_EMPTY_R8G8B8A8_N_TRANSFORM (unsigned char *px, int w, int h, int rs,
					       const unsigned char *spx, int sw, int sh, int srs,
					       const NRMatrixF *d2s, unsigned int alpha, int xd, int yd);
void nr_R8G8B8A8_P_EMPTY_R8G8B8A8_P_TRANSFORM (unsigned char *px, int w, int h, int rs,
					       const unsigned char *spx, int sw, int sh, int srs,
					       const NRMatrixF *d2s, unsigned int alpha, int xd, int yd);

#define FBITS 12

void
nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_N_TRANSFORM (unsigned char *px, int w, int h, int rs,
					       const unsigned char *spx, int sw, int sh, int srs,
					       const NRMatrixF *d2s, unsigned int alpha, int xd, int yd)
{
	unsigned int Falpha;
	int xsize, ysize, dbits;
	double Fs_x_x, Fs_x_y, Fs_y_x, Fs_y_y, Fs__x, Fs__y;
	long FFs_x_x, FFs_x_y;
	long FFs_x_x_S, FFs_x_y_S, FFs_y_x_S, FFs_y_y_S;
	int x, y;

	Falpha = alpha;
	if (Falpha < 1) return;

	xsize = (1 << xd);
	ysize = (1 << yd);
	dbits = xd + yd;

	Fs_x_x = d2s->c[0];
	Fs_x_y = d2s->c[1];
	Fs_y_x = d2s->c[2];
	Fs_y_y = d2s->c[3];
	Fs__x = d2s->c[4];
	Fs__y = d2s->c[5];

	FFs_x_x = (long) floor (Fs_x_x * (1 << FBITS) + 0.5);
	FFs_x_y = (long) floor (Fs_x_y * (1 << FBITS) + 0.5);

	FFs_x_x_S = (long) floor (Fs_x_x * (1 << FBITS) + 0.5) >> xd;
	FFs_x_y_S = (long) floor (Fs_x_y * (1 << FBITS) + 0.5) >> xd;
	FFs_y_x_S = (long) floor (Fs_y_x * (1 << FBITS) + 0.5) >> yd;
	FFs_y_y_S = (long) floor (Fs_y_y * (1 << FBITS) + 0.5) >> yd;

	for (y = 0; y < h; y++) {
		const unsigned char *s0, *s;
		unsigned char *d;
		double Fsx, Fsy;
		long FFsx0, FFsy0, FFdsx, FFdsy, sx, sy;
		d = px + y * rs;
		Fsx = y * Fs_y_x + Fs__x;
		Fsy = y * Fs_y_y + Fs__y;
		s0 = spx + (long) Fsy * srs + (long) Fsx * 4;
		FFsx0 = (long) (floor (Fsx) * (1 << FBITS) + 0.5);
		FFsy0 = (long) (floor (Fsy) * (1 << FBITS) + 0.5);
		FFdsx = (long) ((Fsx - floor (Fsx)) * (1 << FBITS) + 0.5);
		FFdsy = (long) ((Fsy - floor (Fsy)) * (1 << FBITS) + 0.5);
		for (x = 0; x < w; x++) {
			long FFdsx0, FFdsy0;
			unsigned int r, g, b, a;
			int xx, yy;
			FFdsy0 = FFdsy;
			FFdsx0 = FFdsx;
			r = g = b = a = 0;
			for (yy = 0; yy < ysize; yy++) {
				FFdsy = FFdsy0 + yy * (FFs_y_y_S);
				FFdsx = FFdsx0 + yy * (FFs_y_x_S);
				for (xx = 0; xx < xsize; xx++) {
					sx = (FFsx0 + FFdsx) >> FBITS;
					sy = (FFsy0 + FFdsy) >> FBITS;
					if ((sx >= 0) && (sx < sw) && (sy >= 0) && (sy < sh)) {
						s = spx + sy * srs + sx * 4;
						r += s[0];
						g += s[1];
						b += s[2];
						a += s[3];
					}
					FFdsx += FFs_x_x_S;
					FFdsy += FFs_x_y_S;
				}
			}
			if (a > dbits) {
				/* fixme: do it right, plus premul stuff */
				r = r >> dbits;
				g = g >> dbits;
				b = b >> dbits;
				a = ((a * Falpha >> dbits) + 127) / 255;
				if (a > 0) {
					if ((a == 255) || (d[3] == 0)) {
						/* Full coverage, COPY */
						d[0] = r;
						d[1] = g;
						d[2] = b;
						d[3] = a;
					} else {
						unsigned int ca;
						/* Full composition */
						ca = 65025 - (255 - a) * (255 - d[3]);
						d[0] = NR_COMPOSENNN_A7 (r, a, d[0], d[3], ca);
						d[1] = NR_COMPOSENNN_A7 (g, a, d[1], d[3], ca);
						d[2] = NR_COMPOSENNN_A7 (b, a, d[2], d[3], ca);
						d[3] = (ca + 127) / 255;
					}
				}
			}
			FFdsy = FFdsy0;
			FFdsx = FFdsx0;
			/* Advance pointers */
			FFdsx += FFs_x_x;
			FFdsy += FFs_x_y;
			/* Advance destination */
			d += 4;
		}
	}
}

void nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_P_TRANSFORM (unsigned char *px, int w, int h, int rs,
						    const unsigned char *spx, int sw, int sh, int srs,
						    const NRMatrixF *d2s, unsigned int alpha, int xd, int yd);

void nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_TRANSFORM (unsigned char *px, int w, int h, int rs,
						    const unsigned char *spx, int sw, int sh, int srs,
						    const NRMatrixF *d2s, unsigned int alpha, int xd, int yd)
{
	unsigned int Falpha;
	int xsize, ysize, dbits;
	double Fs_x_x, Fs_x_y, Fs_y_x, Fs_y_y, Fs__x, Fs__y;
	long FFs_x_x, FFs_x_y;
	long FFs_x_x_S, FFs_x_y_S, FFs_y_x_S, FFs_y_y_S;
	int x, y;

	Falpha = alpha;
	if (Falpha < 1) return;

	xsize = (1 << xd);
	ysize = (1 << yd);
	dbits = xd + yd;

	Fs_x_x = d2s->c[0];
	Fs_x_y = d2s->c[1];
	Fs_y_x = d2s->c[2];
	Fs_y_y = d2s->c[3];
	Fs__x = d2s->c[4];
	Fs__y = d2s->c[5];

	FFs_x_x = (long) floor (Fs_x_x * (1 << FBITS) + 0.5);
	FFs_x_y = (long) floor (Fs_x_y * (1 << FBITS) + 0.5);

	FFs_x_x_S = (long) floor (Fs_x_x * (1 << FBITS) + 0.5) >> xd;
	FFs_x_y_S = (long) floor (Fs_x_y * (1 << FBITS) + 0.5) >> xd;
	FFs_y_x_S = (long) floor (Fs_y_x * (1 << FBITS) + 0.5) >> yd;
	FFs_y_y_S = (long) floor (Fs_y_y * (1 << FBITS) + 0.5) >> yd;

	for (y = 0; y < h; y++) {
		const unsigned char *s0, *s;
		unsigned char *d;
		double Fsx, Fsy;
		long FFsx0, FFsy0, FFdsx, FFdsy, sx, sy;
		d = px + y * rs;
		Fsx = y * Fs_y_x + Fs__x;
		Fsy = y * Fs_y_y + Fs__y;
		s0 = spx + (long) Fsy * srs + (long) Fsx * 4;
		FFsx0 = (long) (floor (Fsx) * (1 << FBITS) + 0.5);
		FFsy0 = (long) (floor (Fsy) * (1 << FBITS) + 0.5);
		FFdsx = (long) ((Fsx - floor (Fsx)) * (1 << FBITS) + 0.5);
		FFdsy = (long) ((Fsy - floor (Fsy)) * (1 << FBITS) + 0.5);
		for (x = 0; x < w; x++) {
			long FFdsx0, FFdsy0;
			unsigned int r, g, b, a;
			int xx, yy;
			FFdsy0 = FFdsy;
			FFdsx0 = FFdsx;
			r = g = b = a = 0;
			for (yy = 0; yy < ysize; yy++) {
				FFdsy = FFdsy0 + yy * (FFs_y_y_S);
				FFdsx = FFdsx0 + yy * (FFs_y_x_S);
				for (xx = 0; xx < xsize; xx++) {
					sx = (FFsx0 + FFdsx) >> FBITS;
					sy = (FFsy0 + FFdsy) >> FBITS;
					if ((sx >= 0) && (sx < sw) && (sy >= 0) && (sy < sh)) {
						s = spx + sy * srs + sx * 4;
						r += s[0];
						g += s[1];
						b += s[2];
						a += s[3];
					}
					FFdsx += FFs_x_x_S;
					FFdsy += FFs_x_y_S;
				}
			}
			if (a > dbits) {
				/* fixme: do it right, plus premul stuff */
				r = r >> dbits;
				g = g >> dbits;
				b = b >> dbits;
				a = ((a * Falpha >> dbits) + 127) / 255;
				if (a > 0) {
					if ((a == 255) || (d[3] == 0)) {
						/* Transparent BG, premul src */
						d[0] = NR_PREMUL (r, a);
						d[1] = NR_PREMUL (g, a);
						d[2] = NR_PREMUL (b, a);
						d[3] = a;
					} else {
						d[0] = NR_COMPOSENPP (r, a, d[0], d[3]);
						d[1] = NR_COMPOSENPP (g, a, d[1], d[3]);
						d[2] = NR_COMPOSENPP (b, a, d[2], d[3]);
						d[3] = (65025 - (255 - a) * (255 - d[3]) + 127) / 255;
					}
				}
			}
			FFdsy = FFdsy0;
			FFdsx = FFdsx0;
			/* Advance pointers */
			FFdsx += FFs_x_x;
			FFdsy += FFs_x_y;
			/* Advance destination */
			d += 4;
		}
	}
}

void nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_P_TRANSFORM (unsigned char *px, int w, int h, int rs,
						    const unsigned char *spx, int sw, int sh, int srs,
						    const NRMatrixF *d2s, unsigned int alpha, int xd, int yd);

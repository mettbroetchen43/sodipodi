#ifndef __NR_COMPOSE_H__
#define __NR_COMPOSE_H__

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

/* FINAL DST SRC */

void nr_R8G8B8A8_N_EMPTY_R8G8B8A8_N (unsigned char *px, int w, int h, int rs, const unsigned char *spx, int srs, unsigned int alpha);
void nr_R8G8B8A8_N_EMPTY_R8G8B8A8_P (unsigned char *px, int w, int h, int rs, const unsigned char *spx, int srs, unsigned int alpha);
void nr_R8G8B8A8_P_EMPTY_R8G8B8A8_N (unsigned char *px, int w, int h, int rs, const unsigned char *spx, int srs, unsigned int alpha);
void nr_R8G8B8A8_P_EMPTY_R8G8B8A8_P (unsigned char *px, int w, int h, int rs, const unsigned char *spx, int srs, unsigned int alpha);

void nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_N (unsigned char *px, int w, int h, int rs, const unsigned char *spx, int srs, unsigned int alpha);
void nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_P (unsigned char *px, int w, int h, int rs, const unsigned char *spx, int srs, unsigned int alpha);
void nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N (unsigned char *px, int w, int h, int rs, const unsigned char *spx, int srs, unsigned int alpha);
void nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_P (unsigned char *px, int w, int h, int rs, const unsigned char *spx, int srs, unsigned int alpha);

/* FINAL DST SRC MASK */

void nr_R8G8B8A8_N_EMPTY_R8G8B8A8_N_A8 (unsigned char *px, int w, int h, int rs,
					const unsigned char *spx, int srs,
					const unsigned char *mpx, int mrs);
void nr_R8G8B8A8_N_EMPTY_R8G8B8A8_P_A8 (unsigned char *px, int w, int h, int rs,
					const unsigned char *spx, int srs,
					const unsigned char *mpx, int mrs);
void nr_R8G8B8A8_P_EMPTY_R8G8B8A8_N_A8 (unsigned char *px, int w, int h, int rs,
					const unsigned char *spx, int srs,
					const unsigned char *mpx, int mrs);
void nr_R8G8B8A8_P_EMPTY_R8G8B8A8_P_A8 (unsigned char *px, int w, int h, int rs,
					const unsigned char *spx, int srs,
					const unsigned char *mpx, int mrs);

void nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_N_A8 (unsigned char *p, int w, int h, int rs,
					     const unsigned char *s, int srs,
					     const unsigned char *m, int mrs);
void nr_R8G8B8A8_N_R8G8B8A8_N_R8G8B8A8_P_A8 (unsigned char *p, int w, int h, int rs,
					     const unsigned char *s, int srs,
					     const unsigned char *m, int mrs);
void nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_A8 (unsigned char *p, int w, int h, int rs,
					     const unsigned char *s, int srs,
					     const unsigned char *m, int mrs);
void nr_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_P_A8 (unsigned char *p, int w, int h, int rs,
					     const unsigned char *s, int srs,
					     const unsigned char *m, int mrs);

/* FINAL DST MASK COLOR */

void nr_R8G8B8_EMPTY_A8_RGBA32 (unsigned char *px, int w, int h, int rs, const unsigned char *mpx, int mrs, unsigned long rgba);
void nr_R8G8B8A8_N_EMPTY_A8_RGBA32 (unsigned char *px, int w, int h, int rs, const unsigned char *mpx, int mrs, unsigned long rgba);
void nr_R8G8B8A8_P_EMPTY_A8_RGBA32 (unsigned char *px, int w, int h, int rs, const unsigned char *mpx, int mrs, unsigned long rgba);

void nr_R8G8B8_R8G8B8_A8_RGBA32 (unsigned char *px, int w, int h, int rs, const unsigned char *spx, int srs, unsigned long rgba);
void nr_R8G8B8A8_N_R8G8B8A8_N_A8_RGBA32 (unsigned char *px, int w, int h, int rs, const unsigned char *spx, int srs, unsigned long rgba);
void nr_R8G8B8A8_P_R8G8B8A8_P_A8_RGBA32 (unsigned char *px, int w, int h, int rs, const unsigned char *spx, int srs, unsigned long rgba);

/* RGB */

void nr_R8G8B8_R8G8B8_R8G8B8A8_P (unsigned char *px, int w, int h, int rs, const unsigned char *spx, int srs, unsigned int alpha);
void nr_R8G8B8_R8G8B8_R8G8B8A8_N (unsigned char *px, int w, int h, int rs, const unsigned char *spx, int srs, unsigned int alpha);

#endif

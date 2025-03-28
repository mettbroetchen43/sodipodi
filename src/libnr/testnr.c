#define __TESTNR_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "nr-types.h"
#include "nr-pixblock.h"
#include "nr-blit.h"
#include "nr-path.h"

NRPathElement toru[10];

#ifdef __MINGW32__
 #include <sys/timeb.h>
 void gettimeofday(struct timeval* t,void* timezone)
 {       struct timeb timebuffer;
         ftime( &timebuffer );
         t->tv_sec=timebuffer.time;
         t->tv_usec=1000*timebuffer.millitm;
 }
#endif

static double
get_time (void)
{
	struct timeval tv;

	gettimeofday (&tv, NULL);

	return tv.tv_sec + 1e-6 * tv.tv_usec;
}

static unsigned int
rand_byte (void)
{
	return (int) (256.0 * rand () / (RAND_MAX + 1.0));
}

int
main (int argc, const char **argv)
{
	double start, end;
	NRPixBlock d, m[16];
	int count, i;

	srand (time (NULL));

	printf ("lala %d toru[0] %d toru %d\n", sizeof (NRPathElement), sizeof (toru[0]), sizeof (toru));

	printf ("Initializing buffers\n");

	/* Destination */
	nr_pixblock_setup_fast (&d, NR_PIXBLOCK_MODE_R8G8B8A8P, 0, 0, 64, 64, 1);
	d.empty = 0;

	/* Masks */
	for (i = 0; i < 16; i++) {
		int r, b, c;
		nr_pixblock_setup_fast (&m[i], NR_PIXBLOCK_MODE_G8, 0, 0, 64, 64, 0);
		for (r = 0; r < 64; r++) {
			unsigned int q;
			unsigned char *p;
			p = NR_PIXBLOCK_PX (&m[i]) + r * m[i].rs;
			for (b = 0; b < 8; b++) {
				q = rand_byte ();
				if (q < 120) {
					for (c = 0; c < 8; c++) *p++ = 0;
				} else if (q < 240) {
					for (c = 0; c < 8; c++) *p++ = 255;
				} else {
					for (c = 0; c < 8; c++) *p++ = rand_byte ();
				}
			}
		}
		m[i].empty = 0;
	}

	printf ("Random transparency\n");
	count = 0;
	start = end = get_time ();
	while ((end - start) < 5.0) {
		unsigned char r, g, b, a;
		r = rand_byte ();
		g = rand_byte ();
		b = rand_byte ();
		a = rand_byte ();

		for (i = 0; i < 16; i++) {
			nr_blit_pixblock_mask_rgba32 (&d, &m[i], (a << 24) | (g << 16) | (b << 8) | a);
			count += 1;
		}
		end = get_time ();
	}
	printf ("Did %d [64x64] random buffers in %f sec\n", count, end - start);
	printf ("%f buffers per second\n", count / (end - start));
	printf ("%f pixels per second\n", count * (64 * 64) / (end - start));

	return 0;
}

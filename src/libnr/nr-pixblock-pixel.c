#define __NR_PIXBLOCK_PIXEL_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include "nr-pixops.h"
#include "nr-pixblock-pixel.h"

void
nr_compose_pixblock_pixblock_pixel (NRPixBlock *dpb, unsigned char *d, const NRPixBlock *spb, const unsigned char *s)
{
	if (spb->empty) return;

	if (dpb->empty) {
		/* Empty destination */
		switch (dpb->mode) {
		case NR_PIXBLOCK_MODE_G8:
			switch (spb->mode) {
			case NR_PIXBLOCK_MODE_G8:
				break;
			case NR_PIXBLOCK_MODE_R8G8B8:
				d[0] = 255;
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8N:
				d[0] = s[3];
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8P:
				d[0] = s[3];
				break;
			default:
				break;
			}
			break;
		case NR_PIXBLOCK_MODE_R8G8B8:
			switch (spb->mode) {
			case NR_PIXBLOCK_MODE_G8:
				break;
			case NR_PIXBLOCK_MODE_R8G8B8:
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8N:
				d[0] = NR_COMPOSEN11 (s[0], s[3], 255);
				d[1] = NR_COMPOSEN11 (s[1], s[3], 255);
				d[2] = NR_COMPOSEN11 (s[2], s[3], 255);
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8P:
				d[0] = NR_COMPOSEP11 (s[0], s[3], 255);
				d[1] = NR_COMPOSEP11 (s[1], s[3], 255);
				d[2] = NR_COMPOSEP11 (s[2], s[3], 255);
				break;
			default:
				break;
			}
			break;
		case NR_PIXBLOCK_MODE_R8G8B8A8N:
			switch (spb->mode) {
			case NR_PIXBLOCK_MODE_G8:
				break;
			case NR_PIXBLOCK_MODE_R8G8B8:
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				d[3] = 255;
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8N:
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				d[3] = s[3];
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8P:
				if (s[3] == 0) {
					d[0] = 255;
					d[1] = 255;
					d[2] = 255;
				} else {
					d[0] = (s[0] * 255) / s[3];
					d[1] = (s[1] * 255) / s[3];
					d[2] = (s[2] * 255) / s[3];
				}
				d[3] = s[3];
				break;
			default:
				break;
			}
			break;
		case NR_PIXBLOCK_MODE_R8G8B8A8P:
			switch (spb->mode) {
			case NR_PIXBLOCK_MODE_G8:
				break;
			case NR_PIXBLOCK_MODE_R8G8B8:
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				d[3] = 255;
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8N:
				d[0] = NR_PREMUL (s[0], s[3]);
				d[1] = NR_PREMUL (s[1], s[3]);
				d[2] = NR_PREMUL (s[2], s[3]);
				d[3] = s[3];
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8P:
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				d[3] = s[3];
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	} else {
		/* Image destination */
		switch (dpb->mode) {
		case NR_PIXBLOCK_MODE_G8:
			switch (spb->mode) {
			case NR_PIXBLOCK_MODE_G8:
				break;
			case NR_PIXBLOCK_MODE_R8G8B8:
				d[0] = 255;
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8N:
				d[0] = (65025 - (255 - s[3]) * (255 - d[0]) + 127) / 255;
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8P:
				d[0] = (65025 - (255 - s[3]) * (255 - d[0]) + 127) / 255;
				break;
			default:
				break;
			}
			break;
		case NR_PIXBLOCK_MODE_R8G8B8:
			switch (spb->mode) {
			case NR_PIXBLOCK_MODE_G8:
				break;
			case NR_PIXBLOCK_MODE_R8G8B8:
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8N:
				d[0] = NR_COMPOSEN11 (s[0], s[3], d[0]);
				d[1] = NR_COMPOSEN11 (s[1], s[3], d[1]);
				d[2] = NR_COMPOSEN11 (s[2], s[3], d[2]);
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8P:
				d[0] = NR_COMPOSEP11 (s[0], s[3], d[0]);
				d[1] = NR_COMPOSEP11 (s[1], s[3], d[1]);
				d[2] = NR_COMPOSEP11 (s[2], s[3], d[2]);
				break;
			default:
				break;
			}
			break;
		case NR_PIXBLOCK_MODE_R8G8B8A8N:
			switch (spb->mode) {
			case NR_PIXBLOCK_MODE_G8:
				break;
			case NR_PIXBLOCK_MODE_R8G8B8:
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8N:
				if (s[3] != 0) {
					unsigned int ca;
					ca = NR_A7 (s[3], d[3]);
					d[0] = NR_COMPOSENNN_A7 (s[0], s[3], d[0], d[3], ca);
					d[1] = NR_COMPOSENNN_A7 (s[1], s[3], d[1], d[3], ca);
					d[2] = NR_COMPOSENNN_A7 (s[2], s[3], d[2], d[3], ca);
					d[3] = (ca + 127) / 255;
				}
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8P:
				if (s[3] != 0) {
					unsigned int ca;
					ca = NR_A7 (s[3], d[3]);
					d[0] = NR_COMPOSEPNN_A7 (s[0], s[3], d[0], d[3], ca);
					d[1] = NR_COMPOSEPNN_A7 (s[1], s[3], d[0], d[3], ca);
					d[2] = NR_COMPOSEPNN_A7 (s[2], s[3], d[0], d[3], ca);
					d[3] = (ca + 127) / 255;
				}
				break;
			default:
				break;
			}
			break;
		case NR_PIXBLOCK_MODE_R8G8B8A8P:
			switch (spb->mode) {
			case NR_PIXBLOCK_MODE_G8:
				break;
			case NR_PIXBLOCK_MODE_R8G8B8:
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8N:
				d[0] = NR_COMPOSENPP (s[0], s[3], d[0], d[3]);
				d[1] = NR_COMPOSENPP (s[1], s[3], d[1], d[3]);
				d[2] = NR_COMPOSENPP (s[2], s[3], d[2], d[3]);
				d[3] = (65025 - (255 - s[3]) * (255 - d[3]) + 127) / 255;
				break;
			case NR_PIXBLOCK_MODE_R8G8B8A8P:
				d[0] = NR_COMPOSEPPP (s[0], s[3], d[0], d[3]);
				d[1] = NR_COMPOSEPPP (s[1], s[3], d[1], d[3]);
				d[2] = NR_COMPOSEPPP (s[2], s[3], d[2], d[3]);
				d[3] = (65025 - (255 - s[3]) * (255 - d[3]) + 127) / 255;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
}


#define SP_CURSOR_C

#include <stdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "sp-cursor.h"

void
sp_cursor_bitmap_and_mask_from_xpm (GdkBitmap **bitmap, GdkBitmap **mask, gchar **xpm)
{
	int height, width, colors;
	char pixmap_buffer [(32 * 32)/8];
	char mask_buffer [(32 * 32)/8];
	int x, y, pix;
	int transparent_color, black_color;
	
	sscanf (xpm [0], "%d %d %d %d", &height, &width, &colors, &pix);

	g_assert (height == 32);
	g_assert (width  == 32);
	g_assert (colors == 3);

	transparent_color = ' ';
	black_color = '.';
	
	for (y = 0; y < 32; y++){
		for (x = 0; x < 32;){
			char value = 0, maskv = 0;
			
			for (pix = 0; pix < 8; pix++, x++){
				if (xpm [4+y][x] != transparent_color){
					maskv |= 1 << pix;

					if (xpm [4+y][x] != black_color){
						value |= 1 << pix;
					}
				}
			}
			pixmap_buffer [(y * 4 + x/8)-1] = value;
			mask_buffer [(y * 4 + x/8)-1] = maskv;
		}
	}
	*bitmap = gdk_bitmap_create_from_data (NULL, pixmap_buffer, 32, 32);
	*mask   = gdk_bitmap_create_from_data (NULL, mask_buffer, 32, 32);
}


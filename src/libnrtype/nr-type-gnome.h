#ifndef __NR_TYPE_GNOME_H__
#define __NR_TYPE_GNOME_H__

/*
 * Typeface and script library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

typedef struct _NRTypeFaceGnome NRTypeFaceGnome;

#include <libnrtype/nr-rasterfont.h>
#include <libgnomeprint/gnome-font.h>

struct _NRTypeFaceGnome {
	NRTypeFace typeface;

	NRTypeFaceGnome *prev, *next;
	GnomeFontFace *face;
	NRFont *fonts;
};

NRTypeFace *nr_typeface_gnome_new (GnomeFontFace *gff);

#endif

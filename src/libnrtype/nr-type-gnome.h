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

#include <libnrtype/nr-font.h>
#include <libgnomeprint/gnome-font.h>

struct _NRTypeFace {
	unsigned int refcount;
	GnomeFontFace *face;
	unsigned int nglyphs;
};

struct _NRFont {
	unsigned int refcount;
	NRTypeFace *face;
	unsigned int metrics : 2;
	GnomeFont *font;
	unsigned int nglyphs;
};

#endif

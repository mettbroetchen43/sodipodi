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
#include <libnrtype/nr-type-directory.h>
#include <libgnomeprint/gnome-font.h>

struct _NRTypeFaceGnome {
	NRTypeFace typeface;

	GnomeFontFace *face;
	NRFont *fonts;

	NRBPath *voutlines;
};

NRNameList *nr_type_gnome_typefaces_get (NRNameList *typefaces);
NRNameList *nr_type_gnome_families_get (NRNameList *families);
void nr_type_gnome_build_def (NRTypeFaceDef *def, const unsigned char *name, const unsigned char *family);

#endif

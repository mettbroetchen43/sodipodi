#ifndef __NR_TYPE_XFT_H__
#define __NR_TYPE_XFT_H__

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

#include "nr-type-primitives.h"
#include "nr-type-ft2.h"

void nr_type_xft_typefaces_get (NRNameList *names);
void nr_type_xft_families_get (NRNameList *names);

void nr_type_xft_build_def (NRTypeFaceDefFT2 *dft2, const unsigned char *name, const unsigned char *family);

#endif

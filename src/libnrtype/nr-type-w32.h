#ifndef __NR_TYPE_W32_H__
#define __NR_TYPE_W32_H__

/*
 * Wrapper around Win32 font subsystem
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

typedef struct _NRTypeFaceW32 NRTypeFaceW32;
typedef struct _NRTypeFaceGlyphW32 NRTypeFaceGlyphW32;

#include <windows.h>

#include <libnrtype/nr-type-primitives.h>
#include <libnrtype/nr-typeface.h>

struct _NRTypeFaceGlyphW32 {
	NRRectF area;
	NRPointF advance;
	int olref;
	NRBPath outline;
};

struct _NRTypeFaceW32 {
	NRTypeFace typeface;

	NRFont *fonts;

	int *gidx;
	NRTypeFaceGlyphW32 *slots;
	unsigned int slots_length;
	unsigned int slots_size;

	LOGFONT *logfont;
	HFONT hfont;
	LPOUTLINETEXTMETRIC otm;
};

void nr_type_w32_typefaces_get (NRNameList *names);
void nr_type_w32_families_get (NRNameList *names);

void nr_type_w32_build_def (NRTypeFaceDef *def, const unsigned char *name, const unsigned char *family);

#endif


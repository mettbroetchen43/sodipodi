#ifndef __NR_TYPE_FT2_H__
#define __NR_TYPE_FT2_H__

/*
 * Typeface and script library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

typedef struct _NRTypeFaceFT2 NRTypeFaceFT2;
typedef struct _NRTypeFaceDefFT2 NRTypeFaceDefFT2;
typedef struct _NRTypeFaceGlyphFT2 NRTypeFaceGlyphFT2;

#include <freetype/freetype.h>
#include <libnrtype/nr-typeface.h>

struct _NRTypeFaceDefFT2 {
	NRTypeFaceDef def;
	unsigned char *file;
	unsigned int face;
};

struct _NRTypeFaceGlyphFT2 {
	NRRectF area;
	NRPointF advance;
	int olref;
	NRBPath outline;
};

struct _NRTypeFaceFT2 {
	NRTypeFace typeface;

	FT_Face ft_face;
	double ft2ps;
	unsigned int unimap : 1;

	NRFont *fonts;

	int *hgidx;
	int *vgidx;

	NRTypeFaceGlyphFT2 *slots;
	unsigned int slots_length;
	unsigned int slots_size;
};

void
nr_type_ft2_build_def (NRTypeFaceDefFT2 *dft2,
		       const unsigned char *name,
		       const unsigned char *family,
		       const unsigned char *file,
		       unsigned int face);

#endif

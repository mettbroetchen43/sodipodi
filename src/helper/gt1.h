#ifndef SP_GT1_H
#define SP_GT1_H

#include <libart_lgpl/art_bpath.h>

typedef struct _Gt1LoadedFont Gt1LoadedFont;

Gt1LoadedFont * gt1_load_font (char *filename);

ArtBpath * gt1_get_glyph_outline (Gt1LoadedFont * font, int glyphnum, double * p_wx);

double gt1_get_kern_pair (Gt1LoadedFont *font, int glyph1, int glyph2);

char * gt1_get_font_name (Gt1LoadedFont *font);

#endif

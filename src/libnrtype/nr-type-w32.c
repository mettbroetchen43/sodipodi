#define __NR_TYPE_W32_C__

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

#include <windows.h>
#include <windowsx.h>

/* fixme: */
#include <glib.h>

#include "nr-type-w32.h"

static NRTypeFace *nr_typeface_w32_new (NRTypeFaceDef *def);
void nr_typeface_w32_free (NRTypeFace *tf);

unsigned int nr_typeface_w32_attribute_get (NRTypeFace *tf, const unsigned char *key, unsigned char *str, unsigned int size);
NRBPath *nr_typeface_w32_glyph_outline_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRBPath *d, unsigned int ref);
void nr_typeface_w32_glyph_outline_unref (NRTypeFace *tf, unsigned int glyph, unsigned int metrics);
NRPointF *nr_typeface_w32_glyph_advance_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv);
unsigned int nr_typeface_w32_lookup (NRTypeFace *tf, unsigned int rule, unsigned int unival);

NRFont *nr_typeface_w32_font_new (NRTypeFace *tf, unsigned int metrics, NRMatrixF *transform);
void nr_typeface_w32_font_free (NRFont *font);

static NRTypeFaceVMV nr_type_w32_vmv = {
    nr_typeface_w32_new,

    nr_typeface_w32_free,
    nr_typeface_w32_attribute_get,
    nr_typeface_w32_glyph_outline_get,
    nr_typeface_w32_glyph_outline_unref,
    nr_typeface_w32_glyph_advance_get,
    nr_typeface_w32_lookup,

    nr_typeface_w32_font_new,
    nr_typeface_w32_font_free,

	nr_font_generic_glyph_outline_get,
	nr_font_generic_glyph_outline_unref,
	nr_font_generic_glyph_advance_get,
	nr_font_generic_glyph_area_get,

	nr_font_generic_rasterfont_new,
	nr_font_generic_rasterfont_free,

	nr_rasterfont_generic_glyph_advance_get,
	nr_rasterfont_generic_glyph_area_get,
	nr_rasterfont_generic_glyph_mask_render
};

static unsigned int w32i = FALSE;

static HDC hdc = NULL;

static GHashTable *namedict = NULL;
static GSList *namelist = NULL;
static GHashTable *familydict = NULL;
static GSList *familylist = NULL;

static NRNameList NRW32Typefaces = {0, NULL, NULL};
static NRNameList NRW32Families = {0, NULL, NULL};

static void nr_type_w32_init (void);

void
nr_type_w32_typefaces_get (NRNameList *names)
{
    if (!w32i) nr_type_w32_init ();

    *names = NRW32Typefaces;
}

void
nr_type_w32_families_get (NRNameList *names)
{
    if (!w32i) nr_type_w32_init ();

    *names = NRW32Families;
}

void
nr_type_w32_build_def (NRTypeFaceDef *def, const unsigned char *name, const unsigned char *family)
{
    def->vmv = &nr_type_w32_vmv;
    def->name = g_strdup (name);
    def->family = g_strdup (family);
    def->typeface = NULL;
}

static NRTypeFace *
nr_typeface_w32_new (NRTypeFaceDef *def)
{ 
    NRTypeFaceW32 *tfw32;
    LOGFONT *lf;
    TEXTMETRIC tm;
    unsigned int otmsize;

    tfw32 = nr_new (NRTypeFaceW32, 1);

    tfw32->typeface.vmv = def->vmv;
	tfw32->typeface.refcount = 1;
	tfw32->typeface.def = def;

	tfw32->fonts = NULL;

	lf = g_hash_table_lookup (namedict, def->name);
	lf->lfHeight = 1000;
	lf->lfWidth = 1000;
	tfw32->hfont = CreateFontIndirect (lf);

	/* Have to select font to get metrics etc. */
	SelectFont (hdc, tfw32->hfont);

	GetTextMetrics (hdc, &tm);
	tfw32->typeface.nglyphs = tm.tmLastChar - tm.tmFirstChar + 1;

	otmsize = GetOutlineTextMetrics (hdc, 0, NULL);
	tfw32->otm = (LPOUTLINETEXTMETRIC) nr_new (unsigned char, otmsize);
	GetOutlineTextMetrics (hdc, otmsize, tfw32->otm);

	return (NRTypeFace *) tfw32;
}

void
nr_typeface_w32_free (NRTypeFace *tf)
{
    NRTypeFaceW32 *tfw32;

    tfw32 = (NRTypeFaceW32 *) tf;

    nr_free (tfw32->otm);

    DeleteFont (tfw32->hfont);
}


unsigned int
nr_typeface_w32_attribute_get (NRTypeFace *tf, const unsigned char *key, unsigned char *str, unsigned int size)
{
	NRTypeFaceW32 *tfw32;
	const unsigned char *val;
	int len;

	tfw32 = (NRTypeFaceW32 *) tf;

	if (!strcmp (key, "name")) {
		val = tf->def->name;
	} else if (!strcmp (key, "family")) {
		val = tf->def->family;
	} else if (!strcmp (key, "weight")) {
		/* fixme: */
  		val = "normal";
	} else if (!strcmp (key, "style")) {
		/* fixme: */
		val = "normal";
	} else {
		val = "";
	}

	len = MIN (size - 1, strlen (val));
	if (len > 0) {
		memcpy (str, val, len);
	}
	if (size > 0) {
		str[len] = '\0';
	}

	return strlen (val);
}

NRBPath *
nr_typeface_w32_glyph_outline_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRBPath *d, unsigned int ref)
{
	NRTypeFaceW32 *tfw32;
	MAT2 mat = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
	GLYPHMETRICS gmetrics;
	int golsize;
	void *gol;
	LPTTPOLYGONHEADER pgh;
	LPTTPOLYCURVE pc;

	tfw32 = (NRTypeFaceW32 *) tf;

	/* Have to select font */
	SelectFont (hdc, tfw32->hfont);

    golsize = GetGlyphOutline (hdc, glyph, GGO_NATIVE, &gmetrics, 0, NULL, &mat);
    gol = nr_new (unsigned char, golsize);
    GetGlyphOutline (hdc, glyph, GGO_NATIVE, &gmetrics, golsize, gol, &mat);

    pgh = (LPTTPOLYGONHEADER) gol;
    g_print ("Polygon header %d bytes, start %g %g\n", pgh->cb, (double) pgh->pfxStart.x.value, (double) pgh->pfxStart.y.value);
    pc = (LPPOLYCURVE) (gol + sizeof (TTPOLYGONHEADER));

    nr_free (gol);

    return NULL;
}

void
nr_typeface_w32_glyph_outline_unref (NRTypeFace *tf, unsigned int glyph, unsigned int metrics)
{
}

NRPointF *
nr_typeface_w32_glyph_advance_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv)
{
	NRTypeFaceW32 *tfw32;
	MAT2 mat = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
	GLYPHMETRICS gmetrics;

	tfw32 = (NRTypeFaceW32 *) tf;

	/* Have to select font */
	SelectFont (hdc, tfw32->hfont);

    GetGlyphOutline (hdc, glyph, GGO_METRICS, &gmetrics, 0, NULL, &mat);

    /* fixme: Scales */
    adv->x = gmetrics.gmCellIncX;
    adv->y = gmetrics.gmCellIncY;
    g_print ("Advance %g %g\n", adv->x, adv->y);

    return adv;
}

unsigned int
nr_typeface_w32_lookup (NRTypeFace *tf, unsigned int rule, unsigned int unival)
{
	NRTypeFaceW32 *tfw32;

	tfw32 = (NRTypeFaceW32 *) tf;

    return unival & 127;
}


NRFont *
nr_typeface_w32_font_new (NRTypeFace *tf, unsigned int metrics, NRMatrixF *transform)
{
	NRTypeFaceW32 *tfw32;
	NRFont *font;
	float size;

	tfw32 = (NRTypeFaceW32 *) tf;
	size = NR_MATRIX_DF_EXPANSION (transform);

	font = tfw32->fonts;
	while (font != NULL) {
		if (NR_DF_TEST_CLOSE (size, font->size, 0.001 * size) && (font->metrics == metrics)) {
			return nr_font_ref (font);
		}
		font = font->next;
	}
	
	font = nr_font_generic_new (tf, metrics, transform);

	font->next = tfw32->fonts;
	tfw32->fonts = font;

	return font;
}

void
nr_typeface_w32_font_free (NRFont *font)
{
	NRTypeFaceW32 *tfw32;

	tfw32 = (NRTypeFaceW32 *) font->face;

	if (tfw32->fonts == font) {
		tfw32->fonts = font->next;
	} else {
		NRFont *ref;
		ref = tfw32->fonts;
		while (ref->next != font) ref = ref->next;
		ref->next = font->next;
	}

	font->next = NULL;

	nr_font_generic_free (font);
}

/* W32 initialization */

static int CALLBACK
nr_type_w32_inner_enum_proc (ENUMLOGFONTEX *elfex, NEWTEXTMETRICEX *tmex, DWORD fontType, LPARAM lParam)
{
    unsigned char *name;

    if (!g_hash_table_lookup (familydict, elfex->elfLogFont.lfFaceName)) {
        unsigned char *s;
        /* Register family */
        s = g_strdup (elfex->elfLogFont.lfFaceName);
        familylist = g_slist_prepend (familylist, s);
        g_hash_table_insert (familydict, s, GUINT_TO_POINTER (TRUE));
    }

    name = g_strdup_printf ("%s %s", elfex->elfLogFont.lfFaceName, elfex->elfStyle);
    if (!g_hash_table_lookup (namedict, name)) {
        LOGFONT *plf;
        plf = g_new (LOGFONT, 1);
        *plf = elfex->elfLogFont;
        namelist = g_slist_prepend (namelist, name);
        g_hash_table_insert (namedict, name, plf);

        g_print ("%s | ", name);
    } else {
        g_free (name);
    }

    return 1;
}

static int CALLBACK
nr_type_w32_typefaces_enum_proc (LOGFONT *lfp, TEXTMETRIC *metrics, DWORD fontType, LPARAM lParam)
{
    if (fontType == TRUETYPE_FONTTYPE) {
        LOGFONT lf;

        lf = *lfp;

        EnumFontFamiliesExA (hdc, &lf, (FONTENUMPROC) nr_type_w32_inner_enum_proc, lParam, 0);
    }

    return 1;
}

static void
nr_type_w32_init (void)
{
    LOGFONT logfont;
    GSList *l;
    int pos;

    g_print ("Loading W32 type directory...\n");

    hdc = CreateDC ("DISPLAY", NULL, NULL, NULL);

    familydict = g_hash_table_new (g_str_hash, g_str_equal);
    namedict = g_hash_table_new (g_str_hash, g_str_equal);

    /* read system font directory */
    memset (&logfont, 0, sizeof (LOGFONT));
    logfont.lfCharSet = DEFAULT_CHARSET;
    EnumFontFamiliesExA (hdc, &logfont, (FONTENUMPROC) nr_type_w32_typefaces_enum_proc, 0, 0);

    /* Fill in lists */
    NRW32Families.length = g_slist_length (familylist);
    NRW32Families.names = g_new (unsigned char *, NRW32Families.length);
    pos = 0;
    for (l = familylist; l != NULL; l = l->next) {
        NRW32Families.names[pos] = (unsigned char *) l->data;
        pos += 1;
    }
    NRW32Typefaces.length = g_slist_length (namelist);
    NRW32Typefaces.names = g_new (unsigned char *, NRW32Typefaces.length);
    pos = 0;
    for (l = namelist; l != NULL; l = l->next) {
        NRW32Typefaces.names[pos] = (unsigned char *) l->data;
        pos += 1;
    }

    w32i = TRUE;
}




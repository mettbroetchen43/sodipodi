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

#define NR_SLOTS_BLOCK 32

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
static NRTypeFaceGlyphW32 *nr_typeface_w32_ensure_slot (NRTypeFaceW32 *tfw32, unsigned int glyph);
static NRBPath *nr_typeface_w32_ensure_outline (NRTypeFaceW32 *tfw32, NRTypeFaceGlyphW32 *slot, unsigned int glyph, unsigned int metrics);

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
    unsigned int otmsize;

    tfw32 = nr_new (NRTypeFaceW32, 1);

    tfw32->typeface.vmv = def->vmv;
	tfw32->typeface.refcount = 1;
	tfw32->typeface.def = def;

	tfw32->fonts = NULL;

	tfw32->logfont = g_hash_table_lookup (namedict, def->name);
	tfw32->logfont->lfHeight = 1000;
	tfw32->logfont->lfWidth = 0;
	tfw32->hfont = CreateFontIndirect (tfw32->logfont);

	/* Have to select font to get metrics etc. */
	SelectFont (hdc, tfw32->hfont);

	otmsize = GetOutlineTextMetrics (hdc, 0, NULL);
	tfw32->otm = (LPOUTLINETEXTMETRIC) nr_new (unsigned char, otmsize);
	GetOutlineTextMetrics (hdc, otmsize, tfw32->otm);

	tfw32->typeface.nglyphs = tfw32->otm->otmTextMetrics.tmLastChar - tfw32->otm->otmTextMetrics.tmFirstChar + 1;

	tfw32->gidx = NULL;
	tfw32->slots = NULL;
	tfw32->slots_length = 0;
	tfw32->slots_size = 0;

	return (NRTypeFace *) tfw32;
}

void
nr_typeface_w32_free (NRTypeFace *tf)
{
    NRTypeFaceW32 *tfw32;

    tfw32 = (NRTypeFaceW32 *) tf;

    nr_free (tfw32->otm);
    DeleteFont (tfw32->hfont);

    if (tfw32->slots) {
        int i;
        for (i = 0; i < tfw32->slots_length; i++) {
            if (tfw32->slots[i].outline.path > 0) {
				nr_free (tfw32->slots[i].outline.path);
            }
        }
		nr_free (tfw32->slots);
    }
    if (tfw32->gidx) nr_free (tfw32->gidx);

    nr_free (tf);
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
	NRTypeFaceGlyphW32 *slot;

	tfw32 = (NRTypeFaceW32 *) tf;

	slot = nr_typeface_w32_ensure_slot (tfw32, glyph);

	if (slot) {
		nr_typeface_w32_ensure_outline (tfw32, slot, glyph, metrics);
		if (slot->olref >= 0) {
			if (ref) {
				slot->olref += 1;
			} else {
				slot->olref = -1;
			}
		}
		*d = slot->outline;
	} else {
		d->path = NULL;
	}

	return d;
}

void
nr_typeface_w32_glyph_outline_unref (NRTypeFace *tf, unsigned int glyph, unsigned int metrics)
{
	NRTypeFaceW32 *tfw32;
	NRTypeFaceGlyphW32 *slot;

	tfw32 = (NRTypeFaceW32 *) tf;

	slot = nr_typeface_w32_ensure_slot (tfw32, glyph);

	if (slot && slot->olref > 0) {
		slot->olref -= 1;
		if (slot->olref < 1) {
			nr_free (slot->outline.path);
			slot->outline.path = NULL;
		}
	}
}

NRPointF *
nr_typeface_w32_glyph_advance_get (NRTypeFace *tf, unsigned int glyph, unsigned int metrics, NRPointF *adv)
{
	NRTypeFaceW32 *tfw32;
	NRTypeFaceGlyphW32 *slot;

	tfw32 = (NRTypeFaceW32 *) tf;

	slot = nr_typeface_w32_ensure_slot (tfw32, glyph);

	if (slot) {
		*adv = slot->advance;
    return adv;
	}

	return NULL;
}

unsigned int
nr_typeface_w32_lookup (NRTypeFace *tf, unsigned int rule, unsigned int unival)
{
	NRTypeFaceW32 *tfw32;

	tfw32 = (NRTypeFaceW32 *) tf;

	unival = CLAMP (unival, 0, tf->nglyphs);

	/* fixme: Use real lookup tables etc. */
    return unival - tfw32->otm->otmTextMetrics.tmFirstChar;
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

static NRTypeFaceGlyphW32 *
nr_typeface_w32_ensure_slot (NRTypeFaceW32 *tfw32, unsigned int glyph)
{
	if (!tfw32->gidx) {
		int i;
		tfw32->gidx = nr_new (int, tfw32->typeface.nglyphs);
		for (i = 0; i < tfw32->typeface.nglyphs; i++) {
			tfw32->gidx[i] = -1;
		}
	}

	if (tfw32->gidx[glyph] < 0) {
		NRTypeFaceGlyphW32 *slot;
		static MAT2 mat = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
		GLYPHMETRICS gmetrics;

		if (!tfw32->slots) {
			tfw32->slots = nr_new (NRTypeFaceGlyphW32, 8);
			tfw32->slots_size = 8;
		} else if (tfw32->slots_length >= tfw32->slots_size) {
			tfw32->slots_size += NR_SLOTS_BLOCK;
			tfw32->slots = nr_renew (tfw32->slots, NRTypeFaceGlyphW32, tfw32->slots_size);
		}

		slot = tfw32->slots + tfw32->slots_length;

		/* Have to select font */
		SelectFont (hdc, tfw32->hfont);
		GetGlyphOutline (hdc, glyph + tfw32->otm->otmTextMetrics.tmFirstChar, GGO_METRICS, &gmetrics, 0, NULL, &mat);
		slot->area.x0 = gmetrics.gmptGlyphOrigin.x;
		slot->area.y1 = gmetrics.gmptGlyphOrigin.y;
		slot->area.x1 = slot->area.x0 + gmetrics.gmBlackBoxX;
		slot->area.y0 = slot->area.y1 - gmetrics.gmBlackBoxY;
		slot->advance.x = gmetrics.gmCellIncX;
        slot->advance.y = gmetrics.gmCellIncY;

		slot->olref = 0;
		slot->outline.path = NULL;

		tfw32->gidx[glyph] = tfw32->slots_length;
		tfw32->slots_length += 1;
	}

	return tfw32->slots + tfw32->gidx[glyph];
}

#define FIXED_TO_FLOAT(p) ((p)->value + (double) (p)->fract / 65536.0)

static NRBPath *
nr_typeface_w32_ensure_outline (NRTypeFaceW32 *tfw32, NRTypeFaceGlyphW32 *slot, unsigned int glyph, unsigned int metrics)
{
	MAT2 mat = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
	GLYPHMETRICS gmetrics;
	int golsize;
	unsigned char *gol;
	LPTTPOLYGONHEADER pgh;
    LPTTPOLYCURVE pc;
	ArtBpath bpath[8192];
    ArtBpath *bp;
	int pos, stop;
    double Ax, Ay, Bx, By, Cx, Cy;

	/* Have to select font */
	SelectFont (hdc, tfw32->hfont);

    golsize = GetGlyphOutline (hdc, glyph + tfw32->otm->otmTextMetrics.tmFirstChar, GGO_NATIVE, &gmetrics, 0, NULL, &mat);
    gol = nr_new (unsigned char, golsize);
    GetGlyphOutline (hdc, glyph + tfw32->otm->otmTextMetrics.tmFirstChar, GGO_NATIVE, &gmetrics, golsize, gol, &mat);

    bp = bpath;
    pos = 0;
    while (pos < golsize) {
        double Sx, Sy;
        pgh = (LPTTPOLYGONHEADER) (gol + pos);
        stop = pos + pgh->cb;
        /* Initialize current position */
        Ax = FIXED_TO_FLOAT (&pgh->pfxStart.x);
        Ay = FIXED_TO_FLOAT (&pgh->pfxStart.y);
        /* Always starts with moveto */
        bp->code = ART_MOVETO;
        bp->x3 = Ax;
        bp->y3 = Ay;
        bp += 1;
        Sx = Ax;
        Sy = Ay;
        pos = pos + sizeof (TTPOLYGONHEADER);
        while (pos < stop) {
            pc = (LPTTPOLYCURVE) (gol + pos);
            if (pc->wType == TT_PRIM_LINE) {
                int i;
                for (i = 0; i < pc->cpfx; i++) {
                    Cx = FIXED_TO_FLOAT (&pc->apfx[i].x);
                    Cy = FIXED_TO_FLOAT (&pc->apfx[i].y);
                    bp->code = ART_LINETO;
                    bp->x3 = Cx;
                    bp->y3 = Cy;
                    bp += 1;
                    Ax = Cx;
                    Ay = Cy;
                }
            } else if (pc->wType == TT_PRIM_QSPLINE) {
                int i;
                for (i = 0; i < (pc->cpfx - 1); i++) {
                    Bx = FIXED_TO_FLOAT (&pc->apfx[i].x);
                    By = FIXED_TO_FLOAT (&pc->apfx[i].y);
                    if (i < (pc->cpfx - 2)) {
                        Cx = (Bx + FIXED_TO_FLOAT (&pc->apfx[i + 1].x)) / 2;
                        Cy = (By + FIXED_TO_FLOAT (&pc->apfx[i + 1].y)) / 2;
                    } else {
                        Cx = FIXED_TO_FLOAT (&pc->apfx[i + 1].x);
                        Cy = FIXED_TO_FLOAT (&pc->apfx[i + 1].y);
                    }
                    bp->code = ART_CURVETO;
                    bp->x1 = Bx - (Bx - Ax) / 3;
                    bp->y1 = By - (By - Ay) / 3;
                    bp->x2 = Bx + (Cx - Bx) / 3;
                    bp->y2 = By + (Cy - By) / 3;
                    bp->x3 = Cx;
                    bp->y3 = Cy;
                    bp += 1;
                    Ax = Cx;
                    Ay = Cy;
                }
            }
            pos += sizeof (TTPOLYCURVE) + (pc->cpfx - 1) * sizeof (POINTFX);
        }
        if ((Cx != Sx) || (Cy != Sy)) {
            bp->code = ART_LINETO;
            bp->x3 = Sx;
            bp->y3 = Sy;
            bp += 1;
        }
    }

    bp->code = ART_END;

    slot->outline.path = nr_new (ArtBpath, bp - bpath + 1);
    memcpy (slot->outline.path, bpath, (bp - bpath + 1) * sizeof (ArtBpath));

    nr_free (gol);

    return &slot->outline;
}


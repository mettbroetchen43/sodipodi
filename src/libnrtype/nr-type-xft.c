#define __NR_TYPE_XFT_C__

/*
 * Typeface and script library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>

#include <libarikkei/arikkei-dict.h>

#ifdef WITH_FONTCONFIG
#include <fontconfig/fontconfig.h>
#else
#include <X11/Xft/Xft.h>
#include <gdk/gdkx.h>
#endif

#include "nr-type-directory.h"
#include "nr-type-xft.h"

static void nr_type_xft_init (void);

static unsigned int nrxfti = FALSE;

static NRNameList NRXftTypefaces = {0, NULL, NULL};
static NRNameList NRXftFamilies = {0, NULL, NULL};

#ifdef WITH_FONTCONFIG
static FcFontSet *NRXftPatterns = NULL;
#else
static XftFontSet *NRXftPatterns = NULL;
#endif

static ArikkeiDict NRXftNamedict;
static ArikkeiDict NRXftFamilydict;

void
nr_type_xft_typefaces_get (NRNameList *names)
{
	if (!nrxfti) nr_type_xft_init ();

	*names = NRXftTypefaces;
}

void
nr_type_xft_families_get (NRNameList *names)
{
	if (!nrxfti) nr_type_xft_init ();

	*names = NRXftFamilies;
}

void
nr_type_xft_build_def (NRTypeFaceDefFT2 *dft2, const unsigned char *name, const unsigned char *family)
{
#ifdef WITH_FONTCONFIG
	FcPattern *pat;
#else
	XftPattern *pat;
#endif

	pat = arikkei_dict_lookup (&NRXftNamedict, name);
	if (pat) {
		int index;
#ifdef WITH_FONTCONFIG
		FcChar8 *file;
		FcPatternGetString (pat, FC_FILE, 0, &file);
		FcPatternGetInteger (pat, FC_INDEX, 0, &index);
#else
		char *file;
		XftPatternGetString (pat, XFT_FILE, 0, &file);
		XftPatternGetInteger (pat, XFT_INDEX, 0, &index);
#endif
		if (file) {
			nr_type_ft2_build_def (dft2, name, family, file, index);
		}
	}
}

void
nr_type_read_xft_list (void)
{
	NRNameList gnames, gfamilies;
	const char *debugenv;
	int debug;
	int i, j;

	debugenv = getenv ("SODIPODI_DEBUG_XFT");
	debug = (debugenv && *debugenv && (*debugenv != '0'));

	nr_type_xft_typefaces_get (&gnames);
	nr_type_xft_families_get (&gfamilies);

	if (debug) {
		fprintf (stderr, "Number of usable Xft familes: %lu\n", gfamilies.length);
		fprintf (stderr, "Number of usable Xft typefaces: %lu\n", gnames.length);
	}

	for (i = gnames.length - 1; i >= 0; i--) {
		NRTypeFaceDefFT2 *tdef;
		const unsigned char *family;
		family = NULL;
		for (j = gfamilies.length - 1; j >= 0; j--) {
			int len;
			len = strlen (gfamilies.names[j]);
			if (!strncmp (gfamilies.names[j], gnames.names[i], len)) {
				family = gfamilies.names[j];
				break;
			}
		}
		if (family) {
			tdef = nr_new (NRTypeFaceDefFT2, 1);
			tdef->def.next = NULL;
			tdef->def.pdef = NULL;
			nr_type_xft_build_def (tdef, gnames.names[i], family);
			nr_type_register ((NRTypeFaceDef *) tdef);
		}
	}

	nr_name_list_release (&gfamilies);
	nr_name_list_release (&gnames);
}

static void
nr_type_xft_init (void)
{
#ifdef WITH_FONTCONFIG
	FcConfig *cfg;
	FcPattern *pat;
	FcObjectSet *obj;
	FcFontSet *fs;
#else
	XftFontSet *fs;
#endif
	const char *debugenv;
	int debug, ret;
	int i, pos, fpos;

	debugenv = getenv ("SODIPODI_DEBUG_XFT");
	debug = (debugenv && *debugenv && (*debugenv != '0'));

#ifdef WITH_FONTCONFIG
	ret = FcInit ();
	cfg = FcConfigGetCurrent ();
	pat = FcPatternCreate ();
#else
	ret = XftInit (NULL);
#endif

	if (debug) {
		fprintf (stderr, "XftInit result %d\n", ret);
		fprintf (stderr, "Reading Xft font database...\n");
	}

	/* Get family list */
#ifdef WITH_FONTCONFIG
	obj = FcObjectSetBuild (FC_SCALABLE, FC_OUTLINE, FC_FAMILY, 0);
	fs = FcFontList (cfg, pat, obj);
	FcObjectSetDestroy (obj);
#else
	fs = XftListFonts (GDK_DISPLAY (), 0,
			   XFT_SCALABLE, XftTypeBool, 1, XFT_OUTLINE, XftTypeBool, 1, 0,
			   XFT_FAMILY, 0);
#endif
	NRXftFamilies.length = fs->nfont;
	NRXftFamilies.names = nr_new (unsigned char *, NRXftFamilies.length);
	NRXftFamilies.destructor = NULL;
#ifdef WITH_FONTCONFIG
	FcFontSetDestroy (fs);
#else
	XftFontSetDestroy (fs);
#endif
	if (debug) {
		fprintf (stderr, "Read %lu families\n", NRXftFamilies.length);
	}

	/* Get typeface list */
#ifdef WITH_FONTCONFIG
	obj = FcObjectSetBuild (FC_SCALABLE, FC_OUTLINE, FC_FAMILY, FC_WEIGHT, FC_SLANT, FC_FILE, FC_INDEX, 0);
	NRXftPatterns = FcFontList (cfg, pat, obj);
	FcObjectSetDestroy (obj);
#else
	NRXftPatterns = XftListFonts (GDK_DISPLAY (), 0,
				      XFT_SCALABLE, XftTypeBool, 1, XFT_OUTLINE, XftTypeBool, 1, 0,
				      XFT_FAMILY, XFT_WEIGHT, XFT_SLANT, XFT_FILE, XFT_INDEX, 0);
#endif
	NRXftTypefaces.length = NRXftPatterns->nfont;
	NRXftTypefaces.names = nr_new (unsigned char *, NRXftTypefaces.length);
	NRXftTypefaces.destructor = NULL;
	arikkei_dict_setup_string (&NRXftNamedict, 2777);
	arikkei_dict_setup_string (&NRXftFamilydict, 173);

	if (debug) {
		fprintf (stderr, "Read %lu fonts\n", NRXftTypefaces.length);
	}

	pos = 0;
	fpos = 0;
	for (i = 0; i < NRXftPatterns->nfont; i++) {
#ifdef WITH_FONTCONFIG
		FcChar8 *name, *file;
		FcBool scalable, outline;
		FcPatternGetString (NRXftPatterns->fonts[i], FC_FAMILY, 0, &name);
#else
		char *name, *file;
		XftPatternGetString (NRXftPatterns->fonts[i], XFT_FAMILY, 0, &name);
#endif
		if (debug) {
			fprintf (stderr, "Typeface %s\n", name);
		}
#ifdef WITH_FONTCONFIG
		scalable = 0;
		outline = 0;
		file = NULL;
		FcPatternGetBool (NRXftPatterns->fonts[i], FC_SCALABLE, 0, &scalable);
		FcPatternGetBool (NRXftPatterns->fonts[i], FC_OUTLINE, 0, &outline);
		if (scalable && outline) {
			FcPatternGetString (NRXftPatterns->fonts[i], FC_FILE, 0, &file);
		}
#else
		XftPatternGetString (NRXftPatterns->fonts[i], XFT_FILE, 0, &file);
#endif
		if (file) {
			int len;
			if (debug) {
				fprintf (stderr, "Got filename %s\n", file);
			}
			len = strlen (file);
			/* fixme: This is silly and evil */
			/* But Freetype just does not load pfa reliably (Lauris) */
			if (1) /* (len > 4) &&
			    (!strcmp (file + len - 4, ".ttf") ||
			     !strcmp (file + len - 4, ".TTF") ||
			     !strcmp (file + len - 4, ".ttc") ||
			     !strcmp (file + len - 4, ".TTC") ||
			     !strcmp (file + len - 4, ".otf") ||
			     !strcmp (file + len - 4, ".OTF") ||
			     !strcmp (file + len - 4, ".pfb") ||
			     !strcmp (file + len - 4, ".PFB"))) */ {
				char *name;
				int weight;
				int slant;
#ifdef WITH_FONTCONFIG
				FcChar8 *fn, *wn, *sn;
				if (debug) {
					fprintf (stderr, "Seems valid\n");
				}
				FcPatternGetString (NRXftPatterns->fonts[i], FC_FAMILY, 0, &fn);
				FcPatternGetInteger (NRXftPatterns->fonts[i], FC_WEIGHT, 0, &weight);
				FcPatternGetInteger (NRXftPatterns->fonts[i], FC_SLANT, 0, &slant);
				switch (weight) {
				case FC_WEIGHT_LIGHT:
					wn = "Light";
					break;
				case FC_WEIGHT_MEDIUM:
					wn = "Book";
					break;
				case FC_WEIGHT_DEMIBOLD:
					wn = "Demibold";
					break;
				case FC_WEIGHT_BOLD:
					wn = "Bold";
					break;
				case FC_WEIGHT_BLACK:
					wn = "Black";
					break;
				default:
					wn = "Normal";
					break;
				}
				switch (slant) {
				case FC_SLANT_ROMAN:
					sn = "Roman";
					break;
				case FC_SLANT_ITALIC:
					sn = "Italic";
					break;
				case FC_SLANT_OBLIQUE:
					sn = "Oblique";
					break;
				default:
					sn = "Normal";
					break;
				}
#else
				char *fn, *wn, *sn;
				if (debug) {
					fprintf (stderr, "Seems valid\n");
				}
				XftPatternGetString (NRXftPatterns->fonts[i], XFT_FAMILY, 0, &fn);
				XftPatternGetInteger (NRXftPatterns->fonts[i], XFT_WEIGHT, 0, &weight);
				XftPatternGetInteger (NRXftPatterns->fonts[i], XFT_SLANT, 0, &slant);
				switch (weight) {
				case XFT_WEIGHT_LIGHT:
					wn = "Light";
					break;
				case XFT_WEIGHT_MEDIUM:
					wn = "Book";
					break;
				case XFT_WEIGHT_DEMIBOLD:
					wn = "Demibold";
					break;
				case XFT_WEIGHT_BOLD:
					wn = "Bold";
					break;
				case XFT_WEIGHT_BLACK:
					wn = "Black";
					break;
				default:
					wn = "Normal";
					break;
				}
				switch (slant) {
				case XFT_SLANT_ROMAN:
					sn = "Roman";
					break;
				case XFT_SLANT_ITALIC:
					sn = "Italic";
					break;
				case XFT_SLANT_OBLIQUE:
					sn = "Oblique";
					break;
				default:
					sn = "Normal";
					break;
				}
#endif
				if (strlen (fn) < 1024) {
					char c[1280];
					sprintf (c, "%s %s %s", fn, wn, sn);
					if (!arikkei_dict_lookup (&NRXftNamedict, c)) {
						name = strdup (c);
						if (!arikkei_dict_lookup (&NRXftFamilydict, fn)) {
							NRXftFamilies.names[fpos] = strdup (fn);
							arikkei_dict_insert (&NRXftFamilydict, NRXftFamilies.names[fpos++], (void *) TRUE);
						}
						NRXftTypefaces.names[pos++] = name;
						arikkei_dict_insert (&NRXftNamedict, name, NRXftPatterns->fonts[i]);
					}
				}
			}
		}
	}
	NRXftTypefaces.length = pos;
	NRXftFamilies.length = fpos;

	nrxfti = TRUE;
}

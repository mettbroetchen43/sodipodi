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
nr_type_read_xft_list (void)
{
	const char *debugenv;
	int debug;

        /** nr_type_xft_init() now does all the work of registering
         *   the fonts.
         */
        nr_type_xft_init();

	debugenv = getenv ("SODIPODI_DEBUG_XFT");
	debug = (debugenv && *debugenv && (*debugenv != '0'));

	if (debug) {
		NRNameList gnames, gfamilies;
		nr_type_xft_typefaces_get (&gnames);
		nr_type_xft_families_get (&gfamilies);
		fprintf (stderr, "Number of usable Xft familes: %lu\n", gfamilies.length);
		fprintf (stderr, "Number of usable Xft typefaces: %lu\n", gnames.length);
		nr_name_list_release (&gfamilies);
		nr_name_list_release (&gnames);
	}

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

	if (nrxfti) return;

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
				unsigned int nrSlant;
                                unsigned int nrWeight;
#ifdef WITH_FONTCONFIG
				FcChar8 *fn, *wn, *sn;
				if (debug) {
					fprintf (stderr, "Seems valid\n");
				}

                                if (debug) {
                                  FcChar8 *font=FcNameUnparse (NRXftPatterns->fonts[i]);
                                  fprintf(stderr,"fc: %s\n",font);
                                  free(font);
                                }

				FcPatternGetString (NRXftPatterns->fonts[i], FC_FAMILY, 0, &fn);
				FcPatternGetInteger (NRXftPatterns->fonts[i], FC_WEIGHT, 0, &weight);
				FcPatternGetInteger (NRXftPatterns->fonts[i], FC_SLANT, 0, &slant);
				switch (weight) {
				case FC_WEIGHT_LIGHT:
					nrWeight=NR_TYPEFACE_WEIGHT_LIGHT;
					break;
				case FC_WEIGHT_MEDIUM:
					nrWeight=NR_TYPEFACE_WEIGHT_MEDIUM;
					break;
				case FC_WEIGHT_DEMIBOLD:
					nrWeight=NR_TYPEFACE_WEIGHT_DEMIBOLD;
					break;
				case FC_WEIGHT_BOLD:
					nrWeight=NR_TYPEFACE_WEIGHT_BOLD;
					break;
				case FC_WEIGHT_BLACK:
					nrWeight=NR_TYPEFACE_WEIGHT_BLACK;
					break;
				default:
					nrWeight=NR_TYPEFACE_WEIGHT_NORMAL;
					break;
				}
				switch (slant) {
				case FC_SLANT_ROMAN:
					nrSlant=NR_TYPEFACE_SLANT_ROMAN;
					break;
				case FC_SLANT_ITALIC:
					nrSlant=NR_TYPEFACE_SLANT_ITALIC;
					break;
				case FC_SLANT_OBLIQUE:
					nrSlant=NR_TYPEFACE_SLANT_OBLIQUE;
					break;
				default:
					nrSlant=NR_TYPEFACE_SLANT_ROMAN;
					break;
				}

                                wn = (FcChar8*) nr_type_weight_to_string (nrWeight);
                                sn = (FcChar8*) nr_type_slant_to_string (nrSlant);

#else
				const unsigned char *fn, *wn, *sn;
				if (debug) {
					fprintf (stderr, "Seems valid\n");
				}
				XftPatternGetString (NRXftPatterns->fonts[i], XFT_FAMILY, 0, &fn);
				XftPatternGetInteger (NRXftPatterns->fonts[i], XFT_WEIGHT, 0, &weight);
				XftPatternGetInteger (NRXftPatterns->fonts[i], XFT_SLANT, 0, &slant);
				switch (weight) {
				case XFT_WEIGHT_LIGHT:
					nrWeight=NR_TYPEFACE_WEIGHT_LIGHT;
					break;
				case XFT_WEIGHT_MEDIUM:
					nrWeight=NR_TYPEFACE_WEIGHT_MEDIUM;
					break;
				case XFT_WEIGHT_DEMIBOLD:
					nrWeight=NR_TYPEFACE_WEIGHT_DEMIBOLD;
					break;
				case XFT_WEIGHT_BOLD:
					nrWeight=NR_TYPEFACE_WEIGHT_BOLD;
					break;
				case XFT_WEIGHT_BLACK:
					nrWeight=NR_TYPEFACE_WEIGHT_BLACK;
					break;
				default:
					nrWeight=NR_TYPEFACE_WEIGHT_NORMAL;
					break;
				}
				switch (slant) {
				case XFT_SLANT_ROMAN:
					nrSlant=NR_TYPEFACE_SLANT_ROMAN;
					break;
				case XFT_SLANT_ITALIC:
					nrSlant=NR_TYPEFACE_SLANT_ITALIC;
					break;
				case XFT_SLANT_OBLIQUE:
					nrSlant=NR_TYPEFACE_SLANT_OBLIQUE;
					break;
				default:
					nrSlant=NR_TYPEFACE_SLANT_ROMAN;
					break;
				}

                                wn = nr_type_weight_to_string (nrWeight);
                                sn = nr_type_slant_to_string (nrSlant);
#endif

                                if (debug) {
                                  fprintf(stderr,"%s %s (%d) %s (%d)\n",
                                          fn,wn,(int)weight,sn,(int)slant);
                                }

				if (strlen (fn) < 1024) {
					char c[1280];
                                        int index;
#ifdef WITH_FONTCONFIG
                                        FcChar8 *file;
#else
                                        char *file;
#endif
					sprintf (c, "%s %s %s", fn, wn, sn);

                                        name = strdup (c);
                                        if (!arikkei_dict_lookup (&NRXftFamilydict, fn)) {
                                          NRXftFamilies.names[fpos] = strdup (fn);
                                          arikkei_dict_insert (&NRXftFamilydict, NRXftFamilies.names[fpos++], (void *) TRUE);
                                        }
                                        NRXftTypefaces.names[pos++] = name;
#ifdef WITH_FONTCONFIG
                                        FcPatternGetString (NRXftPatterns->fonts[i], FC_FILE, 0, &file);
                                        FcPatternGetInteger (NRXftPatterns->fonts[i], FC_INDEX, 0, &index);
#else
                                        XftPatternGetString (NRXftPatterns->fonts[i], XFT_FILE, 0, &file);
                                        XftPatternGetInteger (NRXftPatterns->fonts[i], XFT_INDEX, 0, &index);
#endif

                                        if (file) {
                                          NRTypeFaceDefFT2 *tdef = nr_new (NRTypeFaceDefFT2, 1);
                                          tdef->def.next = NULL;
                                          tdef->def.pdef = NULL;
                                          nr_type_ft2_build_def (tdef, name, fn, file,
                                                                 nrSlant, nrWeight,
                                                                 index);
                                          nr_type_register ((NRTypeFaceDef *) tdef);
                                        }
				}
			}
		}
	}
	NRXftTypefaces.length = pos;
	NRXftFamilies.length = fpos;

	nrxfti = TRUE;
}

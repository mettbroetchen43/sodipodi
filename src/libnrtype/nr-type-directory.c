#define __NR_TYPE_DIRECTORY_C__

/*
 * Typeface and script library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#define noTFDEBUG

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <stdio.h>
#include <ctype.h>

#include <libarikkei/arikkei-token.h>
#include <libarikkei/arikkei-strlib.h>
#include <libarikkei/arikkei-iolib.h>

#include <libnr/nr-macros.h>
#include <libnr/nr-values.h>
#include "nr-type-primitives.h"
#include "nr-type-ft2.h"
#ifdef WITH_GNOME_PRINT
#include "nr-type-gnome.h"
#endif
#ifdef WITH_XFT
#include "nr-type-xft.h"
#endif
#ifdef WIN32
#include "nr-type-w32.h"
#endif

#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif

#include "nr-type-directory.h"

typedef struct _NRFamilyDef NRFamilyDef;

struct _NRFamilyDef {
	NRFamilyDef *next;
	unsigned char *name;
	NRTypeFaceDef *faces;
};

struct _NRTypePosDef {
	unsigned int italic : 1;
	unsigned int oblique : 1;
	unsigned int weight : 8;
	unsigned int stretch : 8;
};

static void nr_type_directory_build (void);
static void nr_type_calculate_position (NRTypePosDef *pdef, const unsigned char *name);
static float nr_type_distance_family_better (const unsigned char *ask, const unsigned char *bid, float best);
static float nr_type_distance_position_better (NRTypePosDef *ask, NRTypePosDef *bid, float best);

static void nr_type_read_private_list (void);
#if 0
static void nr_type_read_w32_list (void);
#endif

static NRTypeDict *typedict = NULL;
static NRTypeDict *familydict = NULL;

static NRFamilyDef *families = NULL;

NRTypeFace *
nr_type_directory_lookup (const unsigned char *name)
{
	NRTypeFaceDef *tdef;

	if (!typedict) nr_type_directory_build ();

	tdef = nr_type_dict_lookup (typedict, name);

	if (tdef) {
		if (!tdef->typeface) {
			tdef->typeface = nr_typeface_new (tdef);
		} else {
			nr_typeface_ref (tdef->typeface);
		}
		return tdef->typeface;
	}

	return NULL;
}

NRTypeFace *
nr_type_directory_lookup_fuzzy (const unsigned char *family, const unsigned char *description)
{
	NRFamilyDef *fdef, *bestfdef;
	float best, dist;
	NRTypeFaceDef *tdef, *besttdef;
	NRTypePosDef apos;

	if (!typedict) nr_type_directory_build ();

	best = NR_HUGE_F;
	bestfdef = NULL;

	for (fdef = families; fdef; fdef = fdef->next) {
		dist = nr_type_distance_family_better (family, fdef->name, best);
		if (dist < best) {
			best = dist;
			bestfdef = fdef;
		}
		if (best == 0.0) break;
	}

	if (!bestfdef) return NULL;

	best = NR_HUGE_F;
	besttdef = NULL;

	/* fixme: In reality the latter method reqires full qualified name */
	nr_type_calculate_position (&apos, description);

	for (tdef = bestfdef->faces; tdef; tdef = tdef->next) {
		dist = nr_type_distance_position_better (&apos, tdef->pdef, best);
		if (dist < best) {
			best = dist;
			besttdef = tdef;
		}
		if (best == 0.0) break;
	}

	if (!besttdef->typeface) {
		besttdef->typeface = nr_typeface_new (besttdef);
	} else {
		nr_typeface_ref (besttdef->typeface);
	}

	return besttdef->typeface;
}

void
nr_type_directory_forget_face (NRTypeFace *tf)
{
	tf->def->typeface = NULL;
}

NRNameList *
nr_type_directory_family_list_get (NRNameList *flist)
{
	static int flen = 0;
	static unsigned char **fnames = NULL;

	if (!typedict) nr_type_directory_build ();

	if (!fnames) {
		NRFamilyDef *fdef;
		int pos;
		for (fdef = families; fdef; fdef = fdef->next) flen += 1;
		fnames = nr_new (unsigned char *, flen);
		pos = 0;
		for (fdef = families; fdef; fdef = fdef->next) {
			fnames[pos] = fdef->name;
			pos += 1;
		}
	}

	flist->length = flen;
	flist->names = fnames;
	flist->destructor = NULL;

	return flist;
}

unsigned int
nr_type_register (NRTypeFaceDef *def)
{
	NRFamilyDef *fdef;

	if (nr_type_dict_lookup (typedict, def->name)) return 0;

	fdef = nr_type_dict_lookup (familydict, def->family);
	if (!fdef) {
		fdef = nr_new (NRFamilyDef, 1);
		fdef->name = strdup (def->family);
		fdef->faces = NULL;
		fdef->next = families;
		families = fdef;
		nr_type_dict_insert (familydict, fdef->name, fdef);
	}

	def->next = fdef->faces;
	fdef->faces = def;

	nr_type_dict_insert (typedict, def->name, def);

	return 1;
}

static void
nr_type_directory_style_list_destructor (NRNameList *list)
{
	if (list->names) nr_free (list->names);
}

NRNameList *
nr_type_directory_style_list_get (const unsigned char *family, NRNameList *styles)
{
	NRFamilyDef *fdef;

	if (!typedict) nr_type_directory_build ();

	fdef = nr_type_dict_lookup (familydict, family);

	styles->destructor = nr_type_directory_style_list_destructor;

	if (fdef) {
		NRTypeFaceDef *tdef;
		int tlen, pos;
		tlen = 0;
		for (tdef = fdef->faces; tdef; tdef = tdef->next) tlen += 1;
		styles->length = tlen;
		styles->names = nr_new (unsigned char *, styles->length);
		pos = 0;
		for (tdef = fdef->faces; tdef; tdef = tdef->next) {
			styles->names[pos] = tdef->name;
			pos += 1;
		}
	} else {
		styles->length = 0;
		styles->names = NULL;
	}

	return styles;
}

static int
nr_type_family_def_compare (const void *a, const void *b)
{
#ifndef WIN32
	return strcasecmp ((*((NRFamilyDef **) a))->name, (*((NRFamilyDef **) b))->name);
#else
	return stricmp ((*((NRFamilyDef **) a))->name, (*((NRFamilyDef **) b))->name);
#endif
}

static void
nr_type_directory_build (void)
{
	NRFamilyDef *fdef, **ffdef;
	NRTypePosDef *pdefs;
	int fnum, tnum, pos, i;

	typedict = nr_type_dict_new ();
	familydict = nr_type_dict_new ();

	nr_type_read_private_list ();

#ifdef WIN32
	nr_type_read_w32_list ();
#endif

#ifdef WITH_XFT
	nr_type_read_xft_list ();
#endif

#ifdef WITH_GNOME_PRINT
	nr_type_read_gnome_list ();
#endif

	if (!families) {
		NRTypeFaceDef *def;
		/* No families, register empty typeface */
		def = nr_new (NRTypeFaceDef, 1);
		def->next = NULL;
		def->pdef = NULL;
		nr_type_empty_build_def (def, "empty", "Empty");
		nr_type_register (def);
	}

	/* Sort families */
	fnum = 0;
	for (fdef = families; fdef; fdef = fdef->next) fnum += 1;
	ffdef = nr_new (NRFamilyDef *, fnum);
	pos = 0;
	for (fdef = families; fdef; fdef = fdef->next) {
		ffdef[pos] = fdef;
		pos += 1;
	}
	qsort (ffdef, fnum, sizeof (NRFamilyDef *), nr_type_family_def_compare);
	for (i = 0; i < fnum - 1; i++) {
		ffdef[i]->next = ffdef[i + 1];
	}
	ffdef[i]->next = NULL;
	families = ffdef[0];
	nr_free (ffdef);

	/* Build posdefs */
	tnum = 0;
	for (fdef = families; fdef; fdef = fdef->next) {
		NRTypeFaceDef *tdef;
		for (tdef = fdef->faces; tdef; tdef = tdef->next) tnum += 1;
	}

	pdefs = nr_new (NRTypePosDef, tnum);
	pos = 0;
	for (fdef = families; fdef; fdef = fdef->next) {
		NRTypeFaceDef *tdef;
		for (tdef = fdef->faces; tdef; tdef = tdef->next) {
			tdef->pdef = pdefs + pos;
			nr_type_calculate_position (tdef->pdef, tdef->name);
			pos += 1;
		}
	}
}

static void
nr_type_calculate_position (NRTypePosDef *pdef, const unsigned char *name)
{
	unsigned char c[256];
	unsigned char *p;

	strncpy (c, name, 255);
	c[255] = 0;
	for (p = c; *p; p++) *p = tolower (*p);

	pdef->italic = (strstr (c, "italic") != NULL);
	pdef->oblique = (strstr (c, "oblique") != NULL);
	if (strstr (c, "thin")) {
		pdef->weight = 32;
	} else if (strstr (c, "extra light")) {
		pdef->weight = 64;
	} else if (strstr (c, "ultra light")) {
		pdef->weight = 64;
	} else if (strstr (c, "light")) {
		pdef->weight = 96;
	} else if (strstr (c, "book")) {
		pdef->weight = 128;
	} else if (strstr (c, "medium")) {
		pdef->weight = 144;
	} else if (strstr (c, "semi bold")) {
		pdef->weight = 160;
	} else if (strstr (c, "semibold")) {
		pdef->weight = 160;
	} else if (strstr (c, "demi bold")) {
		pdef->weight = 160;
	} else if (strstr (c, "demibold")) {
		pdef->weight = 160;
	} else if (strstr (c, "bold")) {
		pdef->weight = 192;
	} else if (strstr (c, "ultra bold")) {
		pdef->weight = 224;
	} else if (strstr (c, "extra bold")) {
		pdef->weight = 224;
	} else if (strstr (c, "black")) {
		pdef->weight = 255;
	} else {
		pdef->weight = 128;
	}

	if (strstr (c, "narrow")) {
		pdef->stretch = 64;
	} else if (strstr (c, "condensed")) {
		pdef->stretch = 64;
	} else if (strstr (c, "wide")) {
		pdef->stretch = 192;
	} else {
		pdef->stretch = 128;
	}
}

static float
nr_type_distance_family_better (const unsigned char *ask, const unsigned char *bid, float best)
{
	int alen, blen;

#ifndef WIN32
	if (!strcasecmp (ask, bid)) return MIN (best, 0.0F);
#else
	if (!stricmp (ask, bid)) return MIN (best, 0.0F);
#endif

	alen = strlen (ask);
	blen = strlen (bid);

#ifndef WIN32
	if ((blen < alen) && !strncasecmp (ask, bid, blen)) return MIN (best, 1.0F);
	if (!strcasecmp (bid, "bitstream cyberbit")) return MIN (best, 10.0F);
	if (!strcasecmp (bid, "arial")) return MIN (best, 100.0F);
	if (!strcasecmp (bid, "helvetica")) return MIN (best, 1000.0F);
#else
	if ((blen < alen) && !strnicmp (ask, bid, blen)) return MIN (best, 1.0F);
	if (!stricmp (bid, "bitstream cyberbit")) return MIN (best, 10.0F);
	if (!stricmp (bid, "arial")) return MIN (best, 100.0F);
	if (!stricmp (bid, "helvetica")) return MIN (best, 1000.0F);
#endif

	return 10000.0;
}

#define NR_TYPE_ITALIC_SCALE 10000.0F
#define NR_TYPE_OBLIQUE_SCALE 1000.0F
#define NR_TYPE_WEIGHT_SCALE 100.0F
#define NR_TYPE_STRETCH_SCALE 2000.0F

static float
nr_type_distance_position_better (NRTypePosDef *ask, NRTypePosDef *bid, float best)
{
	float ditalic, doblique, dweight, dstretch;
	float dist;

	ditalic = NR_TYPE_ITALIC_SCALE * ((int) ask->italic - (int) bid->italic);
	doblique = NR_TYPE_OBLIQUE_SCALE * ((int) ask->oblique - (int) bid->oblique);
	dweight = NR_TYPE_WEIGHT_SCALE * ((int) ask->weight - (int) bid->weight);
	dstretch = NR_TYPE_STRETCH_SCALE * ((int) ask->stretch - (int) bid->stretch);

	dist = sqrt (ditalic * ditalic + doblique * doblique + dweight * dweight + dstretch * dstretch);

	return MIN (dist, best);
}

static unsigned char privatename[] = "/.sodipodi/private-fonts";

#ifndef WIN32
#include <unistd.h>
#include <sys/mman.h>
#endif

static void
nr_type_read_private_list (void)
{
	unsigned char *homedir, *filename;
	int len;
	struct stat st;

	homedir = getenv ("HOME");
	if (!homedir) return;
	len = strlen (homedir);
	filename = nr_new (unsigned char, len + sizeof (privatename) + 1);
	strcpy (filename, homedir);
	strcpy (filename + len, privatename);

#ifndef S_ISREG
#define S_ISREG(st) 1
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

	if (!stat (filename, &st) && S_ISREG (st.st_mode) && (st.st_size > 8)) {
		const unsigned char *cdata;
		int size;
		ArikkeiToken ft, lt;

		cdata = arikkei_mmap (filename, &size, NULL);
		if ((cdata == NULL) || (cdata == (unsigned char *) -1)) return;
		arikkei_token_set_from_data (&ft, cdata, 0, st.st_size);
		arikkei_token_get_first_line (&ft, &lt);
		while (lt.start < lt.end) {
			if (!arikkei_token_is_empty (&lt) && (lt.cdata[lt.start] != '#')) {
				ArikkeiToken tokens[4];
				int ntokens;
				ntokens = arikkei_token_tokenize_ws (&lt, tokens, 4, ",", FALSE);
				if (ntokens >= 3) {
					ArikkeiToken fnt[2];
					ArikkeiToken filet, namet, familyt;
					int nfnt, face;
					nfnt = arikkei_token_tokenize_ws (&tokens[0], fnt, 2, ":", FALSE);
					arikkei_token_strip (&fnt[0], &filet);
					arikkei_token_strip (&tokens[1], &namet);
					arikkei_token_strip (&tokens[2], &familyt);
					face = 0;
					if (nfnt > 1) {
						unsigned char b[32];
						arikkei_token_strncpy (&fnt[1], b, 32);
						face = atoi (b);
					}
					if (!arikkei_token_is_empty (&filet) &&
					    !arikkei_token_is_empty (&namet) &&
					    !arikkei_token_is_empty (&familyt)) {
						NRTypeFaceDefFT2 *dft2;
						unsigned char f[1024], n[1024], m[1024];
						dft2 = nr_new (NRTypeFaceDefFT2, 1);
						dft2->def.next = NULL;
						dft2->def.pdef = NULL;
						arikkei_token_strncpy (&filet, f, 1024);
						arikkei_token_strncpy (&namet, n, 1024);
						arikkei_token_strncpy (&familyt, m, 1024);
						nr_type_ft2_build_def (dft2, f, n, m, face);
						nr_type_register ((NRTypeFaceDef *) dft2);
					}
				}
			}
			arikkei_token_next_line (&ft, &lt, &lt);
		}
		arikkei_munmap (cdata, size);
	}

	nr_free (filename);
}

NRTypeFace *
nr_type_build (const unsigned char *name, const unsigned char *family,
	       const unsigned char *data, unsigned int size, unsigned int face)
{
	NRTypeFaceDefFT2 *dft2;

	if (!typedict) nr_type_directory_build ();

	dft2 = nr_new (NRTypeFaceDefFT2, 1);
	dft2->def.next = NULL;
	dft2->def.pdef = NULL;
	nr_type_ft2_build_def_data (dft2, name, family, data, size, face);
	nr_type_register ((NRTypeFaceDef *) dft2);

	return nr_type_directory_lookup (name);
}


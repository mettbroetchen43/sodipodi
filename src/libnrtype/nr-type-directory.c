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

#include <config.h>

#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <libnr/nr-macros.h>
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
#ifdef WIN32
static void nr_type_read_w32_list (void);
#endif
#ifdef WITH_GNOME_PRINT
static void nr_type_read_gnome_list (void);
#endif
#ifdef WITH_XFT
static void nr_type_read_xft_list (void);
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
			tdef->typeface = tdef->vmv->new (tdef);
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
		besttdef->typeface = besttdef->vmv->new (besttdef);
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

static int
nr_type_family_def_compare (const void *a, const void *b)
{
	return strcasecmp ((*((NRFamilyDef **) a))->name, (*((NRFamilyDef **) b))->name);
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

	if (!strcasecmp (ask, bid)) return MIN (best, 0.0);

	alen = strlen (ask);
	blen = strlen (bid);

	if ((blen < alen) && !strncasecmp (ask, bid, blen)) return MIN (best, 1.0);

	if (!strcasecmp (bid, "bitstream cyberbit")) return MIN (best, 10.0);
	if (!strcasecmp (bid, "arial")) return MIN (best, 100.0);
	if (!strcasecmp (bid, "helvetica")) return MIN (best, 1000.0);

	return 10000.0;
}

#define NR_TYPE_ITALIC_SCALE 10000.0
#define NR_TYPE_OBLIQUE_SCALE 1000.0
#define NR_TYPE_WEIGHT_SCALE 100.0
#define NR_TYPE_STRETCH_SCALE 2000.0

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

static unsigned int
nr_type_next_token (const unsigned char *img, unsigned int len, unsigned int p, int *tokenp)
{
	/* Skip whitespace */
	while (((img[p] == ' ') || (img[p] == '\t')) && (p < len)) p++;
	if (p >= len) return p;
	if (!isalnum (img[p]) && (img[p] != '/')) return p;
	*tokenp = p;
	while (!iscntrl (img[p]) && (img[p] != ',') && (p < len)) p++;
	return p;
}

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

	if (!stat (filename, &st) && S_ISREG (st.st_mode) && (st.st_size > 8)) {
		unsigned char *img;
		int fh, rbytes, nentries, p;
		img = nr_new (unsigned char, st.st_size + 1);
		if (!img) return;
		fh = open (filename, O_RDONLY);
		if (fh < 1) return;
		rbytes = read (fh, img, st.st_size);
		close (fh);
		if (rbytes < st.st_size) return;
		*(img + st.st_size) = 0;

		/* format: file, name, family */
		nentries = 0;
		p = 0;
		while (p < st.st_size) {
			int filep, namep, familyp;
			int e0, e1, e2;
			filep = -1;
			namep = -1;
			familyp = -1;
			/* File */
			p = nr_type_next_token (img, st.st_size, p, &filep);
			if (p >= st.st_size) break;
			if (!iscntrl (img[p])) {
				e0 = p;
				p += 1;
				/* Name */
				p = nr_type_next_token (img, st.st_size, p, &namep);
				if (p >= st.st_size) break;
				if (!iscntrl (img[p])) {
					e1 = p;
					p += 1;
					/* Family */
					p = nr_type_next_token (img, st.st_size, p, &familyp);
					e2 = p;
					p += 1;
					if ((filep >= 0) && (namep >= 0) && (familyp >= 0)) {
						struct stat st;
						if (!stat (filename, &st) && S_ISREG (st.st_mode)) {
							NRTypeFaceDefFT2 *dft2;
							int face;
							char *cp;
							img[e0] = 0;
							img[e1] = 0;
							img[e2] = 0;
							cp = strchr (img + filep, ':');
							if (cp) {
								*cp = 0;
								face = atoi (cp + 1);
							} else {
								face = 0;
							}
							/* printf ("Found %s | %d | %s | %s\n", img + filep, face, img + namep, img + familyp); */
							dft2 = nr_new (NRTypeFaceDefFT2, 1);
							dft2->def.next = NULL;
							dft2->def.pdef = NULL;
							nr_type_ft2_build_def (dft2, img + namep, img + familyp, img + filep, face);
							nr_type_register ((NRTypeFaceDef *) dft2);
							nentries += 1;
						}
					}
				}
			}
			while (iscntrl (img[p]) && (p < st.st_size)) p++;
		}

		if (nentries > 0) {
		}
	} else {
		printf ("File %s does not exist\n", filename);
	}

	nr_free (filename);
}

#ifdef WIN32
static void
nr_type_read_w32_list (void)
{
	NRNameList wnames, wfamilies;
	int i, j;

	nr_type_w32_typefaces_get (&wnames);
	nr_type_w32_families_get (&wfamilies);

	for (i = wnames.length - 1; i >= 0; i--) {
		NRTypeFaceDef *tdef;
		const unsigned char *family;
		family = NULL;
		for (j = wfamilies.length - 1; j >= 0; j--) {
			int len;
			len = strlen (wfamilies.names[j]);
			if (!strncmp (wfamilies.names[j], wnames.names[i], len)) {
				family = wfamilies.names[j];
				break;
			}
		}
		if (family) {
			tdef = nr_new (NRTypeFaceDef, 1);
			tdef->next = NULL;
			tdef->pdef = NULL;
			nr_type_w32_build_def (tdef, wnames.names[i], family);
			nr_type_register (tdef);
		}
	}

	nr_name_list_release (&wfamilies);
	nr_name_list_release (&wnames);
}
#endif

#ifdef WITH_XFT
static void
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
#endif

#ifdef WITH_GNOME_PRINT
static void
nr_type_read_gnome_list (void)
{
	NRNameList gnames, gfamilies;
	int i, j;

	nr_type_gnome_typefaces_get (&gnames);
	nr_type_gnome_families_get (&gfamilies);

	for (i = gnames.length - 1; i >= 0; i--) {
		NRTypeFaceDef *tdef;
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
			tdef = nr_new (NRTypeFaceDef, 1);
			tdef->next = NULL;
			tdef->pdef = NULL;
			nr_type_gnome_build_def (tdef, gnames.names[i], family);
			nr_type_register (tdef);
		}
	}

	nr_name_list_release (&gfamilies);
	nr_name_list_release (&gnames);
}
#endif


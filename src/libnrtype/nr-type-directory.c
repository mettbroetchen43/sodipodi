#define __NR_TYPE_DIRECTORY_C__

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

#define noTFDEBUG

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
#include "nr-type-gnome.h"
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
};

static void nr_type_directory_build (void);
static void nr_type_calculate_position (NRTypePosDef *pdef, const unsigned char *name);
static float nr_type_distance_family_better (const unsigned char *ask, const unsigned char *bid, float best);
static float nr_type_distance_position_better (NRTypePosDef *ask, NRTypePosDef *bid, float best);

static void nr_type_read_private_list (void);
static void nr_type_read_gnome_list (void);

static NRTypeDict *typedict = NULL;
static NRTypeDict *familydict = NULL;

static NRFamilyDef *families = NULL;

#if 0
static NRTypeFaceDef *tdefs = NULL;
static NRPosDef *pdefs = NULL;
static NRFamilyDef *fdefs = NULL;
static unsigned int ntdefs = 0;
static unsigned int nfdefs = 0;
#endif

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
		unsigned int pos;
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
		unsigned int tlen, pos;
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
	return strcasecmp ((*((NRFamilyDef **) a))->name, (*((NRFamilyDef **) b))->name);
}

static void
nr_type_directory_build (void)
{
	NRFamilyDef *fdef, **ffdef;
	NRTypePosDef *pdefs;
	unsigned int fnum, tnum, pos, i;

	typedict = nr_type_dict_new ();
	familydict = nr_type_dict_new ();

	nr_type_read_private_list ();

	nr_type_read_gnome_list ();

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

static int
nr_type_register (NRTypeFaceDef *def, const unsigned char *family)
{
	NRFamilyDef *fdef;

	if (nr_type_dict_lookup (typedict, def->name)) return 0;

	fdef = nr_type_dict_lookup (familydict, family);
	if (!fdef) {
		fdef = nr_new (NRFamilyDef, 1);
		fdef->name = strdup (family);
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
nr_type_calculate_position (NRTypePosDef *pdef, const unsigned char *name)
{
	unsigned char c[256];
	unsigned char *p;

	strncpy (c, name, 255);
	c[255] = 0;
	for (p = c; *p; p++) *p = tolower (*p);

	pdef->italic = (strstr (c, "italic") != NULL);
	pdef->oblique = (strstr (c, "oblique") != NULL);
	if (strstr (c, "bold")) {
		pdef->weight = 192;
	} else {
		pdef->weight = 128;
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

static float
nr_type_distance_position_better (NRTypePosDef *ask, NRTypePosDef *bid, float best)
{
	float ditalic, doblique, dweight;
	float dist;

	ditalic = NR_TYPE_ITALIC_SCALE * (ask->italic - bid->italic);
	doblique = NR_TYPE_OBLIQUE_SCALE * (ask->oblique - bid->oblique);
	dweight = NR_TYPE_WEIGHT_SCALE * (ask->weight - bid->weight);

	dist = sqrt (ditalic * ditalic + doblique * doblique + dweight * dweight);

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
			if (iscntrl (img[p])) break;
			e0 = p;
			p += 1;
			/* Name */
			p = nr_type_next_token (img, st.st_size, p, &namep);
			if (p >= st.st_size) break;
			if (iscntrl (img[p])) break;
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
					cp = strchr (img + namep, ':');
					if (cp) {
						*cp = 0;
						face = atoi (cp + 1);
					} else {
						face = 0;
					}
					printf ("Found %s | %d | %s | %s\n", img + filep, face, img + namep, img + familyp);
					dft2 = nr_new (NRTypeFaceDefFT2, 1);
					dft2->def.next = NULL;
					dft2->def.pdef = NULL;
					nr_type_ft2_build_def (dft2, img + namep, img + filep, face);
					nr_type_register ((NRTypeFaceDef *) dft2, img + familyp);
					nentries += 1;
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
		tdef = nr_new (NRTypeFaceDef, 1);
		tdef->next = NULL;
		tdef->pdef = NULL;
		nr_type_gnome_build_def (tdef, gnames.names[i]);
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
			nr_type_register (tdef, family);
		}
	}

	nr_name_list_release (&gfamilies);
	nr_name_list_release (&gnames);
}


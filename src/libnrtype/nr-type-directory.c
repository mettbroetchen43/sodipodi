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
#include <ctype.h>
#include <libnr/nr-macros.h>
#include "nr-type-gnome.h"
#include "nr-type-directory.h"

typedef struct _NRFamilyDef NRFamilyDef;
typedef struct _NRPosDef NRPosDef;

struct _NRFamilyDef {
	unsigned char *name;
	NRTypeFaceDef *faces;
	int nfaces;
};

struct _NRPosDef {
	unsigned int italic : 1;
	unsigned int oblique : 1;
	unsigned int weight : 8;
};

static void nr_type_directory_build (void);
static void nr_type_calculate_position (NRPosDef *pdef, const unsigned char *name);
static float nr_type_distance_family_better (const unsigned char *ask, const unsigned char *bid, float best);
static float nr_type_distance_position_better (NRPosDef *ask, NRPosDef *bid, float best);

static NRTypeFaceDef *tdefs = NULL;
static NRPosDef *pdefs = NULL;
static NRFamilyDef *fdefs = NULL;
static unsigned int ntdefs = 0;
static unsigned int nfdefs = 0;

void
nr_name_list_release (NRNameList *list)
{
	if (list->destructor) {
		list->destructor (list);
	}
}

NRTypeFace *
nr_type_directory_lookup (const unsigned char *name)
{
	int i;

	if (!tdefs) {
		nr_type_directory_build ();
	}

	for (i = 0; i < ntdefs; i++) {
		if (!strcmp (name, tdefs[i].name)) {
			if (!tdefs[i].typeface) {
				tdefs[i].typeface = tdefs[i].vmv->new (tdefs + i);
			} else {
				nr_typeface_ref (tdefs[i].typeface);
			}
			return tdefs[i].typeface;
		}
	}

	return NULL;
}

NRTypeFace *
nr_type_directory_lookup_fuzzy (const unsigned char *family, const unsigned char *description)
{
	float best, dist;
	int bestfidx, i;
	NRTypeFaceDef *besttdef;
	NRPosDef apos;

	if (!tdefs) {
		nr_type_directory_build ();
	}

	best = NR_HUGE_F;
	bestfidx = -1;

	for (i = 0; i < nfdefs; i++) {
		dist = nr_type_distance_family_better (family, fdefs[i].name, best);
		if (dist < best) {
			best = dist;
			bestfidx = i;
		}
		if (best == 0.0) break;
	}

	if (bestfidx < 0) return NULL;

	best = NR_HUGE_F;
	besttdef = fdefs[bestfidx].faces;

	/* fixme: In reality the latter method reqires full qualified name */
	nr_type_calculate_position (&apos, description);

	for (i = 0; i < fdefs[bestfidx].nfaces; i++) {
		int idx;
		idx = fdefs[bestfidx].faces - tdefs + i;
		dist = nr_type_distance_position_better (&apos, pdefs + idx, best);
		if (dist < best) {
			best = dist;
			besttdef = fdefs[bestfidx].faces + i;
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
	int i;

	for (i = 0; i < ntdefs; i++) {
		if (tdefs[i].typeface == tf) {
			tdefs[i].typeface = NULL;
			break;
		}
	}
}

NRNameList *
nr_type_directory_family_list_get (NRNameList *families)
{
	return nr_type_gnome_families_get (families);
}

static void
nr_type_directory_style_list_destructor (NRNameList *list)
{
	if (list->names) nr_free (list->names);
}

NRNameList *
nr_type_directory_style_list_get (const unsigned char *family, NRNameList *styles)
{
	int i;

	styles->destructor = nr_type_directory_style_list_destructor;

	for (i = 0; i < nfdefs; i++) {
		if (!strcmp (family, fdefs[i].name)) {
			int j;
			styles->length = fdefs[i].nfaces;
			styles->names = nr_new (unsigned char *, styles->length);
			for (j = 0; j < styles->length; j++) {
				styles->names[j] = fdefs[i].faces[j].name;
			}
			return styles;
		}
	}

	styles->length = 0;
	styles->names = NULL;

	return styles;
}

static void
nr_type_directory_build (void)
{
	NRNameList gnames, gfamilies;
	int fpos, pos, i, j;

	nr_type_gnome_typefaces_get (&gnames);
	nr_type_gnome_families_get (&gfamilies);

	tdefs = nr_new (NRTypeFaceDef, gnames.length);
	pdefs = nr_new (NRPosDef, gnames.length);
	fdefs = nr_new (NRFamilyDef, gfamilies.length);

	for (i = 0; i < gnames.length; i++) {
		nr_type_gnome_build_def (tdefs + i, gnames.names[i]);
	}
	ntdefs = gnames.length;

	fpos = 0;
	pos = 0;
	for (i = 0; i < gfamilies.length; i++) {
		int start, len;
		start = pos;
		len = strlen (gfamilies.names[i]);
		for (j = pos; j < gnames.length; j++) {
			if (!strncmp (gfamilies.names[i], tdefs[j].name, len)) {
				NRTypeFaceDef t;
				t = tdefs[pos];
				tdefs[pos] = tdefs[j];
				tdefs[j] = t;
				pos += 1;
			}
		}
		if (pos > start) {
			fdefs[fpos].name = strdup (gfamilies.names[i]);
			fdefs[fpos].faces = tdefs + start;
			fdefs[fpos].nfaces = pos - start;
			fpos += 1;
		}
	}
	nfdefs = fpos;

	/* Build posdefs */
	for (i = 0; i < ntdefs; i++) {
		nr_type_calculate_position (pdefs + i, tdefs[i].name);
	}

	nr_name_list_release (&gfamilies);
	nr_name_list_release (&gnames);
}

static void
nr_type_calculate_position (NRPosDef *pdef, const unsigned char *name)
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
nr_type_distance_position_better (NRPosDef *ask, NRPosDef *bid, float best)
{
	float ditalic, doblique, dweight;
	float dist;

	ditalic = NR_TYPE_ITALIC_SCALE * (ask->italic - bid->italic);
	doblique = NR_TYPE_OBLIQUE_SCALE * (ask->oblique - bid->oblique);
	dweight = NR_TYPE_WEIGHT_SCALE * (ask->weight - bid->weight);

	dist = sqrt (ditalic * ditalic + doblique * doblique + dweight * dweight);

	return MIN (dist, best);
}



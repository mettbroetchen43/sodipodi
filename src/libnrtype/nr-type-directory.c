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

#include <string.h>
#include <ctype.h>
#include <libnr/nr-macros.h>
#include "nr-type-gnome.h"
#include "nr-type-directory.h"

void
nr_name_list_release (NRNameList *list)
{
	if (list->destructor) {
		list->destructor (list);
	}
}

NRTypeFace *
nr_type_directory_lookup_fuzzy (const unsigned char *family, const unsigned char *style)
{
	NRTypeFace *face;
	unsigned int weight;
	unsigned int italic;

	italic = FALSE;
	weight = GNOME_FONT_BOOK;
	if (style) {
		gchar *s;
		s = g_strdup (style);
		g_strdown (s);
		if (strstr (s, "italic")) italic = TRUE;
		if (strstr (s, "oblique")) italic = TRUE;
		if (strstr (s, "bold")) weight = GNOME_FONT_BOLD;
		g_free (s);
	}

	face = nr_new (NRTypeFace, 1);

	face->refcount = 1;
	face->face = gnome_font_unsized_closest (family, weight, italic);
	face->nglyphs = gnome_font_face_get_num_glyphs (face->face);

	return face;
}

static void
nr_type_directory_family_list_destructor (NRNameList *list)
{
	nr_free (list->names);
}

NRNameList *
nr_type_directory_family_list_get (NRNameList *families)
{
	static GList *fl = NULL;

	families->destructor = nr_type_directory_family_list_destructor;

	if (!fl) fl = gnome_font_family_list ();

	if (fl) {
		GList *l;
		int pos;
		families->length = g_list_length (fl);
		families->names = nr_new (unsigned char *, families->length);
		pos = 0;
		for (l = fl; l; l = l->next) {
			families->names[pos++] = (unsigned char *) l->data;
		}
	} else {
		families->length = 0;
		families->names = NULL;
	}

	return families;
}

static void
nr_type_directory_style_list_destructor (NRNameList *list)
{
	nr_free (list->names);
}

NRNameList *
nr_type_directory_style_list_get (const unsigned char *family, NRNameList *styles)
{
	static GList *fl = NULL;
	GList *sl, *l;

	styles->destructor = nr_type_directory_style_list_destructor;

	if (!fl) fl = gnome_font_list ();

	sl = NULL;
	for (l = fl; l; l = l->next) {
		if (!strncmp (family, (gchar *) l->data, strlen (family))) {
			gchar *p;
			p = (gchar *) l->data + strlen (family);
			while (*p && isspace (*p)) p += 1;
			if (!*p) p = "Normal";
			sl = g_list_prepend (sl, p);
		}
	}
	sl = g_list_reverse (sl);

	if (sl) {
		GList *l;
		int pos;
		styles->length = g_list_length (sl);
		styles->names = nr_new (unsigned char *, styles->length);
		pos = 0;
		for (l = sl; l; l = l->next) {
			styles->names[pos++] = (unsigned char *) l->data;
		}
		g_list_free (sl);
	} else {
		styles->length = 0;
		styles->names = NULL;
	}

	return styles;
}



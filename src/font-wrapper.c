#define FONT_CONTEXT_C

#include <glib.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_bpath.h>
#include <gnome.h>
#include <libgnomeprint/gnome-font.h>
#include "helper/gt1.h"
#include "xml/repr.h"

#include "font-wrapper.h"

#define DEBUG_GLYPH_INFO

typedef struct {
	ArtBpath * bpath;
	double wx;
} SPGlyphInfo;

struct _SPFont {
	int ref_count;
	const gchar * family;
	GnomeFontWeight weight;
	gboolean italic;
	GnomeFontUnsized * gnomefont;
	Gt1LoadedFont * gtfont;
	GHashTable * outlines;
};

#ifdef DEBUG_GLYPH_INFO
static gint n_gi = 0;
#endif

GHashTable * fonts = NULL;
GList * fontlist = NULL;

static void free_glyph_info (gpointer key, gpointer value, gpointer data);

static guint
sp_font_hash (gconstpointer key)
{
	return g_str_hash (((SPFont *) key)->family);
}

static gint
sp_font_equal (gconstpointer a, gconstpointer b)
{
	SPFont * fa, * fb;
	fa = (SPFont *) a;
	fb = (SPFont *) b;
	if (strcmp (fa->family, fb->family) != 0)
		return FALSE;
	if (fa->weight != fb->weight)
		return FALSE;
	if (!fa->italic && fb->italic)
		return FALSE;
	if (fa->italic && !fb->italic)
		return FALSE;
	return TRUE;
}

SPFont * sp_font_get (const gchar * family, GnomeFontWeight weight, gboolean italic)
{
	SPFont * font;
	SPFont sf;
	GnomeFontUnsized * gnomefont;
	Gt1LoadedFont * gtfont;

	if (fonts == NULL) {
		fonts = g_hash_table_new (sp_font_hash, sp_font_equal);
	}

	sf.family = family;
	sf.weight = weight;
	sf.italic = italic;

	font = g_hash_table_lookup (fonts, &sf);

	if (font == NULL) {

		gnomefont = gnome_font_unsized_closest (family, weight, italic);
		if (gnomefont == NULL)
			return sp_font_default ();

		gtfont = gt1_load_font (gnomefont->pfb_fn);
		g_return_val_if_fail (gtfont != NULL, NULL);

		font = g_new (SPFont, 1);

		font->ref_count = 1;

		font->family = g_strdup (family);
		font->weight = weight;
		font->italic = italic;
		font->gnomefont = gnomefont;
		font->gtfont = gtfont;
		font->outlines = g_hash_table_new (NULL, NULL);

		g_hash_table_insert (fonts, font, font);

	} else {
		sp_font_ref (font);
	}

	return font;
}

void
sp_font_ref (SPFont * font)
{
	g_assert (font != NULL);

	font->ref_count++;
}

void
sp_font_unref (SPFont * font)
{
	g_assert (font != NULL);

	font->ref_count--;

	if (font->ref_count < 1) {

		g_hash_table_remove (fonts, font);

		if (font->family)
			g_free ((gchar *) font->family);
#if 0
		if (font->gnomefont)
			g_free (font->gnomefont);
#endif
		if (font->gtfont)
			g_free (font->gtfont);

		if (font->outlines) {
			g_hash_table_foreach (font->outlines, free_glyph_info, NULL);
			g_hash_table_destroy (font->outlines);
		}

		g_free (font);
	}
}

SPFont *
sp_font_copy (SPFont * font)
{
	g_assert (font != NULL);

	sp_font_ref (font);
	return font;
}

SPFont *
sp_font_default (void)
{
	static SPFont * default_font = NULL;

	if (default_font == NULL) {
		default_font = sp_font_get ("Helvetica", GNOME_FONT_BOOK, FALSE);
		g_assert (default_font != NULL);
	}

	sp_font_ref (default_font);

	return default_font;
}

ArtBpath * sp_font_glyph_outline (SPFont * font, guint glyph)
{
	SPGlyphInfo * gi;

	g_return_val_if_fail (font != NULL, NULL);

	if (glyph == 32) return NULL;

	gi = g_hash_table_lookup (font->outlines, GINT_TO_POINTER (glyph));

	if (gi == NULL) {
		gi = g_new (SPGlyphInfo, 1);

#ifdef DEBUG_GLYPH_INFO
	n_gi++;
	g_print ("num_glyph_info = %d\n", n_gi);
#endif

		if (glyph != 32) {
			gi->bpath = gt1_get_glyph_outline (font->gtfont, glyph, &gi->wx);
		} else {
			gi->bpath = NULL;
		}
		g_hash_table_insert (font->outlines, GINT_TO_POINTER (glyph), gi);
	}

	return gi->bpath;
}

gint sp_font_glyph_width (SPFont * font, guint glyph)
{
	SPGlyphInfo * gi;

	g_return_val_if_fail (font != NULL, 0);

	gi = g_hash_table_lookup (font->outlines, GINT_TO_POINTER (glyph));

	if (gi == NULL) {
		gi = g_new (SPGlyphInfo, 1);

#ifdef DEBUG_GLYPH_INFO
	n_gi++;
	g_print ("num_glyph_info = %d\n", n_gi);
#endif
		gi->bpath = gt1_get_glyph_outline (font->gtfont, glyph, &gi->wx);

		g_hash_table_insert (font->outlines, GINT_TO_POINTER (glyph), gi);
	}

	return gi->wx;
}

static void
free_glyph_info (gpointer key, gpointer value, gpointer data)
{
	art_free (((SPGlyphInfo *) value)->bpath);
	g_free (value);
#ifdef DEBUG_GLYPH_INFO
	n_gi--;
	g_print ("num_glyph_info = %d\n", n_gi);
#endif
}

static void
add_all_weights (SPFontFamilyInfo * family)
{
	family->fonts = g_new (SPFontInfo, 8);
	(family->fonts + 0)->weight = GNOME_FONT_BOOK;
	(family->fonts + 0)->italic = FALSE;
	(family->fonts + 1)->weight = GNOME_FONT_BOOK;
	(family->fonts + 1)->italic = TRUE;
	(family->fonts + 2)->weight = GNOME_FONT_LIGHT;
	(family->fonts + 2)->italic = FALSE;
	(family->fonts + 3)->weight = GNOME_FONT_LIGHT;
	(family->fonts + 3)->italic = TRUE;
	(family->fonts + 4)->weight = GNOME_FONT_MEDIUM;
	(family->fonts + 4)->italic = FALSE;
	(family->fonts + 5)->weight = GNOME_FONT_MEDIUM;
	(family->fonts + 5)->italic = TRUE;
	(family->fonts + 6)->weight = GNOME_FONT_BOLD;
	(family->fonts + 6)->italic = FALSE;
	(family->fonts + 7)->weight = GNOME_FONT_BOLD;
	(family->fonts + 7)->italic = TRUE;
}

GList *
sp_font_family_list (void)
{
	GnomeFontClass * gfc;
	GList * list, * l;
	gchar * fullname;
	SPFontFamilyInfo * family;
	gint i;

	if (fontlist == NULL) {
		gfc = gtk_type_class (gnome_font_get_type ());
		list = gnome_font_family_list (gfc);
			family = g_new (SPFontFamilyInfo, 1);
			family->name = "Helvetica";
			family->num_fonts = 8;
			add_all_weights (family);
			fontlist = g_list_prepend (fontlist, family);
			family = g_new (SPFontFamilyInfo, 1);
			family->name = "Times";
			family->num_fonts = 8;
			add_all_weights (family);
			fontlist = g_list_prepend (fontlist, family);
			family = g_new (SPFontFamilyInfo, 1);
			family->name = "Courier";
			family->num_fonts = 8;
			add_all_weights (family);
			fontlist = g_list_prepend (fontlist, family);
			family = g_new (SPFontFamilyInfo, 1);
			family->name = "Palatino";
			family->num_fonts = 8;
			add_all_weights (family);
			fontlist = g_list_prepend (fontlist, family);
		gnome_font_family_list_free (list);
	}

	return fontlist;
}

#if 0
gint
sp_font_glyph (SPFont * font, gint glyph)
{
	return gnome_font_unsized_get_glyph (font->gnomefont, glyph);
}
#endif

const gchar *
sp_font_read_family (SPCSSAttr * css)
{
	const gchar * family;

	g_assert (css != NULL);

	family = sp_repr_css_property (css, "font-family", "Helvetica");

	return family;
}

gdouble
sp_font_read_size (SPCSSAttr * css)
{
	gdouble size;

	g_assert (css != NULL);

	size = sp_repr_css_double_property (css, "font-size", 12.0);

	return size;
}

GnomeFontWeight
sp_font_read_weight (SPCSSAttr * css)
{
	const gchar * weight;

	g_assert (css != NULL);

	weight = sp_repr_css_property (css, "font-weight", "normal");
	if (strcmp (weight, "bold") == 0)
		return GNOME_FONT_BOLD;

	return GNOME_FONT_BOOK;
}

gboolean
sp_font_read_italic (SPCSSAttr * css)
{
	const gchar * italic;

	g_assert (css != NULL);

	italic = sp_repr_css_property (css, "font-style", "normal");
	if (strcmp (italic, "italic") == 0)
		return TRUE;

	return FALSE;
}
